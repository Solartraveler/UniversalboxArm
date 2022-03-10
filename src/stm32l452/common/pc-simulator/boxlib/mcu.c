/* Boxlib emulation
(c) 2022 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause
*/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "mcu.h"


void McuStartOtherProgram(void * startAddress, bool ledSignalling) {
	printf("Would start program with first instruction at %p. But we just exit now.\n", startAddress);
	exit(0);
}

bool McuClockToMsi(uint32_t frequency, uint32_t apbDivider) {
	(void)frequency;
	(void)apbDivider;
	return true;
}

void McuLockCriticalPins(void) {
}

