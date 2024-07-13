/* Boxlib
(c) 2021 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause
*/

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "boxlib/flash.h"

#include "boxlib/peripheral.h"

#include "main.h"

//some devices use 512 byte!
#define FLASHPAGESIZE 256

bool g_flashInit;
uint32_t g_flashPrescaler;

static void FlashCsOn(void) {
	if (g_flashInit) {
		HAL_GPIO_WritePin(FlashCs_GPIO_Port, FlashCs_Pin, GPIO_PIN_RESET);
	}
}

static void FlashCsOff(void) {
	if (g_flashInit) {
		HAL_GPIO_WritePin(FlashCs_GPIO_Port, FlashCs_Pin, GPIO_PIN_SET);
	}
}

void FlashEnable(uint32_t clockPrescaler) {
	GPIO_InitTypeDef state = {0};
	state.Pin = FlashCs_Pin;
	state.Mode = GPIO_MODE_OUTPUT_PP;
	state.Pull = GPIO_NOPULL;
	state.Speed = GPIO_SPEED_FREQ_HIGH;
	g_flashPrescaler = clockPrescaler;
	HAL_GPIO_Init(FlashCs_GPIO_Port, &state);
	g_flashInit = true;
	HAL_Delay(1);
	FlashCsOff();
}

void FlashDisable(void) {
	HAL_GPIO_WritePin(FlashCs_GPIO_Port, FlashCs_Pin, GPIO_PIN_RESET);
	GPIO_InitTypeDef state = {0};
	state.Pin = FlashCs_Pin;
	state.Mode = GPIO_MODE_INPUT;
	state.Pull = GPIO_PULLDOWN;
	HAL_GPIO_Init(FlashCs_GPIO_Port, &state);
	g_flashInit = false;
}

static void FlashTransfer(const uint8_t * dataOut, uint8_t * dataIn, size_t len) {
	if (g_flashInit) {
		FlashCsOn();
		PeripheralTransfer(dataOut, dataIn, len);
		FlashCsOff();
	}
}

uint16_t FlashGetStatus(void) {
	uint8_t out[3] = {0xD7, 0, 0};
	uint8_t in[3] = {0};
	PeripheralLockMt();
	PeripheralPrescaler(g_flashPrescaler);
	FlashTransfer(out, in, sizeof(out));
	PeripheralUnlockMt();
	return (in[1] << 8) | in[2];
}

void FlashGetId(uint8_t * manufacturer, uint16_t * device) {
	uint8_t out[5] = {0x9F, 0, 0};
	uint8_t in[5] = {0};
	PeripheralLockMt();
	PeripheralPrescaler(g_flashPrescaler);
	FlashTransfer(out, in, sizeof(out));
	if (manufacturer) {
		*manufacturer = in[1];
	}
	if (device) {
		*device = (in[2] << 8) | in[3];
	}
	PeripheralUnlockMt();
}

static void FlashWaitNonBusy(void) {
	uint8_t out[2] = {0xD7, 0};
	uint8_t in[2] = {0};
	do {
		FlashTransfer(out, in, sizeof(out));
		//highest bit shows ready. But if everything is 0, some error occurred
	} while (((in[1] & 0x80) == 0) && (in[1] != 0));
}

void FlashPagesizePowertwoSet(void) {
	PeripheralLockMt();
	PeripheralPrescaler(g_flashPrescaler);
	FlashWaitNonBusy();
	uint8_t out[4] = {0x3D, 0x2A, 0x80, 0xA6};
	uint8_t in[4] = {0};
	FlashTransfer(out, in, sizeof(out));
	PeripheralUnlockMt();
}

bool FlashPagesizePowertwoGet(void) {
	uint16_t status = FlashGetStatus();
	uint16_t pageSizeB = (status >> 8) & 1;
	if (pageSizeB) {
		return true;
	}
	return false;
}

bool FlashRead(uint64_t address, uint8_t * buffer, size_t len) {
	uint8_t out[4];
	out[0] = 0x01; //low power read up to 15MHz
	out[1] = (address >> 16) & 0xFF;
	out[2] = (address >> 8) & 0xFF;
	out[3] = address & 0xFF;
	if (g_flashInit) {
		PeripheralLockMt();
		PeripheralPrescaler(g_flashPrescaler);
		FlashWaitNonBusy();
		FlashCsOn();
		PeripheralTransfer(out, NULL, sizeof(out));
		PeripheralTransferBackground(NULL, buffer, len);
		PeripheralTransferWaitDone();
		FlashCsOff();
		PeripheralUnlockMt();
		return true;
	}
	return false;
}



