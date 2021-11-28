#pragma once

#define F_CPU (128000)

#include <stdint.h>

#include <util/delay.h>

static inline void waitms(uint16_t ms) {
	while(ms) {
		_delay_ms(1.0);
		ms--;
	}
}

