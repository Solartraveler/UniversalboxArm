/*UniversalboxARM - AVR coprocessor
  (c) 2022 by Malte Marwedel
  www.marwedels.de/malte

SPDX-License-Identifier:  BSD-3-Clause


  Pin connection:
  PINB.1 = Output, User LED (MISO for ISP)

  Set F_CPU to 100KHz ... 128kHz and the fuses to use the internal 128kHz oscillator.

  Checks if the DI input level can be properly read. The input level can be set
  with the 02-test-everything project from the ARM side.

  Currently this test turned out to *not* find a bug. Is a test for the issue
  documented in hardware.h function PinsPowerdown


*/

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "configuration.h"

#include "hardware.h"


static void LedError(uint8_t error) {
	LedOff();
	waitms(500);
	for (uint8_t i = 0; i < error; i++) {
		LedOn();
		waitms(250);
		LedOff();
		waitms(250);
	}
	waitms(500);
}

int main(void) {
	HardwareInit();
	LedOn();
	waitms(50);
	LedOff();
	ArmRun();
	ArmBatteryOn();
	SensorsOn();
	const uint8_t patternA = 0xFF;
	const uint8_t patternB = (1<<1) | (1<<3) | (1<<4) | (1<<5) | (1<<6) | (1<<7);
	for (;;) {
		DIDR0 = patternA;
		waitms(1);
		uint8_t didrA = DIDR0;
		DIDR0 = patternB;
		waitms(1);
		uint8_t didrB = DIDR0;
		bool diLevel = SpiDiLevel();
		if (didrA != patternA) {
			LedError(1);
		}
		if (didrB != patternB) {
			LedError(2);
		}
		if (diLevel != true) {
			LedError(3);
		}
		LedOn();
		waitms(50); //show the AVR is still running
		LedOff();
		waitms(500);
	}
}
