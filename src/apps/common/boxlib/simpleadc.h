#pragma once

#include <stdint.h>

uint16_t AdcGet(uint32_t channel);

float AdcAvrefGet(void);

void AdcInit(void);

void AdcStop(void);