#pragma once

#include <stdint.h>

#define CONVERTTIMES 8

//See benchmark function how to get the values
//Values determined when running from RAM with 80MHz
const uint32_t g_convertTime[CONVERTTIMES]       = {22, 22, 44, 44, 66, 110, 264, 660};
const uint32_t g_convertTimeOffset[CONVERTTIMES] = {62, 62, 62, 62, 62, 62, 62, 62};
