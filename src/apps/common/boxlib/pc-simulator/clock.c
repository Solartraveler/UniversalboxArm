/* Boxlib
(c) 2022 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause
*/

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <stdio.h>

#include "boxlib/clock.h"

#define CLOCK_BACKUP_REGS 32

uint8_t g_lastSetType;
uint32_t g_lastSetTime;
uint32_t g_lastSetMilliseconds;
uint32_t g_backupRegs[CLOCK_BACKUP_REGS];

bool ClockInit(void) {
	return true;
}

void ClockDeinit(void) {
}

uint32_t ClockUtcGet(uint16_t * pMilliseconds) {
	struct timespec t = {0};
	clock_gettime(CLOCK_REALTIME, &t);
	if (pMilliseconds) {
		*pMilliseconds = t.tv_nsec / 1000000;
	}
	return t.tv_sec;
}

uint8_t ClockSetTimestampGet(uint32_t * pUtc, uint16_t * pMilliseconds) {
	if (pUtc) {
		*pUtc = g_lastSetTime;
	}
	if (pMilliseconds) {
		*pMilliseconds = g_lastSetMilliseconds;
	}
	return g_lastSetType;
}

bool ClockUtcSet(uint32_t timestamp, uint16_t milliseconds, bool precise, int64_t * pDelta) {
	g_lastSetTime = timestamp;
	g_lastSetMilliseconds = milliseconds;
	if (precise) {
		g_lastSetType = 2;
	} else {
		g_lastSetType = 1;
	}
	struct timespec t = {0};
	clock_gettime(CLOCK_REALTIME, &t);
	int64_t delta = (int64_t)timestamp - (int64_t)t.tv_sec;
	delta *= 1000;
	delta += (int64_t)milliseconds - ((int64_t)t.tv_nsec / 1000000);
	if (delta > 0) {
		printf("Set time would be %ums in the future, compared to the system time\n", (unsigned int)delta);
	} else if (delta < 0) {
		printf("Set time would be %ums in the past, compared to the system time\n", (unsigned int)-delta);
	} else {
		printf("Set time would be match the the system time\n");
	}
	if (pDelta) {
		*pDelta = delta;
	}
	return true;
}

int32_t ClockCalibrationGet(void) {
	return 0;
}

uint32_t ClockBackupRegGet(uint32_t idx) {
	if (idx < CLOCK_BACKUP_REGS) {
		return g_backupRegs[idx];
	}
	return 0;
}

void ClockBackupRegSet(uint32_t idx, uint32_t value) {
	if (idx < CLOCK_BACKUP_REGS) {
		g_backupRegs[idx] = value;
	}
}

void ClockPrintRegisters(void) {
}
