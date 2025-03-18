/* Battery capacity tester
(c) 2024 by Malte Marwedel

SPDX-License-Identifier: GPL-3.0-or-later

Input pins to use for ADC

ADC Channel STM32L452
1           PC0      first port with a 10:1 divider
2           PC1      second port witth a 10:1 divider
Relay 1     PA9      enables discharge on first port
Relay 2     PB12     enables discharge on second port
*/

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>

#include "capTester.h"

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

#include "femtoVsnprintf.h"
#include "filesystem.h"
#include "gui.h"
#include "io.h"
#include "utility.h"

#define BATTERIES 2

typedef struct {
	bool en[BATTERIES]; //if output is active right now
	bool userEn[BATTERIES]; //if output should be active at the start
	uint8_t cntDown[BATTERIES]; //[s] do not measure right after relay enable, as the relay has some delay
	uint32_t switchOffMv[BATTERIES]; //[mV]
	uint32_t rMohm[BATTERIES]; //[mOhm]
	uint32_t tStart[BATTERIES]; //[ms]
	uint32_t tStop[BATTERIES]; //[ms]
	uint32_t u[BATTERIES]; //[mV]
	uint32_t uMax[BATTERIES]; //[mV]
	uint32_t uMaxIdle[BATTERIES]; //[mV]
	uint64_t c[BATTERIES]; //[mAs]
	uint64_t e[BATTERIES]; //[mWs]
} capState_t;

uint32_t g_cycleTick;

capState_t g_capState;


void AppHelp(void) {
	printf("h: Print help\r\n");
	printf("r: Reset\r\n");
	printf("wasd: Input keys\r\n");
}

