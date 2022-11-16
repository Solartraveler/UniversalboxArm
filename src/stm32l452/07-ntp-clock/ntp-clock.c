/* NTP clock
(c) 2022 by Malte Marwedel

SPDX-License-Identifier: GPL-3.0-or-later

Allows configuring WIFI

TODO:
Then shows a clock on the display

*/

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>

#include "ntp-clock.h"

#include "boxlib/keys.h"
#include "boxlib/leds.h"
#include "boxlib/rs232debug.h"
#include "boxlib/lcd.h"
#include "boxlib/flash.h"
#include "boxlib/peripheral.h"
#include "boxlib/coproc.h"
#include "boxlib/mcu.h"
#include "boxlib/esp.h"
#include "boxlib/clock.h"
#include "boxlib/readLine.h"

#include "main.h"

#include "utility.h"
#include "femtoVsnprintf.h"
#include "dateTime.h"

#include "gui.h"
#include "filesystem.h"
#include "json.h"

#define WIFI_FILENAME "/etc/wifi.json"

#define TEXT_MAX 64

typedef struct {
	bool clockEnabled;
	char ap[TEXT_MAX];
	char password[TEXT_MAX];
} state_t;


state_t g_state;

uint32_t g_cycleTick;

void ControlHelp(void) {
	printf("h: Print help\r\n");
	printf("r: Reset\r\n");
	printf("c: Configure network parameters\r\n");
	printf("p: Save network params to flash\r\n");
	printf("z: Configure and save timezone\r\n");
	printf("t: Print time of RTC\r\n");
	printf("n: Call ntp update\r\n");
	printf("m: Set clock manually\r\n");
	printf("b: Print calibration value and last set time\r\n");
	printf("e: Disable / enable RTC, (disable looses the time and calibration)\r\n");
	printf("w-a-s-d: Send key code to GUI\r\n");
}

void LoadParams(void) {
	uint8_t wificonf[TEXT_MAX * 3] = {0};
	FIL f;
	UINT r = 0;
	if (FR_OK == f_open(&f, WIFI_FILENAME, FA_READ)) {
		FRESULT res = f_read(&f, wificonf, sizeof(wificonf) - 1, &r);
		if (res != FR_OK) {
			printf("Warning, could not read wifi file\r\n");
		}
		f_close(&f);
	} else {
		printf("No wifi configured\r\n");
		return;
	}
	bool success = JsonValueGet(wificonf, r, "ap1", g_state.ap, TEXT_MAX);
	success &= JsonValueGet(wificonf, r, "password1", g_state.password, TEXT_MAX);
	if (success) {
		printf("Wifi config loaded\r\n");
	} else {
		printf("Wifi config file incomplete\r\n");
	}
}

void AppInit(void) {
	LedsInit();
	Led1Green();
	PeripheralPowerOff();
	HAL_Delay(100);
	PeripheralPowerOn();
	Rs232Init();
	printf("\r\nNTP clock %s\r\n", APPVERSION);
	printf("h: Print help\r\n");
	Rs232Flush();
	KeysInit();
	CoprocInit();
	PeripheralInit();
	FlashEnable(4); //4MHz
	FilesystemMount();
	GuiInit();
	LoadParams();
	g_state.clockEnabled = ClockInit();
	Led1Off();
	g_cycleTick = HAL_GetTick();
}

void ExecReset(void) {
	printf("Reset selected\r\n");
	Rs232Flush();
	NVIC_SystemReset();
}

void EnterParams(void) {
	printf("Enter access point name\r\n");
	ReadSerialLine(g_state.ap, TEXT_MAX);
	printf("\r\nEnter password\r\n");
	ReadSerialLine(g_state.password, TEXT_MAX);
	printf("\r\nOk\r\n");
}

void SaveParams(void) {
	char buffer[TEXT_MAX * 3];
	snprintf(buffer, sizeof(buffer), "{\n  \"ap1\": \"%s\",\n  \"password1\": \"%s\"\n}\n", g_state.ap, g_state.password);
	if (FilesystemWriteEtcFile(WIFI_FILENAME, buffer, strlen(buffer))) {
		printf("Saved to %s\r\n", WIFI_FILENAME);
	} else {
		printf("Error, could not create file %s\r\n", WIFI_FILENAME);
	}
}

void CheckEspPrint(const char * buffer) {
	const uint32_t maxCharsLine = 80;
	uint32_t forceNewline = maxCharsLine;
	while (*buffer) {
		if (isprint(*buffer) || (*buffer == '\r') || (*buffer == '\n')) {
			if (*buffer == '\r') {
				forceNewline = maxCharsLine;
			}
			putchar(*buffer);
			forceNewline--;
		}
		buffer++;
		if (forceNewline == 0) {
			printf("\r\n");
			forceNewline = maxCharsLine;
		}
	}
}