static bool FlashWriteBuffer1Int(const uint8_t * buffer) {
	uint8_t out[4];
	FlashWaitNonBusy();
	out[0] = 0x84; //write to sram buffer 1
	out[1] = 0x0;
	out[2] = 0x0;
	out[3] = 0x0;
	FlashCsOn();
	PeripheralTransfer(out, NULL, sizeof(out));
	PeripheralTransfer(buffer, NULL, FLASHPAGESIZE);
	FlashCsOff();
	return true;
}


bool FlashWriteBuffer1(const uint8_t * buffer) {
	bool result;
	PeripheralLockMt();
	PeripheralPrescaler(g_flashPrescaler);
	result = FlashWriteBuffer1Int(buffer);
	PeripheralUnlockMt();
	return result;
}

//Thread safe if peripheralMt.c is used
static bool FlashWritePage(uint32_t address, const uint8_t * buffer) {
	//0. thread safetyness
	PeripheralLockMt();
	//1. set prescaler
	PeripheralPrescaler(g_flashPrescaler);
	//1. delete page
	FlashWaitNonBusy();
	uint8_t out[4];
	out[0] = 0x81; //page erase
	out[1] = (address >> 16) & 0xFF;
	out[2] = (address >> 8) & 0xFF;
	out[3] = address & 0xFF;
	FlashTransfer(out, NULL, sizeof(out));
	//2. send data to 1. buffer
	bool success = FlashWriteBuffer1Int(buffer);
	//3. write data to flash
	FlashWaitNonBusy();
	out[0] = 0x88; //sram 1 to flash without erase
	out[1] = (address >> 16) & 0xFF;
	out[2] = (address >> 8) & 0xFF;
	out[3] = address & 0xFF;
	FlashTransfer(out, NULL, sizeof(out));
	FlashWaitNonBusy();
	//4. unlock
	PeripheralUnlockMt();
	return success;
}

bool FlashWrite(uint64_t address, const uint8_t * buffer, size_t len) {
	if ((address % FLASHPAGESIZE) || (len % FLASHPAGESIZE)) {
		return false;
	}
	if (!g_flashInit) {
		return false;
	}
	while (len) {
		if (!FlashWritePage(address, buffer)) {
			return false;
		}
		address += FLASHPAGESIZE;
		buffer += FLASHPAGESIZE;
		len -= FLASHPAGESIZE;
	}
	return true;
}

bool FlashReadBuffer1(uint8_t * buffer, uint32_t offset, size_t len) {
	if (!g_flashInit) {
		return false;
	}
	PeripheralLockMt();
	PeripheralPrescaler(g_flashPrescaler);
	FlashWaitNonBusy();
	uint8_t out[4];
	out[0] = 0xD1; //read buffer 1 at low frequency
	out[1] = (offset >> 16) & 0xFF;
	out[2] = (offset >> 8) & 0xFF;
	out[3] = offset & 0xFF;
	FlashCsOn();
	PeripheralTransfer(out, NULL, sizeof(out));
	PeripheralTransfer(NULL, buffer, len);
	FlashCsOff();
	PeripheralUnlockMt();
	return true;
}

uint64_t FlashSizeGet(void) {
	uint8_t manufacturer;
	uint16_t device;
	FlashGetId(&manufacturer, &device);
	uint8_t density2 = (device >> 8) & 0x1F;
	if (density2 == 0x7) {
		return (4 * 1024 * 1024);
	}
	if (density2 == 0x8) {
		return (8 * 1024 * 1024);
	}
	return 0;
}

uint32_t FlashBlocksizeGet(void) {
	return FLASHPAGESIZE;
}

bool FlashReady(void) {
	uint16_t status = FlashGetStatus();
	uint16_t error = (status >> 5) & 1;
	uint16_t ready = (status >> 15) & 1;
	if ((ready) && (error == 0)) {
		return true;
	}
	return false;
}

bool FlashTest(void) {
	if (FlashReady() == false) {
		return false;
	}
	uint8_t dataOut[FLASHPAGESIZE];
	uint8_t dataIn[FLASHPAGESIZE] = {0};
	//1. write round
	for (size_t i = 0; i < FLASHPAGESIZE; i++) {
		dataOut[i] = i;
	}
	if (FlashWriteBuffer1(dataOut) == false) {
		return false;
	}
	if (FlashReadBuffer1(dataIn, 0, FLASHPAGESIZE) == false) {
		return false;
	}
	if (memcmp(dataOut, dataIn, FLASHPAGESIZE)) {
		return false;
	}
	//2. write round with different pattern
	memset(dataOut, 0x23, FLASHPAGESIZE);
	if (FlashWriteBuffer1(dataOut) == false) {
		return false;
	}
	if (FlashReadBuffer1(dataIn, 0, FLASHPAGESIZE) == false) {
		return false;
	}
	if (memcmp(dataOut, dataIn, FLASHPAGESIZE)) {
		return false;
	}
	return true;
}
