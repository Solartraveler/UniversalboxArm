
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "clockConfig.h"

#include "filesystem.h"
#include "json.h"
#include "utility.h"

#include "FreeRTOS.h"
#include "semphr.h"

#define WIFI_FILENAME "/etc/wifi.json"

#define TIME_FILENAME "/etc/time.json"


#define TEXT_MAX 64

typedef struct {
	bool clockEnabled;
	char ap[TEXT_MAX];
	char password[TEXT_MAX];
	char timeserver[TEXT_MAX];
	int16_t timeOffset; //unit is [min]
	bool summerTime; //EU calculation only
	uint16_t refreshInterval; //unit is [h]
	uint8_t fgColor;
	uint8_t bgColor;
} config_t;

config_t g_config;

SemaphoreHandle_t g_configSemaphore;
StaticSemaphore_t g_configSemaphoreState;


static void ConfigLock(void) {
	//Simply must succeed
	while (xSemaphoreTake(g_configSemaphore, 1000) == pdFALSE);
}

static void ConfigUnlock(void) {
	xSemaphoreGive(g_configSemaphore);
}

void ConfigInit(void) {
	g_configSemaphore = xSemaphoreCreateMutexStatic(&g_configSemaphoreState);
	ConfigLock();
	uint8_t jsonfile[TEXT_MAX * 3] = {0};
	size_t r = 0;
	if (FilesystemReadFile(WIFI_FILENAME, jsonfile, sizeof(jsonfile) - 1, &r)) {
		bool success = JsonValueGet(jsonfile, r, "ap1", g_config.ap, TEXT_MAX);
		success &= JsonValueGet(jsonfile, r, "password1", g_config.password, TEXT_MAX);
		if (success) {
			printf("Wifi config loaded\r\n");
		} else {
			printf("Wifi config file incomplete\r\n");
		}
	} else {
		printf("No wifi configured\r\n");
	}
	bool hasServer = false;
	bool hasInterval = false;
	bool hasFgColor = false;
	bool hasBgColor = false;
	if (FilesystemReadFile(TIME_FILENAME, jsonfile, sizeof(jsonfile) - 1, &r)) {
		hasServer = JsonValueGet(jsonfile, r, "ntpserver", g_config.timeserver, TEXT_MAX);
		char value[TEXT_MAX];
		hasInterval = JsonValueGet(jsonfile, r, "ntprefresh", value, TEXT_MAX);
		if (hasInterval) {
			g_config.refreshInterval = atoi(value);
			if (g_config.refreshInterval == 0) {
				hasInterval = false;
			}
		}
		if (JsonValueGet(jsonfile, r, "timeoffset", value, TEXT_MAX)) {
			g_config.timeOffset = atoi(value);
		}
		if (JsonValueGet(jsonfile, r, "summertime", value, TEXT_MAX)) {
			if (strcmp(value, "true") == 0) {
				g_config.summerTime = true;
			}
		}
		if (JsonValueGet(jsonfile, r, "fgcolor", value, TEXT_MAX)) {
			g_config.fgColor = atoi(value);
			hasFgColor = true;
		}
		if (JsonValueGet(jsonfile, r, "bgcolor", value, TEXT_MAX)) {
			g_config.bgColor = atoi(value);
			hasBgColor = true;
		}
	} else {
		printf("No time configured\r\n");
	}
	if (!hasServer) {
		strncpy(g_config.timeserver, DEFAULT_TIMESERVER, TEXT_MAX - 1);
	}
	if (!hasInterval) {
		g_config.refreshInterval = 48;
	}
	if (!hasFgColor) {
		g_config.fgColor = 0;
	}
	if (!hasBgColor) {
		g_config.fgColor = 0xFF;
	}
	ConfigUnlock();
}

