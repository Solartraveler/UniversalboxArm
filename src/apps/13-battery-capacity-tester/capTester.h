#pragma once

#include <stdbool.h>
#include <stdint.h>


void AppInit(void);

void AppCycle(void);

void CapStart(uint8_t battery, bool enabled, uint32_t rMohm, uint32_t offMv);

//gets the data. t in [s], u in [mV], e in [mWh]
//returns true if the sink is enabled
bool CapDataGet(uint8_t battery, uint32_t * t, uint32_t * u, uint32_t * e);

void CapStop(void);

void CapSave(void);

//call once every second
void CapCheck(void);

