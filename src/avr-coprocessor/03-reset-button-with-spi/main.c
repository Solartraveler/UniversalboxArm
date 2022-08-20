/*UniversalboxARM - AVR coprocessor
  (c) 2021 by Malte Marwedel
  www.marwedels.de/malte

  This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  Features:
  Implementation to reset the ARM and select the boot mode
  Hold down the right key by 1 s to toggle the boot pin
  Hold down the left key by 1 s to generate a reset.
  Also allows communication with ARM processor over SPI.

  The following commands are supported:
  Test read
  Read version
  Read vcc
  Reboot
  Signal LED

  The minium clock of the SPI is 10Hz, the maximum ~1kHz.
  If there is no clock for 150ms, the SPI resets its state to the first bit.
  If a write command (beginning with 0x80) has been sent, there should be a
  delay of at least 20ms until the next write command is sent.

  The LED signals:
  1s on on power-on to signal operation
  Two short flashes to signal setting the boot option to bootloader mode
       (high pin)
  One short flash to signal setting the boot option to normal boot (low pin)
  LED gets on after holding the left key for 1s, the reset pin is released as
  soon as the button is released.

  Pin connection:
  PINA.0 = Input, AVR DI
  PINA.1 = Output, AVR DO and boot pin state (high state = bootloader)
  PINA.2 = Input, AVR SCK
  PINA.3 = Input, AREF
  PINA.4 = Input, Battery voltage measurement
  PINA.5 = Input, Vcc measurement
  PINA.6 = Input, Voltage drop for charge current measurement
  PINA.7 = Output, pull high to set ARM into reset state.
  PINB.0 = With PCB fix: pull down to power the ARM by battery. (MOSI for ISP)
         = Without PCB fix: Input, Non working battery temperature measurement (MOSI for ISP)
  PINB.1 = Output, User LED (MISO for ISP)
  PINB.2 = Input with pullup, right key (SCK for ISP)
  PINB.3 = Output, PWM for battery charging (OC1B). Set to 0V to disable charging
  PINB.4 = Output, Power save option, set high to enable temperature and battery
                   voltage measurement, low disconnects the voltage dividers
  PINB.5 = With PCB Fix: Input, battery temperature measurement
           Without PC fix: Output, pull down to power the ARM by battery.
  PINB.6 = Input with pullup, left key (allows wakeup from power down by Int0)
           If workaround for teperature sensor is applied: Input without pullup,
           temperature sensor can be read as long as key is not pressed
  PINB.7 = reset pin

  Set F_CPU to 128kHz and the fuses to use the internal 128kHz oscillator.

  WARNING: While an ISP programmer is connected, the right button may not be
           pressed, as this would short circuit SCK of the programmer to ground.
           With the workaround for the temperature sensor, the same is true for
           the left button too.

  If no temperature measurement is needed, the PCB fix can be skipped.
  This program drives both pins PB0 and PB5 and therefore can be used for both
  variants.

*/

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "hardware.h"
#include "timing.h"
#include "spi.h"
#include "coprocCommands.h"

int main(void) {
	HardwareInit(); //this sets the ARM to the reset state
	LedOn();
	TimerInit(false);
	waitms(50);
	ArmRun();
	waitms(950);
	LedOff();
	//otherwise the boot pin may not be set to high if no external
	//Vcc is applied (battery):
	ArmBatteryOn();
	SensorsOn();
	SpiInit();
	SpiDataSet(CMD_VERSION, 0x0303); //03 for the program (folder name), 3 for the version
	uint8_t pressedLeft = 0, pressedRight = 0;
	uint8_t resetHold = 0;
	uint8_t armNormal = 1;
	uint8_t ledFlashCycle = 0;
	uint8_t ledFlashRemaining = 0;
	uint8_t ledFlashOnSpiCommand = 1;
	uint8_t adcCycle = 49;
	uint8_t reinitSpiDelay = 0;
	uint16_t watchdogHighest = 0; // 0 = disabled
	uint16_t watchdogCurrent = 0;
	for (;;) { //Main loop, we run every 10ms
		if (KeyPressedLeft()) {
			if (pressedLeft < 255) {
				pressedLeft++;
			}
			if (pressedLeft == 100) {
				resetHold = 20; //200ms reset signal
				LedOn();
				if (armNormal) {
					ArmUserprog(); //could have been overwritten by corpcCommand
				} else {
					ArmBootload();
				}
				reinitSpiDelay = 200;
				SpiDisable();
				ArmReset();
			}
		} else {
			pressedLeft = 0;
			if (resetHold) {
				resetHold--;
				if (resetHold == 0) {
					ArmRun();
					LedOff();
				}
			}
		}
		if (KeyPressedRight()) {
			if (pressedRight < 255) {
				pressedRight++;
			}
			if (pressedRight == 100) {
				armNormal = 1 - armNormal;
				if (armNormal) {
					ArmUserprog();
					ledFlashRemaining = 2; //1x flash
				} else {
					ArmBootload();
					ledFlashRemaining = 4; //2x flash
				}
			}
		} else {
			pressedRight = 0;
		}
		if (reinitSpiDelay) {
			reinitSpiDelay--;
			if (reinitSpiDelay == 0) {
				SpiInit();
			}
		}
		if ((reinitSpiDelay == 0) && (SpiProcess())) {
			uint16_t parameter;
			uint8_t command = SpiCommandGet(&parameter);
			if (ledFlashOnSpiCommand) {
				ledFlashRemaining = 2;
			}
			if (command == CMD_REBOOT) {
				bool rebootValid = false;
				if (parameter == 0xA600) {
					rebootValid = true;
				}
				if (parameter == 0xA601) {
					ArmUserprog();
					rebootValid = true;
				}
				if (parameter == 0xA602) {
					ArmBootload();
					rebootValid = true;
				}
				if (rebootValid) {
					ArmReset();
					resetHold = 20; //to be released in 200ms
				}
			} else if (command == CMD_LED) {
				ledFlashOnSpiCommand = parameter & 1;
			} else if (command == CMD_WATCHDOG_CTRL) {
				watchdogHighest = parameter;
				watchdogCurrent = watchdogHighest;
			} else if ((command == CMD_WATCHDOG_RESET) && (parameter == 0x42)) {
				watchdogCurrent = watchdogHighest;
			}
		}
		if (watchdogCurrent) {
			if (watchdogCurrent > 10) {
				watchdogCurrent -= 10;
			} else {
				watchdogCurrent = 0;
				ArmReset();
				resetHold = 20; //to be released in 200ms
			}
		}
		//update every 100ms
		if (ledFlashRemaining) {
			ledFlashCycle++;
			if (ledFlashCycle == 10) {
				ledFlashRemaining--;
				ledFlashCycle = 0;
				if (ledFlashRemaining & 1) {
					LedOn();
				} else {
					LedOff();
				}
			}
		}
		//update every 500ms
		adcCycle++;
		if (adcCycle == 50) {
			uint16_t vcc = SensorsInputvoltageGet();
			SpiDataSet(CMD_VCC, vcc);
			adcCycle = 0;
		}
		WatchdogReset();
		while (TimerHasOverflown() == false);
	}
}
