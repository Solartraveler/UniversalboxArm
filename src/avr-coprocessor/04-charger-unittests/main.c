/*UniversalboxARM - AVR coprocessor
  (c) 2022 by Malte Marwedel
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
  This project just runs the unit tests for the charger logic on the AVR.
  As sizeof(int) is just 16bit, results may be different than when run on a PC.
  So running on the target AVR verifies the tests already done on the PC.

  Connect a serial cable to PORTA pin 1 with 1200 baud to see some data.

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

  Note: Strings have been moved to the eeprom - simply because the flash is full.
  The resulting code could be shorter - but the old gcc 5 provided by Debian 11
  ignores some optimization, like removing the unused interrupt vector table
  (-mno-interrupts) or unused functions (-ffunction-sections).

History:
 v0.10 2022-06-26
 v0.9 2022-06-24
 v0.1 2022-05-31

*/

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include <avr/eeprom.h>

#include "hardware.h"
#include "timing.h"
#include "softtx.h"
#include "femtoVsnprintf.h"
#include "chargerStatemachine.h"
#include "utility.h"
#include "counter.h"

#define TASSH(X, Y) if (!(X)) {return Y;}

typedef struct {
	uint16_t battU;
	uint16_t inU;
	uint16_t battI;
	int16_t battTemp;
	uint16_t inImax;
	uint8_t requestCharge;
	uint16_t pwmMax;
	uint16_t pwmMin;
	uint16_t pwmOut;
	uint16_t maxTicks;
} input_t;

uint32_t g_maxTicks;

static void myprintf(const char *format, ...) {
	va_list args;
	va_start(args, format);
	char formatRam[64];
	formatRam[sizeof(formatRam) - 1] = '\0';
	uint8_t i = 0;
	do {
		formatRam[i] = eeprom_read_byte((const uint8_t*)format);
		format++;
		i++;
	} while ((i < (sizeof(formatRam) - 1)) && (formatRam[i - 1]));
	char buffer[96];
	femtoVsnprintf(buffer, sizeof(buffer), formatRam, args);
	va_end(args);
	print(buffer);
}

const char EEMEM strPwm[] = "Pwm out of bounds, is %u\r\n";

static bool StepForward(chargerState_t * pCs, uint32_t miliseconds, input_t * data) {
	CounterStart();
	data->pwmOut = ChargerCycle(pCs, data->battU, data->inU, data->battI, data->battTemp, data->inImax, miliseconds, data->requestCharge);
	uint32_t ticks = CounterGet();
	data->maxTicks = MAX(data->maxTicks, ticks);
	if ((data->pwmOut > data->pwmMax) || (data->pwmOut < data->pwmMin)) {
		myprintf(strPwm, (unsigned int)data->pwmOut);
		return false;
	}
	return true;
}

static bool TimeForward(chargerState_t * pCs, uint32_t miliseconds, input_t * data) {
	bool success = true;
	do {
		uint32_t cyc = miliseconds;
		if (miliseconds > 150) {
			cyc = 100;
		}
		miliseconds -= cyc;
		success &= StepForward(pCs, cyc, data);
	} while (miliseconds);
	return success;
}

static bool TimeForwardS(chargerState_t * pCs, uint32_t seconds, input_t * data) {
	return TimeForward(pCs, seconds * 1000UL, data);
}

const char EEMEM strError[] = "Error should %u, is %u\r\n";

static bool CheckErrorExpected(chargerState_t * pCs, uint8_t should) {
	uint8_t errorState = ChargerGetError(pCs);
	if (errorState == should) {
		return true;
	}
	myprintf(strError, (unsigned int)should, (unsigned int)errorState);
	return false;
}

static bool CheckStateExpected(chargerState_t * pCs, uint8_t should) {
	if (ChargerGetState(pCs) == should) {
		return true;
	}
	return false;
}

const char EEMEM strFailed[] = "Test %u failed, with code %u\r\n";

static void PrintFailed(uint16_t testnumber, uint8_t errorCode) {
	myprintf(strFailed, (unsigned int)testnumber, (unsigned int)errorCode);
}

const char EEMEM strTickAnalzye[] = "Max ticks: %u\r\n";

