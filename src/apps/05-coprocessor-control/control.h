#pragma once

#include <stdbool.h>
#include <stdint.h>

#define STATES_MAX 11

#define ERRORS_MAX 5

extern const char * g_chargerState[STATES_MAX];

extern const char * g_chargerError[ERRORS_MAX];

void ControlInit(void);

void ControlCycle(void);

void TemperatureToString(char * output, size_t len, int16_t temperature);

