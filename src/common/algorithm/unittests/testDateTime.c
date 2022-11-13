
#include <stdint.h>
#include <stdio.h>


#include "../dateTime.h"

#define CHECK(A, B) if ((A) != (B)) { printf("Error in line %u, should %lli, is %lli\n", __LINE__, (long long int)B, (long long int)A); result = 1;}

int TestUnixDecode1(void) {
	int result = 0;
	uint16_t year, doy;
	uint8_t month, day, hour, minute, second;
	//2022-11-07, 23:19:15
	TimestampDecode(1667863155, &year, &month, &day, &doy, &hour, &minute, &second);
	CHECK(year, 2022);
	CHECK(month, 10);
	CHECK(day, 6);
	CHECK(hour, 23);
	CHECK(minute, 19);
	CHECK(second, 15);
	return result;
}

int TestUnixDecode2(void) {
	int result = 0;
	uint16_t year, doy;
	uint8_t month, day, hour, minute, second;
	//2023-08-01, 12:34:56
	TimestampDecode(1690893296, &year, &month, &day, &doy, &hour, &minute, &second);
	CHECK(year, 2023);
	CHECK(month, 7);
	CHECK(day, 0);
	CHECK(hour, 12);
	CHECK(minute, 34);
	CHECK(second, 56);
	return result;
}

int TestUnixDecode3(void) {
	int result = 0;
	uint16_t year, doy;
	uint8_t month, day, hour, minute, second;
	//2042-01-01, 0:0:0
	TimestampDecode(2272147200, &year, &month, &day, &doy, &hour, &minute, &second);
	CHECK(year, 2042);
	CHECK(month, 0);
	CHECK(day, 0);
	CHECK(hour, 0);
	CHECK(minute, 0);
	CHECK(second, 0);
	return result;
}

int TestUnixDecode4(void) {
	int result = 0;
	uint16_t year, doy;
	uint8_t month, day, hour, minute, second;
	//2099-12-31, 23:59:59 -> day 4748 since 1.1.1970
	TimestampDecode(4102444799, &year, &month, &day, &doy, &hour, &minute, &second);
	CHECK(year, 2099);
	CHECK(month, 11);
	CHECK(day, 30);
	CHECK(hour, 23);
	CHECK(minute, 59);
	CHECK(second, 59);
	return result;
}

int TestUnixDecode5(void) {
	int result = 0;
	uint16_t year, doy;
	uint8_t month, day, hour, minute, second;
	//2020-01-01, 22:33:44
	TimestampDecode(946766024, &year, &month, &day, &doy, &hour, &minute, &second);
	CHECK(year, 2000);
	CHECK(month, 0);
	CHECK(day, 0);
	CHECK(hour, 22);
	CHECK(minute, 33);
	CHECK(second, 44);
	return result;
}

int TestUnixEncode1(void) {
	int result = 0;
	uint32_t stamp = TimestampCreate(2022, 10, 8, 21, 19, 12);
	CHECK(stamp, 1668028752);
	return result;
}

int TestUnixDecodeEncode(void) {
	uint32_t tStart = SECONDS_1970_2000 + 30 + 20 * 60 + 10 * 60 * 60; //10:20:30
	const uint32_t aDay = 24 * 60 * 60;
	const uint32_t tEnd = tStart + 99 * 365 * aDay;
	uint16_t year, doy;
	uint8_t month, day, hour, minute, second;
	uint32_t cycle = 0;
	for (uint32_t tIn = tStart; tIn < tEnd; tIn += aDay) {
		cycle++;
		TimestampDecode(tIn, &year, &month, &day, &doy, &hour, &minute, &second);
		uint32_t tOut = TimestampCreate(year, month, day, hour, minute, second);
		if (tOut != tIn) {
		uint16_t year2, doy2;
		uint8_t month2, day2, hour2, minute2, second2;
			TimestampDecode(tOut, &year2, &month2, &day2, &doy2, &hour2, &minute2, &second2);
			printf("Error %u (%u) -> %u-%u-%u (doy %u) %u:%u:%u  -> %u -> %u-%u-%u (doy %u) %u:%u:%u \n",
			        tIn, cycle, year,  month + 1,  day + 1,  doy,  hour,  minute,  second,
			        tOut,       year2, month2 + 1, day2 + 1, doy2, hour2, minute2, second2);
			return 1;
		}
	}
	return 0;
}

int TestWeekday1(void) {
	int result = 0;
	const uint32_t year = 2022;
	uint32_t t = TimestampCreate(year, 10, 11, 1, 2, 3);
	uint16_t doy;
	TimestampDecode(t, NULL, NULL, NULL, &doy, NULL, NULL, NULL);
	uint8_t dow = WeekdayFromDoy(doy, year);
	CHECK(dow, 5);
	return result;
}

int TestDerivation1(void) {
	int result = 0;
	const uint32_t oldTime = 1000;
	const uint32_t oldTimeMs = 500;
	const uint32_t currTime = 1001000;
	const uint32_t currTimeMs = 500;
	const uint32_t newTime = 1001000;
	const uint32_t newTimeMs = 500;
	int32_t derivation = 0;
	bool success = DerivationPPB(oldTime, oldTimeMs, currTime, currTimeMs, newTime, newTimeMs, &derivation);
	CHECK(success, true);
	CHECK(derivation, 0);
	return result;
}

int TestDerivation2(void) {
	int result = 0;
	const uint32_t oldTime = 1000;
	const uint32_t oldTimeMs = 500;
	const uint32_t currTime = 1001001;
	const uint32_t currTimeMs = 500;
	const uint32_t newTime = 1001000;
	const uint32_t newTimeMs = 500;
	int32_t derivation = 0;
	bool success = DerivationPPB(oldTime, oldTimeMs, currTime, currTimeMs, newTime, newTimeMs, &derivation);
	CHECK(success, true);
	CHECK(derivation, 1000);
	return result;
}

int TestDerivation3(void) {
	int result = 0;
	const uint32_t oldTime = 1000;
	const uint32_t oldTimeMs = 500;
	const uint32_t currTime = 1000998;
	const uint32_t currTimeMs = 500;
	const uint32_t newTime = 1001000;
	const uint32_t newTimeMs = 500;
	int32_t derivation = 0;
	bool success = DerivationPPB(oldTime, oldTimeMs, currTime, currTimeMs, newTime, newTimeMs, &derivation);
	CHECK(success, true);
	CHECK(derivation, -2000);
	return result;
}

int TestDerivation4(void) {
	int result = 0;
	const uint32_t oldTime = 1000;
	const uint32_t oldTimeMs = 500;
	const uint32_t currTime = 2001000;
	const uint32_t currTimeMs = 250;
	const uint32_t newTime = 2001000;
	const uint32_t newTimeMs = 500;
	int32_t derivation = 0;
	bool success = DerivationPPB(oldTime, oldTimeMs, currTime, currTimeMs, newTime, newTimeMs, &derivation);
	CHECK(success, true);
	CHECK(derivation, -125);
	return result;
}


int main(void) {
	int result = 0;
	result |= TestUnixDecode1();
	result |= TestUnixDecode2();
	result |= TestUnixDecode3();
	result |= TestUnixDecode4();
	result |= TestUnixDecode5();
	result |= TestUnixEncode1();
	result |= TestUnixDecodeEncode();
	result |= TestWeekday1();
	result |= TestDerivation1();
	result |= TestDerivation2();
	result |= TestDerivation3();
	result |= TestDerivation4();

	return result;
}
