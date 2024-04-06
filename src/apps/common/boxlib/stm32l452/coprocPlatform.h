/* Boxlib
(c) 2021-2024 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause

This file should only be included by coproc.c. Including it multiple times
will not work.
*/

#include <stdbool.h>

#include "boxlib/coproc.h"

#include "boxlib/mcu.h"
#include "main.h"


void CoprocInit(void) {
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	__HAL_RCC_GPIOC_CLK_ENABLE();
	HAL_GPIO_WritePin(GPIOC, AvrSpiSck_Pin | AvrSpiMosi_Pin, GPIO_PIN_RESET);

	GPIO_InitStruct.Pin = AvrSpiSck_Pin | AvrSpiMosi_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = AvrSpiMiso_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	HAL_GPIO_Init(AvrSpiMiso_GPIO_Port, &GPIO_InitStruct);
}

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
	/*Tests show a delay of 1200µs works in every case, so we use 1400 to be safe.
	 HAL_Delay(1) only worked, because it does a 2ms nearly every time
	*/
	McuDelayUs(1400);
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
