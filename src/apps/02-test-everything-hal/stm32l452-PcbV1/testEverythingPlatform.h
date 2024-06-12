#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "testEverything.h"

#define PIN_NUM 11

#define CHANNELS 19

extern const pin_t g_pins[];

extern const char * g_adcNames[];


void ClockToHsi(void);

void ClockToHse(void);

void CheckHseCrystal(void);

bool ClockToMsi(uint32_t frequency);

void ManualSpiCoprocLevel(void);

void ReadSensorsPlatform(void);