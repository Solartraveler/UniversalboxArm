/* Boxlib emulation
(c) 2022 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause
*/

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "flash.h"

#define FILENAME "emulatedFlash.bin"

uint8_t * g_flashData;
size_t g_flashDataSize;
uint32_t g_flashLastTransferred;

void FlashEnable(void) {
	//load from file
	if (!g_flashData) {
		g_flashDataSize = (1024 * 1024 *8);
		g_flashData = (uint8_t*)malloc(g_flashDataSize);
		if (g_flashData) {
			memset(g_flashData, 0xFF, g_flashDataSize);
			FILE * f = fopen(FILENAME, "rb");
			if (f) {
				fread(g_flashData, 1, g_flashDataSize, f);
				fclose(f);
			}
		}
	}
}

void FlashDisable(void) {
	if (g_flashData) {
		//save to file
		FILE * f = fopen(FILENAME, "wb");
		if (f) {
			fwrite(g_flashData, 1, g_flashDataSize, f);
			fclose(f);
		}
	}
}

uint16_t FlashGetStatus(void) {
	return 0xBD80; //Ready, 64MBit, 2^n pagesize
}

void FlashGetId(uint8_t * manufacturer, uint16_t * device) {
	if (manufacturer) {
		*manufacturer = 0x1F; //tell we are adesto
	}
	if (device) {
		*device = 0x3C00; //8MiB device
	}
}

void FlashWaitNonBusy(void) {
}

void FlashPagesizePowertwo(void) {
}

bool FlashRead(uint32_t address, uint8_t * buffer, size_t len) {
	if ((g_flashData) && ((address + len) <= g_flashDataSize)) {
		memcpy(buffer, g_flashData + address, len);
		return true;
	}
	return false;
}

bool FlashWrite(uint32_t address, const uint8_t * buffer, size_t len) {
	if ((address % AT45PAGESIZE) || (len % AT45PAGESIZE)) {
		return false;
	}
	if (!g_flashData) {
		return false;
	}
	if ((g_flashData) && ((address + len) <= g_flashDataSize)) {
		memcpy(g_flashData + address, buffer, len);
		return true;
	}
	return false;
}

bool FlashReadBuffer1(uint8_t * buffer, uint32_t offset, size_t len) {
	(void)buffer;
	(void)offset;
	(void)len;
	return false;
}