static void TicksAnalyze(input_t * data) {
	myprintf(strTickAnalzye, (unsigned int)data->maxTicks);
	g_maxTicks = MAX(g_maxTicks, data->maxTicks);
}

//Under this conditions, the charging should start
static void CommonStartCondition(input_t * data) {
	data->battU = 3100;
	data->inU = 4700;
	data->battI = 0;
	data->battTemp = 250;
	data->inImax = 100;
	data->requestCharge = 0;
	data->pwmMin = CHARGER_PWM_MAX / 2;
	data->pwmMax = CHARGER_PWM_MAX;
	data->maxTicks = 0;
}

static void ChargerInitCommon(chargerState_t * pCs) {
	ChargerInit(pCs, 0, 0, 0, 0);
}

//just do nothing, because the battery is full
static uint8_t test1(void) {
	chargerState_t cs;
	ChargerInitCommon(&cs);
	input_t data;
	CommonStartCondition(&data);
	data.battU = 3350;
	data.pwmMin = 0;
	data.pwmMax = 0;
	TASSH(TimeForwardS(&cs, 100, &data), 1);
	TASSH(CheckErrorExpected(&cs, 0), 2);
	TASSH(CheckStateExpected(&cs, 0), 3);
	TicksAnalyze(&data);
	return 0;
}

//just do nothing, because the charging current is limited to 0
static uint8_t test2(void) {
	chargerState_t cs;
	ChargerInitCommon(&cs);
	input_t data;
	CommonStartCondition(&data);
	data.inImax = 0;
	data.pwmMin = 0;
	data.pwmMax = 0;
	TASSH(TimeForwardS(&cs, 100, &data), 1);
	TASSH(CheckErrorExpected(&cs, 0), 2);
	TASSH(CheckStateExpected(&cs, 0), 3);
	TicksAnalyze(&data);
	return 0;
}

//reject, if initialized in error state
static uint8_t test3(void) {
	chargerState_t cs;
	ChargerInit(&cs, 1, 0, 0, 0);
	input_t data;
	CommonStartCondition(&data);
	data.pwmMin = 0;
	data.pwmMax = 0;
	TASSH(TimeForwardS(&cs, 1, &data), 1);
	TASSH(CheckErrorExpected(&cs, 1), 2);
	TASSH(CheckStateExpected(&cs, 0), 3);
	TicksAnalyze(&data);
	return 0;
}

//lets do a simple charge
static uint8_t test4(void) {
	chargerState_t cs;
	ChargerInitCommon(&cs);
	input_t data;
	CommonStartCondition(&data);
	//start the charge
	TASSH(StepForward(&cs, 100, &data), 1);
	TASSH(CheckErrorExpected(&cs, 0), 2);
	TASSH(CheckStateExpected(&cs, 1), 3);

	//let's do a 6h charge
	data.battI = 100;
	for (uint32_t i = 0; i < (1000UL * 60UL * 60UL * 6UL); i+= 100) {
		TASSH(StepForward(&cs, 100, &data), 4);
		TASSH(CheckErrorExpected(&cs, 0), 5);
		TASSH(CheckStateExpected(&cs, 1), 6);
	}

	//now the battery should be at maximum voltage and current goes to zero
	data.battI = 0;
	data.battU = 3550;
	data.pwmMin = 0;
	data.pwmMax = 0;
	TASSH(StepForward(&cs, 100, &data), 7);
	TASSH(CheckErrorExpected(&cs, 0), 8);
	TASSH(CheckStateExpected(&cs, 0), 9);
	TASSH(ChargerGetCharged(&cs) == (600UL*60UL*60UL), 10);
	TASSH(ChargerGetChargedTotal(&cs) == (600UL*60UL*60UL), 11);
	TASSH(ChargerGetCycles(&cs) == 1, 12);
	TASSH(ChargerGetPreCycles(&cs) == 0, 13);
	TicksAnalyze(&data);
	return 0;
}

