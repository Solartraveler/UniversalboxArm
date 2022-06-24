#pragma once

#include <stdint.h>

typedef struct {
	/*
	 0: No error
	 1: Battery voltage too high - defective
	 2: Battery voltage too low - defective
	 3: Failed to fully charge - charger or battery defective
	 4: Failed to measure current - charger or battery defective
	*/
	uint8_t error;
	/*
	  0: Battery full, not charging
	  1: Battery charging
	  2: Battery temperature out of bounds
	  3: No battery / battery defective
	  4: Battery voltage overflow. Battery defective
	  5: Input voltage too low for charge
	  6: Input voltage too high for charge
	  7: Battey precharging because of very low voltage
	  8: Failed to fully charge
	  9: Software failure
	 10: Charger defective
	*/
	uint8_t state;
	uint16_t pwm;
	uint32_t chargingTime; //ms
	//sum of the measured charges
	uint64_t chargingSum; //mAms
} chargerState_t;

#define CHARGER_PWM_MAX 127


/* Initializes pCS. If some error has not been manually reset, it should be
re-entered as errorState since the last power-fail.
*/
void ChargerInit(chargerState_t * pCS, uint8_t errorState);

/*Call every 100ms
The internal state is stored in pCS. No data within this call should be modified from the outside.
battU: Battery voltage in [mV]
inU: Input voltage in [mV]
battI: Charging current in [mA]
battTemp: Battery temperature in [0.1°C]
inImax: Maximum allowed charging current in [mA]
timePassed: Time in [ms] since the last call. Minimum is 50ms to avoid oscilations.

To not trigger an error, the charger current must be measured before the input voltage,
because a current of 0 with an input voltage is considered a hardware error.
There could be a power cord removal between the two measurements.

Returns: PWM value
*/
uint16_t ChargerCycle(chargerState_t * pCS, uint16_t battU, uint16_t inU, uint16_t battI, int16_t battTemp, uint16_t inImax, uint16_t timePassed);

inline static uint8_t ChargerGetError(chargerState_t * pCS) {
	return pCS->error;
}

inline static uint8_t ChargerGetState(chargerState_t * pCS) {
	return pCS->state;
}

//returns mAs
inline static uint32_t ChargerGetCharged(chargerState_t * pCS) {
	return pCS->chargingSum / 1000UL;
}
