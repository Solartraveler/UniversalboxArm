/* Coprocessor-control
(c) 2022 by Malte Marwedel

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

#include "boxlib/keys.h"
#include "boxlib/leds.h"
#include "boxlib/rs232debug.h"
#include "boxlib/lcd.h"
#include "boxlib/flash.h"
#include "boxlib/peripheral.h"
#include "boxlib/coproc.h"
#include "boxlib/mcu.h"
#include "femtoVsnprintf.h"
#include "ff.h"
#include "filesystem.h"
#include "gui.h"
#include "main.h"
#include "mad.h"
#include "sequenceToPwm.h"
#include "utility.h"

#define F_CPU 80000000UL

/*The behaviour of libmad is mostly undocumented.
  It seems like mad_stream_buffer is using the given buffers internally (no copy)
  until... when?
  Assumption: libmad processes one frame after another and then the buffer can
  be freed/reused. So as long as every buffer is at least one frame in size,
  it can not spawn more than two buffers. So two buffers should be enough. Frame
  size can be about 421 bytes for 320kbit at 44100Hz samplerate. So 512 might
  be too little in some cases?
*/

#define IN_BUFFER_NUM 2
#define IN_BUFFER_SIZE 2048

/*Should buffer 0.5s at 8bit, 44100Hz, mono -> 22050 bytes needed, but we need
  to save memory in order to fit mp3 into the 160KiB RAM, so make the buffer
  smaller.
*/
#define FIFO_SIZE 2500

typedef struct {
	FIL f;
	bool opened;
	uint8_t inBuffer[IN_BUFFER_NUM][IN_BUFFER_SIZE];
	uint32_t inBufferIdx;
	uint8_t fifoBuffer[FIFO_SIZE];
	uint32_t bytesProcessed;
	bool play;
	uint32_t sampleRate;
	uint32_t bitRate;
	uint32_t channels;
	struct mad_decoder madDecoder;
} playerState_t;


playerState_t g_player;

uint32_t g_cycleTick;

void PlayerHelp(void) {
	printf("h: Print help\r\n");
	printf("r: Reset\r\n");
	printf("w-a-s-d: Send key code to GUI\r\n");
}

void PlayerStop(void) {
	if (g_player.opened) {
		if (g_player.play) {
			mad_decoder_finish(&g_player.madDecoder);
			g_player.play = false;
		}
		f_close(&g_player.f);
		g_player.opened = false;
		g_player.bytesProcessed = 0;
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

//Function mostly copied from minimad.c found in libmad
enum mad_flow MadError(void *data, struct mad_stream *stream, struct mad_frame *frame) {
	(void)data;
	(void)frame;
	printf("Error, decoding 0x%04x (%s) at byte offset %u\r\n", stream->error, mad_stream_errorstr(stream), (unsigned int)g_player.bytesProcessed);
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
	unsigned int nchannels, nsamples;
	mad_fixed_t const *left_ch, *right_ch;
	/* pcm->samplerate contains the sampling frequency */
	nchannels = pcm->channels;
	nsamples  = pcm->length;
	left_ch   = pcm->samples[0];
	right_ch  = pcm->samples[1];
	//printf("Output has %u samples ready\r\n", (unsigned int)nsamples);
	while (nsamples--) {
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
		while (SeqFifoFree() == 0);
		SeqFifoPut(&sample8, 1);
	}
	return MAD_FLOW_CONTINUE;
}

//Function partially copied from minimad.c found in libmad
enum mad_flow MadInput(void *data, struct mad_stream *stream) {
	(void)data;
	if ((g_player.opened == false) || (g_player.play == false)) {
		g_player.play = false;
		return MAD_FLOW_STOP;
	}
	UINT r = 0;
	uint32_t idx = g_player.inBufferIdx;
	if ((f_read(&(g_player.f), g_player.inBuffer[idx], IN_BUFFER_SIZE, &r) == FR_OK)) {
		if (r > 0) {
			mad_stream_buffer(stream, g_player.inBuffer[idx], r);
			g_player.bytesProcessed += r;
			idx++;
			if (idx >= IN_BUFFER_NUM) {
				idx = 0;
			}
			g_player.inBufferIdx = idx;
			//printf("Input read %u bytes\r\n", (unsigned int)r);
			return MAD_FLOW_CONTINUE;
		}
	}
	g_player.play = false;
	return MAD_FLOW_STOP;
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

void PlayerStart(const char * filepath, bool playback) {
	PlayerStop();
	if (f_open(&g_player.f, filepath, FA_READ) != FR_OK) {
		printf("Error, could not open file\r\n");
		return;
	}
	g_player.sampleRate = 0;
	g_player.bitRate = 0;
	g_player.opened = true;
	if (playback) {
		PlayerSetupOutput();
		mad_decoder_init(&g_player.madDecoder, NULL, MadInput, MadHeader, NULL, MadOutput, MadError, 0 /* message */);
		g_player.play = true;
	}
}

void PlayerFileGetMeta(char * text, size_t maxLen) {
	snprintf(text, maxLen, "%uCh, %uHz, %uBit/s", (unsigned int)g_player.channels, (unsigned int)g_player.sampleRate, (unsigned int)g_player.bitRate);
}

void PlayerFileGetState(char * text, size_t maxLen) {
	uint32_t bytesPerSecond = 1; //TODO
	if (bytesPerSecond) {
		uint32_t secondsPlayed = g_player.bytesProcessed / bytesPerSecond;
		snprintf(text, maxLen, "%u seconds", (unsigned int)secondsPlayed);
	} else {
		snprintf(text, maxLen, "---");
	}
}

void AppInit(void) {
	LedsInit();
	Led1Yellow();
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
	if (ledCycle >= 1000) {
		CoprocWatchdogReset();
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
	if (g_player.play) {
		int result = mad_decoder_run(&g_player.madDecoder, MAD_DECODER_MODE_SYNC);
		printf("Processed with result %i\r\n", result);
	}
	GuiCycle(input);
	/* Call this function 1000x per second, if one cycle took more than 1ms,
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
