/* Boxlib
(c) 2022 by Malte Marwedel

SPDX-License-Identifier: BSD-3-Clause
*/

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "filesystem.h"

#include "ff.h"
#include "json.h"

#include "boxlib/lcd.h"
#include "boxlib/flash.h"

FATFS g_fatfs;

bool FilesystemMount(void) {
	uint8_t manufacturer;
	uint16_t device;
	FlashGetId(&manufacturer, &device);
	if (manufacturer != 0x1F) {
		printf("Error, no valid answer from flash\r\n");
		return false;
	}
	FRESULT fres;
	fres = f_mount(&g_fatfs, "0", 1);
	if (fres == FR_OK) {
		return true;
	} else if (fres == FR_NO_FILESYSTEM) {
		printf("Warning, no filesystem\r\n");
		return false;
	} else {
		printf("Error, mounting returned %u\r\n", (unsigned int)fres);
		return false;
	}
}

eDisplay_t FilesystemReadLcd(void) {
	uint8_t displayconf[64] = {0};
	char lcdtype[32];
	FIL f;
	UINT r = 0;
	if (FR_OK == f_open(&f, DISPLAYFILENAME, FA_READ)) {
		FRESULT res = f_read(&f, displayconf, sizeof(displayconf) - 1, &r);
		if (res != FR_OK) {
			printf("Warning, could not read display config file\r\n");
		}
		f_close(&f);
	} else {
		printf("Warning, no display configured\r\n");
		return NONE;
	}
	if (JsonValueGet(displayconf, r, "lcd", lcdtype, sizeof(lcdtype))) {
		if (strcmp(lcdtype, "ST7735_128x128") == 0) {
			printf("LCD 128x128 selected\r\n");
			return ST7735_128;
		}
		if (strcmp(lcdtype, "ST7735_160x128") == 0) {
			printf("LCD 128x160 selected\r\n");
			return ST7735_160;
		}
		if (strcmp(lcdtype, "ILI9341_320x240") == 0) {
			printf("LCD 320x240 selected\r\n");
			return ILI9341;
		}
		if (strcmp(lcdtype, "NONE") == 0) {
			printf("no LCD selected\r\n");
			return NONE;
		}
	}
	printf("Warning, could not get LCD value\r\n");
	return NONE;
}

bool FilesystemWriteFile(const char * filename, const void * data, size_t dataLen) {
	FIL f;
	bool success = false;
	if (FR_OK == f_open(&f, filename, FA_WRITE | FA_CREATE_ALWAYS)) {
		UINT written = 0;
		FRESULT res = f_write(&f, data, dataLen, &written);
		if ((res == FR_OK) && (written == dataLen)) {
			success = true;
		}
		f_close(&f);
	}
	return success;
}

bool FilesystemWriteEtcFile(const char * filename, const void * data, size_t dataLen) {
	f_mkdir("/etc");
	return FilesystemWriteFile(filename, data, dataLen);
}

void FilesystemWriteLcd(const char * lcdType) {
	char buffer[256];
	snprintf(buffer, sizeof(buffer), "{\n  \"lcd\": \"%s\"\n}\n", lcdType);
	if (FilesystemWriteEtcFile(DISPLAYFILENAME, buffer, strlen(buffer))) {
		printf("Display set\r\n");
	} else {
		printf("Error, could not create file\r\n");
	}
}

void FilesystemLcdSet(const char * type) {
	if (strcmp(type, "128x128") == 0) {
		FilesystemWriteLcd("ST7735_128x128");
	} else if (strcmp(type, "160x128") == 0) {
		FilesystemWriteLcd("ST7735_160x128");
	} else if (strcmp(type, "320x240") == 0) {
		FilesystemWriteLcd("ILI9341_320x240");
	} else {
		FilesystemWriteLcd("NONE");
	}
}
