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

#include "main.h"

#include "utility.h"
#include "femtoVsnprintf.h"

#include "gui.h"
#include "filesystem.h"
#include "json.h"


#define WIFI_FILENAME "/etc/wifi.json"

#define TEXT_MAX 64

typedef struct {
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
	printf("n: Call ntp update\r\n");
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
	bool success = JsonValueGet(wificonf, r, "ap", g_state.ap, TEXT_MAX);
	success &= JsonValueGet(wificonf, r, "password", g_state.password, TEXT_MAX);
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
	Led1Off();
	g_cycleTick = HAL_GetTick();
}

void ExecReset(void) {
	printf("Reset selected\r\n");
	Rs232Flush();
	NVIC_SystemReset();
}

void ReadSerialLine(char * input, size_t len) {
	memset(input, 0, len);
	size_t i = 0;
	while (i < (len - 1)) {
		char c = Rs232GetChar();
		if (c != 0) {
			printf("%c", c);
			if ((c == '\r') || (c == '\n')) {
				break;
			}
			if ((c == '\b') && (i)) {
				i--;
				input[i] = 0;
			} else {
				input[i] = c;
				i++;
			}
		}
	}
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
	f_mkdir("/etc");
	snprintf(buffer, sizeof(buffer), "{\n  \"ap\": \"%s\",\n  \"password\": \"%s\"\n}\n", g_state.ap, g_state.password);
	FIL f;
	if (FR_OK == f_open(&f, WIFI_FILENAME, FA_WRITE | FA_CREATE_ALWAYS)) {
		UINT written = 0;
		FRESULT res = f_write(&f, buffer, strlen(buffer), &written);
		if (res == FR_OK) {
			printf("Saved to %s\r\n", WIFI_FILENAME);
		}
		f_close(&f);
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
	uint32_t transmitTimestampS;
	uint32_t transmitTimestampFrac;
} ntpData_t;

_Static_assert(sizeof(ntpData_t) == 48, "Please fix alignment!");

bool EspGetNtp(const char * domain, uint32_t * pTimestamp) {
	//most simple request, according to https://stackoverflow.com/questions/14171366/ntp-request-packet
	uint8_t ntpRequest[48] = {0};
	ntpRequest[0] = 0x1B; //version = 3, mode = client
	size_t written = 0;
	ntpData_t ntpAnswer;
	uint32_t error;
	for (uint32_t i = 0; i < 3; i++) {
		error = EspUdpRequestResponse(domain, 123, ntpRequest, sizeof(ntpRequest), (uint8_t*)(&ntpAnswer), sizeof(ntpAnswer), &written);
		if ((error == 0) && (written == sizeof(ntpAnswer))) {
			if (pTimestamp) {
				*pTimestamp = BytesFlip(ntpAnswer.transmitTimestampS);
			}
			return true;
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
			} else {
				printf("Error, response too short\r\n");
			}
		}
		printf("Request no %u failed\r\n", (unsigned int)i + 1);
	}
	return false;
}

void NtpUpdate(void) {
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
				uint32_t timestamp = 0;
				if (EspGetNtp("pool.ntp.org", &timestamp)) {
					printf("NTP timestamp: %u\r\n", (unsigned int)timestamp);
					uint32_t d = timestamp / (60 * 60 * 24);
					uint32_t h = timestamp / (60 * 60) % 24;
					uint32_t m = timestamp / 60 % 60;
					uint32_t s = timestamp % 60;
					printf("%u days, UTC %02u:%02u:%02u\r\n", (unsigned int)d, (unsigned int)h, (unsigned int)m, (unsigned int)s);
					PrintHex(&timestamp, 4);
				} else {
					printf("Error, could not get timestamp\r\n");
				}
			} else {
				printf("Error, could not connect\r\n");
			}
			printf("Disconnecting from access point\r\n");
			EspDisconnect();
		} else {
			printf("Error, Could not become a client\r\n");
		}
	} else {
		printf("Error, no answer\r\n");
	}
	EspStop();
	printf("Esp stopped\r\n");
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
