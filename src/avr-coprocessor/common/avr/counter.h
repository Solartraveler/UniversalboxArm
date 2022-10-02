#pragma once

#include <stdint.h>
#include <avr/io.h>

#include "counter.h"

/* Do not use when Timer functions from hardware.h are used
*/

static inline void CounterStart(void) {
	TCCR0B = 0; //stop timer
	TCCR0A = (1<<TCW0); //16bit mode
	TCNT0H = 0; //clear counter
	TCNT0L = 0; //clear counter
	TCCR0B = (1<<CS01); //start timer with prescaler = 8
}

/* Gets a timestamp with 1/8 of the F_CPU resolution
*/
static inline uint16_t CounterGet(void) {
	uint16_t c = TCNT0L;
	c |= TCNT0H << 8;
	return c;
}

//can count up to 0.5s at 1MHz
static inline uint16_t CounterGetMs(void) {
	uint16_t c = CounterGet();
	c /= (F_CPU / 8 / 1000);
	return c;
}
