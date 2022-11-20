#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/*Important:
32MBit AT45DB321E: 512byte pagesize
Device id: 0x28, second byte: 0x0

64MBit AT45DB641E: 256byte pagesize
Device id: 0x27, second byte: 0x1

Surprising discovery: While the datasheet does not require a minimum SPI
frequency, data writing seem to abort at some point with a frequency of
15.625kHz after 191 byte + 4 command bytes. This is 0.09984s, so there seems
to be a 0.1s timeout within the device.
*/

void FlashEnable(uint32_t clockPrescaler);

void FlashDisable(void);

uint16_t FlashGetStatus(void);

void FlashGetId(uint8_t * manufacturer, uint16_t * device);

//Sets the mode to 2^n adressing.
void FlashPagesizePowertwoSet(void);
bool FlashPagesizePowertwoGet(void);

bool FlashRead(uint32_t address, uint8_t * buffer, size_t len);

//Internally used by FlashWritePage, otherwise use for debug only.
//Writes to the SRAM1 buffer, so no flash is actually written.
//buffer must have FLASHPAGESIZE number of bytes.
bool FlashWriteBuffer1(const uint8_t * buffer);

bool FlashWrite(uint32_t address, const uint8_t * buffer, size_t len);

//This should return the last FLASHPAGESIZE byte written by FlashWrite
//or FlashWriteBuffer1 data
//Intended for debug purpose
bool FlashReadBuffer1(uint8_t * buffer, uint32_t offset, size_t len);

uint32_t FlashSizeGet(void);

uint32_t FlashBlocksizeGet(void);

bool FlashReady(void);

/*Writes some data to the internal SRAM, then read them back and compares
  for validity. Returns true if content fits. No data is written to the flash.
*/
bool FlashTest(void);