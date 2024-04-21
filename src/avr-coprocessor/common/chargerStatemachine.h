#pragma once

#include <stdint.h>

typedef struct {
	/*
	 0: No error
	 1: Battery voltage too high - defective
	 2: Battery voltage too low - defective, or started without a battery inserted
	 3: Failed to fully charge - charger or battery defective
	 4: Too high current - charger or battery defective (too high current)
	 5: Too low current - charger or battery defective (too low current)
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
	//variables for statistics only. Do not change the charging behaviour:
	uint64_t chargingSumAllTime; //mAms
	uint32_t chargingCycles; //increments every time a charging starts
	uint32_t prechargingCycles; //increments every time a precharging starts
} chargerState_t;

#define CHARGER_PWM_MAX 127


/* Initializes pCS. If some error has not been manually reset, it should be
re-entered as errorState since the last power-fail.
errorState: Previous recorded error state
chargingCycles: Number of charging cycles done
prechargingCycles: Number of precharging cycles done
chargingSumAllTimes: Total charged in [mAms]
*/
void ChargerInit(chargerState_t * pCS, uint8_t errorState, uint32_t chargingCycles,
                 uint32_t prechargingCycles, uint64_t chargingSumAllTime);

/* Call every 100ms
The internal state is stored in pCS. No data within this call should be modified from the outside.
battU: Battery voltage in [mV]
inU: Input voltage in [mV]
battI: Charging current in [mA]
battTemp: Battery temperature in [0.1°C]
inImax: Maximum allowed charging current in [mA]
timePassed: Time in [ms] since the last call. Minimum is 50ms to avoid oscilations.
requestFullCharge: Automatic recharging starts when the battery is already partly empty.
  Setting this to 1 for one cycle, starts a charge even if the voltage is not low enough
  to start the automatic recharge.

To not trigger an error, the charger current must be measured before the input voltage,
because a current of 0 with an input voltage is considered a hardware error.
There could be a power cord removal between the two measurements.

Returns: PWM value
*/
uint16_t ChargerCycle(chargerState_t * pCS, uint16_t battU, uint16_t inU, uint16_t battI, int16_t battTemp, uint16_t inImax, uint16_t timePassed, uint8_t requestFullCharge);

inline static uint8_t ChargerGetError(chargerState_t * pCS) {
	return pCS->error;
}

inline static uint8_t ChargerGetState(chargerState_t * pCS) {
	return pCS->state;
}

//returns mAs
inline static uint32_t ChargerGetCharged(chargerState_t * pCS) {
	return pCS->chargingSum / 1000ULL;
}

//returns mAs
inline static uint64_t ChargerGetChargedTotal(chargerState_t * pCS) {
	return pCS->chargingSumAllTime;
}

inline static uint32_t ChargerGetCycles(chargerState_t * pCS) {
	return pCS->chargingCycles;
}

inline static uint32_t ChargerGetPreCycles(chargerState_t * pCS) {
	return pCS->prechargingCycles;
}

inline static uint32_t ChargerGetTime(chargerState_t * pCS) {
	return pCS->chargingTime;
}
