/*UniversalboxARM - AVR coprocessor
  (c) 2021-2022 by Malte Marwedel
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
  Hold down the right key by 1s to toggle the boot pin
  Hold down the left key by 1s to generate a reset.
  When running on battery:
  Hold down the left key and than the right key at the same time by 1s to power
    down the ARM.
  Hold down the right and left at the same time to power up the ARM.
  Also allows communication with ARM processor over SPI.
  And adds charger control logic for the LiFePo cell.

  The following commands are supported:
  All written in coprocCommands.h

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
  If the battery is charging, the LED is inverted. So it lights up when charged
  and flashes off when pressing the keys or running SPI commands.

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

 TODO: Save power by going to sleep mode when waiting for a timer
       Use interrupts of the keys to wake up from deep power down

*/

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include <avr/eeprom.h>
#include <util/crc16.h>

#include "hardware.h"
#include "timing.h"
#include "spi.h"
#include "coprocCommands.h"
#include "chargerStatemachine.h"

#define VCC_CONNECTED 3200

typedef struct {
	uint8_t chargerError;
	uint32_t chargingCycles;
	uint32_t prechargingCycles;
	uint64_t chargingSumAllTimes;
	uint32_t opTotal; //time in [minutes]
	uint16_t crc; //must be the last in the struct
} persistent_t;

persistent_t EEMEM g_settingsEepromA;

//some space holder, so the delete - write algorithm does only operate on one copy
uint32_t EEMEM g_eepromDummy;

persistent_t EEMEM g_settingsEepromB;

static uint16_t CrcCalc(const uint8_t * input, size_t len) {
	uint16_t crc = 0xFFFF;
	for (size_t i = 0; i < len; i++) {
		crc = _crc16_update(crc, input[i]);
	}
	return crc;
}

static bool SettingsLoadSub(persistent_t * pSettings, persistent_t * pSource) {
	eeprom_read_block(pSettings, pSource, sizeof(persistent_t));
	uint16_t crc = CrcCalc((uint8_t *)pSettings, sizeof(persistent_t) - sizeof(uint16_t));
	if (pSettings->crc == crc) {
		return true;
	}
	return false;
}

static void SettingsLoad(persistent_t * pSettings) {
	if (!SettingsLoadSub(pSettings, &g_settingsEepromA)) {
		if (!SettingsLoadSub(pSettings, &g_settingsEepromB)) {
			memset(pSettings, 0, sizeof(persistent_t));
		}
	}
}

static void SettingsSave(persistent_t * pSettings) {
	uint16_t crc = CrcCalc((uint8_t *)pSettings, sizeof(persistent_t) - sizeof(uint16_t));
	pSettings->crc = crc;
	eeprom_update_block(pSettings, &g_settingsEepromA, sizeof(persistent_t));
	eeprom_update_block(pSettings, &g_settingsEepromB, sizeof(persistent_t));
}

//just switch everything off
static void DeepDischargePrevention(void) {
	SensorsOff();
	TimerStop();
	WatchdogDisable();
	WaitForExternalInterrupt(); //get out by a keypress only
	HardwareInit();
	TimerInit(true);
}

//if alarm is non 0, its a timout in [s] until a wakeup of the ARM CPU occurrs
//after the return of the function, reinitSpiDelay should be set
static void PowerDownLoop(uint16_t alarm) {
	uint8_t checkInputVoltage = 0;
	uint8_t checkBatteryVoltage = 0;
	uint8_t checkAlarm = 0;
	//we must first release the keys, otherwise the power up will be instantly follow a power down
	uint8_t powerByKeyState = 0;
	PwmBatterySet(0);
	SpiDisable();
	SensorsOff();
	ArmReset();
	ArmUserprog(); //must be set low, before battery is powered off!
	ArmBatteryOff();
	waitms(10); //make sure ARM finally is in reset state and capacitors are discharged
	ArmRun(); //the low level saves some power, so we do not drive against the pull-down resistor.
	LedOff();
	while (1) { //loop runs every 10ms
		//wakeup by input voltage -> USB powered
		if (checkInputVoltage == 0) { //check every 500ms
			checkInputVoltage = 50;
			SensorsOn();
			waitms(1);
			uint16_t inputVoltage = SensorsInputvoltageGet();
			if (inputVoltage >= VCC_CONNECTED) {
				break;
			}
			if (checkBatteryVoltage == 0) { //check every 10s
				checkBatteryVoltage = 20;
				uint16_t batteryVoltage = SensorsBatteryvoltageGet();
				if (batteryVoltage <= 2500) {
					DeepDischargePrevention();
				}
			}
			checkBatteryVoltage--;
			SensorsOff();
		}
		checkInputVoltage--;
		//wakeup by alarm
		if (alarm) {
			if (checkAlarm == 0) { //1s passed
				alarm--;
				if (alarm == 0) {
					break; //wakeup
				}
				checkAlarm = 100;
			}
			checkAlarm--;
		}
		//wakeup by keypress, minimum 500ms, without keypress and then 500ms with keypress
		if ((powerByKeyState < 50) && (KeyPressedLeft() == 0) && (KeyPressedRight() == 0)) {
			powerByKeyState++;
		} else if ((powerByKeyState >= 50) && (powerByKeyState < 100)) {
			if ((KeyPressedLeft()) && (KeyPressedRight())) {
				powerByKeyState++;
			} else {
				powerByKeyState = 50;
			}
		} else if (powerByKeyState == 100) {
			LedOn();
			if ((KeyPressedLeft() == 0) && (KeyPressedRight() == 0)) {
				break;
			}
		}
		WatchdogReset();
		while (TimerHasOverflownIsr() == false) {
			WaitForInterrupt();
		}
	}
	LedOn();
	SensorsOn();
	ArmReset();
	waitms(1);
	ArmBatteryOn();
	waitms(10);
	ArmRun(); //we still have the pin set to start the usermode application
}

