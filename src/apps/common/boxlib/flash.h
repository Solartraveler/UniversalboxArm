#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "flashPlatform.h"

/*Important:
32MBit AT45DB321E: 512byte pagesize
Device id: 0x28, second byte: 0x0

64MBit AT45DB641E: 256byte pagesize
Device id: 0x27, second byte: 0x1

Surprising discovery: While the datasheet does not require a minimum SPI
frequency, data writing seem to abort at some point with a frequency of
15.625kHz after 191 byte + 4 command bytes. This is 0.09984s, so there seems
to be a 0.1s timeout within the device.

If a function is marked as thread safe, it can be used by
multiple threads, unlike the LCD functions.
*/

//Not thread safe
void FlashEnable(uint32_t clockPrescaler);

//Not thread safe
void FlashDisable(void);

//Thread safe if peripheralMt.c is used
uint16_t FlashGetStatus(void);

//Thread safe if peripheralMt.c is used
void FlashGetId(uint8_t * manufacturer, uint16_t * device);

//Sets the mode to 2^n adressing.
//Thread safe if peripheralMt.c is used
void FlashPagesizePowertwoSet(void);

//Thread safe if peripheralMt.c is used
bool FlashPagesizePowertwoGet(void);

//Thread safe if peripheralMt.c is used
bool FlashRead(uint64_t address, uint8_t * buffer, size_t len);

//Use for debug only.
//Writes to the SRAM1 buffer, so no flash is actually written.
//buffer must have FLASHPAGESIZE number of bytes.
//Thread safe if peripheralMt.c is used
bool FlashWriteBuffer1(const uint8_t * buffer);

/* len must be a multiple of FLASHPAGESIZE.
Thread safe if peripheralMt.c is used. If two threads are writing at overlapping
memory, each page will only contain data from one thread. But it is not defined
from which.
*/
bool FlashWrite(uint64_t address, const uint8_t * buffer, size_t len);

//This should return the last FLASHPAGESIZE byte written by FlashWrite
//or FlashWriteBuffer1 data
//Intended for debug purpose
//Thread safe if peripheralMt.c is used
bool FlashReadBuffer1(uint8_t * buffer, uint32_t offset, size_t len);

/*Thread safe if peripheralMt.c is used
  Returns the size in bytes.
*/
uint64_t FlashSizeGet(void);

//Thread safe
uint32_t FlashBlocksizeGet(void);

//Thread safe if peripheralMt.c is used
bool FlashReady(void);

/*Writes some data to the internal SRAM, then read them back and compares
  for validity. Returns true if content fits. No data is written to the flash.
  If peripheralMt.c is used, thread safe against reading at the same time.
  Not thread safe against writing at the same time - in this case false positive
  results are expected.
*/
bool FlashTest(void);