/* ADC scope
(c) 2024 by Malte Marwedel

SPDX-License-Identifier: GPL-3.0-or-later
*/

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>

#include "adcScope.h"

#include "boxlib/adcDma.h"
#include "boxlib/coproc.h"
#include "boxlib/flash.h"
#include "boxlib/keys.h"
#include "boxlib/lcd.h"
#include "boxlib/leds.h"
#include "boxlib/mcu.h"
#include "boxlib/peripheral.h"
#include "boxlib/readLine.h"
#include "boxlib/rs232debug.h"

#include "main.h"

#include "adcSample.h"
#include "femtoVsnprintf.h"
#include "filesystem.h"
#include "gui.h"
#include "screenshot.h"
#include "utility.h"

uint32_t g_cycleTick;

void AppHelp(void) {
	printf("h: Print help\r\n");
	printf("wasd: Input keys\r\n");
	printf("t: Single shot trigger\r\n");
	printf("x: Save screenshot\r\n");
	printf("b: Make ADC benchmark\r\n");
	printf("o: Test the ADC\r\n");
	printf("p: Print ADC registers\r\n");
	printf("r: Reset\r\n");
}

void AppInit(void) {
	LedsInit();
	Led2Red();
	PeripheralPowerOff();
	McuClockToHsiPll(80000000, RCC_HCLK_DIV1);
	HAL_Delay(100);
	PeripheralPowerOn();
	Rs232Init();
	printf("\r\nADC scope %s\r\n", APPVERSION);
	printf("h: Print help\r\n");
	Rs232Flush();
	KeysInit();
	CoprocInit();
	PeripheralInit();
	FlashEnable(32); //2.5MHz
	FilesystemMount();
	SampleInit();
	GuiInit();
	Led2Green();
	g_cycleTick = HAL_GetTick();
}

void ExecReset(void) {
	printf("Reset selected\r\n");
	Rs232Flush();
	NVIC_SystemReset();
}

void AppCycle(void) {
	static uint32_t ledCycle = 0;
	static uint32_t guiTickPerformance = 0;
	//led flash
	if (ledCycle < 500) {
		uint32_t delta = HAL_GetTick() - g_cycleTick;
		if (delta > 500) {
			//indicate we are behind in time
			Led2Red();
		} else {
			//flash green - everything is ok
			Led2Green();
		}
	} else {
		Led2Off();
	}
	if (ledCycle >= 1000) {
		ledCycle = 0;
		//guiTickPerformance = 0; //enable to get a measurement every second, and not just the all time worstcase
		CoprocWatchdogReset();
	}
	ledCycle++;
	char input = Rs232GetChar();
	if (input) {
		printf("%c", input);
		if (input == 'h') {
			AppHelp();
		}
		if (input == 'r') {
			ExecReset();
		}
		if (input == 'x') {
			Screenshot();
		}
		if (input == 't') {
			SampleStart();
		}
		if (input == 'b') {
			SampleAdcPerformanceTest();
			//the right inputs need to be reinitialized by the user after this call!
		}
		if (input == 'o') {
			if (SampleAdcTest()) {
				printf("...Looks good\r\n");
			} else {
				printf("...Failed\r\n");
			}
		}
		if (input == 'p') {
			AdcPrintRegisters(NULL);
		}
	}
	uint32_t tStart = HAL_GetTick();
	GuiCycle(input);
	uint32_t tStop = HAL_GetTick();
	uint32_t delta = tStop - tStart;
	if (delta > guiTickPerformance) {
		printf("Gui redraw needs up to %ums\r\n", (unsigned int)delta);
		guiTickPerformance = delta;
	}
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
