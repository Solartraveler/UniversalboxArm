/* Boxlib
(c) 2021 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause
*/

#include <stdbool.h>
#include <stdint.h>

#include "flash.h"

#include "peripheral.h"

#include "main.h"

bool g_FlashInit;

static void FlashCsOn(void) {
	if (g_FlashInit) {
		HAL_GPIO_WritePin(FlashCs_GPIO_Port, FlashCs_Pin, GPIO_PIN_RESET);
	}
}

static void FlashCsOff(void) {
	if (g_FlashInit) {
		HAL_GPIO_WritePin(FlashCs_GPIO_Port, FlashCs_Pin, GPIO_PIN_SET);
	}
}

void FlashEnable(void) {
	GPIO_InitTypeDef state = {0};
	state.Pin = FlashCs_Pin;
	state.Mode = GPIO_MODE_OUTPUT_PP;
	state.Pull = GPIO_NOPULL;
	state.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(FlashCs_GPIO_Port, &state);
	g_FlashInit = true;
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
	g_FlashInit = false;
}

static void FlashTransfer(const uint8_t * dataOut, uint8_t * dataIn, size_t len) {
	if (g_FlashInit) {
		FlashCsOn();
		PeripheralTransfer(dataOut, dataIn, len);
		FlashCsOff();
	}
}

uint16_t FlashGetStatus(void) {
	uint8_t out[3] = {0xD7, 0, 0};
	uint8_t in[3] = {0};
	FlashTransfer(out, in, sizeof(out));
	return (in[1] << 8) | in[2];
}

void FlashGetId(uint8_t * manufacturer, uint16_t * device) {
	uint8_t out[5] = {0x9F, 0, 0};
	uint8_t in[5] = {0};
	FlashTransfer(out, in, sizeof(out));
	if (manufacturer) {
		*manufacturer = in[1];
	}
	if (device) {
		*device = (in[2] << 8) | in[3];
	}
}

void FlashWaitNonBusy(void) {
	uint8_t out[2] = {0xD7, 0};
	uint8_t in[2] = {0};
	do {
		FlashTransfer(out, in, sizeof(out));
		//highest bit shows ready. But if everything is 0, some error occurred
	} while (((in[1] & 0x80) == 0) && (in[1] != 0));
}

void FlashPagesizePowertwo(void) {
	FlashWaitNonBusy();
	uint8_t out[4] = {0x3D, 0x2A, 0x80, 0xA6};
	uint8_t in[4] = {0};
	FlashTransfer(out, in, sizeof(out));
}

bool FlashRead(uint32_t address, uint8_t * buffer, size_t len) {
	uint8_t out[4];
	out[0] = 0x01; //low power read up to 15MHz
	out[1] = (address >> 16) & 0xFF;
	out[2] = (address >> 8) & 0xFF;
	out[3] = address & 0xFF;
	if (g_FlashInit) {
		FlashWaitNonBusy();
		FlashCsOn();
		PeripheralTransfer(out, NULL, sizeof(out));
		PeripheralTransfer(NULL, buffer, len);
		FlashCsOff();
		return true;
	}
	return false;
}

static bool FlashWritePage(uint32_t address, const uint8_t * buffer) {
	//1. delete page
	FlashWaitNonBusy();
	uint8_t out[4];
	out[0] = 0x81; //page erase
	out[1] = (address >> 16) & 0xFF;
	out[2] = (address >> 8) & 0xFF;
	out[3] = address & 0xFF;
	FlashTransfer(out, NULL, sizeof(out));
	//2. send data to 1. buffer
	FlashWaitNonBusy();
	out[0] = 0x84; //write to sram buffer 1
	out[1] = 0x0;
	out[2] = 0x0;
	out[3] = 0x0;
	FlashCsOn();
	PeripheralTransfer(out, NULL, sizeof(out));
	PeripheralTransfer(buffer, NULL, AT45PAGESIZE);
	FlashCsOff();
	//3. write data to flash
	FlashWaitNonBusy();
	out[0] = 0x88; //sram 1 to flash without erase
	out[1] = (address >> 16) & 0xFF;
	out[2] = (address >> 8) & 0xFF;
	out[3] = address & 0xFF;
	FlashTransfer(out, NULL, sizeof(out));
	FlashWaitNonBusy();
	return true;
}

bool FlashWrite(uint32_t address, const uint8_t * buffer, size_t len) {
	if ((address % AT45PAGESIZE) || (len % AT45PAGESIZE)) {
		return false;
	}
	if (!g_FlashInit) {
		return false;
	}
	while (len) {
		if (!FlashWritePage(address, buffer)) {
			return false;
		}
		address += AT45PAGESIZE;
		buffer += AT45PAGESIZE;
		len -= AT45PAGESIZE;
	}
	return true;
}

bool FlashReadBuffer1(uint8_t * buffer, uint32_t offset, size_t len) {
	if (!g_FlashInit) {
		return false;
	}
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
	return true;
}

