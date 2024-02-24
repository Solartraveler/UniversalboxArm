/* Boxlib
(c) 2024 by Malte Marwedel

SPDX-License-Identifier: BSD-3-Clause
*/

#include <stdint.h>

#include "boxlib/timer16Bit.h"

#include "main.h"

void Timer16BitInit(uint16_t prescaler) {
	__HAL_RCC_TIM6_CLK_ENABLE();
	TIM6->CR1 = 0; //all stopped
	TIM6->CR2 = 0;
	TIM6->CNT = 0;
	TIM6->PSC = prescaler;
}

void Timer16BitDeinit(void) {
	TIM6->CR1 = 0;
	__HAL_RCC_TIM6_CLK_DISABLE();
}

