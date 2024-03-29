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
  Test everything by manually controlling the outputs
  Press the right key to execute the test
  Press the left key to toggle the test

  Connect a serial cable to the LED pin at 1200 baud to see some data

  Pin connection:
  PINA.0 = Input, AVR DI
  PINA.1 = Output, AVR DO and boot pin state (high state = bootloader)
  PINA.2 = Input, AVR SCK
  PINA.3 = Input, AREF
  PINA.4 = Input, Battery voltage measurement
  PINA.5 = Input, Vcc measurement
  PINA.6 = Input, Voltage drop for charge current measurement
  PINA.7 = Output, pull high to set ARM into reset state.
  PINB.0 = Output, pull down to power the ARM by battery (PCB with fix). (MOSI for ISP)
  PINB.1 = Output, User LED (MISO for ISP)
  PINB.2 = Input with pullup, right key (SCK for ISP)
  PINB.3 = Output, PWM for battery charging (OC1B). Set to 0V to disable charging
  PINB.4 = Output, Power save option, set high to enable temperature and battery
                   voltage measurement, low disconnects the voltage dividers
  PINB.5 = Input, Battery temperature measurement (PCB with fix)
  PINB.6 = Input with pullup, left key (allows wakeup from power down by Int0)
           If workaround for teperature sensor is applied: Input without pullup,
           temperature sensor can be read as long as key is not pressed
  PINB.7 = reset pin

  Set F_CPU to 8MHz and the fuses to use the internal 8MHz oscillator.

  WARNING: While an ISP programmer is connected, the right button may not be
           pressed, as this would short circuit SCK of the programmer to ground.

  Hardware used:
   Timer0: For timekeeping
   Timer1: For PWM generation

History:
 v0.9 2022-06-29: Add force charge test
 v0.8 2022-06-24: Complete testing for charger control logic
 v0.5 2021-12-19: All pins can be tested as intended and sensors can be read.
      Charging controller is missing.

*/

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "hardware.h"
#include "timing.h"
#include "softtx.h"
#include "femtoVsnprintf.h"
#include "chargerStatemachine.h"
#include "counter.h"

typedef void (*testSelect)(void);

typedef struct
{
	testSelect f;
	const char * name;
} test_t;

#define TESTS 8

static void readSensors(void);
static void toggleArmPower(void);
static void toggleArmBoot(void);
static void toggleArmReset(void);
static void toggleSensorPower(void);
static void chargerPwm(void);
static void chargerTestNormal(void);
static void chargerTestForce(void);


const char name0[] PROGMEM = "0-Read all sensors\r\n";
const char name1[] PROGMEM = "1-Toggle ARM power state\r\n";
const char name2[] PROGMEM = "2-Toggle boot pin state\r\n";
const char name3[] PROGMEM = "3-Toggle reset pin state\r\n";
const char name4[] PROGMEM = "4-Toggle sensors power pin state\r\n";
const char name5[] PROGMEM = "5-Set charger PWM\r\n";
const char name6[] PROGMEM = "6-Run charger logic\r\n";
const char name7[] PROGMEM = "7-Run charger logic, force charge\r\n";



const test_t g_tests[TESTS] = {
	{&readSensors, name0},
	{&toggleArmPower, name1},
	{&toggleArmBoot, name2},
	{&toggleArmReset, name3},
	{&toggleSensorPower, name4},
	{&chargerPwm, name5},
	{&chargerTestNormal, name6},
	{&chargerTestForce, name7},
};

bool g_ArmPowerState;
bool g_ArmBootState;

static void temperatureToString(char * output, size_t len, const char * prefix, int16_t temperature) {
	char sign = ' ';
	if (temperature < 0) {
		sign = '-';
		temperature *= -1;
	}
	unsigned int degree = temperature / 10;
	unsigned int degree10th = temperature % 10;
	femtoSnprintf(output, len, "%s %c%u.%u°C\r\n", prefix, sign, degree, degree10th);
}

static void readSensors(void) {
	char text[64];
	size_t len = sizeof(text);

	CounterStart();
	uint16_t temp = SensorsBatterytemperatureGet();
	uint16_t timePassed1 = CounterGet();
	temperatureToString(text, len, "Bat:", temp);
	print(text);

	CounterStart();
	temp = SensorsChiptemperatureGet();
	uint16_t timePassed2 = CounterGet();
	temperatureToString(text, len, "MCU:", temp);
	print(text);
	femtoSnprintf(text, len, "Ticks %u, %u\r\n", timePassed1, timePassed2);
	print(text);

	CounterStart();
	uint16_t inputVoltage = SensorsInputvoltageGet();
	timePassed1 = CounterGet();
	femtoSnprintf(text, len, "Uin:  %umV, t: %u\r\n", inputVoltage, timePassed1);
	print(text);

	CounterStart();
	temp = VccRaw();
	timePassed1 = CounterGet();
	femtoSnprintf(text, len, "Vcc raw: %u, t: %u\r\n", temp, timePassed1);
	print(text);

	CounterStart();
	temp = DropRaw();
	timePassed1 = CounterGet();
	femtoSnprintf(text, len, "Drop raw: %u, t: %u\r\n", temp, timePassed1);
	print(text);

	CounterStart();
	uint16_t batteryVoltage = SensorsBatteryvoltageGet();
	timePassed1 = CounterGet();
	femtoSnprintf(text, len, "Ubat: %umV, t: %u\r\n", batteryVoltage, timePassed1);
	print(text);

	CounterStart();
	uint16_t batteryCurrent = SensorsBatterycurrentGet();
	timePassed1 = CounterGet();
	femtoSnprintf(text, len, "Ibat: %umA, t: %u\r\n", batteryCurrent, timePassed1);
	print(text);

	char sck = SpiSckLevel() ? '1' : '0';
	char di = SpiDiLevel() ? '1' : '0';
	femtoSnprintf(text, len, "SPI DI: %c SCK: %c\r\n", sck, di);
	print(text);
}

