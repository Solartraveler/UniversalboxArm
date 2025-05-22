/* led-matrix-control
(c) 2025 by Malte Marwedel

SPDX-License-Identifier: GPL-3.0-or-later
*/
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>

#include "boxlib/coproc.h"
#include "boxlib/keys.h"
#include "boxlib/leds.h"
#include "boxlib/mcu.h"
#include "boxlib/peripheral.h"
#include "boxlib/readLine.h"
#include "boxlib/rs232debug.h"
#include "femtoVsnprintf.h"
#include "main.h"
#include "matrixIo.h"
#include "utility.h"

#define F_CPU 48000000ULL

uint32_t g_cycleTick;

#define CONTROL_NUM 5
#define COLORS_MAX 256

uint32_t g_matrixLines;
uint32_t g_colorMax;
uint8_t g_colors[CONTROL_NUM];
uint32_t g_colorData[COLORS_MAX];

void MatrixHelp(void) {
	printf("h: Print help\r\n");
	printf("r: Reset\r\n");
	printf("0: Simply toggle through the colors\r\n");
	printf("1: PWM brightness control\r\n");
}

void AppInit(void) {
	LedsInit();
	Led1Yellow();
	McuClockToHsiPll(F_CPU, RCC_HCLK_DIV1);
	PeripheralPowerOn();
	Rs232Init();
	printf("\r\nLed matrix tester %s\r\n", APPVERSION);
	printf("h: Print help\r\n");
	Rs232Flush();
	KeysInit();
	CoprocInit();
	PeripheralInit();
	MatrixGpioInit();
	Led1Off();
	g_cycleTick = HAL_GetTick();
}

void ExecReset(void) {
	printf("Reset selected\r\n");
	Rs232Flush();
	NVIC_SystemReset();
}

void MatrixSimple(void) {
	printf("Press any key to exit mode\r\n");
	uint8_t counter = 0;
	while (Rs232GetChar() == '\0') {
		if (counter == 7) {
			counter = 0;
		}
		counter++;
		MatrixColorSet(counter);
		printf("\r%u\r", (unsigned int)counter);
		MatrixColorSet(counter);
		HAL_Delay(1000);
		CoprocWatchdogReset();
	}
}

void MatrixControlPrint(void) {
	for (uint32_t i = 0; i < CONTROL_NUM; i++) {
		printf("%03u", g_colors[i]);
		if (i < CONTROL_NUM) {
			printf(" ");
		}
	}
	printf("\r");
}

static void MatrixRecalcData(void) {
	//re-calculate ISR data
	for (uint32_t i = 0; i < g_colorMax; i++) {
		uint32_t colorState = 0;
		for (uint32_t j = 0; j < CONTROL_NUM; j++) {
			if (g_colors[j] > i) {
				colorState |= 1 << j;
			}
		}
		g_colorData[i] = colorState;
	}
}

static void MatrixUpdateColor(uint32_t idx, int32_t direction) {
	int32_t value = g_colors[idx] + direction;
	int32_t colMax = g_colorMax;
	g_colors[idx] = MAX(MIN(colMax, value), 0);
	MatrixRecalcData();
}

void MatrixControl(void) {

	//The watchdog collides with the implementation of ReadSerialLine
	uint16_t watchdogState = CoprocReadWatchdogCtrl();
	CoprocWatchdogCtrl(0);
	char buffer[10];
	printf("Enter number of different colors (min 2, max 256):\r\n");

	ReadSerialLine(buffer, sizeof(buffer));
	g_colorMax = atoi(buffer);

	printf("Enter number of matrix lines (min 1):\r\n");
	ReadSerialLine(buffer, sizeof(buffer));
	g_matrixLines = atoi(buffer);

	printf("Enter refresh rate in Hz (min 1):\r\n");
	ReadSerialLine(buffer, sizeof(buffer));
	uint32_t frameRate = atoi(buffer);

	CoprocWatchdogCtrl(watchdogState); //restore original value

	printf("MaxColors: %u, Lines: %u, rate: %uHz\r\n", (unsigned int)g_colorMax, (unsigned int)g_matrixLines, (unsigned int)frameRate);

	if ((g_colorMax < 2) || (g_colorMax > COLORS_MAX)) {
		printf("Out of range\r\n");
		return;
	}
	g_colorMax--; //because we need 15 ISRs if we want 16 different colors.

	uint32_t frequency = g_colorMax * g_matrixLines * frameRate;
	if (frequency == 0) {
		printf("Invalid input\r\n");
		return;
	}
	uint32_t isrEveryCycle = F_CPU / frequency;
	if ((isrEveryCycle < 200) || (isrEveryCycle > (1<<16))) {
		printf("Values not supported\r\n");
		return;
	}
	printf("Create an ISR every %u ticks - %uHz\r\n", (unsigned int)isrEveryCycle, (unsigned int)frequency);
	printf("s/x: out1 +- brighness\r\n");
	printf("d/c: out2 +- brighness\r\n");
	printf("f/v: out3 +- brighness\r\n");
	printf("g/b: out4 +- brighness\r\n");
	printf("h/n: out5 +- brighness\r\n");
	printf("j/m: all +- brighness\r\n");
	printf("q: quit to main menu\r\n");
	for (uint32_t i = 0; i < CONTROL_NUM; i++) {
		g_colors[i] = g_colorMax / 2;
	}
	MatrixRecalcData();
	MatrixTimerInit(isrEveryCycle);
	MatrixControlPrint();
	char c;
	uint32_t watchdog = 0;
	do {
		c = Rs232GetChar();
		if (c) {
			uint32_t idx = 0;
			int8_t dir = 0;
			if (c == 's') {
				idx = 0;
				dir = 1;
			}
			if (c == 'x') {
				idx = 0;
				dir = -1;
			}
			if (c == 'd') {
				idx = 1;
				dir = 1;
			}
			if (c == 'c') {
				idx = 1;
				dir = -1;
			}
			if (c == 'f') {
				idx = 2;
				dir = 1;
			}
			if (c == 'v') {
				idx = 2;
				dir = -1;
			}
			if (c == 'g') {
				idx = 3;
				dir = 1;
			}
			if (c == 'b') {
				idx = 3;
				dir = -1;
			}
			if (c == 'h') {
				idx = 4;
				dir = 1;
			}
			if (c == 'n') {
				idx = 4;
				dir = -1;
			}
			if (c == 'j') {
				idx = CONTROL_NUM;
				dir = 1;
			}
			if (c == 'm') {
				idx = CONTROL_NUM;
				dir = -1;
			}
			if (dir) {
				if (idx >= CONTROL_NUM) {
					for (uint32_t i = 0; i < CONTROL_NUM; i++) {
						MatrixUpdateColor(i, dir);
					}
				} else {
					MatrixUpdateColor(idx, dir);
				}
			}
			MatrixControlPrint();
		}
		watchdog++;
		if (watchdog == 100) {
			CoprocWatchdogReset();
			watchdog = 0;
		}
	} while (c != 'q');
	MatrixTimerStop();
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
			MatrixHelp();
		} else if (input == 'r') {
			ExecReset();
		}
		if (input == '0') {
			MatrixSimple();
		}
		if (input == '1') {
			MatrixControl();
		}
	}
	uint32_t cycleTickLast = g_cycleTick;
	g_cycleTick++; //next call expected tick value
	uint32_t tick;
	do {
		tick = HAL_GetTick();
	} while ((tick < g_cycleTick) && (tick >= cycleTickLast));
}
