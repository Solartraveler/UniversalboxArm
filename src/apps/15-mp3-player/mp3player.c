/* mp3 player
(c) 2025 by Malte Marwedel

SPDX-License-Identifier: GPL-3.0-or-later
*/
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>

#include "mp3player.h"

#include "boxlib/coproc.h"
#include "boxlib/flash.h"
#include "boxlib/keys.h"
#include "boxlib/lcd.h"
#include "boxlib/leds.h"
#include "boxlib/mcu.h"
#include "boxlib/peripheral.h"
#include "boxlib/rs232debug.h"
#include "boxlib/sequenceToPwm.h"
#include "boxlib/stackSampler.h"
#include "boxlib/timer32Bit.h"
#include "femtoVsnprintf.h"
#include "ff.h"
#include "filesystem.h"
#include "gui.h"
#include "main.h"
#include "mad.h"
#include "utility.h"

#define SD_BLOCKSIZE 512

/*The behaviour of libmad is mostly undocumented.
  According to https://lists.mars.org/hyperkitty/list/mad-dev@lists.mars.org/message/23ACZCLN3DMTR62GDAQNBGNUUMXORWYR/
  the maximum mp3 frame size is 2881 bytes, then libmad needs 8 additional guard bytes.
  Then we round this up to 2 additional SD card block size -> 3584, so we can always read
  at least one frame into the buffer without needing to read a partial block from the source.

*/
#define IN_BUFFER_SIZE (SD_BLOCKSIZE * 7)

/*Should buffer 0.5s at 8bit, 44100Hz, mono -> 22050 bytes needed, but we need
  to save memory in order to fit the app into the 160KiB RAM, so make the buffer
  smaller, risking underruns. With 8KiB there are no underruns in the app,
  as long as there is some pause between every keypress. With 5KiB there were underruns
  even when not pressing any keys.
*/
//GCC version provided by Debian 12 for arm-none-eabi-gcc
#define FIFO_SIZE 8000

//Reverse engineered by looking into steam and mad.h:
//22652 byte for 32-bit ARM, 22732 byte for 64-bit x64
#define SYNC_BUFFER_SIZE (sizeof(struct mad_stream) + sizeof(struct mad_frame) + sizeof(struct mad_synth))

//See layer3.c function mad_layer_III
#define OVERLAP_BUFFER_SIZE (2 * 32 * 18 * sizeof(mad_fixed_t))

typedef struct {
	FIL f; //file to read the input data from
	bool opened; //if true, f is valid
	uint8_t inBuffer[IN_BUFFER_SIZE]; //file buffer to be used by libmad
	size_t inBufferUsed; //until which position inBuffer contains valid data
	uint8_t fifoBuffer[FIFO_SIZE]; //output buffer to be used for PWM generation
	uint32_t bytesRead; //number of bytes read from the input file
	uint32_t bytesGenerated; //number of decoded samples generated (converted to 8bit, so equal to output bytes)
	bool play; //if true, the file is not at the end yet (or the user did not select stop)
	bool needData; //if true, decoding function should refill inBuffer
	uint32_t sampleRate; //in [Hz]
	uint32_t bitRate; //in [Hz]
	uint32_t channels; //1 = mono, 2 = stereo
	uint64_t ticksTotal; //[CPU ticks] for performance measurement
	uint64_t ticksInput; //[CPU ticks] for performance measurement
	uint64_t ticksOutput; //[CPU ticks] for performance measurement
	struct mad_decoder madDecoder; //the libmad state struct
	//Internal buffers for libmad:
	uint8_t syncBuffer[SYNC_BUFFER_SIZE];
	unsigned char main_data[MAD_BUFFER_MDLEN];
	uint8_t overlap[OVERLAP_BUFFER_SIZE];
} playerState_t;


playerState_t g_player;

uint32_t g_cycleTick;

