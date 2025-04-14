/* Boxlib
(c) 2025 by Malte Marwedel

SPDX-License-Identifier: BSD-3-Clause
*/


#include <stdint.h>
#include <stdio.h>

#include "boxlib/stackSampler.h"

#include "main.h"

static uintptr_t g_stackMin;
static uintptr_t g_stackMax;


void StackSampleInit(void) {
	uint8_t * dummy;
	uintptr_t addr = (uintptr_t)&dummy;
	g_stackMax = g_stackMin = addr;
	__HAL_RCC_TIM15_CLK_ENABLE();
	TIM15->CR1 = 0; //all stopped
	TIM15->CR2 = 0;
	TIM15->CNT = 0;
	TIM15->PSC = 0;
	TIM15->SR = 0;
	TIM15->DIER = TIM_DIER_UIE;
	TIM15->ARR = 255;
	HAL_NVIC_SetPriority(TIM1_BRK_TIM15_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(TIM1_BRK_TIM15_IRQn);
	TIM15->CR1 |= TIM_CR1_CEN;
}

void TIM1_BRK_TIM15_IRQHandler(void) {
	uint8_t dummy;
	uintptr_t addr = (uintptr_t)&dummy;
	if (addr < g_stackMin) {
		g_stackMin = addr;
	}
	if (addr > g_stackMax) {
		g_stackMax = addr;
	}
	TIM15->SR = 0;
	NVIC_ClearPendingIRQ(TIM1_BRK_TIM15_IRQn);
}

void StackSampleCheck(void) {
	uintptr_t delta = g_stackMax - g_stackMin;
	static uintptr_t deltaLast = 0;
	if (delta > deltaLast) {
		printf("Stack max 0x%x min 0x%x delta %u\r\n", (unsigned int)g_stackMax, (unsigned int)g_stackMin, (unsigned int)delta);
		deltaLast = delta;
	}
}