static void toggleArmPower(void) {
	if (g_ArmBootState) {
		ArmRun();
		g_ArmBootState = false;
		print_p(PSTR("Arm in user program mode\r\n"));
	}
	if (g_ArmPowerState == false) {
		ArmBatteryOn();
		g_ArmPowerState = true;
		print_p(PSTR("Arm connected to battery\r\n"));
	} else {
		ArmBatteryOff();
		g_ArmPowerState = false;
		print_p(PSTR("Arm disconnected from battery\r\n"));
	}
}

static void toggleArmBoot(void) {
	if (g_ArmPowerState) {
		if (g_ArmBootState == false) {
			ArmBootload();
			g_ArmBootState = true;
			print_p(PSTR("Arm in bootloader mode\r\n"));
		} else {
			ArmUserprog();
			g_ArmBootState = false;
			print_p(PSTR("Arm in user program mode\r\n"));
		}
	} else {
		print_p(PSTR("Error, run mode may only be set if power is enabled\r\n"));
	}
}

static void toggleArmReset(void) {
	static bool state = false;
	if (state == false) {
		ArmReset();
		state = true;
		print_p(PSTR("Arm in reset mode\r\n"));
	} else {
		ArmRun();
		state = false;
		print_p(PSTR("Arm in run mode\r\n"));
	}
}

static void toggleSensorPower(void) {
	static bool state = false;
	if (state == false) {
		SensorsOn();
		state = true;
		print_p(PSTR("Sensors on\r\n"));
	} else {
		SensorsOff();
		state = false;
		print_p(PSTR("Sensors off\r\n"));
	}
}

static void chargerPwm(void) {
	static uint8_t pwmValue;
	pwmValue++;
	if (pwmValue > PWM_MAX) {
		pwmValue = 0;
	}
	char text[64];
	femtoSnprintf(text, sizeof(text), "PWM now: %u\r\n", pwmValue);
	print(text);
	PwmBatterySet(pwmValue);
}

static void chargerTest(uint8_t force) {
	chargerState_t cS;
	ChargerInit(&cS, 0, 0, 0, 0);
	uint8_t cycle = 0;
	char text[64];
	size_t len = sizeof(text);
	CounterStart(); //starts an 1ms counter
	while (KeyPressedLeft() == 0) {
		uint16_t timePassed = CounterGetMs();
		if (timePassed < 100) {
			waitms(100 - timePassed);
		}
		uint16_t batteryTemperature = SensorsBatterytemperatureGet();
		uint16_t inputVoltage = SensorsInputvoltageGet();
		uint16_t batteryVoltage = SensorsBatteryvoltageGet();
		uint16_t batteryCurrent = SensorsBatterycurrentGet();
		timePassed = CounterGetMs();
		CounterStart();
		uint16_t pwm = ChargerCycle(&cS, batteryVoltage, inputVoltage, batteryCurrent, batteryTemperature, 100, timePassed, force);
		PwmBatterySet(pwm);
		cycle++;
		if (cycle == 1) {
			femtoSnprintf(text, len, "%04umV-", batteryVoltage);
			print(text);
		}
		if (cycle == 2) {
			femtoSnprintf(text, len, "%03umA-", batteryCurrent);
			print(text);
		}
		if (cycle == 3) {
			femtoSnprintf(text, len, "%03uPWM\r\n", pwm);
			print(text);
		}
		if (cycle == 4) {
			femtoSnprintf(text, len, "%uSt-", ChargerGetState(&cS));
			print(text);
		}
		if (cycle == 5) {
			femtoSnprintf(text, len, "%uErr-", ChargerGetError(&cS));
			print(text);
		}
		if (cycle == 6) {
			femtoSnprintf(text, len, "%05umAm\r\n", ChargerGetCharged(&cS) / 60);
			print(text);
		}
		if (cycle == 20) {
			cycle = 0;
		}
	}
	PwmBatterySet(0);
}

static void chargerTestNormal(void) {
	chargerTest(0);
}

static void chargerTestForce(void) {
	chargerTest(1);
}

int main(void) {
	HardwareInit();
	softtx_init();
	waitms(10); //let the uart have one level for a longer time
	print_p(PSTR("Test everything 1.0\r\n"));
	print_p(g_tests[0].name);
	uint8_t testSelected = 0;
	uint8_t pressedLeft = 0, pressedRight = 0, pressedRepeatRight = 0;
	ArmUserprog();
	waitms(1);
	ArmRun();
	for (;;) { //Main loop
		if (KeyPressedLeft()) {
			pressedLeft = 1;
		} else if (pressedLeft) {
			pressedLeft = 0;
			testSelected++;
			if (testSelected >= TESTS)
			{
				testSelected = 0;
			}
			print_p(g_tests[testSelected].name);
		}
		if (KeyPressedRight()) {
			if (pressedRight < 255) {
				pressedRight++;
			}
			if (pressedRight >= 100) {
				pressedRepeatRight++;
				if (pressedRepeatRight == 10) {
					g_tests[testSelected].f();
					pressedRepeatRight = 0;
				}
			}
		} else if (pressedRight) {
			pressedRight = 0;
			pressedRepeatRight = 0;
			g_tests[testSelected].f();
		}
		waitms(10);
	}
}