//lets do a charge but needing to adjust the pwm value to maintain the desired current
static uint8_t test5(void) {
	chargerState_t cs;
	ChargerInitCommon(&cs);
	input_t data;
	CommonStartCondition(&data);
	data.pwmMax = 101;
	//start the charge
	TASSH(StepForward(&cs, 100, &data), 1);
	TASSH(CheckErrorExpected(&cs, 0), 2);
	TASSH(CheckStateExpected(&cs, 1), 3);

	//let's do a 6h charge
	data.battI = 100;
	for (uint32_t i = 0; i < (1000UL * 60UL * 60UL * 6UL); i+= 100) {
		TASSH(StepForward(&cs, 100, &data), 4);
		TASSH(CheckErrorExpected(&cs, 0), 5);
		TASSH(CheckStateExpected(&cs, 1), 6);
		data.battI = data.pwmOut; //feed in the pwm value as current
		//printf("Output %i\n", data.battI);
	}

	//now the battery should be at maximum voltage and current goes to zero
	data.battI = 0;
	data.battU = 3550;
	data.pwmMin = 0;
	data.pwmMax = 0;
	TASSH(StepForward(&cs, 100, &data), 7);
	TASSH(CheckErrorExpected(&cs, 0), 8);
	TASSH(CheckStateExpected(&cs, 0), 9);
	TASSH(ChargerGetCharged(&cs) < (600UL*60UL*60UL), 10);
	TASSH(ChargerGetCharged(&cs) > (600UL*60UL*59UL), 11);
	TicksAnalyze(&data);
	return 0;
}


//lets do a charge but needing to adjust the pwm value to maintain the desired current
//and regulating gets difficult cause the 100mA is always missed
static uint8_t test6(void) {
	chargerState_t cs;
	ChargerInitCommon(&cs);
	input_t data;
	CommonStartCondition(&data);
	data.pwmMin = 45;
	data.pwmMax = 100;
	//start the charge
	TASSH(StepForward(&cs, 100, &data), 1);
	TASSH(CheckErrorExpected(&cs, 0), 2);
	TASSH(CheckStateExpected(&cs, 1), 3);

	//let's do a 6h charge
	data.battI = 0;
	for (uint32_t i = 0; i < (1000UL * 60UL * 60UL * 6UL); i+= 100) {
		TASSH(StepForward(&cs, 100, &data), 4);
		TASSH(CheckErrorExpected(&cs, 0), 5);
		TASSH(CheckStateExpected(&cs, 1), 6);
		data.battI = MIN(data.pwmOut * 2U + 1U, i / 25); //feed in the pwm value as current
	}

	//now the battery should be at maximum voltage and current goes to zero
	data.battI = 0;
	data.battU = 3550;
	data.pwmMin = 0;
	data.pwmMax = 0;
	TASSH(StepForward(&cs, 100, &data), 7);
	TASSH(CheckErrorExpected(&cs, 0), 8);
	TASSH(CheckStateExpected(&cs, 0), 9);
	TASSH(ChargerGetCharged(&cs) <= (610UL*60UL*60UL), 10);
	TASSH(ChargerGetCharged(&cs) > (600UL*60UL*59UL), 11);
	TicksAnalyze(&data);
	return 0;
}

//reject, if not enough time passed
static uint8_t test7(void) {
	chargerState_t cs;
	ChargerInitCommon(&cs);
	input_t data;
	CommonStartCondition(&data);
	data.pwmMin = 0;
	data.pwmMax = 0;
	TASSH(StepForward(&cs, 25, &data), 1);
	TASSH(StepForward(&cs, 100, &data), 2);
	TASSH(CheckErrorExpected(&cs, 0), 3);
	TASSH(CheckStateExpected(&cs, 9), 4);
	TicksAnalyze(&data);
	return 0;
}

//reject, if too much time passed
static uint8_t test23(void) {
	chargerState_t cs;
	ChargerInitCommon(&cs);
	input_t data;
	CommonStartCondition(&data);
	data.pwmMin = 0;
	data.pwmMax = 0;
	TASSH(StepForward(&cs, 1100, &data), 1);
	TASSH(StepForward(&cs, 100, &data), 2);
	TASSH(CheckErrorExpected(&cs, 0), 3);
	TASSH(CheckStateExpected(&cs, 9), 4);
	TicksAnalyze(&data);
	return 0;
}

