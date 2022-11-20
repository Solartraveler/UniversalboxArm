#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

void FlashEnable(uint32_t clockPrescaler);

void FlashDisable(void);

uint16_t FlashGetStatus(void);

void FlashGetId(uint8_t * manufacturer, uint16_t * device);

//Sets the mode to 2^n addressing.
void FlashPagesizePowertwoSet(void);

bool FlashPagesizePowertwoGet(void);

bool FlashRead(uint32_t address, uint8_t * buffer, size_t len);

bool FlashWrite(uint32_t address, const uint8_t * buffer, size_t len);

//This should return the last FLASHPAGESIZE byte written by FlashWrite
//Intended for debug purpose
bool FlashReadBuffer1(uint8_t * buffer, uint32_t offset, size_t len);

uint32_t FlashSizeGet(void);

uint32_t FlashBlocksizeGet(void);

//Returns true if allocating memory for simulation by FlashEnable was a success.
bool FlashReady(void);

//always returns the same as FlashReady
bool FlashTest(void);