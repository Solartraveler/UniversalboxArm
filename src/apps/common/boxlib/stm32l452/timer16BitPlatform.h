#pragma once

#include <stdint.h>

#include "main.h"

static inline void Timer16BitReset(void) {
	TIM6->CNT = 0;
}

static inline void Timer16BitStart(void) {
	TIM6->CR1 |= TIM_CR1_CEN;
}

static inline void Timer16BitStop(void) {
	TIM6->CR1 &= ~TIM_CR1_CEN;
}

static inline uint16_t Timer16BitGet(void) {
	return TIM6->CNT;
}
