/*
   Gamebox
    Copyright (C) 2023 by Malte Marwedel
    m.talk AT marwedels dot de

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "main.h"

#include "FreeRTOS.h"
#include "task.h"
#include "boxlib/mcu.h"

void timer_start(uint8_t prescaler) {
	__HAL_RCC_TIM15_CLK_ENABLE();
	uint16_t div = 1;
	if (prescaler == (1<<CS12)) { //should simulate 8MHz / 256 divider
		div = 256;
	}
	div = configCPU_CLOCK_HZ / (F_CPU / div);
	TIM15->CR1 &= ~TIM_CR1_CEN;
	TIM15->PSC = (div - 1);
	TIM15->CR1 = (0 << TIM_CR1_CKD_Pos);
	TIM15->CR2 = 0;
	TIM15->CNT = 0;
	TIM15->CR1 |= TIM_CR1_CEN;
}

void timer_set(uint16_t newValue) {
	TIM15->CNT = newValue;
}

uint16_t timer_get(void) {
	return TIM15->CNT;
}

void timer_stop(void) {
	TIM15->CR1 &= ~TIM_CR1_CEN;
}

/* The whole gamebox was developed with timing values being 20% too large,
so we have to adjust the timing here.
*/
void waitms(uint16_t timeMs) {
	int64_t starttime, endtime;
	static int32_t overflowTime = 0;
	if (no_delays != 1) {
		int32_t waitingTime = overflowTime + timeMs * 800; //decrease by ~20%
		if (waitingTime >= 1000) {
			starttime = McuTimestampUs();
			vTaskDelay(waitingTime / 1000);
			endtime = McuTimestampUs();
			waitingTime -= endtime - starttime;
		}
		overflowTime = waitingTime;
	}
}
