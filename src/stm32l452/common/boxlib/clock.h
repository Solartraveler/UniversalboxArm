#pragma once

#include <stdint.h>
#include <stdbool.h>

//Gives access to the RTC clock, starts the external LSE, sets up the clock source.
//Returns true on success
bool ClockInit(void);

//Resets the clock, looses the calibration and stops the LSE.
void ClockDeinit(void);

//Gets the unix UTC timestamp. Unit is [s] since 1.1.1970.
//Should work from 2000 up to the year 2100.
//pMiliseconds is optional
uint32_t ClockUtcGet(uint16_t * pMiliseconds);

/*
Returns the value of the last ClockUtcSet timestamp.
Return value: 0 = No value was ever set, written values might be random
              1 = Value returned is imprecise
              2 = Value returned is precise
*/
uint8_t ClockSetTimestampGet(uint32_t * pUtc, uint16_t * pMiliseconds);


/*Sets the unix UTC timestamp. Unit of timestamp is [s] since 1.1.1970.
  Should work from 2000 up to the year 2100.
  miliseconds allows a the time to set more precise.
  The timestamp is stored in the first two backup registers too.
  An automatic calibration of the clock is done, if all of the following conditions are met:
  1. Precision must be set to true
  2. The previous call must have been precision to be true too.
  3. The previous call must have been between 8h and 24 days in the past.
  4. The resulting delta must fit within the calibration range +-488ppm
  pDelta: If not null, the delta in [ms] between the new and old time is written to the variable.
  Returns true on success
*/
bool ClockUtcSet(uint32_t timestamp, uint16_t miliseconds, bool precise, int64_t * pDelta);

/* Returns the current calibration value in ppb (parts per billion)
*/
int32_t ClockCalibrationGet(void);

//For debugging the clock
void ClockPrintRegisters(void);
