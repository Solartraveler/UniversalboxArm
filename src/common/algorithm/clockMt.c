/* Boxlib
(c) 2022 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause
*/

#include <stdint.h>
#include <stdbool.h>

#include "clockMt.h"

#include "boxlib/clock.h"
#include "FreeRTOS.h"
#include "semphr.h"

#define MUTEX_TIMEOUT 100

SemaphoreHandle_t g_clockSemaphore;
StaticSemaphore_t g_clockSemaphoreState;


void ClockPrintRegistersMt(void) {
	if (xSemaphoreTake(g_clockSemaphore, MUTEX_TIMEOUT)) {
		ClockPrintRegisters();
		xSemaphoreGive(g_clockSemaphore);
	}
}

bool ClockInitMt(void) {
	if (g_clockSemaphore == NULL) {
		g_clockSemaphore = xSemaphoreCreateMutexStatic(&g_clockSemaphoreState);
	}
	bool result = ClockInit();
	xSemaphoreGive(g_clockSemaphore);
	return result;
}

void ClockDeinitMt(void) {
	if (xSemaphoreTake(g_clockSemaphore, MUTEX_TIMEOUT)) {
		ClockDeinit();
		//do not give back the mutex. Another call to init will overwrite the state.
	}
}

uint32_t ClockUtcGetMt(uint16_t * pMiliseconds) {
	uint32_t result = 0;
	if (xSemaphoreTake(g_clockSemaphore, MUTEX_TIMEOUT)) {
		result = ClockUtcGet(pMiliseconds);
		xSemaphoreGive(g_clockSemaphore);
	}
	return result;
}

uint8_t ClockSetTimestampGetMt(uint32_t * pUtc, uint16_t * pMiliseconds) {
	uint8_t result = 0;
	if (xSemaphoreTake(g_clockSemaphore, MUTEX_TIMEOUT)) {
		result = ClockSetTimestampGet(pUtc, pMiliseconds);
		xSemaphoreGive(g_clockSemaphore);
	}
	return result;
}

bool ClockUtcSetMt(uint32_t timestamp, uint16_t miliseconds, bool precise, int64_t * pDelta) {
	bool result = false;
	if (xSemaphoreTake(g_clockSemaphore, MUTEX_TIMEOUT)) {
		result = ClockUtcSet(timestamp, miliseconds, precise, pDelta);
		xSemaphoreGive(g_clockSemaphore);
	}
	return result;
}

int32_t ClockCalibrationGetMt(void) {
	int32_t result = 0;
	if (xSemaphoreTake(g_clockSemaphore, MUTEX_TIMEOUT)) {
		result = ClockCalibrationGet();
		xSemaphoreGive(g_clockSemaphore);
	}
	return result;
}
