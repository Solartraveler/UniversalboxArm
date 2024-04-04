#pragma once

#include <stdint.h>

#include "main.h"

/*Provides a simple abstract functions for using a timer for performance tests.
  For longer performance tests, it is better to use the 1ms values from HAL_GetTick()
  The timer counts with the peripheral clock divided by (prescaler + 1).
*/
void Timer16BitInit(uint16_t prescaler);

static inline void Timer16BitReset(void) {
	TIM10->CNT = 0;
}

static inline void Timer16BitStart(void) {
	TIM10->CR1 |= TIM_CR1_CEN;
}

static inline void Timer16BitStop(void) {
	TIM10->CR1 &= ~TIM_CR1_CEN;
}

static inline uint16_t Timer16BitGet(void) {
	return TIM10->CNT;
}

void Timer16BitDeinit(void);

