/* Boxlib
(c) 2022 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause
*/

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "clock.h"

#include "main.h"

#include "dateTime.h"

//448.5ppm -> 448500ppb
#define RTC_CALIB_P_VAL 448500

#define RTC_PRECISE_STAMP 0xAB700000
#define RTC_NONPRECISE_STAMP 0xAB800000

void ClockPrintRegisters(void) {
	printf("SSR: %08x\r\n", (unsigned int)RTC->SSR); //this disables the shadow register update
	printf("TR: %08x\r\n", (unsigned int)RTC->TR);
	printf("DR: %08x\r\n", (unsigned int)RTC->DR); //read allows updates again
	printf("CR: %08x\r\n", (unsigned int)RTC->CR);
	printf("ISR: %08x\r\n", (unsigned int)RTC->ISR);
	printf("PRER: %08x\r\n", (unsigned int)RTC->PRER);
	printf("CALR: %08x\r\n", (unsigned int)RTC->CALR);
	printf("OR: %08x\r\n", (unsigned int)RTC->OR);
}

bool ClockInit(void) {
	__HAL_RCC_BACKUPRESET_RELEASE();
	__HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_HIGH);
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSE;
	RCC_OscInitStruct.LSEState = RCC_LSE_ON;
	HAL_StatusTypeDef result = HAL_RCC_OscConfig(&RCC_OscInitStruct);
	if (result == HAL_OK) {
		__HAL_RCC_RTC_CONFIG(RCC_RTCCLKSOURCE_LSE);
	} else {
		return false;
	}
	HAL_PWR_EnableBkUpAccess();
	__HAL_RCC_RTC_ENABLE();
	return true;
}

void ClockDeinit(void) {
	HAL_PWR_EnableBkUpAccess();
	__HAL_RCC_RTC_DISABLE();
	__HAL_RCC_BACKUPRESET_FORCE();
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_OscInitStruct.LSEState = RCC_LSE_OFF;
	HAL_RCC_OscConfig(&RCC_OscInitStruct);
}

bool ClockReady(void) {
	RTC->ISR = RTC_ISR_INIT; //init
	uint32_t tStart = HAL_GetTick();
	while ((RTC->ISR & RTC_ISR_RSF) == 0) {
		if (HAL_GetTick() - tStart > 10) {
			/*The second read after the timeout is needed, should a preemtive scheduler
			  be responsible for the time passed.
			*/
			if ((RTC->ISR & RTC_ISR_RSF) == 0) {
				return false;
			}
		}
	}
	return true;
}


uint32_t ClockUtcGet(uint16_t * pMilliseconds) {
	uint32_t fraction, time, date;

	if (ClockReady() == false) {
		return 0;
	}
	fraction = RTC->SSR; //the read results in the other regs to be locked
	time = RTC->TR;
	date = RTC->DR;
	RTC->ISR &= ~RTC_ISR_RSF;
	uint32_t second  =((time & RTC_TR_SU) >> RTC_TR_SU_Pos) + 10 * ((time & RTC_TR_ST) >> RTC_TR_ST_Pos);
	uint32_t minute = ((time & RTC_TR_MNU) >> RTC_TR_MNU_Pos) + 10 * ((time & RTC_TR_MNT) >> RTC_TR_MNT_Pos);
	uint32_t hour = ((time & RTC_TR_HU) >> RTC_TR_HU_Pos) + 10 * ((time & RTC_TR_HT) >> RTC_TR_HT_Pos);
	uint32_t day = ((date & RTC_DR_DU) >> RTC_DR_DU_Pos) + 10 * ((date & RTC_DR_DT) >> RTC_DR_DT_Pos) - 1;
	uint32_t month = ((date & RTC_DR_MU) >> RTC_DR_MU_Pos) + 10 * ((date & RTC_DR_MT) >> RTC_DR_MT_Pos) - 1;
	uint32_t year = ((date & RTC_DR_YU) >> RTC_DR_YU_Pos) + 10 * ((date & RTC_DR_YT) >> RTC_DR_YT_Pos) + 2000;
	if (pMilliseconds) {
		uint32_t subsMax = ((RTC->PRER & RTC_PRER_PREDIV_S) >> RTC_PRER_PREDIV_S_Pos) + 1;
		if (subsMax) {
			uint32_t millis = fraction * 1000 / subsMax;
			* pMilliseconds = millis;
		}
	}
	return TimestampCreate(year, month, day, hour, minute, second);
}

uint8_t ClockToBcd(uint8_t binary) {
	return binary % 10 + binary / 10 * 16;
}

int32_t ClockRegToPPB(uint32_t reg) {
	int32_t ppb = 0;
	if (reg & RTC_CALR_CALP) {
		ppb = RTC_CALIB_P_VAL;
	}
	ppb -= ((reg & RTC_CALR_CALM) >> RTC_CALR_CALM_Pos) * 9537 / 10;
	return ppb;
}

