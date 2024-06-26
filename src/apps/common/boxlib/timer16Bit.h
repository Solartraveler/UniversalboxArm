#pragma once

#include <stdint.h>

#include "main.h"

#include "timer16BitPlatform.h"

/*Provides a simple abstract functions for using a timer for performance tests.
  For longer performance tests, it is better to use the 1ms values from HAL_GetTick()
  The timer counts with the peripheral clock divided by (prescaler + 1).
*/
void Timer16BitInit(uint16_t prescaler);

void Timer16BitDeinit(void);