//reject, if battery too low charged
static uint8_t test8(void) {
	chargerState_t cs;
	ChargerInitCommon(&cs);
	input_t data;
	CommonStartCondition(&data);
	data.battU = 1500;
	data.pwmMin = 0;
	data.pwmMax = 0;
	TASSH(StepForward(&cs, 100, &data), 1);
	TASSH(StepForward(&cs, 100, &data), 2);
	TASSH(CheckErrorExpected(&cs, 2), 3);
	TASSH(CheckStateExpected(&cs, 3), 4);
	TicksAnalyze(&data);
	return 0;
}

//reject, if battery too high charged
static uint8_t test9(void) {
	chargerState_t cs;
	ChargerInitCommon(&cs);
	input_t data;
	CommonStartCondition(&data);
	data.battU = 3700;
	data.pwmMin = 0;
	data.pwmMax = 0;
	TASSH(StepForward(&cs, 100, &data), 1);
	TASSH(StepForward(&cs, 100, &data), 2);
	TASSH(CheckErrorExpected(&cs, 1), 3);
	TASSH(CheckStateExpected(&cs, 4), 4);
	TicksAnalyze(&data);
	return 0;
}

//reject, charger fails to measure any current
static uint8_t test10(void) {
	chargerState_t cs;
	ChargerInitCommon(&cs);
	input_t data;
	CommonStartCondition(&data);
	data.pwmMin = 0;
	TASSH(TimeForwardS(&cs, 10, &data), 1);
	TASSH(CheckErrorExpected(&cs, 4), 3);
	TASSH(CheckStateExpected(&cs, 8), 4);
	TicksAnalyze(&data);
	return 0;
}


//battery should be full, but fails to reach maximum voltage
static uint8_t test11(void) {
	chargerState_t cs;
	ChargerInitCommon(&cs);
	input_t data;
	CommonStartCondition(&data);
	data.pwmMin = 0;
	TASSH(StepForward(&cs, 100, &data), 1);
	data.battI = 100;
	TASSH(TimeForwardS(&cs, 7UL * 60UL * 60UL, &data), 1);
	TASSH(CheckStateExpected(&cs, 1), 2);
	TASSH(CheckErrorExpected(&cs, 0), 3);
	TASSH(StepForward(&cs, 100, &data), 4);
	TASSH(CheckErrorExpected(&cs, 3), 5);
	TASSH(CheckStateExpected(&cs, 8), 6);
	TASSH(ChargerGetCharged(&cs) >= (700UL*60UL*60UL), 7);
	TASSH(ChargerGetCharged(&cs) < (701UL*60UL*60UL), 8);
	TicksAnalyze(&data);
	return 0;
}

//test stop by overtemperature and restart if cooldown is done
static uint8_t test12(void) {
	chargerState_t cs;
	ChargerInitCommon(&cs);
	input_t data;
	CommonStartCondition(&data);
	//start the charge
	TASSH(StepForward(&cs, 100, &data), 1);
	TASSH(CheckStateExpected(&cs, 1), 2);
	TASSH(CheckErrorExpected(&cs, 0), 3);
	data.battI = 100;
	TASSH(CheckStateExpected(&cs, 1), 4);
	TASSH(CheckErrorExpected(&cs, 0), 5);

	//too high temperature
	data.battTemp = 500;
	data.pwmMin = 0;
	data.pwmMax = 0;
	TASSH(StepForward(&cs, 100, &data), 6);
	TASSH(CheckStateExpected(&cs, 2), 7);
	TASSH(CheckErrorExpected(&cs, 0), 8);
	data.battI = 0;
	TASSH(StepForward(&cs, 100, &data), 9);
	TASSH(CheckStateExpected(&cs, 2), 10);
	TASSH(CheckErrorExpected(&cs, 0), 11);

	//drop temperature a little
	data.battTemp = 420;
	TASSH(StepForward(&cs, 100, &data), 12);
	TASSH(CheckStateExpected(&cs, 2), 13);
	TASSH(CheckErrorExpected(&cs, 0), 14);

	//drop temperature more, should allow restart charging
	data.battTemp = 350;
	TASSH(StepForward(&cs, 100, &data), 15);
	TASSH(CheckStateExpected(&cs, 0), 16);
	TASSH(CheckErrorExpected(&cs, 0), 17);

	//start charge again
	data.pwmMin = CHARGER_PWM_MAX / 2;
	data.pwmMax = CHARGER_PWM_MAX;
	TASSH(StepForward(&cs, 100, &data), 18);
	TASSH(CheckStateExpected(&cs, 1), 19);
	TASSH(CheckErrorExpected(&cs, 0), 20);
	TicksAnalyze(&data);
	return 0;
}


