/* Date time convesion
(c) 2022 by Malte Marwedel

SPDX-License-Identifier: BSD-3-Clause
*/

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "dateTime.h"

#define MONTHS 12

const uint16_t g_daysInMonthsBefore[MONTHS] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};


//this works from 1.1.2000 (0.0.2000) to 31.12.2099 (30.11.2099)
uint32_t TimestampCreate(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second) {
	year -= 2000;
	if (month >= MONTHS) {
		return 0;
	}
	uint32_t stamp = SECONDS_1970_2000;
	stamp += second;
	stamp += minute * 60;
	stamp += hour * 60 * 60;

	uint32_t days = year * 365UL + ((year + 3) / 4);
	days += day;
	days += g_daysInMonthsBefore[month];
	bool leapYear = (year % 4 == 0) ? true : false;
	if ((leapYear) && (month > 1)) { //if march, add a day
		days++;
	}
	stamp += days * 24UL * 60UL * 60UL;
	return stamp;
}

void TimestampDecode(uint32_t timestamp, uint16_t * pYear, uint8_t * pMonth, uint8_t * pDay,
                     uint16_t * pDoy, uint8_t * pHour, uint8_t * pMinute, uint8_t * pSecond) {
	timestamp -= SECONDS_1970_2000;
	if (pSecond) {
		*pSecond = timestamp % 60UL;
	}
	if (pMinute) {
		*pMinute = (timestamp / 60UL) % 60UL;
	}
	if (pHour) {
		*pHour = (timestamp / (60UL * 60UL)) % 24UL;
	}
	uint32_t days = timestamp / (60UL * 60UL * 24UL);
	uint32_t year2000 = days * 100UL / 36525UL; //divide by 365.25
	if (pYear) {
		*pYear = year2000 + 2000UL;
	}
	uint32_t dayInYear = (days * 100UL - (uint32_t)year2000 * 36525UL) / 100UL;
	if (pDoy) {
		*pDoy = dayInYear;
	}
	bool leapYear = ((year2000 + 4) % 4 == 0) ? true : false;
	uint8_t month = 0;
	uint8_t day = dayInYear;
	for (uint8_t i = 1; i < MONTHS; i++) {
		uint32_t addLeap = 0;
		if ((i > 1) && (leapYear)) {
			addLeap = 1;
		}
		uint32_t limit = g_daysInMonthsBefore[i] + addLeap;
		if (dayInYear < limit) {
			break;
		}
		month = i;
		day = dayInYear - limit;
	}
	if (pMonth) {
		*pMonth = month;
	}
	if (pDay) {
		*pDay = day;
	}
}

//0 = monday, 6 = sunday.
uint8_t WeekdayFromDoy(uint16_t dayofyear, uint16_t year) {
	if (year > 2000) {
		year -= 2000;
	}
	//calc weekday
	uint16_t weekday = year + dayofyear;
	if (year > 0) {
		weekday += (year - 1) / 4 + 1; //+1 because year of reference (2000) is a leapyear
	}
	//Usually saturday would be = 0 (because 1.1.2000 is a saturday), however since saturday 6th day (5) of week, add 5.
	weekday += 5;
	weekday %= 7;
	return weekday;
}

bool DerivationPPB(uint32_t oldTimestamp, uint16_t oldTimestampMs,
                   uint32_t currentTimestamp, uint16_t currentTimestampMs,
                   uint32_t newTimestamp, uint16_t newTimestampMs,
                   int32_t * pDerivation) {
	if ((newTimestamp <= oldTimestamp) || (currentTimestamp <= oldTimestamp)) {
		return false;
	}
	uint32_t timePassed = newTimestamp - oldTimestamp;
	if ((timePassed < (60UL * 60UL * 8UL)) || (timePassed > (60UL * 60UL * 24UL * 28UL))) {
		return false;
	}
	timePassed *= 1000; //make miliseconds
	timePassed += newTimestampMs - oldTimestampMs;
	uint32_t timeMeasured = currentTimestamp - oldTimestamp;
	timeMeasured *= 1000; //miliseconds
	timeMeasured += currentTimestampMs - oldTimestampMs;
	int64_t delta = (int64_t)timeMeasured - (int64_t)timePassed;
	delta *= 1000000000LL; //1 billion
	delta /= timePassed;
	if ((delta > INT32_MAX) || (delta < INT32_MIN)) {
		return false;
	}
	*pDerivation = delta;
	return true;
}

bool IsSummertimeInEurope(uint32_t utcTime) {
	/*The rule for europe:
	  a) Last sunday of march, it should be summer time after 1:00 -> jump from 1h -> 2h
	  b) Last sunday of october, it should be winter time after 1:00 -> jump back from 2h -> 1h
	*/
	uint16_t year;
	//decode to year
	TimestampDecode(utcTime, &year, NULL, NULL, NULL, NULL, NULL, NULL);
	//2. get day in week of last days in months
	uint16_t doyLastDayMarch = g_daysInMonthsBefore[3] - 1;
	uint16_t doyLastDayOctober = g_daysInMonthsBefore[10] - 1;
	if ((year % 4) == 0) { //append day in leapyears
		doyLastDayMarch++;
		doyLastDayOctober++;
	}
	uint8_t dayinweekLastDayMarch = WeekdayFromDoy(doyLastDayMarch, year);
	uint8_t dayinweekLastDayOctober = WeekdayFromDoy(doyLastDayOctober, year);
	//3. get last sunday in month
	uint8_t lastSundayMarch = 30; //proper if last day of month = sunday
	if (dayinweekLastDayMarch < 6) { //if not a sunday
		lastSundayMarch -= dayinweekLastDayMarch + 1; //monday = 0 -> substract one -> get sunday, tuesday = 1 -> substract two -> get sunday
	}
	uint8_t lastSundayOctober = 30; //proper if last day of month = sunday
	if (dayinweekLastDayOctober < 6) { //if not a sunday
		lastSundayOctober -= dayinweekLastDayOctober + 1;
	}
	//4. get new timestamp of last sundays
	uint32_t timestampSummertimeStart = TimestampCreate(year, 2, lastSundayMarch, 1, 0, 0);
	uint32_t timestampSummertimeEnd = TimestampCreate(year, 9, lastSundayOctober, 1, 0, 0);
	if ((timestampSummertimeStart <= utcTime) && (timestampSummertimeEnd > utcTime)) {
		return true;
	}
	return false;
}

