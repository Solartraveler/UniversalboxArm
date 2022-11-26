#pragma once

#include <stdint.h>
#include <stdbool.h>

#define SECONDS_1900_2000 3155673600UL
#define SECONDS_1970_2000 946684800UL
#define SECONDS_1900_1970 (SECONDS_1900_2000 - SECONDS_1970_2000)

/* Creates a UTC timestamp based on the 1.1.1970-
All values are 0 based. Minimum date is 1.1.2000, maximum 31.12.2099
*/
uint32_t TimestampCreate(uint16_t year, uint8_t month, uint8_t day,
                   uint8_t hour, uint8_t minute, uint8_t second);

//Opposite of the function above. Pointers may be NULL
void TimestampDecode(uint32_t timestamp, uint16_t * pYear, uint8_t * pMonth, uint8_t * pDay,
               uint16_t * pDoy, uint8_t * pHour, uint8_t * pMinute, uint8_t * pSecond);

/*Gets the weekday within the year 2000...2099 range.
The year may be two digits (0...99) or 4 digits (2000...2099).
dayofyear must be zero based.
returns: 0 = monday, 6 = sunday
*/
uint8_t WeekdayFromDoy(uint16_t dayofyear, uint16_t year);

/* PPB - calculated delta in parts per billion [1000ppm].
   A negative returned value means, the clock should have run faster.
   A positive returned value menas, the clock should have run slower.
   The newTimestamp must be between 8h and 28 days in the future compared
   to oldTimestamp.
   Returns true if the conditions are met and pDerivation is set.
*/
bool DerivationPPB(uint32_t oldTimestamp, uint16_t oldTimestampMs,
                   uint32_t currentTimestamp, uint16_t currentTimestampMs,
                   uint32_t newTimestamp, uint16_t newTimestampMs,
                   int32_t * pDerivation);

/* Returns true if the timestamp is summer time.
	The rule:
	  a) Last sunday of march, it should be summer time after 1:00 UTC
	  b) Last sunday of october, it should be winter time after 1:00 UTC
*/
bool IsSummertimeInEurope(uint32_t utcTime);