//test stop by undertemperature and restart if temperature has risen again
static uint8_t test13(void) {
	chargerState_t cs;
	ChargerInitCommon(&cs);
	input_t data;
	CommonStartCondition(&data);
	//start the charge
	TASSH(StepForward(&cs, 100, &data), 1);
	TASSH(CheckStateExpected(&cs, 1), 2);
	TASSH(CheckErrorExpected(&cs, 0), 3);
	data.battI = 100;
	TASSH(CheckStateExpected(&cs, 1), 4);
	TASSH(CheckErrorExpected(&cs, 0), 5);

	//too low temperature
	data.battTemp = -150;
	data.pwmMin = 0;
	data.pwmMax = 0;
	TASSH(StepForward(&cs, 100, &data), 6);
	TASSH(CheckStateExpected(&cs, 2), 7);
	TASSH(CheckErrorExpected(&cs, 0), 8);
	data.battI = 0;
	TASSH(StepForward(&cs, 100, &data), 9);
	TASSH(CheckStateExpected(&cs, 2), 10);
	TASSH(CheckErrorExpected(&cs, 0), 11);

	//increase temperature a little
	data.battTemp = -80;
	TASSH(StepForward(&cs, 100, &data), 12);
	TASSH(CheckStateExpected(&cs, 2), 13);
	TASSH(CheckErrorExpected(&cs, 0), 14);

	//increase temperature more, should allow restart charging
	data.battTemp = 0;
	TASSH(StepForward(&cs, 100, &data), 15);
	TASSH(CheckStateExpected(&cs, 0), 16);
	TASSH(CheckErrorExpected(&cs, 0), 17);

	//start charge again
	data.pwmMin = CHARGER_PWM_MAX / 2;
	data.pwmMax = CHARGER_PWM_MAX;
	TASSH(StepForward(&cs, 100, &data), 18);
	TASSH(CheckStateExpected(&cs, 1), 19);
	TASSH(CheckErrorExpected(&cs, 0), 20);
	TicksAnalyze(&data);
	return 0;
}

//do not start charge if battery is too hot
static uint8_t test14(void) {
	chargerState_t cs;
	ChargerInitCommon(&cs);
	input_t data;
	CommonStartCondition(&data);
	data.battTemp = 460;
	data.pwmMin = 0;
	data.pwmMax = 0;
	//not start the charge
	TASSH(StepForward(&cs, 100, &data), 1);
	TASSH(CheckStateExpected(&cs, 2), 2);
	TASSH(CheckErrorExpected(&cs, 0), 3);
	TicksAnalyze(&data);
	return 0;
}

//do not start charge if battery is too cold
static uint8_t test15(void) {
	chargerState_t cs;
	ChargerInitCommon(&cs);
	input_t data;
	CommonStartCondition(&data);
	data.battTemp = -110;
	data.pwmMin = 0;
	data.pwmMax = 0;
	//not start the charge
	TASSH(StepForward(&cs, 100, &data), 1);
	TASSH(CheckStateExpected(&cs, 2), 2);
	TASSH(CheckErrorExpected(&cs, 0), 3);
	TicksAnalyze(&data);
	return 0;
}