/*Dummy functions to save memory. They are referenced by libmad, but are not called
  in the setup this app is providing for the lib. The compiler does a #define malloc mallocIncept
  for all files.
*/
void * mallocIncept(size_t size) {
	printf("Error, malloc disabled - want %u bytes\r\n", (unsigned int)size);
	return NULL;
}

void * callocIncept(size_t nmem, size_t size) {
	printf("Error, calloc disabled - want %u bytes\r\n", (unsigned int)(nmem * size));
	return NULL;
}

void freeIncept(void * ptr) {
	printf("Error, free disabled - 0x%x\r\n", (unsigned int)(uintptr_t)ptr);
}

void abortIncept(void) {
	printf("Error, abort called\r\n");
	while(1);
}

void PlayerHelp(void) {
	printf("h: Print help\r\n");
	printf("r: Reset\r\n");
	printf("w-a-s-d: Send key code to GUI\r\n");
}

void PlayerStop(void) {
	if (g_player.opened) {
		if (g_player.play) {
			g_player.play = false;
		}
		f_close(&g_player.f);
		g_player.opened = false;
		g_player.bytesGenerated = 0;
		g_player.bytesRead = 0;
	}
	SeqStop();
}

void PlayerSetupOutput(void) {
	uint32_t sampleRate = g_player.sampleRate;
	if (sampleRate) {
		uint32_t prescaler = F_CPU  / sampleRate;
		float calcBack = (float)F_CPU / (float)prescaler;

		if (calcBack != sampleRate) {
			printf("Warning, samplerate will not be exact, next match %uHz\r\n", (unsigned int)calcBack);
		}
		//printf("SampleRate: %uHz, prescaler: %u\r\n", (unsigned int)sampleRate, (unsigned int)prescaler);
		SeqStart(0, prescaler, g_player.fifoBuffer, FIFO_SIZE);
	} else {
		printf("Warning, samplerate not known\r\n");
	}
}

//A big thank you to https://stackoverflow.com/questions/39803572/libmad-playback-too-fast-if-read-in-chunks
enum mad_flow MadInput(void *data, struct mad_stream *stream) {
	(void)data;
	if ((g_player.opened == false) || (g_player.play == false)) {
		g_player.play = false;
		return MAD_FLOW_STOP;
	}
	uint32_t tStart = Timer32BitGet();
	size_t maxRead = IN_BUFFER_SIZE;
	//Did the decoder processed some data?
	size_t leftBefore = 0;
	size_t consumed = 0;
	if (stream->error == MAD_ERROR_BUFLEN) {
		if (stream->next_frame) {
			consumed = stream->next_frame - stream->buffer;
			leftBefore = g_player.inBufferUsed - consumed;
		}
	}
	if (leftBefore < maxRead) {
		if (leftBefore) {
			memmove(g_player.inBuffer, g_player.inBuffer + consumed, leftBefore);
		}
		maxRead -= leftBefore;
	} else {
		leftBefore = 0; //if no data were processed - skip the data alltogehter - they seem to contain no frame at all
	}
	//round it down to the next block size
	maxRead -= maxRead % SD_BLOCKSIZE;
	//printf("Consumed: %u, left before %u, will read %u\r\n", (unsigned int)consumed, (unsigned int)leftBefore, (unsigned int)maxRead);
	//read data and give the buffer back
	UINT r = 0;
	enum mad_flow result = MAD_FLOW_STOP;
	if ((f_read(&(g_player.f), g_player.inBuffer + leftBefore, maxRead, &r) == FR_OK) && (r > 0)) {
		mad_stream_buffer(stream, g_player.inBuffer, r + leftBefore);
		g_player.bytesRead += r;
		g_player.inBufferUsed = r + leftBefore;
		//printf("Input read %u bytes\r\n", (unsigned int)r);
		result = MAD_FLOW_CONTINUE;
	} else {
		g_player.play = false;
	}
	uint32_t tStop = Timer32BitGet();
	g_player.ticksInput += tStop - tStart;
	return result;
}