typedef struct {
	/* bits 0..2: mode, bits 3..5: version number, bits 6..7: special like 59s min or 61s min, or out of sync */
	uint8_t metadata;
	uint8_t stratum;
	uint8_t poll;
	uint8_t precision;
	//NOTE: All bytes need to be flipped in order to be little endian. Use BytesFlip() from utility.h
	uint32_t rootDelay;
	uint32_t rootDispersion;
	uint32_t referenceIdentifier;
	uint32_t referenceTimestampS;
	uint32_t referenceTimestampFrac;
	uint32_t originalTimestampS;
	uint32_t originalTimestampFrac;
	uint32_t receiveTimestampS;
	uint32_t receiveTimestampFrac;
	uint32_t transmitTimestampS; //[s] since 1.1.1900
	uint32_t transmitTimestampFrac; //[1s/(2^32)]
} ntpData_t;

_Static_assert(sizeof(ntpData_t) == 48, "Please fix alignment!");

bool UdpGetNtp(const char * domain, uint32_t * pTimestamp, uint16_t * pTimestampMs, uint32_t * pTimeRequestStart) {
	//most simple request, according to https://stackoverflow.com/questions/14171366/ntp-request-packet
	ntpData_t ntpRequest;
	memset(&ntpRequest, 0, sizeof(ntpData_t));
	ntpRequest.metadata = 0x1B; //version = 3, mode = client
	size_t written = 0;
	ntpData_t ntpAnswer;
	uint32_t error;
	for (uint32_t i = 0; i < 3; i++) {
		uint32_t tStart = HAL_GetTick();
		error = EspUdpRequestResponse(domain, 123, (uint8_t*)(&ntpRequest), sizeof(ntpRequest), (uint8_t*)(&ntpAnswer), sizeof(ntpAnswer), &written);
		uint32_t tStop = HAL_GetTick();
		uint32_t delta = tStop - tStart;
		if (pTimeRequestStart) {
			/* This assumes udp transmit and receive take the same time, but this is
			   most likely not true, because the system needs to do a DNS resolve before,
			   if a hostname and not not a IP is given. Also the internal firmware
			   timings of the ESP12 are unknown.
			*/
			*pTimeRequestStart = tStop + (delta / 2);
		}
		if ((error == 0) && (written == sizeof(ntpAnswer)) && (delta < 1000)) {
			printf("Response time %ums\r\n", (unsigned int)delta);
			uint8_t metadata = ntpAnswer.metadata;
			if ((metadata & 0xC0) != 0xC0) {
				if (pTimestamp) {
					*pTimestamp = BytesFlip(ntpAnswer.transmitTimestampS);
				}
				if (pTimestampMs) {
					*pTimestampMs = BytesFlip(ntpAnswer.transmitTimestampFrac) / 4294967;
				}
				return true;
			} else {
				printf("Error, remote clock not set\r\n");
			}
		} else {
			if (error == 1) {
				printf("Error, starting connection\r\n");
			} else if (error == 2) {
				printf("Error, starting sending\r\n");
			} else if (error == 3) {
				printf("Error, could not send data\r\n");
			} else if (error == 4) {
				printf("Error, no response\r\n");
			} else if (error == 5) {
				printf("Error, response too long\r\n");
			} else if (written != sizeof(ntpAnswer)) {
				printf("Error, response too short\r\n");
			} else {
				printf("Error, response too slow, took %ums\r\n", (unsigned int)delta);
			}
		}
		printf("Request no %u failed\r\n", (unsigned int)i + 1);
	}
	return false;
}

/* If getting failed, it retuns the 1.1.2000, not 0!, so the 0 overflow in 2036
can be properly handled.
*/
uint32_t NtpTimestampGet(uint16_t * pTimestampMs, uint32_t * pTimeRequestStart) {
	const char * server = "pool.ntp.org";
	uint32_t timestamp = SECONDS_1900_2000;
	EspInit();
	printf("Enabling ESP\r\n");
	EspEnable();
	if (EspWaitPowerupReady()) {
		printf("Module ready\r\n");
		if (EspSetClient()) {
			printf("Module is a client\r\n");
			bool success = EspConnect(g_state.ap, g_state.password);
			if (success) {
				printf("Connected to access point\r\n");
				if (UdpGetNtp(server, &timestamp, pTimestampMs, pTimeRequestStart)) {
				} else {
					printf("Error, could not get timestamp\r\n");
				}
			} else {
				printf("Error, could not connect\r\n");
			}
			printf("Disconnecting from access point\r\n");
			EspDisconnect();
		} else {
			printf("Error, could not become a client\r\n");
		}
	} else {
		printf("Error, no answer\r\n");
	}
	EspStop();
	printf("Esp stopped\r\n");
	return timestamp;
}

void TimestampPrint(uint32_t unixStamp, uint16_t timestampMs) {
	uint16_t year, doy;
	uint8_t month, day, hour, minute, second;
	TimestampDecode(unixStamp, &year, &month, &day, &doy, &hour, &minute, &second);
	day++;
	month++;
	printf("%u-%02u-%02u, doy %u, %02u:%02u:%02u.%03u",
	(unsigned int)year, (unsigned int)month, (unsigned int)day,
	(unsigned int)doy, (unsigned int)hour, (unsigned int)minute, (unsigned int)second, (unsigned int)timestampMs);
}

