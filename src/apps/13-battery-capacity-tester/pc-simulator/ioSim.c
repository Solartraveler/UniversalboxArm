#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "io.h"

#include "simulated.h"

#define BATTERIES 2

static bool g_sinkEnabled[BATTERIES];
static uint32_t g_sinkStarted[BATTERIES];

float AdcVoltage(uint8_t battery) {
	if (battery < BATTERIES) {
		uint32_t delta = HAL_GetTick() - g_sinkStarted[battery];
		if (battery == 0) {
			if (g_sinkEnabled[0]) {
				if ((delta < (1000 * 60 * 1))) { //1min until empty
					return 4.75;
				}
			} else {
				return 4.85;
			}
		}
		if (battery == 1) {
			if (g_sinkEnabled[1]) {
				if (delta < (1000 * 60 * 5)) { //5min until empty
					return 5.25;
				}
			} else {
				return 5.30;
			}
		}
	}
	return 0.25;
}

void SinkInit(void) {
}

void SinkSet(uint8_t battery, bool enabled) {
	if (battery < BATTERIES) {
		if ((g_sinkEnabled[battery] == false) && (enabled)) {
			g_sinkStarted[battery] = HAL_GetTick();
		}
		g_sinkEnabled[battery] = enabled;
	}
}