//Function mostly copied from minimad.c found in libmad
enum mad_flow MadError(void *data, struct mad_stream *stream, struct mad_frame *frame) {
	(void)data;
	(void)frame;
	printf("Error, decoding 0x%04x (%s) at byte offset %u\r\n", stream->error, mad_stream_errorstr(stream), (unsigned int)g_player.bytesRead);
	if (stream->error == MAD_ERROR_NONE) {
		return MAD_FLOW_CONTINUE;
	}
	return MAD_FLOW_IGNORE;
}

enum mad_flow MadHeader(void *data, struct mad_header const * header) {
	(void)data;
	if ((g_player.sampleRate == 0) && (header->samplerate)) {
		g_player.sampleRate = header->samplerate;
		PlayerSetupOutput();
	}
	g_player.bitRate = header->bitrate;
	if (header->mode == MAD_MODE_SINGLE_CHANNEL) {
		g_player.channels = 1;
	} else {
		g_player.channels = 2;
	}
	//printf("Mad header callback, bitrate %u\r\n", (unsigned int)g_player.bitRate);
	return MAD_FLOW_CONTINUE;
}

//Function mostly copied from minimad.c found in libmad
int32_t MadScale(mad_fixed_t sample) {
	/* round */
	sample += (1L << (MAD_F_FRACBITS - 16));
	/* clip */
	if (sample >= MAD_F_ONE) {
		sample = MAD_F_ONE - 1;
	} else if (sample < -MAD_F_ONE) {
		sample = -MAD_F_ONE;
	}
	/* quantize */
	return sample >> (MAD_F_FRACBITS + 1 - 16);
}

//Function mostly copied from minimad.c found in libmad
enum mad_flow MadOutput(void *data, struct mad_header const *header, struct mad_pcm *pcm) {
	(void)data;
	(void)header;
	uint32_t nchannels, nsamples;
	mad_fixed_t const *left_ch, *right_ch;
	uint32_t tStart = Timer32BitGet();
	/* pcm->samplerate contains the sampling frequency */
	nchannels = pcm->channels;
	nsamples  = pcm->length;
	left_ch   = pcm->samples[0];
	right_ch  = pcm->samples[1];
	//size_t freeStart = SeqFifoFree();
	//printf("Output has %u samples ready\r\n", (unsigned int)nsamples);
	for (uint32_t i = 0; i < nsamples; i++) {
		int32_t sample;
		/* output sample(s) in 16-bit signed little-endian PCM */
		sample = MadScale(*left_ch++);
		if (nchannels == 2) {
			sample += MadScale(*right_ch++);
			sample /= 2; //smash left and right channel together
		}
		//signed 16bit to unsigned 8bit PCM
		sample /= 256;
		sample += 128;
		sample = MAX(sample, 0);
		sample = MIN(sample, 255);
		uint8_t sample8 = sample;
		while (SeqFifoFree() == 0) {
			//This will happen because the next call to PlayerMp3Cycle does not care about the FIFO state
			//printf("Waiting for FIFO, need to put %u, free at start %u\r\n", (unsigned int)nsamples, (unsigned int)freeStart);
		}
		SeqFifoPut(&sample8, 1);
	}
	g_player.bytesGenerated += nsamples;
	uint32_t tStop = Timer32BitGet();
	g_player.ticksOutput += tStop - tStart;
	if (SeqFifoFree() < nsamples) {
		//We simply assume we get the same amount of data next time, so it should fit into the buffer without waiting
		//printf("Only %u bytes free in FIFO\r\n", (unsigned int)SeqFifoFree());
		return MAD_FLOW_STOP;
	}
	return MAD_FLOW_CONTINUE;
}