//stop the charge if input voltage drops too low, restart if it is ok again
static uint8_t test16(void) {
	chargerState_t cs;
	ChargerInitCommon(&cs);
	input_t data;
	CommonStartCondition(&data);
	//start the charge
	TASSH(StepForward(&cs, 100, &data), 1);
	TASSH(CheckStateExpected(&cs, 1), 2);
	TASSH(CheckErrorExpected(&cs, 0), 3);
	data.battI = 100;
	TASSH(CheckStateExpected(&cs, 1), 4);
	TASSH(CheckErrorExpected(&cs, 0), 5);

	//too low input voltage, also no current (eg USB connector disconnected)
	data.pwmMin = 0;
	data.pwmMax = 0;
	data.battI = 0;
	data.inU = 4000;
	TASSH(StepForward(&cs, 100, &data), 6);
	TASSH(CheckStateExpected(&cs, 5), 7);
	TASSH(CheckErrorExpected(&cs, 0), 8);
	TASSH(StepForward(&cs, 100, &data), 9);
	TASSH(CheckStateExpected(&cs, 5), 10);
	TASSH(CheckErrorExpected(&cs, 0), 11);

	//increase voltage a little
	data.inU = 4450;
	TASSH(StepForward(&cs, 100, &data), 12);
	TASSH(CheckStateExpected(&cs, 5), 13);
	TASSH(CheckErrorExpected(&cs, 0), 14);

	//increase voltage more, should allow restart charging
	data.inU = 4700;
	TASSH(StepForward(&cs, 100, &data), 15);
	TASSH(CheckStateExpected(&cs, 0), 16);
	TASSH(CheckErrorExpected(&cs, 0), 17);

	//start charge again
	data.pwmMin = CHARGER_PWM_MAX / 2;
	data.pwmMax = CHARGER_PWM_MAX;
	TASSH(StepForward(&cs, 100, &data), 18);
	TASSH(CheckStateExpected(&cs, 1), 19);
	TASSH(CheckErrorExpected(&cs, 0), 20);
	TicksAnalyze(&data);
	return 0;
}

//stop the charge if input voltage is too high, restart if it is ok again
static uint8_t test17(void) {
	chargerState_t cs;
	ChargerInitCommon(&cs);
	input_t data;
	CommonStartCondition(&data);
	//start the charge
	TASSH(StepForward(&cs, 100, &data), 1);
	TASSH(CheckStateExpected(&cs, 1), 2);
	TASSH(CheckErrorExpected(&cs, 0), 3);
	data.battI = 100;
	TASSH(CheckStateExpected(&cs, 1), 4);
	TASSH(CheckErrorExpected(&cs, 0), 5);

	//too high input voltage
	data.pwmMin = 0;
	data.pwmMax = 0;
	data.inU = 5600;
	TASSH(StepForward(&cs, 100, &data), 6);
	TASSH(CheckStateExpected(&cs, 6), 7);
	TASSH(CheckErrorExpected(&cs, 0), 8);
	TASSH(StepForward(&cs, 100, &data), 9);
	TASSH(CheckStateExpected(&cs, 6), 10);
	TASSH(CheckErrorExpected(&cs, 0), 11);

	//decrease voltage, should allow restart charging
	data.inU = 5400;
	TASSH(StepForward(&cs, 100, &data), 12);
	TASSH(CheckStateExpected(&cs, 0), 13);
	TASSH(CheckErrorExpected(&cs, 0), 14);

	//start charge again
	data.pwmMin = CHARGER_PWM_MAX / 2;
	data.pwmMax = CHARGER_PWM_MAX;
	TASSH(StepForward(&cs, 100, &data), 15);
	TASSH(CheckStateExpected(&cs, 1), 16);
	TASSH(CheckErrorExpected(&cs, 0), 17);
	TicksAnalyze(&data);
	return 0;
}

//do not start charge if input voltage is too low
static uint8_t test18(void) {
	chargerState_t cs;
	ChargerInitCommon(&cs);
	input_t data;
	CommonStartCondition(&data);
	data.inU = 4200;
	data.pwmMin = 0;
	data.pwmMax = 0;
	//not start the charge
	TASSH(StepForward(&cs, 100, &data), 1);
	TASSH(CheckStateExpected(&cs, 5), 2);
	TASSH(CheckErrorExpected(&cs, 0), 3);
	TicksAnalyze(&data);
	return 0;
}

//do not start charge if input voltage is too high
static uint8_t test19(void) {
	chargerState_t cs;
	ChargerInitCommon(&cs);
	input_t data;
	CommonStartCondition(&data);
	data.inU = 5600;
	data.pwmMin = 0;
	data.pwmMax = 0;
	//not start the charge
	TASSH(StepForward(&cs, 100, &data), 1);
	TASSH(CheckStateExpected(&cs, 6), 2);
	TASSH(CheckErrorExpected(&cs, 0), 3);
	TicksAnalyze(&data);
	return 0;
}

