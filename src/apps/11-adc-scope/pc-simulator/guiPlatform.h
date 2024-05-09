#pragma once

/* ADC scope
(c) 2024 by Malte Marwedel

SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "gui.h"

#define SCALETIME_INDEX_DEFAULT 18

/*sample rate 20px and samples/sec
unit: [ns/pix]
*/
const textUnit_t g_scaleTime[] = {
{"5s",   250000000.0},
{"2s",   100000000.0},
{"1.5s",  75000000.0},
{"1s",    50000000.0},
{"0.5s",  25000000.0},
{"0.3s",  15000000.0},
{"0.2s",  10000000.0},
{"0.15s",  7500000.0},
{"0.1s",   5000000.0},
{"50ms",   2500000.0},
{"30ms",   1500000.0},
{"20ms",   1000000.0},
{"15ms",    750000.0},
{"10ms",    500000.0},
{"5ms",     250000.0},
{"3ms",     150000.0},
{"2ms",     100000.0},
{"1.5ms",    75000.0},
{"1ms",      50000.0}, //<- default value
{"0.5ms",    25000.0},
{"0.3ms",    15000.0},
{"0.2ms",    10000.0},
{"150µs",     7500.0},
{"100µs",     5000.0},
};

#define INPUT_RED_DEFAULT 2
#define INPUT_GREEN_DEFAULT 3
#define INPUT_BLUE_DEFAULT 4

char * g_inputsList = "\
None\n\
ADC0-Zero\n\
ADC1-Tri\n\
ADC2-Sine\n\
ADC3-Rect\n";
