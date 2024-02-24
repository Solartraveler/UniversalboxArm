/* Boxlib emulation
(c) 2022 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause
*/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#include "boxlib/mcu.h"

#include "simulated.h"


void McuStartOtherProgram(void * startAddress, bool ledSignalling) {
	(void)ledSignalling;
	printf("Would start program with first instruction at %p. But we just exit now.\n", startAddress);
	SimulatedDeinit();
	exit(0);
}

bool McuClockToMsi(uint32_t frequency, uint32_t apbDivider) {
	(void)frequency;
	(void)apbDivider;
	return true;
}

uint8_t McuClockToHsiPll(uint32_t frequency, uint32_t apbDivider) {
	(void)frequency;
	(void)apbDivider;
	return 0;
}

void McuLockCriticalPins(void) {
}

uint64_t McuTimestampUs(void) {
	struct timespec t = {0};
	clock_gettime(CLOCK_MONOTONIC, &t);
	return (t.tv_sec * 1000000ULL) + (t.tv_nsec / 1000ULL);
}

void McuDelayUs(uint32_t us) {
	usleep(us);
}
