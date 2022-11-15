/* Boxlib
(c) 2022 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause
*/

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <stdio.h>

#include "clock.h"


bool ClockInit(void) {
	return true;
}

void ClockDeinit(void) {
}

uint32_t ClockUtcGet(uint16_t * pMiliseconds) {
	struct timespec t = {0};
	clock_gettime(CLOCK_REALTIME, &t);
	if (pMiliseconds) {
		*pMiliseconds = t.tv_nsec / 1000000;
	}
	return t.tv_sec;
}

uint8_t ClockSetTimestampGet(uint32_t * pUtc, uint16_t * pMiliseconds) {
	*pUtc = 0;
	*pMiliseconds =  0;
	return 0;
}

bool ClockUtcSet(uint32_t timestamp, uint16_t miliseconds, bool precise, int64_t * pDelta) {
	(void)precise;
	struct timespec t = {0};
	clock_gettime(CLOCK_REALTIME, &t);
	int64_t delta = (int64_t)timestamp - (int64_t)t.tv_sec;
	delta *= 1000;
	delta += (int64_t)miliseconds - ((int64_t)t.tv_nsec / 1000000);
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

void ClockPrintRegisters(void) {
}
