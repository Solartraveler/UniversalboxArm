/* Boxlib
(c) 2021 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause
*/

#include <stdbool.h>

#include "coproc.h"

#include "main.h"

#include "coprocCommands.h"

bool CoprocInGet(void) {
	if (HAL_GPIO_ReadPin(AvrSpiMiso_GPIO_Port, AvrSpiMiso_Pin) == GPIO_PIN_RESET) {
		return false;
	}
	return true;
}

static void CoprocClockSet(bool state) {
	if (state) {
		HAL_GPIO_WritePin(AvrSpiSck_GPIO_Port, AvrSpiSck_Pin, GPIO_PIN_SET);
	} else {
		HAL_GPIO_WritePin(AvrSpiSck_GPIO_Port, AvrSpiSck_Pin, GPIO_PIN_RESET);
	}
}

static void CoprocDataSet(bool state) {
	if (state) {
		HAL_GPIO_WritePin(AvrSpiMosi_GPIO_Port, AvrSpiMosi_Pin, GPIO_PIN_SET);
	} else {
		HAL_GPIO_WritePin(AvrSpiMosi_GPIO_Port, AvrSpiMosi_Pin, GPIO_PIN_RESET);
	}
}

static void CoprocCycleDelay(void) {
	HAL_Delay(1);
}

uint16_t CoprocSendCommand(uint8_t command, uint16_t data) {
	uint32_t dataIn = 0;
	uint32_t dataOut = (command << 16) | data;
	for (uint32_t i = 0; i < 24; i++) {
		if (dataOut & 0x800000) {
			CoprocDataSet(true);
		} else {
			CoprocDataSet(false);
		}
		dataOut <<= 1;
		CoprocCycleDelay();
		dataIn <<= 1;
		if (CoprocInGet()) {
			dataIn |= 1;
		}
		CoprocClockSet(true);
		CoprocCycleDelay();
		CoprocClockSet(false);
	}
	return dataIn & 0xFFFFFF;
}

uint16_t CoprocReadTestpattern(void) {
	uint16_t pattern = CoprocSendCommand(CMD_TESTREAD, 0);
	return pattern;
}

uint16_t CoprocReadVersion(void) {
	uint16_t version = CoprocSendCommand(CMD_VERSION, 0);
	return version;
}

uint16_t CoprocReadVcc(void) {
	uint16_t vcc = CoprocSendCommand(CMD_VCC, 0);
	return vcc;
}

void CoprocWriteReboot(uint8_t mode) {
	CoprocSendCommand(CMD_REBOOT, 0xA600 | mode);
}

