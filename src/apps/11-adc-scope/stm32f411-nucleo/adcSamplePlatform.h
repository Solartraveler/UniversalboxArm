#pragma once

#include <stdint.h>

#define CONVERTTIMES 8


//See benchmark function how to get the values
//Values determined when running from flash without cache and 80MHz CPU frequency
const uint32_t g_convertTime[CONVERTTIMES]       = {75, 125, 175, 275, 400, 500, 625, 1975};
const uint32_t g_convertTimeOffset[CONVERTTIMES] = {97, 97, 97, 97, 97, 97, 97, 97};
