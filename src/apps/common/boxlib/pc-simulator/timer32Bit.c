/* Boxlib
(c) 2025 by Malte Marwedel

SPDX-License-Identifier: BSD-3-Clause
*/

#include <stdint.h>

#include "boxlib/timer32Bit.h"

#include "main.h"

static uint16_t g_timer32Bit_divider;

void Timer32BitInit(uint16_t prescaler) {
	g_timer32Bit_divider = prescaler + 1;
}

void Timer32BitDeinit(void) {
}

void Timer32BitReset(void) {

}

void Timer32BitStart(void) {

}

void Timer32BitStop(void) {

}

uint32_t Timer32BitGet(void) {
	return 0;
}
