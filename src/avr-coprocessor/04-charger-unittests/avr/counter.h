#pragma once

#include <stdint.h>
#include <avr/io.h>

#include "counter.h"


static inline void CounterStart(void) {
	TCCR0B = 0; //stop timer
	TCCR0A = (1<<TCW0); //16bit mode
	TCNT0H = 0; //clear counter
	TCNT0L = 0; //clear counter
	TCCR0B = (1<<CS00); //start timer with prescaler = 1
}

static inline uint32_t CounterGet(void) {
	uint16_t c = TCNT0L;
	c |= TCNT0H << 8;
	return c;
}
