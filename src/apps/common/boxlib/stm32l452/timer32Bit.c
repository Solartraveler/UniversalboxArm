/* Boxlib
(c) 2025 by Malte Marwedel

SPDX-License-Identifier: BSD-3-Clause
*/

#include <stdint.h>

#include "boxlib/timer32Bit.h"

#include "main.h"

void Timer32BitInit(uint16_t prescaler) {
	__HAL_RCC_TIM2_CLK_ENABLE();
	TIM2->CR1 = 0; //all stopped
	TIM2->CR2 = 0;
	TIM2->CNT = 0;
	TIM2->PSC = prescaler;
}

void Timer32BitDeinit(void) {
	TIM2->CR1 = 0;
	__HAL_RCC_TIM2_CLK_DISABLE();
}

