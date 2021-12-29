#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/*Important:
32MBit AT45DB321E: 512byte pagesize
Device id: 0x28, second byte: 0x0

64MBit AT45DB641E: 256byte pagesize
Device id: 0x27, second byte: 0x1
*/

#define AT45PAGESIZE 256

void FlashEnable(void);

void FlashDisable(void);

uint16_t FlashGetStatus(void);

void FlashGetId(uint8_t * manufacturer, uint16_t * device);

//Sets the mode to 2^n adressing.
void FlashPagesizePowertwo(void);

bool FlashRead(uint32_t address, uint8_t * buffer, size_t len);

bool FlashWrite(uint32_t address, const uint8_t * buffer, size_t len);

//This should return the last AT45PAGESIZE byte written by FlashWrite
//Intended for debug purpose
bool FlashReadBuffer1(uint8_t * buffer, uint32_t offset, size_t len);
