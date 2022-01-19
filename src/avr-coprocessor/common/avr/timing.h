#pragma once

#include <stdint.h>

#include "configuration.h"

#include <util/delay.h>

static inline void waitms(uint16_t ms) {
	while(ms) {
		_delay_ms(1.0);
		ms--;
	}
}

