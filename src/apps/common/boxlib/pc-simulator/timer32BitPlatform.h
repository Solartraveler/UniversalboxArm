#pragma once

#include <stdint.h>

#include "main.h"

void Timer32BitReset(void);

void Timer32BitStart(void);

void Timer32BitStop(void);

uint32_t Timer32BitGet(void);
