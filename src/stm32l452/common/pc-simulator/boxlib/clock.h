#pragma once

#include <stdint.h>
#include <stdbool.h>

//Dummy
bool ClockInit(void);

//Dummy
void ClockDeinit(void);

//Supported
uint32_t ClockUtcGet(uint16_t * pMiliseconds);

//Unsupported, always returns 0
uint8_t ClockSetTimestampGet(uint32_t * pUtc, uint16_t * pMiliseconds);

//Unsupported, just prints the difference to the PC clock, always returns true
bool ClockUtcSet(uint32_t timestamp, uint16_t miliseconds, bool precise, int64_t * pDelta);

//Dummy
int32_t ClockCalibrationGet(void);

//Dummy
void ClockPrintRegisters(void);
