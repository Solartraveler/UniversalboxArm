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

#include "wavplayer.h"

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
#include "sequenceToPwm.h"
#include "utility.h"
#include "wav.h"

#define F_CPU 32000000

//should buffer 0.5s at 8bit, 44100Hz, mono
#define FIFO_SIZE 22050

typedef struct {
	FIL f;
	bool opened;
	fmtHeader_t fmt;
	dataHeader_t dh;
	uint8_t buffer[FIFO_SIZE];
	uint32_t bytesProcessed;
	bool play;
} playerState_t;


playerState_t g_player;

uint32_t g_cycleTick;

void PlayerHelp(void) {
	printf("h: Print help\r\n");
	printf("r: Reset\r\n");
	printf("w-a-s-d: Send key code to GUI\r\n");
}

bool ReadRiff(FIL * f, riffHeader_t * pRh) {
	UINT r = 0;
	if ((f_read(f, pRh, sizeof(riffHeader_t), &r) == FR_OK) && (r == sizeof(riffHeader_t))) {
		if ((memcmp(pRh->signature, "RIFF", SIGNATUREBYTES) == 0) &&
			 (memcmp(pRh->id, "WAVE", RIFFIDBYTES) == 0)) {
			return true;
		}
	}
	return false;
}

bool ReadSkip(FIL * f, size_t toSkip) {
	//TODO: This is slow, but never called for most wav files anyway. Only there to fully support the format.
	uint8_t dummy;
	for (size_t i = 0; i < toSkip; i++) {
		UINT r = 0;
		if (f_read(f, &dummy, 1, &r) != FR_OK) {
			return false;
		}
	}
	return true;
}

bool ReadFmt(FIL * f, fmtHeader_t * pFh) {
	while (1) {
		UINT r = 0;
		if ((f_read(f, pFh, METAHEADER, &r) == FR_OK) && (r == METAHEADER)) {
			if (memcmp(pFh->signature, "fmt ", SIGNATUREBYTES) == 0) {
				size_t toRead = sizeof(fmtHeader_t) - METAHEADER;
				if ((f_read(f, &(pFh->format), toRead, &r) == FR_OK) && (r == toRead)) {
					return true;
				} else {
					return false;
				}
			} else {
				//skip over this header
				printf("Skipping unknown header >%c%c%c%c< with size %u\n", pFh->signature[0], pFh->signature[1], pFh->signature[2], pFh->signature[3], (unsigned int)pFh->headerSize);
				if (pFh->headerSize > METAHEADER) {
					if (!ReadSkip(f, pFh->headerSize - METAHEADER)) {
						return false;
					}
				}
			}
		} else {
			return false;
		}
	}
}

bool ReadDataHeader(FIL * f, dataHeader_t * pDh) {
	while (1) {
		UINT r = 0;
		if ((f_read(f, pDh, METAHEADER, &r) == FR_OK) && (r == METAHEADER)) {
			if (memcmp(pDh->signature, "data", SIGNATUREBYTES) == 0) {
				return true;
			} else {
				//skip over this header
				printf("Skipping unknown header >%c%c%c%c< with size %u\n", pDh->signature[0], pDh->signature[1], pDh->signature[2], pDh->signature[3], (unsigned int)pDh->blockSize);
				if (pDh->blockSize > METAHEADER) {
					if (!ReadSkip(f, pDh->blockSize - METAHEADER)) {
						return false;
					}
				}
			}
		} else {
			return false;
		}
	}
}

void PlayerStop(void) {
	if (g_player.opened) {
		f_close(&g_player.f);
		g_player.opened = false;
		g_player.play = false;
		g_player.bytesProcessed = 0;
	}
	SeqStop();
}

void PlayerSetupOutput(void) {
	uint32_t sampleRate = g_player.fmt.sampleRate;
	uint32_t prescaler = F_CPU  / sampleRate;
	float calcBack = (float)F_CPU / (float)prescaler;
	if (calcBack != sampleRate) {
		printf("Warning, samplerate will not be exact, next match %uHz\r\n", (unsigned int)calcBack);
	}
	SeqStart(0, prescaler, g_player.buffer, FIFO_SIZE);
}

void PlayerStart(const char * filepath, bool playback) {
	PlayerStop();
	if (f_open(&g_player.f, filepath, FA_READ) != FR_OK) {
		printf("Error, could not open file\r\n");
		return;
	}
	g_player.opened = true;
	riffHeader_t rh;
	if (ReadRiff(&g_player.f, &rh)) {
		if (ReadFmt(&g_player.f, &g_player.fmt)) {
			printf("Format: %u\r\n", g_player.fmt.format);
			printf("Channels: %u\r\n", g_player.fmt.channels);
			printf("Samplerate: %u\r\n", (unsigned int)g_player.fmt.sampleRate);
			printf("BytesPerSecond: %u\r\n", (unsigned int)g_player.fmt.bytesPerSecond);
			printf("BlockAlign: %u\r\n", g_player.fmt.blockAlign);
			printf("BitsPerSample: %u\r\n", g_player.fmt.bitsPerSample);
			if (ReadDataHeader(&g_player.f, &g_player.dh)) {
				printf("Data size: %u\r\n", (unsigned int)g_player.dh.blockSize);
				if ((g_player.fmt.format == 1) &&
				    ((g_player.fmt.channels == 1) || (g_player.fmt.channels == 2)) &&
				    ((g_player.fmt.bitsPerSample == 8) || (g_player.fmt.bitsPerSample == 16)) &&
				    ((g_player.fmt.sampleRate >= 100) && (g_player.fmt.sampleRate <= 44200)) &&
				    (g_player.dh.blockSize > 0)) {
					printf("Format supported\r\n");
					if (playback) {
						g_player.play = true;
						PlayerSetupOutput();
					}
				} else {
					printf("Error, unsupported format\r\n");
				}
			} else {
				printf("Error, could not find data\r\n");
			}
		} else {
			printf("Error, could not find metainformation\r\n");
		}
	} else {
		printf("Error, not a valid wave file\r\n");
	}
}

