#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "io.h"

#include "simulated.h"

#define BATTERIES 2

static bool g_sinkState[BATTERIES];
static uint32_t g_sinkEnabled[BATTERIES];

float AdcVoltage(uint8_t battery) {
	if (battery < BATTERIES) {
		uint32_t delta = HAL_GetTick() - g_sinkEnabled[battery];
		if ((battery == 0) && (delta < (1000 * 60 * 1))) { //1min until empty
			return 4.75;
		}
		if ((battery == 1) && (delta < (1000 * 60 * 5))) { //5min until empty
			return 5.25;
		}
	}
	return 0.25;
}

void SinkInit(void) {
}

void SinkSet(uint8_t battery, bool enabled) {
	if (battery < BATTERIES) {
		if ((g_sinkState[battery] == false) && (enabled)) {
			g_sinkEnabled[battery] = HAL_GetTick();
		}
		g_sinkState[battery] = enabled;
	}
}
