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

/* Minimum flash size for fat is 128 sectors, 512byte each.
  Also the first 4Kib are left unsued (could be changed).
  So the minimum memory needed is 68KiB. But there are no 68KiB RAM left...
  Otherwise a RAM disk could be created.
*/

void FlashEnable(uint32_t clockPrescaler) {
	(void)clockPrescaler;
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
	return false;
}

uint32_t FlashBlocksizeGet(void) {
	return 1;
}

bool FlashReadBuffer1(uint8_t * buffer, uint32_t offset, size_t len) {
	(void)buffer;
	(void)offset;
	(void)len;
	return false;
}

uint32_t FlashSizeGet(void) {
	return 0;
}

bool FlashRead(uint32_t address, uint8_t * buffer, size_t len) {
	(void)address;
	(void)buffer;
	(void)len;
	return false;
}

bool FlashWrite(uint32_t address, const uint8_t * buffer, size_t len) {
	(void)address;
	(void)buffer;
	(void)len;
	return false;
}