//successfully relive the battey by precharging
static uint8_t test20(void) {
	chargerState_t cs;
	ChargerInitCommon(&cs);
	input_t data;
	CommonStartCondition(&data);
	data.battU = 2100; //start the precharge
	data.pwmMin = CHARGER_PWM_MAX / 3;
	data.pwmMax = 107;
	TASSH(StepForward(&cs, 100, &data), 1);
	TASSH(CheckErrorExpected(&cs, 0), 2);
	TASSH(CheckStateExpected(&cs, 7), 3);

	//let's do a 25min charge
	for (uint32_t i = 0; i < (1000UL * 60UL * 25UL); i+= 100) {
		TASSH(StepForward(&cs, 100, &data), 4);
		TASSH(CheckErrorExpected(&cs, 0), 5);
		TASSH(CheckStateExpected(&cs, 7), 6);
		if (data.pwmOut < 100) {
			data.battI = 0;
		} else {
			data.battI = data.pwmOut - 100;
		}
	}
	TASSH(ChargerGetCharged(&cs) < (7UL*60UL*25UL), 7); //max 2.92mAh (~7mA average)
	TASSH(ChargerGetCharged(&cs) > (5UL*60UL*24UL), 8); //min 2.0mAh (~5mA average)

	data.battU = 3050; //precharge successful, go to state 0
	data.pwmMin = 0;
	data.pwmMax = 0;
	TASSH(StepForward(&cs, 100, &data), 9);
	TASSH(CheckErrorExpected(&cs, 0), 10);
	TASSH(CheckStateExpected(&cs, 0), 11);
	//go to normal charge
	data.pwmMin = CHARGER_PWM_MAX / 2;
	data.pwmMax = CHARGER_PWM_MAX;
	TASSH(StepForward(&cs, 100, &data), 9);
	TASSH(CheckErrorExpected(&cs, 0), 10);
	TASSH(CheckStateExpected(&cs, 1), 11);
	TASSH(ChargerGetCycles(&cs) == 1, 12);
	TASSH(ChargerGetPreCycles(&cs) == 1, 13);
	TicksAnalyze(&data);
	return 0;
}

//failing to relive the battey by precharging
static uint8_t test21(void) {
	chargerState_t cs;
	ChargerInitCommon(&cs);
	input_t data;
	CommonStartCondition(&data);
	data.battU = 2100; //start the precharge
	data.pwmMin = CHARGER_PWM_MAX / 3;
	data.pwmMax = 107;
	TASSH(StepForward(&cs, 100, &data), 1);
	TASSH(CheckErrorExpected(&cs, 0), 2);
	TASSH(CheckStateExpected(&cs, 7), 3);

	//let's do a 29.9min charge
	for (uint32_t i = 0; i < (1000UL * 60UL * 30UL - 100UL); i+= 100) {
		TASSH(StepForward(&cs, 100, &data), 4);
		TASSH(CheckErrorExpected(&cs, 0), 5);
		TASSH(CheckStateExpected(&cs, 7), 6);
		if (data.pwmOut < 100) {
			data.battI = 0;
		} else {
			data.battI = data.pwmOut - 100;
		}
	}

	//on the 30th min, the battery should be considered defective
	data.pwmMin = 0;
	data.pwmMax = 0;
	TASSH(StepForward(&cs, 100, &data), 7);
	TASSH(CheckErrorExpected(&cs, 2), 8);
	TASSH(CheckStateExpected(&cs, 3), 9);
	TASSH(ChargerGetCharged(&cs) < (7UL*60UL*25UL), 10); //max 2.92mAh (~7mA average)
	TASSH(ChargerGetCharged(&cs) > (5UL*60UL*24UL), 11); //min 2.0mAh (~5mA average)

	//magically recovered, but out of time, stay in error case
	data.battU = 3050;
	data.pwmMin = 0;
	data.pwmMax = 0;
	TASSH(StepForward(&cs, 100, &data), 12);
	TASSH(CheckErrorExpected(&cs, 2), 13);
	TASSH(CheckStateExpected(&cs, 3), 14);
	TicksAnalyze(&data); //not enough flash
	return 0;
}

