/* NTP clock
(c) 2022 by Malte Marwedel

SPDX-License-Identifier: GPL-3.0-or-later

Allows configuring WIFI and colors

TODO:
Allow configure timezone, server and refresh interval from the GUI
Automatically refresh the time from NTP
Dimming the backlight

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
#include "boxlib/readLine.h"
#include "boxlib/systickWithFreertos.h"

#include "clockMt.h"
#include "peripheralMt.h"

#include "main.h"

#include "utility.h"
#include "femtoVsnprintf.h"
#include "dateTime.h"

#include "gui.h"
#include "clockConfig.h"
#include "filesystem.h"
#include "json.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#define TEXT_MAX 64

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


typedef struct {
	bool clockEnabled;
} state_t;

uint32_t g_cycleTick;

state_t g_state;

StaticTask_t g_IdleTcb;
StackType_t g_IdleStack[configMINIMAL_STACK_SIZE];

//512 is not enough for the GUI
#define TASK_STACK_ELEMENTS 1024

//for redrawing the display and key presses
StaticTask_t g_guiTask;
StackType_t g_guiStack[TASK_STACK_ELEMENTS];

//for serial in and output and WIFI control
StaticTask_t g_mainTask;
StackType_t g_mainStack[TASK_STACK_ELEMENTS];


//Queue to control the GUI from the RS232 port and trigger state updates
#define KEY_QUEUE_NUM 8
QueueHandle_t g_keyQueue;
StaticQueue_t g_keyQueueState;
uint8_t g_keyQueueData[KEY_QUEUE_NUM];

void ControlHelp(void) {
	printf("h: Print help\r\n");
	printf("r: Reset\r\n");
	printf("c: Configure network parameters\r\n");
	printf("p: Save network params to file\r\n");
	printf("z: Configure and save time\r\n");
	printf("t: Print time of RTC\r\n");
	printf("n: Call ntp update\r\n");
	printf("m: Set clock manually\r\n");
	printf("b: Print calibration value and last set time\r\n");
	printf("e: Disable / enable RTC, (disable looses the time and calibration)\r\n");
	printf("w-a-s-d: Send key code to GUI\r\n");
}

void vApplicationGetIdleTaskMemory(StaticTask_t ** ppxIdleTaskTCBBuffer, StackType_t ** ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize) {
	*ppxIdleTaskTCBBuffer = &g_IdleTcb;
	*ppxIdleTaskStackBuffer = g_IdleStack;
	*pulIdleTaskStackSize = sizeof(g_IdleStack) / sizeof(StackType_t);
}

void PrintTimeParams(void) {
	char timeserver[TEXT_MAX];
	TimeserverGet(timeserver, sizeof(timeserver));
	uint16_t refreshInterval = TimeserverRefreshGet();
	uint16_t timeOffset = UtcOffsetGet();
	bool summerTime = SummertimeGet();
	printf("Ntp server %s, refresh every %uh, offset to UTC %imin, summertime adjust %s\r\n",
	       timeserver, refreshInterval, timeOffset, summerTime ? "true" : "false");
}

void ExecReset(void) {
	printf("Reset selected\r\n");
	Rs232Flush();
	NVIC_SystemReset();
}

static void NotifyGui(void) {
	char command = 'u'; //code for re-reading all the values from clockConfig
	xQueueSendToBack(g_keyQueue, &command, 0);
}

void EnterWifiParams(void) {
	char buffer[TEXT_MAX];
	printf("Enter access point name\r\n");
	ReadSerialLine(buffer, TEXT_MAX);
	ApSet(buffer);
	printf("\r\nEnter password\r\n");
	ReadSerialLine(buffer, TEXT_MAX);
	PasswordSet(buffer);
	NotifyGui();
	printf("\r\nOk\r\n");
}

#if 0
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
#endif

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
	char buffer1[TEXT_MAX];
	char buffer2[TEXT_MAX];
	uint32_t timestamp = SECONDS_1900_2000;
	EspInit();
	printf("Enabling ESP\r\n");
	EspEnable();
	if (EspWaitPowerupReady()) {
		printf("Module ready\r\n");
		if (EspSetClient()) {
			printf("Module is a client\r\n");
			ApGet(buffer1, sizeof(buffer1));
			PasswordGet(buffer2, sizeof(buffer2));
			bool success = EspConnect(buffer1, buffer2);
			if (success) {
				printf("Connected to access point\r\n");
				TimeserverGet(buffer1, sizeof(buffer1));
				if (UdpGetNtp(buffer1, &timestamp, pTimestampMs, pTimeRequestStart)) {
				} else {
					printf("Error, could not get timestamp from >%s<\r\n", buffer1);
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
		ClockUtcSetMt(unixStamp, timestampMs, true, &deltaMs);
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

void PrintTime(void) {
	uint16_t timestampMs;
	uint32_t unixTimestamp = ClockUtcGetMt(&timestampMs);
	printf("\r\nUTC from RTC clock        ");
	TimestampPrint(unixTimestamp, timestampMs);
	printf("\r\n");
	uint32_t localTimestamp = unixTimestamp + (int32_t)UtcOffsetGet() * 60;
	if (SummertimeGet()) {
		if (IsSummertimeInEurope(unixTimestamp)) {
			localTimestamp += 60UL * 60UL; //+1h
		}
	}
	printf("Local time from RTC clock ");
	TimestampPrint(localTimestamp, timestampMs);
	printf("\r\n");
}

void PrintExtraClockData(void) {
	int32_t cal = ClockCalibrationGetMt();
	printf("Calibration %i ppb\r\n", (int)cal);
	uint32_t setTime = 0;
	uint16_t setTimeMs = 0;
	uint8_t state = ClockSetTimestampGetMt(&setTime, &setTimeMs);
	if (state) {
		printf("Last set time (%s) ", state == 2 ? "precise" : "imprecise");
		TimestampPrint(setTime, setTimeMs);
		printf("\r\n");
	} else {
		printf("Time never set\r\n");
	}
	ClockPrintRegistersMt();
}

void ClockToggle(void) {
	if (g_state.clockEnabled) {
		ClockDeinitMt();
		printf("Clock stopped\r\n");
	} else {
		ClockInitMt();
		printf("Clock enabled\r\n");
	}
	g_state.clockEnabled = !g_state.clockEnabled;
}

void EnterClockState(void) {
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
	if (ClockUtcSetMt(timestamp, 0, false, NULL)) {
		printf("\r\nClock set\r\n");
	} else {
		printf("\r\nClock set failed\r\n");
	}
	PrintTime();
}

void EnterTimeParams(void) {
	char value[TEXT_MAX];
	printf("Enter NTP server, empty for default\r\n");
	ReadSerialLine(value, sizeof(value));
	if (strlen(value) > 0) {
		TimeserverSet(value);
	} else {
		TimeserverSet(DEFAULT_TIMESERVER);
	}
	printf("\r\nEnter NTP refresh interval in hours\r\n");
	ReadSerialLine(value, sizeof(value));
	TimeserverRefreshSet(atoi(value));
	printf("\r\nEnter UTC to local (winter) time offset in minutes. (60 for Berlin)\r\n");
	ReadSerialLine(value, sizeof(value));
	UtcOffsetSet(atoi(value));
	printf("\r\nAdjust for EU summer time? y/n\r\n");
	ReadSerialLine(value, sizeof(value));
	printf("\r\n");
	if (value[0] == 'y') {
		SummertimeSet(true);
	} else {
		SummertimeSet(false);
	}
	ConfigSaveClock();
	PrintTimeParams();
	NotifyGui();
}

uint32_t UtcToLocalTime(uint32_t utcTime) {
	uint32_t localTime = utcTime + UtcOffsetGet() * 60L;
	if (IsSummertimeInEurope(utcTime)) {
		localTime += 60UL * 60UL;
	}
	return localTime;
}

void GuiTask(void * param) {
	(void)param;
	GuiInit();
	while (1) {
		//call GuiCycle as soon as queue has an element, otherwise every 10ms
		char input = 0;
		xQueueReceive(g_keyQueue, &input, 10);
		GuiCycle(input);
	}
}

//continue AppInit but now from within a FreeRTOS thread
void AppInit2(void) {
	g_state.clockEnabled = ClockInitMt();
	KeysInit();
	CoprocInit();
	PeripheralInitMt();
	FlashEnable(4); //4MHz
	FilesystemMount();
	ConfigInit();
	PrintTimeParams();
	xTaskCreateStatic(&GuiTask, "gui", TASK_STACK_ELEMENTS, NULL, 1, g_guiStack, &g_guiTask);
}

void MainTask(void * param) {
	(void)param;
	AppInit2();
	uint32_t ledCycle = 0;
	while(1) {
		//led flash
		if (ledCycle < 50) {
			Led2Green();
		} else {
			Led2Off();
		}
		if (ledCycle >= 100) {
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
				EnterWifiParams();
			}
			if (input == 'p') {
				ConfigSaveWifi();
			}
			if (input == 'z') {
				EnterTimeParams();
			}
			if (input == 'n') {
				NtpUpdate();
			}
			if (input == 'm') {
				EnterClockState();
			}
			if (input == 't') {
				PrintTime();
			}
			if (input == 'e') {
				ClockToggle();
			}
			if (input == 'b') {
				PrintExtraClockData();
			}
			if ((input == 'w') || (input == 'a') || (input == 's') || (input == 'd')) {
			 xQueueSendToBack(g_keyQueue, &input, 0);
			}
		}
		vTaskDelay(10);
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
	xTaskCreateStatic(&MainTask, "main", TASK_STACK_ELEMENTS, NULL, 1, g_mainStack, &g_mainTask);
	g_keyQueue = xQueueCreateStatic(KEY_QUEUE_NUM, sizeof(uint8_t), g_keyQueueData, &g_keyQueueState);
	Rs232Flush();
	SystickDisable();
	SystickForFreertosEnable();
	Led1Off();
	vTaskStartScheduler();
}



