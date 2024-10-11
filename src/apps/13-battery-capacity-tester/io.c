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
	return adc * uRef / 409.5f; //10:1 scaling included
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
