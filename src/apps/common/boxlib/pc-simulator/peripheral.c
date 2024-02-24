/* Boxlib emulation
(c) 2022 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause
*/

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "boxlib/peripheral.h"

#include "boxlib/lcd.h"
#include "boxlib/flash.h"

void PeripheralInit(void) {
}

void PeripheralPowerOn(void) {
}

void PeripheralPowerOff(void) {
	LcdDisable();
	FlashDisable();
}

void PeripheralTransfer(const uint8_t * dataOut, uint8_t * dataIn, size_t len) {
	(void)dataOut;
	(void)dataIn;
	(void)len;
}

void PeripheralPrescaler(uint32_t prescaler) {
	(void)prescaler;
}

