/* Make peripheral of Boxlib threadsafe
(c) 2021 by Malte Marwedel

SPDX-License-Identifier: BSD-3-Clause

It is not sufficient to lock the individual calls within peripheral.c, because
things linke chip select and setting the clock divisor are multiple calls
to peripheral.c and need to be within one lock.

*/

#include <stdint.h>

#include "peripheralMt.h"

#include "boxlib/peripheral.h"
#include "FreeRTOS.h"
#include "semphr.h"

SemaphoreHandle_t g_peripheralSemaphore;
StaticSemaphore_t g_peripheralSemaphoreState;

void PeripheralInitMt(void) {
	g_peripheralSemaphore = xSemaphoreCreateMutexStatic(&g_peripheralSemaphoreState);
	PeripheralInit();
}

void PeripheralLockMt(void) {
	//Simply must succeed
	while (xSemaphoreTake(g_peripheralSemaphore, 1000) == pdFALSE);
}

void PeripheralUnlockMt(void) {
	xSemaphoreGive(g_peripheralSemaphore);
}

