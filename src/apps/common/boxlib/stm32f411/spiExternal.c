/* Boxlib
(c) 2021 by Malte Marwedel

SPDX-License-Identifier: BSD-3-Clause
*/

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "boxlib/spiExternal.h"
#include "boxlib/spiGeneric.h"
#include "spiPlatform.h"

#include "main.h"

void SpiExternalBaseInit(void) {
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	__HAL_RCC_GPIOA_CLK_ENABLE(); //for CS pin
	__HAL_RCC_GPIOB_CLK_ENABLE(); //SPI1 pins
	__HAL_RCC_GPIOC_CLK_ENABLE(); //SPI1 pins

	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET);

	GPIO_InitStruct.Pin = GPIO_PIN_9;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	__HAL_RCC_SPI2_CLK_ENABLE();

	GPIO_InitStruct.Pin = GPIO_PIN_14 | GPIO_PIN_15; //PB14 = MISO, PB15 = MOSI
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM; //up to 50MHz
	GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = GPIO_PIN_7; //PC7 = SCK
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
	GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	SpiPlatformInit(SPI2);
}

void SpiExternalBaseDeinit(void) {
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	GPIO_InitStruct.Pin = GPIO_PIN_9;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = GPIO_PIN_14 | GPIO_PIN_15;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = GPIO_PIN_7;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	__HAL_RCC_SPI2_CLK_DISABLE();
}

__weak void SpiExternalInit(void) {
	SpiExternalBaseInit();
}

void SpiExternalChipSelect(uint8_t chipSelect, bool selected) {
	GPIO_PinState state = GPIO_PIN_SET;
	if (selected) {
		state = GPIO_PIN_RESET;
	}
	if (chipSelect == 1) { //PA9
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, state);
	}
}

void SpiExternalTransferPolling(const uint8_t * dataOut, uint8_t * dataIn, size_t len, uint8_t chipSelect, bool resetChipSelect) {
	SpiExternalChipSelect(chipSelect, true);
	SpiGenericPolling(SPI2, dataOut, dataIn, len);
	if (resetChipSelect) {
		SpiExternalChipSelect(chipSelect, false);
	}
}

__weak void SpiExternalTransfer(const uint8_t * dataOut, uint8_t * dataIn, size_t len, uint8_t chipSelect, bool resetChipSelect) {
	SpiExternalTransferPolling(dataOut, dataIn, len, chipSelect, resetChipSelect);
}

void SpiExternalPrescaler(uint32_t prescaler) {
	SpiGenericPrescaler(SPI2, prescaler);
}