void NtpUpdate(void) {
	uint32_t timeRequestStart;
	uint16_t timestampMs;
	uint32_t timestamp = NtpTimestampGet(&timestampMs, &timeRequestStart);
	if (timestamp != SECONDS_1900_2000) {
		printf("NTP timestamp: %u\r\n", (unsigned int)timestamp);
		uint32_t unixStamp = timestamp - SECONDS_1900_1970;
		uint32_t timePassedMs = HAL_GetTick() - timeRequestStart;
		timestampMs += timePassedMs;
		unixStamp += timestampMs / 1000;
		timestampMs %= 1000;
		int64_t deltaMs = 0;
		ClockUtcSet(unixStamp, timestampMs, true, &deltaMs);
		uint16_t year, doy;
		uint8_t month, day, hour, minute, second;
		TimestampDecode(unixStamp, &year, &month, &day, &doy, &hour, &minute, &second);
		day++;
		month++;
		printf("UTC from server ");
		TimestampPrint(unixStamp, timestampMs);
		printf("\r\n");
		printf("Delta local to new: %ims\r\n", (int)deltaMs);
	}
}

void RtcTime(void) {
	uint16_t timestampMs;
	uint32_t unixTimestamp = ClockUtcGet(&timestampMs);
	printf("UTC from local clock ");
	TimestampPrint(unixTimestamp, timestampMs);
	printf("\r\n");
}

void PrintExtraClockData(void) {
	int32_t cal = ClockCalibrationGet();
	printf("Calibration %i ppb\r\n", (int)cal);
	uint32_t setTime = 0;
	uint16_t setTimeMs = 0;
	uint8_t state = ClockSetTimestampGet(&setTime, &setTimeMs);
	if (state) {
		printf("Last set time (%s) ", state == 2 ? "precise" : "imprecise");
		TimestampPrint(setTime, setTimeMs);
		printf("\r\n");
	} else {
		printf("Time never set\r\n");
	}
	ClockPrintRegisters();
}

void ClockToggle(void) {
	if (g_state.clockEnabled) {
		ClockDeinit();
		printf("Clock stopped\r\n");
	} else {
		ClockInit();
		printf("Clock enabled\r\n");
	}
	g_state.clockEnabled = !g_state.clockEnabled;
}

void ManualInput(void) {
	char buffer[8];
	printf("Enter Year\r\n");
	ReadSerialLine(buffer, sizeof(buffer));
	uint16_t year = atol(buffer);
	printf("\r\nEnter month\r\n");
	ReadSerialLine(buffer, sizeof(buffer));
	uint16_t month = atol(buffer);
	month = MIN(MAX(month, 1), 12);
	printf("\r\nEnter day\r\n");
	ReadSerialLine(buffer, sizeof(buffer));
	uint16_t day = atol(buffer);
	day = MIN(MAX(day, 1), 31);
	printf("\r\nEnter hour (UTC)\r\n");
	ReadSerialLine(buffer, sizeof(buffer));
	uint16_t hour = atol(buffer);
	hour = MIN(hour, 23);
	printf("\r\nEnter minute (UTC)\r\n");
	ReadSerialLine(buffer, sizeof(buffer));
	uint16_t minute = atol(buffer);
	minute = MIN(minute, 59);
	uint32_t timestamp = TimestampCreate(year, month - 1, day - 1, hour, minute, 0);
	if (ClockUtcSet(timestamp, 0, false, NULL)) {
		printf("\r\nClock set\r\n");
	} else {
		printf("\r\nClock set failed\r\n");
	}
	RtcTime();
}

void AppCycle(void) {
	static uint32_t ledCycle = 0;
	static uint8_t guiUpdate = 1;
	//led flash
	if (ledCycle < 500) {
		Led2Green();
	} else {
		Led2Off();
	}
	if (ledCycle >= 1000) {
		ledCycle = 0;
	}
	ledCycle++;
	char input = Rs232GetChar();
	if (input) {
		printf("%c", input);
		if (input == 'h') {
			ControlHelp();
		}
		if (input == 'r') {
			ExecReset();
		}
		if (input == 'c') {
			EnterParams();
		}
		if (input == 'p') {
			SaveParams();
		}
		if (input == 'n') {
			NtpUpdate();
		}
		if (input == 'm') {
			ManualInput();
		}
		if (input == 't') {
			RtcTime();
		}
		if (input == 'e') {
			ClockToggle();
		}
		if (input == 'b') {
			PrintExtraClockData();
		}
	}
	if (guiUpdate) {
		GuiCycle(input);
	}
	/* Call this function 1000x per second, if one cycle took more than 1ms,
	   we skip the wait to catch up with calling.
	   cycleTick last is needed to prevent endless wait in the case of a 32bit
	   overflow.
	*/
	uint32_t cycleTickLast = g_cycleTick;
	g_cycleTick++; //next call expected tick value
	uint32_t tick;
	do {
		tick = HAL_GetTick();
		if (tick < g_cycleTick) {
			HAL_Delay(1);
		}
	} while ((tick < g_cycleTick) && (tick >= cycleTickLast));
}