//just never gets full
static uint8_t test22(void) {
	chargerState_t cs;
	ChargerInitCommon(&cs);
	input_t data;
	CommonStartCondition(&data);
	//let's do a 60h charge
	const uint16_t timeDelta = 500; //500ms steps allows faster testing than 100ms steps
	for (uint32_t i = 0; i < (1000UL * 60UL * 60UL * 60UL); i+= timeDelta) {
		TASSH(StepForward(&cs, timeDelta, &data), 4);
		TASSH(CheckErrorExpected(&cs, 0), 5);
		TASSH(CheckStateExpected(&cs, 1), 6);
		data.battI = 8;
	}

	//on the 30th min, the battery should be considered defective
	data.pwmMin = 0;
	data.pwmMax = 0;
	TASSH(StepForward(&cs, timeDelta, &data), 7);
	TASSH(CheckErrorExpected(&cs, 3), 8);
	TASSH(CheckStateExpected(&cs, 8), 9);

	//stay in the error state
	TASSH(StepForward(&cs, 100, &data), 12);
	TASSH(CheckErrorExpected(&cs, 3), 13);
	TASSH(CheckStateExpected(&cs, 8), 14);
	TicksAnalyze(&data); //not enough flash
	return 0;
}

//charger defective
static uint8_t test24(void) {
	chargerState_t cs;
	ChargerInitCommon(&cs);
	input_t data;
	CommonStartCondition(&data);
	data.battI = 110; //too much. We want only 100mA
	data.pwmMin = 0;
	TimeForwardS(&cs, 30, &data);
	TASSH(CheckErrorExpected(&cs, 4), 2);
	TASSH(CheckStateExpected(&cs, 10), 3);
	TicksAnalyze(&data); //not enough flash
	return 0;
}

//start charge because we requested this, even when the battery is 99% full
static uint8_t test25(void) {
	chargerState_t cs;
	ChargerInitCommon(&cs);
	input_t data;
	CommonStartCondition(&data);
	data.battU = 3350;
	data.requestCharge = 1;
	TASSH(StepForward(&cs, 100, &data), 1);
	TASSH(CheckErrorExpected(&cs, 0), 2);
	TASSH(CheckStateExpected(&cs, 1), 3);
	TASSH(TimeForwardS(&cs, 1, &data), 4);
	TASSH(CheckErrorExpected(&cs, 0), 5);
	TASSH(CheckStateExpected(&cs, 1), 6);
	TicksAnalyze(&data);
	return 0;
}


typedef uint8_t (*test_t)(void);

test_t g_tests[] = {
&test1,
&test2,
&test3,
&test4,
&test5,
&test6,
&test7,
&test8,
&test9,
&test10,
&test11,
&test12,
&test13,
&test14,
&test15,
&test16,
&test17,
&test18,
&test19,
&test20,
//unfortunately, the AVR flash is just full. ...split into two binaries?
#ifndef __AVR_ARCH__
&test21,
&test22,
&test23,
&test24,
&test25,
#endif
};


#ifdef __AVR_ARCH__
static int8_t RunTests(void) __attribute__((noreturn));
#endif

const char EEMEM strStartTest[] = "Start test %u\r\n";
const char EEMEM strMaxTick[] = "Max ticks of all tests: %u\r\n";
const char EEMEM strDoneSucces[] = "Test done - failure\r\n";
const char EEMEM strDoneFail[] = "Test done - success\r\n";

static int8_t RunTests(void) {
	int8_t result = 0; //succes
	uint8_t entries = sizeof(g_tests) / sizeof(test_t);
	for (uint8_t i = 0; i < entries; i++) {
		myprintf(strStartTest, (unsigned int)(i + 1));
		uint8_t e = g_tests[i]();
		if (e != 0) {
			PrintFailed(i + 1, e);
			result = -1;
		}
	}
	//worst case found in tests 5 and 6 with 687 ticks
	myprintf(strMaxTick, (unsigned int)g_maxTicks);

	if (result) {
		myprintf(strDoneSucces);
	} else {
		myprintf(strDoneFail);
	}
#ifdef __AVR_ARCH__
	while(1);
#else
	return result;
#endif
}

const char EEMEM strStart[] = "Unit tests 0.10\r\n";

int main(void) {
	HardwareInit();
	ArmUserprog();
	waitms(1);
	ArmRun();
	waitms(5000); //let the ARM start his UART forward application
	myprintf(strStart);
	return RunTests();
}
