/* Boxlib emulation
(c) 2022 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause
*/


#include <stdint.h>

#include "simpleadc.h"


void AdcInit(void) {
}

uint16_t AdcGet(uint32_t channel) {
	uint16_t val = 0;
	if (channel >= 32) {
		return 0xFFFF; //impossible value
	}
	//TODO: Do return useful values for some channels (temperature, voltage)
	return val;
}

void AdcStop(void) {
}
