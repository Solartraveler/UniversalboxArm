/* Boxlib
(c) 2024 by Malte Marwedel

SPDX-License-Identifier: BSD-3-Clause

As there is no flash connected to the nucleo board, this implementation
provides a simple 32KiB ramdisk.
*/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "boxlib/flash.h"

#include "boxlib/flash.h"
#include "boxlib/spiExternal.h"
#include "sdmmcAccess.h"

bool g_flashInitSuccess;

void FlashEnable(uint32_t clockPrescaler) {
	SpiExternalInit();
	SpiExternalPrescaler(clockPrescaler);
	if (SdmmcInit(&SpiExternalTransfer, 1) == 0) {
		g_flashInitSuccess = true;
		//Assuming 250kHz before, now we run at 1MHz, a breadboard propably will not allow anything faster
		SpiExternalPrescaler(clockPrescaler / 4);
	}
}

void FlashDisable(void) {
}

uint16_t FlashGetStatus(void) {
	return 0;
}

void FlashGetId(uint8_t * manufacturer, uint16_t * device) {
	*manufacturer = 0;
	*device = 0;
}

void FlashPagesizePowertwoSet(void) {
}

bool FlashPagesizePowertwoGet(void) {
	return true;
}

bool FlashReady(void) {
	return g_flashInitSuccess;
}

uint32_t FlashBlocksizeGet(void) {
	return SDMMC_BLOCKSIZE;
}

bool FlashReadBuffer1(uint8_t * buffer, uint32_t offset, size_t len) {
	(void)buffer;
	(void)offset;
	(void)len;
	return false;
}

uint64_t FlashSizeGet(void) {
	return SdmmcCapacity() * SDMMC_BLOCKSIZE;
}

bool FlashRead(uint64_t address, uint8_t * buffer, size_t len) {
	return SdmmcRead(buffer, address / SDMMC_BLOCKSIZE, len / SDMMC_BLOCKSIZE);
}

bool FlashWrite(uint64_t address, const uint8_t * buffer, size_t len) {
	return SdmmcWrite(buffer, address / SDMMC_BLOCKSIZE, len / SDMMC_BLOCKSIZE);
}