bool ClockPPBToReg(int32_t ppb, uint32_t * pReg) {
	if ((ppb <= RTC_CALIB_P_VAL) && (ppb > -448294)) {
		uint32_t reg = RTC_CALR_CALW8;
		if (ppb > 0) {
			reg |= RTC_CALR_CALP;
			ppb -= RTC_CALIB_P_VAL;
		}
		if (ppb < 0) {
			ppb = -ppb;
			uint32_t temp = ppb * 10 / 9537;
			reg |= temp;
		}
		*pReg = reg;
		return true;
	}
	return false;
}

bool ClockEnterInitMode(void) {
	RTC->ISR = RTC_ISR_INIT; //init
	uint32_t tStart = HAL_GetTick();
	while ((RTC->ISR & RTC_ISR_INITF) == 0) {
		if (HAL_GetTick() - tStart > 10) {
			/* The second read after the timeout is needed, should a preemptive
			   scheduler be responsible for the time passed.
			*/
			if ((RTC->ISR & RTC_ISR_INITF) == 0) {
				return false;
			}
		}
	}
	return true;
}

uint8_t ClockSetTimestampGet(uint32_t * pUtc, uint16_t * pMilliseconds) {
	if (pUtc) {
		*pUtc = RTC->BKP0R;
	}
	uint32_t oldData = RTC->BKP1R;
	if (pMilliseconds) {
		*pMilliseconds = oldData & 0xFFFF;
	}
	oldData &= 0xFFFF0000;
	if (oldData == RTC_PRECISE_STAMP) {
		return 2;
	}
	if (oldData == RTC_NONPRECISE_STAMP) {
		return 1;
	}
	return 0;
}

bool ClockUtcSet(uint32_t timestamp, uint16_t milliseconds, bool precise, int64_t * pDelta) {
	uint32_t calibrationValue;
	bool calibrationWrite = false;

	if (precise) {
		int32_t derivation;
		uint16_t currentTimestampMs = 0;
		uint32_t currentTimestamp = ClockUtcGet(&currentTimestampMs);
		uint32_t oldTimestamp;
		uint16_t oldTimestampMs;
		if (ClockSetTimestampGet(&oldTimestamp, &oldTimestampMs) == 2) {
			oldTimestampMs &= 0xFFFF;
			if (pDelta) { //might be useful for debug
				uint64_t delta = (uint64_t)currentTimestamp - (uint64_t)timestamp;
				delta *= 1000;
				delta += (uint64_t)currentTimestampMs - (uint64_t)milliseconds;
				*pDelta = (int64_t)delta;
			}
			bool calcOk = DerivationPPB(oldTimestamp, oldTimestampMs,
			                      currentTimestamp, currentTimestampMs,
			                      timestamp, milliseconds, &derivation);
			if (calcOk) {
				int32_t ppb = ClockRegToPPB(RTC->CALR);
				ppb -= derivation;
				calibrationWrite = ClockPPBToReg(ppb, &calibrationValue);
			}
		}
	}
	uint16_t year, doy;
	uint8_t month, day, hour, minute, second;
	TimestampDecode(timestamp, &year, &month, &day, &doy, &hour, &minute, &second);
	year -= 2000;
	month++;
	day++;
	year = ClockToBcd(year);
	month = ClockToBcd(month);
	day = ClockToBcd(day);
	hour = ClockToBcd(hour);
	minute = ClockToBcd(minute);
	second = ClockToBcd(second);
	uint32_t dateReg = (year << RTC_DR_YU_Pos) | (month << RTC_DR_MU_Pos) | (day << RTC_DR_DU_Pos);
	uint32_t timeReg = (hour << RTC_TR_HU_Pos) | (minute << RTC_TR_MNU_Pos) | (second << RTC_TR_SU_Pos);
	uint8_t weekday = WeekdayFromDoy(doy, year) + 1;
	dateReg |= (weekday << RTC_DR_WDU_Pos);
	//enable write access
	RTC->WPR = 0xCA;
	RTC->WPR = 0x53;
	if (ClockEnterInitMode() == false) {
		return false;
	}
	RTC->TR = timeReg;
	RTC->DR = dateReg;
	RTC->CR = 0; //disable all alarms and interrupts, choose 24h format
	//Dividers 0x80 * 0x100 -> divide 32786Hz to 1s.
	RTC->PRER = (0x7F << RTC_PRER_PREDIV_A_Pos) | (0xFF << RTC_PRER_PREDIV_S_Pos);
	if (calibrationWrite) {
		RTC->CALR = calibrationValue;
	}
	RTC->BKP0R = timestamp;
	if (precise) {
		RTC->BKP1R = milliseconds | RTC_PRECISE_STAMP;
	} else {
		RTC->BKP1R = milliseconds | RTC_NONPRECISE_STAMP;
	}
	while ((RTC->ISR & RTC_ISR_INITF) == 0);
	RTC->ISR &= ~RTC_ISR_INIT; //stop init mode
	//disable write access
	RTC->WPR = 0;
	return true;
}

int32_t ClockCalibrationGet(void) {
	return ClockRegToPPB(RTC->CALR);
}
