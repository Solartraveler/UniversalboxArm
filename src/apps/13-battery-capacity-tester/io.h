#pragma once

#include <stdbool.h>
#include <stdint.h>

float AdcVoltage(uint8_t battery);

void SinkInit(void);

void SinkSet(uint8_t battery, bool enabled);