void PlayerFileGetMeta(char * text, size_t maxLen) {
	snprintf(text, maxLen, "%uCh, %uHz, %uBit", g_player.fmt.channels, (unsigned int)g_player.fmt.sampleRate, g_player.fmt.bitsPerSample);
}

//Must be multiple of 4 (because of channels and bits per channel supported)
#define BUFFERCYCLE_MAX 2048

void PlayerFillFifo(void) {
	//We always want to have data for 0.5s of play in the FIFO.
	if ((g_player.play) && (g_player.bytesProcessed < g_player.dh.blockSize)) {
		uint8_t channels = g_player.fmt.channels;
		uint8_t bits = g_player.fmt.bitsPerSample;
		uint8_t bytes = bits / 8;
		uint8_t dataMultiplier = bytes * channels;
		uint32_t bytesPerSecond = dataMultiplier * g_player.fmt.sampleRate;
		size_t bufferFree = SeqFifoFree();
		size_t bufferUsed = FIFO_SIZE - bufferFree;
		size_t bufferWanted = bytesPerSecond / 2; //because of 0.5s.
		if (bufferUsed < bufferWanted) {
			//so far, all calculations are in bytes to the FIFO, but this might mean a larger amount of data from the file (dataMultiplier)
			//so toDo is the number of bytes read from the file
			size_t toRead = g_player.dh.blockSize - g_player.bytesProcessed;
			uint8_t buffer[BUFFERCYCLE_MAX];
			size_t bufferFreeToRead = bufferFree * dataMultiplier;
			toRead = MIN(toRead, bufferFreeToRead);
			size_t bufferFifoRead = (bufferWanted - bufferUsed) * dataMultiplier;
			toRead = MIN(toRead, bufferFifoRead);
			toRead = MIN(toRead, BUFFERCYCLE_MAX); //because we need to allocate the buffer on the stack
			//printf("ToRead: %zu\n", toRead);
			if (toRead > 0) {
				UINT r = 0;
				if ((f_read(&(g_player.f), buffer, toRead, &r) == FR_OK) && (r == toRead)) {
					size_t toWrite = toRead;
					if (bytes == 2) {
						toWrite /= 2;
						for (size_t i = 0; i < toWrite; i++) {
							//16bit data are signed, while 8bit data are unsigned
							int16_t value;
							memcpy(&value, &(buffer[i * 2]), sizeof(value));
							value /= 256;
							value += 128;
							buffer[i] = value;
						}
					}
					if (channels == 2) {
						toWrite /= 2;
						for (size_t i = 0; i < toWrite; i++) {
							buffer[i] = ((uint16_t)(buffer[i * 2]) + (uint16_t)(buffer[i * 2 + 1])) / 2; //mix both channels into one
						}
					}
					SeqFifoPut(buffer, toWrite);
					g_player.bytesProcessed += toRead;
				}
			}
		}
	}
}

void PlayerFileGetState(char * text, size_t maxLen) {
	uint32_t bytesPerSecond = (g_player.fmt.bitsPerSample / 8) * g_player.fmt.channels * g_player.fmt.sampleRate;
	if (bytesPerSecond) {
		uint32_t secondsPlayed = g_player.bytesProcessed / bytesPerSecond;
		uint32_t secondsTotal = g_player.dh.blockSize / bytesPerSecond;
		snprintf(text, maxLen, "%u of %u seconds", (unsigned int)secondsPlayed, (unsigned int)secondsTotal);
	} else {
		snprintf(text, maxLen, "---");
	}
}

void AppInit(void) {
	LedsInit();
	Led1Yellow();
	PeripheralPowerOff();
	HAL_Delay(100);
	PeripheralPowerOn();
	Rs232Init();
	printf("\r\nWav player %s\r\n", APPVERSION);
	printf("h: Print help\r\n");
	Rs232Flush();
	uint8_t error = McuClockToHsiPll(F_CPU, RCC_HCLK_DIV2);
	if (error) {
		printf("Error, failed to increase CPU clock - %u\r\n", error);
	}
	KeysInit();
	CoprocInit();
	PeripheralInit();
	FlashEnable(4); //4MHz
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
			//Only fill the fifo, if we are "in time"
			PlayerFillFifo();
			HAL_Delay(1);
		}
	} while ((tick < g_cycleTick) && (tick >= cycleTickLast));
}
