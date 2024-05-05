#include <stdint.h>
#include <string.h>

#include "chargerStatemachine.h"

//#define DEBUG

#ifdef DEBUG
#include <stdio.h>
//use printf for debug messages
#define CHARGER_DEBUGMSG printf

#else
//ignore debug messages
#define CHARGER_DEBUGMSG(...)

#endif


//According to the datasheet the battery should not go below 2V
//[mV]
#define BATT_MIN 2100

//This is the voltage where normal charge may start
//[mV]
#define BATT_EMPTY 3000

//At this point we restart the charging cycle
//[mV]
#define BATT_NOT_FULL 3270


/*We never charge to more than 3600mV, to prevent damange on the ARM side
  with some tolerance, the voltage should be cut off at ~3550mV +-1%
  3550mV -1% - 50mV measurement error -> 3464mV
*/
#define BATT_FULL 3464


//According to the datasheet at 3.65V charging should stop. We remove 50mV for measurement tolerance.
//[mV]
#define BATT_MAX 3600

//According to the datasheet the battery may never exceed 3.7V. We remove 50mV for measurement tolerance
//[mV]
#define BATT_DEFECTIVE 3650

//According to the datasheet charging is allowed between -10°C and 45°C
//[0.1°C]
#define BATT_TEMPERATURE_MAX 450
//[0.1°C]
#define BATT_TEMPERATURE_MIN -100

//This is 1/3C of the 600mAh battery. Fuse is 250mA.
//[mA]
#define BATT_CURRENT_MAX 200

//maximum time is 60h
//[ms]
#define BATT_TIME_MAX (60ULL * 60ULL * 60ULL * 1000ULL)

//put up to 700mAh into the 600mAh battery
//[mAms]
#define BATT_CAP_MAX (700UL * 60UL * 60UL * 1000UL)

//0.01C according to the datasheet. 600mAh -> 6mA
//[mA]
#define BATT_PRECHARGE_CURRENT_MAX 6

//Must reach 3V within 30min, or battery is defective
//[ms]
#define BATT_PRECHARGE_TIME_MAX (30ULL * 60ULL * 1000ULL)

//[mV]
#define INPUT_START_MIN 4500

//[mV]
#define INPUT_CONT_MIN 4400
//[mV]
#define INPUT_MAX 5500


void ChargerInit(chargerState_t * pCS, uint8_t errorState, uint32_t chargingCycles,
                 uint32_t prechargingCycles, uint64_t chargingSumAllTime) {
	memset(pCS, 0, sizeof(chargerState_t));
	pCS->error = errorState;
	pCS->chargingSumAllTime = chargingSumAllTime;
	pCS->chargingCycles = chargingCycles;
	pCS->prechargingCycles = prechargingCycles;
}


//called by state 1 (battery charging) or state 7 (battery precharging)
static uint16_t ChargerRegulator(chargerState_t * pCS, uint16_t battU, uint16_t inU, uint16_t battI, int16_t battTemp, uint16_t inImax, uint16_t timePassed, uint16_t battUtarget, uint16_t battImax, uint32_t timeOut) {
	pCS->chargingTime += timePassed;
	pCS->chargingSum += battI * timePassed;
	pCS->chargingSumAllTime += battI * timePassed;
	if (battU >= battUtarget) { //stop charging, its full
		pCS->state = 0;
		CHARGER_DEBUGMSG("Stop charge - full (1)\n");
		return 0; //testcase 20
	}
	if ((battTemp >= BATT_TEMPERATURE_MAX) || (battTemp <= BATT_TEMPERATURE_MIN)) {
		pCS->state = 2;
		CHARGER_DEBUGMSG("Stop charge - temperature\n");
		return 0;  //testcase 12, testcase 13
	}
	if (inU < INPUT_CONT_MIN) {
		pCS->state = 5;
		CHARGER_DEBUGMSG("Stop charge - input voltage too low\n");
		return 0; //testcase 16
	}
	if (inU > INPUT_MAX) {
		pCS->state = 6;
		CHARGER_DEBUGMSG("Stop charge - input voltage too high\n");
		return 0; //testcase 17
	}

	if (pCS->chargingSum > BATT_CAP_MAX) {
		pCS->state = 8;
		pCS->error = 3;
		CHARGER_DEBUGMSG("Stop charge - energy vanished\n");
		return 0; //testcase 11
	}
	if (pCS->chargingTime >= timeOut) {
		if (battUtarget == BATT_EMPTY) {
			pCS->state = 3;
			pCS->error = 2; //ERROR: Could not recover deep discharged battery
			return 0; //testcase 21
		} else {
			pCS->state = 8;
			pCS->error = 3; //ERROR: The energy vanishes somehow while charging
			return 0; //testcase 22
		}
	}
	if (battI <= 1) {
		if (battU >= BATT_FULL) {
			pCS->state = 0;
			CHARGER_DEBUGMSG("Stop charge - full (2)\n");
			return 0; //testcase 4
		}
		if (pCS->pwm == CHARGER_PWM_MAX) {
			pCS->state = 8;
			pCS->error = 5; //ERROR: Maximum PWM, but no current measured!
			pCS->noCurrentVoltage = inU;
			CHARGER_DEBUGMSG("Stop charge - no current but max PWM\n");
			return 0; //testcase 10
		}
	}
	if (((battI + 1) < battImax) && ((battI + 1) < inImax)) {
		if (inU >= INPUT_START_MIN) {
			if (pCS->pwm < CHARGER_PWM_MAX) { //ramp up the current
				if (battU < BATT_FULL) { //do not generate a higher signal than required
					pCS->pwm++;
				}
			}
		}
	}
	if ((battI > (battImax + 1)) || (battI > (inImax + 1))) {
		if (pCS->pwm > 0) { //ramp down the current
			pCS->pwm--;
		} else {
			pCS->state = 10;
			pCS->error = 4;
			CHARGER_DEBUGMSG("Stop charge - PWM min, but still too much current\n");
			return 0; //testcase 24
		}
	}
	return pCS->pwm; //just continue to charge
}