void AppInit(void) {
	LedsInit();
	Led2Red();
	PeripheralPowerOn();
	Rs232Init();
	printf("\r\nBattery capacity tester %s\r\n", APPVERSION);
	printf("h: Print help\r\n");
	Rs232Flush();
	KeysInit();
	CoprocInit();
	PeripheralInit();
	FlashEnable(8); //2MHz
	FilesystemMount();
	GuiInit();
	SinkInit();
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
		CapCheck();
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

void CapUpdateLeds(void) {
	bool enabled1 = g_capState.en[0];
	bool enabled2 = g_capState.en[1];
	if ((enabled1) && (enabled2)) {
		Led1Yellow();
	} else if (enabled1) {
		Led1Green();
	} else if (enabled2) {
		Led1Red();
	} else {
		Led1Off();
	}
}

void CapStart(uint8_t battery, bool enabled, uint32_t rMohm, uint32_t offMv) {
	if (battery >= BATTERIES) {
		return;
	}
	if (enabled) {
		printf("Enabling sink %u\r\n", (unsigned int)battery);
	}
	g_capState.en[battery] = enabled;
	g_capState.userEn[battery] = enabled;
	g_capState.switchOffMv[battery] = offMv;
	g_capState.rMohm[battery] = rMohm;
	g_capState.tStop[battery] = g_capState.tStart[battery] = HAL_GetTick();
	g_capState.c[battery] = 0;
	g_capState.e[battery] = 0;
	g_capState.u[battery] = 0;
	g_capState.uMax[battery] = 0;
	g_capState.cntDown[battery] = 1;
	g_capState.uMaxIdle[battery] = AdcVoltage(battery) * 1000.0f;
	SinkSet(battery, enabled);
	CapUpdateLeds();
}

bool CapDataGet(uint8_t battery, uint32_t * t, uint32_t * u, uint32_t * e) {
	if (battery >= BATTERIES) {
		return false;
	}
	uint32_t tick = HAL_GetTick();
	if (g_capState.en[battery]) {
		*t = tick - g_capState.tStart[battery];
	} else {
		*t = g_capState.tStop[battery] - g_capState.tStart[battery];
	}
	*t /= 1000; //[ms] -> [s]
	*u = g_capState.u[battery];
	*e = g_capState.e[battery] / (60ULL * 60ULL); //mWs -> mWh
	return g_capState.en[battery];
}

void CapStop(void) {
	for (uint8_t battery = 0; battery < BATTERIES; battery++) {
		if (g_capState.en[battery]) {
			printf("Disabling sink %u by user request\r\n", (unsigned int)battery);
			g_capState.en[battery] = false;
			SinkSet(battery, false);
			g_capState.tStop[battery] = HAL_GetTick();
		}
	}
	CapUpdateLeds();
}

void CapSave(void) {
	char text[1024];
	char filename[128];
	text[0] = '\0';
	printf("\r\n");
	for (uint8_t battery = 0; battery < BATTERIES; battery++) {
		size_t used = strlen(text);
		if (g_capState.userEn[battery]) {
			unsigned int rl = g_capState.rMohm[battery];
			unsigned int u = g_capState.u[battery];
			unsigned int umax = g_capState.uMax[battery];
			unsigned int i = u * 1000UL / rl;
			unsigned int imax = umax * 1000UL / rl;
			unsigned int c = g_capState.c[battery] / (60ULL * 60ULL);
			unsigned int pmax = umax * umax / rl;
			uint32_t stot;
			if (g_capState.en[battery]) {
				stot = HAL_GetTick() - g_capState.tStart[battery];
			} else {
				stot = g_capState.tStop[battery] - g_capState.tStart[battery];
			}
			printf("%u-%u-%u-%u\r\n", (unsigned int)HAL_GetTick(), (unsigned int)g_capState.tStart[battery], (unsigned int)g_capState.tStop[battery], (unsigned int)stot);
			stot /= 1000; //[ms] -> [s]
			unsigned int h = stot / (60UL * 60UL);
			unsigned int m = stot / 60UL % 60UL;
			unsigned int s = stot % 60;
			unsigned int e = g_capState.e[battery] / (60ULL * 60ULL);
			unsigned int iAvg = g_capState.c[battery] / stot; //in [mA]
			unsigned int uCable = 0;
			unsigned int rCable = 0;
			unsigned int eCable = 0;
			if (g_capState.uMaxIdle[battery] >= g_capState.uMax[battery]) {
				uCable = g_capState.uMaxIdle[battery] - g_capState.uMax[battery]; //in [mV]
				/*This assumes the maximum current correspondents with the maximum voltage, which is true as long as the load
				  did not change during the discharge.
				*/
				rCable = uCable * 1000UL / imax; //in [mΩ]
				unsigned int pCable = uCable * iAvg / 1000UL; //in [mW]
				eCable = pCable * stot / (60ULL * 60ULL); ; //in [mWh]
			}
			unsigned int eTot = e + eCable;
			snprintf(text + used, sizeof(text) - used,
			 "Discharger input %u\r\n  RLoad=%umΩ\r\n  U=%umV\r\n  I=%umA\r\n  ULoadMax=%umV\r\n  ILoadMax=%umA\r\n  ILoadAvg=%umA\r\n  Pmax=%umW\r\n  T=%u:%02u:%02u (%us)\r\n  C=%umAh\r\n  E=%umWh\r\n  Ucable=%umV\r\n  Rcable=%umΩ\r\n  Ecable=%umWh\r\n  Etotal=%umWh\r\n\r\n",
			battery, rl, u, i, umax, imax, iAvg, pmax, h, m, s, (unsigned int)stot, c, e, uCable, rCable, eCable, eTot);
		} else {
			snprintf(text + used, sizeof(text) - used,
			 "Discharger input %u\r\n  never enabled\r\n\r\n", battery);
		}
		printf("%s", text + used);
	}
	f_mkdir("/logs");
	uint32_t unusedId = FilesystemGetUnusedFilename("/logs", "discharge");
	snprintf(filename, sizeof(filename), "/logs/discharge%04u.txt", (unsigned int)unusedId);
	size_t l = strlen(text);
	if (FilesystemWriteFile(filename, text, l)) {
		printf("-------> Log with %u bytes written to %s\r\n", (unsigned int)l, filename);
	} else {
		printf("-------> Error, writing log to %s failed\r\n", filename);
	}
}

//Function is called every second once
void CapCheck(void) {
	for (uint8_t battery = 0; battery < BATTERIES; battery++) {
		uint32_t umv = AdcVoltage(battery) * 1000.0f;
		//printf("%u-%u\n", battery, umv);
		g_capState.u[battery] = umv;
		if (g_capState.en[battery]) {
			if (g_capState.cntDown[battery] == 0) {
				uint32_t p = umv * umv / g_capState.rMohm[battery]; //[mW]
				g_capState.c[battery] += umv * 1000UL / g_capState.rMohm[battery] ; //add every second the currently discharged mA
				g_capState.e[battery] += p; //add every second the currently discharged mW
				g_capState.uMax[battery] = MAX(g_capState.uMax[battery], umv);
				if (umv <= g_capState.switchOffMv[battery]) {
					printf("Disabling sink %u because of low voltage\r\n", (unsigned int)battery);
					SinkSet(battery, false);
					g_capState.en[battery] = false;
					g_capState.tStop[battery] = HAL_GetTick();
				}
			} else {
				g_capState.cntDown[battery]--;
			}
		}
	}
	CapUpdateLeds();
}
