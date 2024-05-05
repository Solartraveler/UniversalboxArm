/*UniversalboxARM - AVR coprocessor
  (c) 2021-2022, 2024 by Malte Marwedel
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

  The minimum clock of the SPI is 10Hz, the maximum ~1kHz.
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
           If workaround for temperature sensor is applied: Input without pullup,
           temperature sensor can be read as long as key is not pressed
  PINB.7 = reset pin

  Set F_CPU to 128kHz and the fuses to use the internal 128kHz oscillator.

  WARNING: While an ISP programmer is connected, the right button may not be
           pressed, as this would short circuit SCK of the programmer to ground.
           With the workaround for the temperature sensor, the same is true for
           the left button too.

Current consumption when idle sleep mode (wakeup every 10ms) @ 3.3V
360-380µA, which splits up:
--> 285µA at R25 with D22 -> can not be fixed in software
--> 1.7µA at R28 -> acceptable
-->   9nA at R30 -> acceptable
--> 0.5µA at D64 -> For RTC of ARM, as intended
--> 86-110µA at D21
---->    1.2µA for comparator U22
----> 90-110µA consumption of the AVR
------> 11µA for oscillator
------> 18µA for the brown out detector


Current consumption when in power down mode @ 2.4V (simulate with 2x NIMH battery)
200µA
--> 200µA at R25 with D22 -> can not be fixed in software
Everything else nearly needs no power at all.

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
	uint64_t chargingSumAllTimes; //[mAms]
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
	AdPowerdown();
	TimerStop();
	PinsWakeupByKeyPressOn();
	WatchdogDisable();
	WaitForExternalInterrupt(); //get out by a keypress only
	PinsWakeupByKeyPressOff(); //must be called before SPI is enabled again
	HardwareInit();
	TimerInit(true);
	TimerSlow();
}

/*if alarm is non 0, its a timeout in [s] until a wakeup of the ARM CPU occurs
  after the return of the function, reinitSpiDelay should be set
  The return value tells the reason for the wakeup:
  0: undefined
  1: wakeup by alarm
  2: wakeup by USB power applied
  3: wakeup by user keypress
*/

static uint8_t PowerDownLoop(uint16_t alarm) {
	uint8_t checkInputVoltage = 0;
	uint8_t checkBatteryVoltage = 0;
	uint8_t checkAlarm = 0;
	//we must first release the keys, otherwise the power up will be instantly follow a power down
	uint8_t powerByKeyState = 0;
	uint8_t wakeupReason = 0;
	PwmBatterySet(0);
	SpiDisable();
	SensorsOff();
	ArmReset();
	ArmUserprog(); //must be set low, before battery is powered off!
	ArmBatteryOff();
	waitms(10); //make sure ARM finally is in reset state and capacitors are discharged
	ArmRun(); //the low level saves some power, so we do not drive against the pull-down resistor.
	LedOff();
	PinsPowerdown(); //we only need the key inputs, HardwareInit() will set the configuration back
	TimerSlow();
	while (1) { //loop runs every 100ms
		//wakeup by input voltage -> USB powered
		if (checkInputVoltage == 0) { //check every 500ms
			checkInputVoltage = 5;
			SensorsOn();
			waitms(1);
			uint16_t inputVoltage = SensorsInputvoltageGet();
			if (inputVoltage >= VCC_CONNECTED) {
				wakeupReason = 2;
				break;
			}
			if (checkBatteryVoltage == 0) { //check every 10s
				checkBatteryVoltage = 2;
				uint16_t batteryVoltage = SensorsBatteryvoltageGet();
				if (batteryVoltage <= 2800) {
					DeepDischargePrevention();
				}
			}
			checkBatteryVoltage--;
			SensorsOff();
			AdPowerdown();
		}
		checkInputVoltage--;
		//wakeup by alarm
		if (alarm) {
			if (checkAlarm == 0) { //1s passed
				alarm--;
				if (alarm == 0) {
					wakeupReason = 1;
					break; //wakeup
				}
				checkAlarm = 10;
			}
			checkAlarm--;
		}
		//wakeup by keypress, minimum 500ms, without keypress and then 500ms with keypress
		if ((powerByKeyState < 5) && (KeyPressedLeft() == 0) && (KeyPressedRight() == 0)) {
			powerByKeyState++;
		} else if ((powerByKeyState >= 5) && (powerByKeyState < 10)) {
			if ((KeyPressedLeft()) && (KeyPressedRight())) {
				powerByKeyState++;
			} else {
				powerByKeyState = 5;
			}
		} else if (powerByKeyState == 10) {
			LedOn();
			if ((KeyPressedLeft() == 0) && (KeyPressedRight() == 0)) {
				wakeupReason = 1;
				break;
			}
		}
		WatchdogReset();
		while (TimerHasOverflownIsr() == false) {
			WaitForInterrupt();
		}
	}
	LedOn();
	PinsPowerup();
	TimerFast();
	SensorsOn();
	ArmReset();
	waitms(1);
	ArmBatteryOn();
	waitms(10);
	ArmRun(); //we still have the pin set to start the usermode application
	return wakeupReason;
}