void PlayerStart(const char * filepath, bool playback) {
	PlayerStop();
	if (f_open(&g_player.f, filepath, FA_READ) != FR_OK) {
		printf("Error, could not open file\r\n");
		return;
	}
	g_player.inBufferUsed = 0;
	g_player.sampleRate = 0;
	g_player.bitRate = 0;
	g_player.ticksInput = 0;
	g_player.ticksOutput = 0;
	g_player.ticksTotal = 0;
	g_player.opened = true;
	if (playback) {
		PlayerSetupOutput();
		g_player.play = true;
		g_player.needData = true;
		mad_decoder_init(&g_player.madDecoder, NULL, MadInput, MadHeader, NULL, MadOutput, MadError, 0 /* message */);
		//This would play, but would block until the mp3 is finished (or the watchdog bites us):
		//mad_decoder_run(&g_player.madDecoder, MAD_DECODER_MODE_SYNC);
		//instead we run the internal loop manually, and this is the init code:
		struct mad_decoder * pMad = &(g_player.madDecoder);
		pMad->sync = (void *)g_player.syncBuffer;
		struct mad_stream * pStream = &pMad->sync->stream;
		struct mad_frame * pFrame = &pMad->sync->frame;
		struct mad_synth * pSynth = &pMad->sync->synth;
		mad_stream_init(pStream);
		mad_frame_init(pFrame);
		mad_synth_init(pSynth);
		mad_stream_options(pStream, pMad->options);
		//source analysis show we can simply set a fixed buffer, so malloc and calloc will never be called
		//this however forbids to call the deinit functions, as otherwise they would free the static buffers
		pStream->main_data = (void *)g_player.main_data;
		memset(g_player.overlap, 0, OVERLAP_BUFFER_SIZE); //cause original code uses calloc - clear data from previous play
		pFrame->overlap = (void *)g_player.overlap;
	}
}

void PlayerEvaluatePerformance(void) {
	uint32_t bytesPerSecond = g_player.sampleRate;
	if (!bytesPerSecond) {
		return;
	}
	uint32_t secondsPlayed = g_player.bytesGenerated / bytesPerSecond;
	uint64_t cycles = (uint64_t)secondsPlayed * (uint64_t)F_CPU;
	if (!cycles) {
		return;
	}
	uint64_t percentageTotal = g_player.ticksTotal * (uint64_t)100 / cycles;
	uint64_t ticksComputing = g_player.ticksTotal - g_player.ticksInput - g_player.ticksOutput;
	uint64_t percentageInput = g_player.ticksInput * (uint64_t)100 / cycles;
	uint64_t percentageComputing = ticksComputing * (uint64_t)100 / cycles;
	uint64_t percentageOutput = g_player.ticksOutput * (uint64_t)100 / cycles;
	printf("CPU Ticks for playing %9uM - %02u%c\r\n", (unsigned int)(g_player.ticksTotal / 1000000ULL), (unsigned int)percentageTotal, '%');
	printf("  Input               %9uM - %02u%c\r\n", (unsigned int)(g_player.ticksInput / 1000000ULL), (unsigned int)percentageInput, '%');
	printf("  Computing           %9uM - %02u%c\r\n", (unsigned int)(ticksComputing / 1000000ULL), (unsigned int)percentageComputing, '%');
	printf("  Output              %9uM - %02u%c\r\n", (unsigned int)(g_player.ticksOutput / 1000000ULL), (unsigned int)percentageOutput, '%');
}

//reverse engineed form decoder.c
void PlayerMp3Cycle(void) {
	Timer32BitInit(0);
	Timer32BitStart();
	void *error_data = NULL;
	struct mad_decoder * pMad = &(g_player.madDecoder);
	struct mad_stream * stream = &pMad->sync->stream;
	struct mad_frame * frame = &pMad->sync->frame;
	struct mad_synth * synth = &pMad->sync->synth;
	if (g_player.needData) {
		if (pMad->input_func(pMad->cb_data, stream) != MAD_FLOW_CONTINUE) {
			uint32_t stamp = Timer32BitGet();
			g_player.ticksTotal += stamp;
			Timer32BitStop();
			PlayerEvaluatePerformance();
			return;
		}
	}
	g_player.needData = true;
	while (1) {
		if (pMad->header_func) {
			if (mad_header_decode(&frame->header, stream) == -1) {
				if (!MAD_RECOVERABLE(stream->error)) {
					break;
				}
				pMad->error_func(error_data, stream, frame);
			}
			enum mad_flow headerResult = pMad->header_func(pMad->cb_data, &frame->header);
			if ((headerResult == MAD_FLOW_STOP) || ((headerResult == MAD_FLOW_BREAK))) {
				break;
			}
			if (headerResult == MAD_FLOW_IGNORE) {
				continue;
			}
		}
		if (mad_frame_decode(frame, stream) == -1) {
			if (!MAD_RECOVERABLE(stream->error)) {
				break;
			}
			if (pMad->error_func(error_data, stream, frame) != MAD_FLOW_CONTINUE) {
				continue;
			}
		}
		//We have no filter func...
		mad_synth_frame(synth, frame);
		if (pMad->output_func(pMad->cb_data,&frame->header, &synth->pcm) == MAD_FLOW_STOP) {
			g_player.needData = false;
			break;
		}
	}
	uint32_t stamp = Timer32BitGet();
	g_player.ticksTotal += stamp;
	Timer32BitStop();
}

