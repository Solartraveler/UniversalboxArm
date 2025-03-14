#include <stdbool.h>
#include <stdint.h>


#include "io.h"

#include "boxlib/simpleadc.h"
#include "boxlib/relays.h"


float AdcVoltage(uint8_t battery) {
	uint32_t channel = 0;
	if (battery == 0) {
		channel = 1;
	}
	if (battery == 1) {
		channel = 2;
	}
	float adc = AdcGet(channel);
	float uRef = AdcAvrefGet();
	/*voltage = (Adc * Uref * (R1 + R2) / R2) / AdcMax
	  Uref = 3.3V, AdcMax = 4095, R1 = 33kOhm, R2 = 10kOhm
	  voltage = (ADC * 3.3V * 43 / 10) / 4095
	  voltage = (ADC * 14.19V) / 4095
	  voltage = ADC / (4095 / 14.19V)
	*/
	return adc / 288.58f;
}

void SinkInit(void) {
	AdcInit();
	RelaysInit();
}

void SinkSet(uint8_t battery, bool enabled) {
	if (battery == 0) {
		Relay1Set(enabled);
	}
	if (battery == 1) {
		Relay2Set(enabled);
	}
}