int main(void) {
	HardwareInit(); //this sets the ARM to the reset state
	LedOn();
	TimerInit(true);
	waitms(50);
	ArmRun();
	waitms(950);
	LedOff();
	//otherwise the boot pin may not be set to high if no external
	//Vcc is applied (battery):
	ArmBatteryOn();
	SensorsOn();
	SpiInit();
	SpiDataSet(CMD_VERSION, 0x0502); //05 for the program (folder name), 2 for the version
	uint8_t pressedLeft = 0, pressedRight = 0; //Time the left or right button is hold down [10ms]
	uint8_t resetHold = 0; //count down until the reset of the ARM CPU is released [10ms]
	uint8_t armNormal = 1; //startup mode of the ARM cpu 0: DFU bootloader, 1: normal program start
	uint8_t ledFlashCycle = 0; //controls the speed of the LED flashing, counts up. [10ms]
	uint8_t ledFlashRemaining = 0; //if non 0, the LED is flashing
	uint8_t ledFlashOnSpiCommand = 1; //if 1, each SPI command is confirmed by a LED flash
	uint8_t adcCycle = 0; //cycle for measuring the analog inputs and calculating the battery charger logic
	uint8_t reinitSpiDelay = 0; //if reached 0, the SPI interface is reset
	uint16_t watchdogHighest = 0; //reload value of the ARM cpu watchdog, 0 = disabled [1ms]
	uint16_t watchdogCurrent = 0; //count down value of the ARM CPU watchdog. On a 0, a reset is issured. [1ms]
	uint16_t inU = 0; //measured input voltage [mV]
	uint16_t battU = 0; //measured battery voltage [mV]
	uint16_t battI = 0; //measured battery current [mA]
	int16_t battTemp = 0; //measured battery temperature [0.1°C]
	uint16_t inMax = 70; //maximum allowed charging current [mA]
	uint8_t requestFullCharge = 0; //If set to 1, a full charge is requested
	uint8_t battPwm = 0; //last output PWM value controlling the battery charging current
	uint16_t cycle = 0; //cycle for statistics. Resets every minute
	uint32_t opRunning = 0; //counts the minutes since last power down
	uint16_t minutesSinceSave = 0; //If reached 1day, statistics are saved int eeprom
	uint8_t requestPowerDown = 0; //if 1 the ARM device will be powered down if currently running on battery
	uint16_t batteryWakeupTime = 0; //power up time in [s] when running on battery
	uint8_t batteryMode = 0; //if on battery, 0 = power down, 1 = continue to operate
	uint8_t onUsbPower = 1; //current source for the power. 0 = battery, 1 = USB
	chargerState_t CS;
	persistent_t settings;
	SettingsLoad(&settings);
	SpiDataSet(CMD_OPTIME, settings.opTotal / (60UL * 24UL));
	ChargerInit(&CS, settings.chargerError, settings.chargingCycles, settings.prechargingCycles, settings.chargingSumAllTimes);
	for (;;) { //Main loop, we run every 10ms
		if (KeyPressedLeft()) {
			if (pressedLeft < 255) {
				pressedLeft++;
			}
			if (pressedLeft == 100) { //1s press
				if (KeyPressedRight() == 0) {
					resetHold = 20; //200ms reset signal
					if (armNormal) {
						ArmUserprog(); //could have been overwritten by coprocCommand
					} else {
						ArmBootload();
					}
					reinitSpiDelay = 200;
					SpiDisable();
					ArmReset();
				} else {
					requestPowerDown = 1;
				}
			}
			if (pressedLeft >= 100) {
				ledFlashRemaining = 1;
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
		if ((KeyPressedRight()) && (KeyPressedLeft() == 0)) {
			if (pressedRight < 255) {
				pressedRight++;
			}
			if (pressedRight == 100) { //1s press
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
			} else if ((command == CMD_BAT_CURRENT_MAX) && (parameter <= 200)) {
				inMax = parameter;
			} else if ((command == CMD_BAT_FORCE_CHARGE) && (parameter == 0)) {
				requestFullCharge = 1;
			} else if ((command == CMD_BAT_NEW) && (parameter == 0x1291)) {
				settings.chargerError = 0;
				ChargerInit(&CS, settings.chargerError, settings.chargingCycles, settings.prechargingCycles, settings.chargingSumAllTimes);
				PwmBatterySet(0);
				SettingsSave(&settings);
			} else if ((command == CMD_BAT_STAT_RESET) && (parameter == 0x3344)) {
				settings.chargingCycles = 0;
				settings.prechargingCycles = 0;
				settings.chargingSumAllTimes = 0;
				ChargerInit(&CS, settings.chargerError, settings.chargingCycles, settings.prechargingCycles, settings.chargingSumAllTimes);
				PwmBatterySet(0);
				SettingsSave(&settings);
			} else if ((command == CMD_POWERMODE) && (parameter < 2)) {
				batteryMode = parameter;
			} else if (command == CMD_ALARM) {
				batteryWakeupTime = parameter; // 0 == never
			} else if (command == CMD_POWERDOWN) {
				requestPowerDown = 1;
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
			if (ledFlashCycle == 0) {
				uint8_t light = (ledFlashRemaining & 1) ^ (battPwm > 0);
				if (light) {
					LedOn();
				} else {
					LedOff();
				}
			}
			ledFlashCycle++;
			if (ledFlashCycle == 10) {
				ledFlashRemaining--;
				ledFlashCycle = 0;
			}
		} else if (battPwm) {
			LedOn();
		} else {
			LedOff();
		}
		//resets every 100ms, but each cycle does something different
		adcCycle++;
		if (adcCycle == 1) {
			inU = SensorsInputvoltageGet(); //we should do this in the first cycle!
			SpiDataSet(CMD_VCC, inU);
		}
		if (adcCycle == 2) {
			battU = SensorsBatteryvoltageGet();
			SpiDataSet(CMD_BAT_VOLTAGE, battU);
		}
		if (adcCycle == 3) {
			battI = SensorsBatterycurrentGet();
			SpiDataSet(CMD_BAT_CURRENT, battI);
		}
		if (adcCycle == 4) {
			battTemp = SensorsBatterytemperatureGet();
			SpiDataSet(CMD_BAT_TEMPERATURE, battTemp);
		}
		if (adcCycle == 5) {
			battPwm = ChargerCycle(&CS, battU, inU, battI, battTemp, inMax, 100, requestFullCharge);
			requestFullCharge = 0;
			PwmBatterySet(battPwm);
		}
		if (adcCycle == 6) {
			SpiDataSet(CMD_BAT_CHARGE_STATE, ChargerGetState(&CS));
			SpiDataSet(CMD_BAT_CHARGE_ERR, ChargerGetError(&CS));
			SpiDataSet(CMD_BAT_CHARGED, ChargerGetCharged(&CS) / (60ULL*60ULL)); //mAs -> mAh
			settings.chargingSumAllTimes = ChargerGetChargedTotal(&CS);
			SpiDataSet(CMD_BAT_CHARGED_TOT, settings.chargingSumAllTimes / (60ULL * 60ULL * 1000ULL)); //mAs ->Ah
			settings.chargingCycles = ChargerGetCycles(&CS);
			SpiDataSet(CMD_BAT_CHARGE_CYC, settings.chargingCycles);
			settings.prechargingCycles = ChargerGetPreCycles(&CS);
			SpiDataSet(CMD_BAT_PRECHARGE_CYC, settings.prechargingCycles);
			SpiDataSet(CMD_BAT_PWM, battPwm);
		}
		if (adcCycle == 7) {
			uint16_t cpuTemperature = SensorsChiptemperatureGet();
			SpiDataSet(CMD_CPU_TEMPERATURE, cpuTemperature);
		}
		if (adcCycle == 10) {
			adcCycle = 0;
		}
		//check if we should power down. Triggers when >= 4500mV and then <= 4000mV
		if (inU >= 4500) {
			onUsbPower = 1;
		}
		if (inU <= 4000) {
			if ((onUsbPower) && (batteryMode == 0)) {
				requestPowerDown = 1;
			}
			onUsbPower = 0;
		}
		if ((onUsbPower == 0) && (battU <= 3100) && (battU)) { //prevent full discharge. We may go down to 3V
			requestPowerDown = 1;
		}
		if (requestPowerDown) {
			requestPowerDown = 0;
			if (onUsbPower == 0) {
				PowerDownLoop(batteryWakeupTime);
				reinitSpiDelay = 100;
			}
		}
		//update statistics and save to eeprom
		cycle++;
		if (cycle == (100 * 60)) { //1m passed
			cycle = 0;
			opRunning++;
			SpiDataSet(CMD_UPTIME, opRunning / 60UL);
			settings.opTotal++;
			SpiDataSet(CMD_OPTIME, settings.opTotal / (60UL * 24UL));
			minutesSinceSave++;
			if (minutesSinceSave == (60 * 24)) { //1 day passed
				minutesSinceSave = 0;
				SettingsSave(&settings);
			}
		}
		WatchdogReset();
		while (TimerHasOverflownIsr() == false) {
			WaitForInterrupt();
		}
	}
}
