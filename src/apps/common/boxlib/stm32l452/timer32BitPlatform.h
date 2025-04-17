#pragma once

#include <stdint.h>

#include "main.h"

static inline void Timer32BitReset(void) {
	TIM2->CNT = 0;
}

static inline void Timer32BitStart(void) {
	TIM2->CR1 |= TIM_CR1_CEN;
}

static inline void Timer32BitStop(void) {
	TIM2->CR1 &= ~TIM_CR1_CEN;
}

static inline uint32_t Timer32BitGet(void) {
	return TIM2->CNT;
}
