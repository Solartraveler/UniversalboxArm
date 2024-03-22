/* Boxlib
(c) 2024 by Malte Marwedel

SPDX-License-Identifier: BSD-3-Clause
*/

#include <stdint.h>

#include "boxlib/timer16Bit.h"

#include "main.h"

void Timer16BitInit(uint16_t prescaler) {
	__HAL_RCC_TIM10_CLK_ENABLE();
	TIM10->CR1 = 0; //all stopped
	TIM10->CR2 = 0;
	TIM10->CNT = 0;
	TIM10->PSC = prescaler;
}

void Timer16BitDeinit(void) {
	TIM10->CR1 = 0;
	__HAL_RCC_TIM10_CLK_DISABLE();
}

