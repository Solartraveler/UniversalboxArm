#pragma once

#include <stdint.h>
#include <stdbool.h>

//Dummy
bool ClockInit(void);

//Dummy
void ClockDeinit(void);

//Supported
uint32_t ClockUtcGet(uint16_t * pMilliseconds);

//Supported
uint8_t ClockSetTimestampGet(uint32_t * pUtc, uint16_t * pMilliseconds);

/*Partially supported, prints the difference to the PC clock, provides data to ClockSetTimestampGet
  and fills pDelta. But never updates the real PC clock.
  always returns true
*/
bool ClockUtcSet(uint32_t timestamp, uint16_t milliseconds, bool precise, int64_t * pDelta);

//Dummy
int32_t ClockCalibrationGet(void);

//Dummy
void ClockPrintRegisters(void);