void PlayerFileGetMeta(char * text, size_t maxLen) {
	snprintf(text, maxLen, "%uCh, %uHz, %uBit/s", (unsigned int)g_player.channels, (unsigned int)g_player.sampleRate, (unsigned int)g_player.bitRate);
}

void PlayerFileGetState(char * text, size_t maxLen) {
	uint32_t bytesPerSecond = g_player.sampleRate;
	if (bytesPerSecond) {
		uint32_t secondsPlayed = g_player.bytesGenerated / bytesPerSecond;
		snprintf(text, maxLen, "%u seconds", (unsigned int)secondsPlayed);
	} else {
		snprintf(text, maxLen, "---");
	}
}

void AppInit(void) {
	LedsInit();
	Led1Yellow();
	StackSampleInit();
	PeripheralPowerOff();
	uint8_t error = McuClockToHsiPll(F_CPU, RCC_HCLK_DIV1);
	HAL_Delay(100);
	PeripheralPowerOn();
	Rs232Init();
	printf("\r\nMp3 player %s\r\n", APPVERSION);
	printf("h: Print help\r\n");
	if (error) {
		printf("Error, failed to increase CPU clock - %u\r\n", error);
	}
	KeysInit();
	CoprocInit();
	PeripheralInit();
	FlashEnable(16); //5MHz
	FilesystemMount();
	GuiInit();
	Led1Off();
	g_cycleTick = HAL_GetTick();
}

void ExecReset(void) {
	printf("Reset selected\r\n");
	Rs232Flush();
	NVIC_SystemReset();
}

void AppCycle(void) {
	static uint32_t ledCycle = 0;
	//led flash
	if (ledCycle < 500) {
		Led2Green();
	} else {
		Led2Off();
	}
	/*Don't do it in the same cycle as the GUI update to not let the mp3 update
	  wait longer than needed */
	if (ledCycle == 200) {
		CoprocWatchdogReset();
	}
	if (ledCycle >= 1000) {
		ledCycle = 0;
	}
	ledCycle++;
	char input = Rs232GetChar();
	if (input) {
		printf("%c", input);
		if (input == 'h') {
			PlayerHelp();
		} else if (input == 'r') {
			ExecReset();
		}
	}
	if ((g_player.play) && (HAL_GetTick() < (g_cycleTick + 2))) {
		PlayerMp3Cycle();
	}
	GuiCycle(input); //processes on the 400th and 500th call
	StackSampleCheck();
	/*Call this function 1000x per second, if one cycle took more than 1ms,
	  we skip the wait to catch up with calling.
	  cycleTick last is needed to prevent endless wait in the case of a 32bit
	  overflow.
	*/
	uint32_t cycleTickLast = g_cycleTick;
	g_cycleTick++; //next call expected tick value
	uint32_t tick;
	do {
		tick = HAL_GetTick();
		if (tick < g_cycleTick) {
			HAL_Delay(1);
		}
	} while ((tick < g_cycleTick) && (tick >= cycleTickLast));
}
