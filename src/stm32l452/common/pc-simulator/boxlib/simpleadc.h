#pragma once

#include <stdint.h>

uint16_t AdcGet(uint32_t channel);

void AdcInit(void);

void AdcStop(void);