uint16_t ChargerCycle(chargerState_t * pCS, uint16_t battU, uint16_t inU, uint16_t battI, int16_t battTemp, uint16_t inImax, uint16_t timePassed, uint8_t requestFullCharge) {
	if (pCS->error) {
		CHARGER_DEBUGMSG("Error state present\n");
		return 0; //testcase 3
	}
	if (battU < BATT_MIN) { //allowed minium voltage is 2.00V
		pCS->error = 2; //ERROR: Battery voltage too low
		pCS->state = 3;
		CHARGER_DEBUGMSG("Battery voltage too low\n");
		return 0; //testcase 8
	}
	if (battU >= BATT_DEFECTIVE) {
		pCS->error = 1; //ERROR: Battery voltage too high
		pCS->state = 4;
		CHARGER_DEBUGMSG("Battery voltage too high\n");
		return 0; //testcase 9
	}
	if (timePassed < 50) {
		CHARGER_DEBUGMSG("Too little time\n");
		pCS->state = 9;
		return 0; //testcase 7
	}
	if (timePassed > 1000) {
		CHARGER_DEBUGMSG("Too much time passed\n");
		pCS->state = 9;
		return 0; //testcase 23
	}

	if (inImax == 0) { //obviously, charging should be disabled
		CHARGER_DEBUGMSG("Disable charging\n");
		pCS->state = 0;
		return 0; //testcase 2
	}
	if (pCS->state == 0) { //check if we should start to charge?
		CHARGER_DEBUGMSG("Check charging start\n");
		if ((battU < BATT_NOT_FULL) || ((requestFullCharge) && (battU < BATT_FULL))) {
			CHARGER_DEBUGMSG("Check charging start - battery not full\n");
			if ((battTemp >= BATT_TEMPERATURE_MAX) || (battTemp <= BATT_TEMPERATURE_MIN)) {
				pCS->state = 2;
				return 0; //testcase 14, testcase 15
			}
			if (inU < INPUT_START_MIN) {
				pCS->state = 5;
				return 0; //testcase 18
			}
			if (inU >= INPUT_MAX) {
				pCS->state = 6;
				return 0; //testcase 19
			}
			if (battU > BATT_EMPTY) { //ok, we start a normal charge
				CHARGER_DEBUGMSG("Start normal charge\n");
				pCS->state = 1;
				pCS->pwm = CHARGER_PWM_MAX / 2;
				pCS->chargingTime = 0;
				pCS->chargingSum = 0;
				pCS->chargingCycles++;
				return pCS->pwm; //testcase 4
			} else { //precharge, look if battery could be recovered
				CHARGER_DEBUGMSG("Start precharge\n");
				pCS->state = 7;
				pCS->pwm = CHARGER_PWM_MAX / 3;
				pCS->chargingTime = 0;
				pCS->chargingSum = 0;
				pCS->prechargingCycles++;
				return pCS->pwm; //testcase 20, testcase 21
			}
		} else {
			CHARGER_DEBUGMSG("Battery already full enough\n");
			pCS->state = 0;
			return 0; //testcase 1
		}
	}
	if (pCS->state == 1) { //battery does a normal charge -> ramp pwm as long as the current is < battery and usb limit
		return ChargerRegulator(pCS, battU, inU, battI, battTemp, inImax, timePassed, BATT_MAX, BATT_CURRENT_MAX, BATT_TIME_MAX);
	}
	if (pCS->state == 2) { //check if still out of temperature bounds
		if (((battTemp < BATT_TEMPERATURE_MAX - 50)) && (battTemp > (BATT_TEMPERATURE_MIN + 50))) {
			pCS->state = 0;
			return 0; //testcase 12, testcase 13
		}
	}
	//never go out of state 3, 4
	if (pCS->state == 5) { //check if usb source is valid again
		if (inU > INPUT_START_MIN) {
			pCS->state = 0;
			return 0; //testcase 16
		}
	}
	if (pCS->state == 6) { //input voltage too high
		if (inU < INPUT_MAX) {
			pCS->state = 0;
			return 0; //testcase 17
		}
	}
	if (pCS->state == 7) { //battery precharge
		return ChargerRegulator(pCS, battU, inU, battI, battTemp, inImax, timePassed, BATT_EMPTY, BATT_PRECHARGE_CURRENT_MAX, BATT_PRECHARGE_TIME_MAX);
	}
	//never go out of state 8
	return 0; //testcases 9, 10, 11
}

