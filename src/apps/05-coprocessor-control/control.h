#pragma once

#include <stdbool.h>
#include <stdint.h>

#define STATES_MAX 11

#define ERRORS_MAX 6

extern const char * g_chargerState[STATES_MAX];

extern const char * g_chargerError[ERRORS_MAX];

void AppInit(void);

void AppCycle(void);

void TemperatureToString(char * output, size_t len, int16_t temperature);