void ConfigSaveWifi(void) {
	ConfigLock();
	char buffer[TEXT_MAX * 3];
	snprintf(buffer, sizeof(buffer), "{\n  \"ap1\": \"%s\",\n  \"password1\": \"%s\"\n}\n", g_config.ap, g_config.password);
	if (FilesystemWriteEtcFile(WIFI_FILENAME, buffer, strlen(buffer))) {
		printf("Saved to %s\r\n", WIFI_FILENAME);
	} else {
		printf("Error, could not create file %s\r\n", WIFI_FILENAME);
	}
	ConfigUnlock();
}

void ConfigSaveClock(void) {
	ConfigLock();
	char buffer[TEXT_MAX * 4];
	snprintf(buffer, sizeof(buffer), "{\n  \"ntpserver\": \"%s\",\n  \"ntprefresh\": \"%u\",\n  \"timeoffset\": \"%i\",\n  \"summertime\": \"%s\",\n  \"fgcolor\": \"%u\",\n  \"bgcolor\":  \"%u\"\n\n}\n",
	          g_config.timeserver, g_config.refreshInterval, g_config.timeOffset, g_config.summerTime ? "true" : "false", g_config.fgColor, g_config.bgColor);
	if (FilesystemWriteEtcFile(TIME_FILENAME, buffer, strlen(buffer))) {
		printf("Saved to %s\r\n", TIME_FILENAME);
	} else {
		printf("Error, could not create file %s\r\n", TIME_FILENAME);
	}
	ConfigUnlock();
}

void ApGet(char * buffer, size_t bufferMax) {
	ConfigLock();
	strlcpy(buffer, g_config.ap, bufferMax);
	ConfigUnlock();
}

void ApSet(const char * newAp) {
	ConfigLock();
	strlcpy(g_config.ap, newAp, TEXT_MAX);
	ConfigUnlock();
}

void PasswordGet(char * buffer, size_t bufferMax) {
	ConfigLock();
	strlcpy(buffer, g_config.password, bufferMax);
	ConfigUnlock();
}

void PasswordSet(const char * newPassword) {
	ConfigLock();
	strlcpy(g_config.password, newPassword, TEXT_MAX);
	ConfigUnlock();
}

void TimeserverGet(char * buffer, size_t bufferMax) {
	ConfigLock();
	strlcpy(buffer, g_config.timeserver, bufferMax);
	ConfigUnlock();
}

void TimeserverSet(const char * newServer) {
	ConfigLock();
	strlcpy(g_config.timeserver, newServer, TEXT_MAX);
	ConfigUnlock();
}

uint16_t TimeserverRefreshGet(void) {
	uint16_t interval;
	ConfigLock();
	interval = g_config.refreshInterval;
	ConfigUnlock();
	return interval;
}

void TimeserverRefreshSet(uint16_t interval) {
	ConfigLock();
	g_config.refreshInterval = interval;
	ConfigUnlock();
}

int16_t UtcOffsetGet(void) {
	int16_t offset;
	ConfigLock();
	offset = g_config.timeOffset;
	ConfigUnlock();
	return offset;
}

void UtcOffsetSet(int16_t newOffset) {
	ConfigLock();
	g_config.timeOffset = newOffset;
	ConfigUnlock();
}

bool SummertimeGet(void) {
	return g_config.summerTime;
}

void SummertimeSet(bool enabled) {
	g_config.summerTime = enabled;
}

uint8_t ColorBgGet(void) {
	uint8_t color;
	ConfigLock();
	color = g_config.bgColor;
	ConfigUnlock();
	return color;
}

void ColorBgSet(uint8_t color) {
	ConfigLock();
	g_config.bgColor = color;
	ConfigUnlock();
}

uint8_t ColorFgGet(void) {
	uint8_t color;
	ConfigLock();
	color = g_config.fgColor;
	ConfigUnlock();
	return color;
}

void ColorFgSet(uint8_t color) {
	ConfigLock();
	g_config.fgColor = color;
	ConfigUnlock();
}