//value in [10ms]
#define KEY_PRESS_TIME_DEFAULT 100

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
	SpiDataSet(CMD_VERSION, 0x0506); //05 for the program (folder name), 06 for the version
	uint16_t pressedLeft = 0, pressedRight = 0; //Time the left or right button is hold down [10ms]
	uint8_t resetHold = 0; //count down until the reset of the ARM CPU is released [10ms]
	uint8_t armNormal = 1; //startup mode of the ARM cpu 0: DFU bootloader, 1: normal program start
	uint8_t ledFlashCycle = 0; //controls the speed of the LED flashing, counts up. [10ms]
	uint8_t ledFlashRemaining = 0; //if non 0, the LED is flashing
	uint8_t ledFlashOnSpiCommand = 1; //if 1, each SPI command is confirmed by a LED flash
	uint8_t adcCycleFast = 0; //cycle for measuring the analog inputs and calculating the battery charger logic
	uint8_t adcCycleSlow = 0; //cycle for measuring the analog inputs and calculating the battery charger logic
	uint8_t reinitSpiDelay = 0; //if reached 0, the SPI interface is reset
	uint16_t watchdogHighest = 0; //reload value of the ARM cpu watchdog, 0 = disabled [1ms]
	uint16_t watchdogCurrent = 0; //count down value of the ARM CPU watchdog. On a 0, a reset is issured. [1ms]
	uint16_t inU = 0; //measured input voltage [mV]
	uint16_t battU = 0; //measured battery voltage [mV]
	uint16_t battUmin = 0xFFFF; //minimum measured battery voltage [mV]
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
	uint8_t delayedPowerDown = 0; //if this counter reached zero when running on battery, there will be a request to power down
	uint8_t timeLoadLast = 0; //timestamp the last time the Load was updated [10ms]
	uint32_t ticksIdle = 0; //accumulated ticks of noting to do since timeLoadLast was updated
	uint8_t percentIdlePrevious = 0; //previous value of being idle in [%]
	uint16_t keyPressTime = KEY_PRESS_TIME_DEFAULT; //duration of the left or right key being pressed in [10ms]

	chargerState_t CS;
	persistent_t settings;
	SettingsLoad(&settings);
	SpiDataSet(CMD_OPTIME, settings.opTotal / (60UL * 24UL));
	SpiDataSet(CMD_LED_READ, ledFlashOnSpiCommand);
	SpiDataSet(CMD_BAT_CURRENT_MAX_READ, inMax);
	ChargerInit(&CS, settings.chargerError, settings.chargingCycles, settings.prechargingCycles, settings.chargingSumAllTimes);
	uint8_t timestampChargeprocessLast = TimerGetValue(); //time in [10ms]
	for (;;) { //Main loop, we run every 10ms
		if (KeyPressedLeft()) {
			if (pressedLeft < 0xFFFF) {
				pressedLeft++;
			}
			if (pressedLeft == keyPressTime) { //1s press (if default value)
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
			if (pressedLeft >= keyPressTime) {
				ledFlashRemaining = 1;
			}
		} else {
			pressedLeft = 0;
			if (resetHold) {
				resetHold--;
				if (resetHold == 0) {
					keyPressTime = KEY_PRESS_TIME_DEFAULT;
					ArmRun();
					LedOff();
				}
			}
		}
		if ((KeyPressedRight()) && (KeyPressedLeft() == 0)) {
			if (pressedRight < 0xFFFF) {
				pressedRight++;
			}
			if (pressedRight == keyPressTime) { //1s press (if default value)
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
				SpiDataSet(CMD_LED_READ, ledFlashOnSpiCommand);
			} else if (command == CMD_WATCHDOG_CTRL) {
				watchdogHighest = parameter;
				watchdogCurrent = watchdogHighest;
				SpiDataSet(CMD_WATCHDOG_CTRL_READ, watchdogHighest);
			} else if ((command == CMD_WATCHDOG_RESET) && (parameter == 0x42)) {
				watchdogCurrent = watchdogHighest;
			} else if ((command == CMD_BAT_CURRENT_MAX) && (parameter <= 200)) {
				inMax = parameter;
				SpiDataSet(CMD_BAT_CURRENT_MAX_READ, inMax);
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
				SpiDataSet(CMD_POWERMODE_READ, batteryMode);
			} else if (command == CMD_ALARM) {
				batteryWakeupTime = parameter; // 0 == never
				SpiDataSet(CMD_ALARM_READ, batteryWakeupTime);
			} else if (command == CMD_POWERDOWN) {
				requestPowerDown = 1;
			} else if ((command == CMD_KEYPRESSTIME) && (parameter >= 1000) && (parameter <= 60000)) {
				keyPressTime = parameter / 10; //[ms] -> [10ms]
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
		adcCycleFast++;

		if (adcCycleFast == 1) {
			inU = SensorsInputvoltageGet(); //we should do this in the first cycle!
			SpiDataSet(CMD_VCC, inU);
		}
		if (adcCycleFast == 3) {
			battU = SensorsBatteryvoltageGet();
			SpiDataSet(CMD_BAT_VOLTAGE, battU);
			if (battU < battUmin) {
				battUmin = battU;
				SpiDataSet(CMD_BAT_MIN_VOLTAGE, battUmin);
			}
		}
		if (adcCycleFast == 5) {
			battI = SensorsBatterycurrentGet();
			SpiDataSet(CMD_BAT_CURRENT, battI);
		}
		if (adcCycleFast == 7) {
			uint8_t timestamp = TimerGetValue();
			uint8_t delta = timestamp - timestampChargeprocessLast;
			uint16_t msPassed = (uint16_t)delta * 10; //timer is running every 10ms
			if (msPassed >= 50) { //in the case of a time catchup or first start, calling could be faster than 50ms
				battPwm = ChargerCycle(&CS, battU, inU, battI, battTemp, inMax, msPassed, requestFullCharge);
				requestFullCharge = 0;
				PwmBatterySet(battPwm);
			}
			timestampChargeprocessLast = timestamp;
		}
		if (adcCycleFast == 10) { //updates every 100ms
			adcCycleFast = 0;
			adcCycleSlow++;
			if (adcCycleSlow == 1) { //call every 500ms
				//calculation takes 24ms with F_CPU=107000
				battTemp = SensorsBatterytemperatureGet();
				SpiDataSet(CMD_BAT_TEMPERATURE, battTemp);
			}
			if (adcCycleSlow == 2) {
				//needs 13ms with F_CPU=107000
				uint16_t cpuTemperature = SensorsChiptemperatureGet();
				SpiDataSet(CMD_CPU_TEMPERATURE, cpuTemperature);
			}
			if (adcCycleSlow == 3) {
				SpiDataSet(CMD_BAT_CHARGE_STATE, ChargerGetState(&CS));
				SpiDataSet(CMD_BAT_CHARGE_ERR, ChargerGetError(&CS));
				SpiDataSet(CMD_BAT_CHARGED, ChargerGetCharged(&CS) / (60ULL*60ULL)); //mAs -> mAh
				settings.chargingSumAllTimes = ChargerGetChargedTotal(&CS);
				SpiDataSet(CMD_BAT_CHARGED_TOT, settings.chargingSumAllTimes / (1000ULL * 60ULL * 60ULL * 1000ULL)); //mAms ->Ah
				settings.chargingCycles = ChargerGetCycles(&CS);
				SpiDataSet(CMD_BAT_CHARGE_CYC, settings.chargingCycles);
				settings.prechargingCycles = ChargerGetPreCycles(&CS);
				SpiDataSet(CMD_BAT_PRECHARGE_CYC, settings.prechargingCycles);
				SpiDataSet(CMD_BAT_PWM, battPwm);
				uint32_t timeS = ChargerGetTime(&CS) / 1000;
				SpiDataSet(CMD_BAT_TIME, timeS);
				SpiDataSet(CMD_BAT_CHARGE_NOCURR_VOLT, ChargerGetNoCurrentVoltage(&CS));
			}
			if (adcCycleSlow == 5) {
				adcCycleSlow = 0;
			}
		}
		//check if we should power down. Triggers when >= 4500mV and then <= 4000mV
		if (inU >= 4500) {
			onUsbPower = 1;
		}
		if (inU <= 4000) {
			if ((onUsbPower) && (batteryMode == 0)) {
				/*Sometimes power just goes off for a short moment, in this case
				  the battery should just act as UPS for 500ms */
				delayedPowerDown = 50;
			}
			onUsbPower = 0;
			if (delayedPowerDown) {
				delayedPowerDown--;
				if (delayedPowerDown == 0) {
					requestPowerDown = 1;
				}
			}
		}
		if ((onUsbPower == 0) && (battU <= 3100) && (battU)) { //prevent full discharge. We may go down to 3V
			requestPowerDown = 1;
		}
		if (requestPowerDown) {
			requestPowerDown = 0;
			if (onUsbPower == 0) {
				keyPressTime = KEY_PRESS_TIME_DEFAULT;
				uint8_t wakeupReason = PowerDownLoop(batteryWakeupTime);
				timestampChargeprocessLast = TimerGetValue();
				reinitSpiDelay = 100;
				if (wakeupReason == 2) {
					/* Otherwise the device would stay on, when the USB power is only
					   applied for a very short time, enough for only one measurement.
					   This happens at least on one PC, when it is shut down.
					*/
					onUsbPower = 1;
				}
			}
		}
		//update CPU load (nees ~400byte flash, so remove, should flash size be a problem)
		{
			uint8_t time = TimerGetValue();
			uint8_t delta = time - timeLoadLast;
			if (delta >= 100) {
				/* A delta of 100 would be best. But if not we need to scale down. So:
				   scaledIdle = ticksIdle * 100 / delta;
				   To get percent of ticks idle:
				   percent = scaledIdle * 100 / ticksPerSecond
				   This merges to:
				   percent = (ticksIdle * 100000) / (delta * ticksPerSecond)
				*/
				uint32_t percentIdle = ((uint32_t)ticksIdle * 10000UL) / ((uint32_t)delta * TimerGetTicksPerSecond());
				SpiDataSet(CMD_CPU_LOAD, 100 - (uint8_t)percentIdlePrevious); // convert from idle to load
				percentIdlePrevious = percentIdle; //have 1s delay to poll this value without the polling changing this value
				timeLoadLast = time;
				ticksIdle = 0;
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
		uint16_t ticksToInterrupt = TimerGetTicksLeft();
		if (TimerHasOverflownIsr() == false) {
			ticksIdle += ticksToInterrupt;
			while (TimerHasOverflownIsr() == false) {
				WaitForInterrupt();
			}
		}
	}
}
