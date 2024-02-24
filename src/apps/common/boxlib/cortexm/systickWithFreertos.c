/* Boxlib
(c) 2022 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause
*/

#include <stdbool.h>

#include "boxlib/systickWithFreertos.h"

#include "main.h"



bool g_systickForFreertos;

//This allows to use the file even without FreeRTOS
__attribute__((weak)) void xPortSysTickHandler(void) {
}

void SysTick_Handler(void)
{
	if (g_systickForFreertos) {
		xPortSysTickHandler();
	}
	HAL_IncTick();
}

void SystickDisable(void) {
	SysTick->CTRL = 0;
	SysTick->LOAD = 0;
	SysTick->VAL = 0;
}

void SystickForFreertosEnable(void) {
	g_systickForFreertos = true;
}
