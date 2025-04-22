#pragma once

#include <stdint.h>

#include "main.h"

#include "timer32BitPlatform.h"

/*Provides a simple abstract functions for using a timer for performance tests.
  Use if the resolution of HAL_GetTick() would be not precise enough.
  The timer counts with the peripheral clock divided by (prescaler + 1).
*/
void Timer32BitInit(uint16_t prescaler);

/*Deinits the timer. Basically resets it and disables the clock to save power.
*/
void Timer32BitDeinit(void);

