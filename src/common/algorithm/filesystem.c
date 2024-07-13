/* Boxlib
(c) 2022 by Malte Marwedel

SPDX-License-Identifier: BSD-3-Clause
*/

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "filesystem.h"

#include "ff.h"
#include "json.h"
#include "utility.h"

#include "boxlib/lcd.h"
#include "boxlib/flash.h"

FATFS g_fatfs;

bool FilesystemMount(void) {
	if (FlashReady() != true) {
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

bool FilesystemReadFile(const char * filename, void * data, size_t bufferLen, size_t * pReadLen) {
	bool success = false;
	FIL f;
	if (FR_OK == f_open(&f, filename, FA_READ)) {
		UINT r = 0;
		FRESULT res = f_read(&f, data, bufferLen, &r);
		if (res == FR_OK) {
			success = true;
		} else {
			printf("Warning, could not read >%s<\r\n", filename);
		}
		f_close(&f);
		if (pReadLen) {
			*pReadLen = r;
		}
	}
	return success;
}

eDisplay_t FilesystemReadLcd(void) {
	uint8_t displayconf[64] = {0};
	char lcdtype[32];
	size_t r = 0;
	if (!FilesystemReadFile(DISPLAYFILENAME, displayconf, sizeof(displayconf) - 1, &r)) {
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

bool FilesystemBufferwriterStart(fileBuffer_t * pFileBuffer, const char * filename) {
	if ((pFileBuffer) && (filename)) {
		if (FR_OK == f_open(&(pFileBuffer->f), filename, FA_WRITE | FA_CREATE_ALWAYS)) {
			pFileBuffer->index = 0;
			return true;
		}
	}
	return false;
}

bool FilesystemBufferwriterWb(fileBuffer_t * pFileBuffer) {
	UINT written = 0;
	FRESULT res = f_write(&(pFileBuffer->f), pFileBuffer->buffer, pFileBuffer->index, &written);
	if ((res == FR_OK) && (written == pFileBuffer->index)) {
		pFileBuffer->index = 0;
		return true;
	}
	return false;
}

bool FilesystemBufferwriterAppend(const void * data, size_t len, void * pFileBuffer) {
	size_t i = 0;
	if ((!pFileBuffer) || (!data)) {
		return false;
	}
	fileBuffer_t * pFB = (fileBuffer_t *)pFileBuffer;
	while (i < len) {
		if (pFB->index == FILEBUFFER_SIZE) {
			if (FilesystemBufferwriterWb(pFB) != true) {
				return false;
			}
		}
		size_t todo = len - i;
		size_t left = FILEBUFFER_SIZE - pFB->index;
		size_t add = MIN(todo, left);
		memcpy(pFB->buffer + pFB->index, data + i, add);
		i += add;
		pFB->index += add;
	}
	return true;
}

bool FilesystemBufferwriterClose(fileBuffer_t * pFileBuffer) {
	bool success = false;
	if (pFileBuffer) {
		success = true;
		if (pFileBuffer->index) {
			success &= FilesystemBufferwriterWb(pFileBuffer);
		}
		if (f_close(&(pFileBuffer->f)) != FR_OK) {
			success = false;
		}
	}
	return success;
}

uint32_t FilesystemGetUnusedFilename(const char * directory, const char * prefix) {
	DIR d;
	FILINFO fi;
	uint32_t goodId = 0;
	size_t prefixLen = strlen(prefix);
	if (f_opendir(&d, directory) == FR_OK) {
		while (f_readdir(&d, &fi) == FR_OK) {
			if (!fi.fname[0]) {
				break;
			}
			if (BeginsWith(fi.fname, prefix)) {
				uint32_t num = AsciiScanDec(fi.fname + prefixLen);
				if (num > goodId) {
					goodId = num;
				}
			}
		}
		f_closedir(&d);
		goodId++;
	}
	return goodId;
}

