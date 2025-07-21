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

	__HAL_RCC_GPIOA_CLK_ENABLE(); //for two CS pins
	__HAL_RCC_GPIOB_CLK_ENABLE(); //SPI1 pins

	HAL_GPIO_WritePin(Extern12_GPIO_Port, Extern12_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(Extern13_GPIO_Port, Extern13_Pin, GPIO_PIN_SET);

	GPIO_InitStruct.Pin = Extern12_Pin | Extern13_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH; //up to 50MHz
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	__HAL_RCC_SPI1_CLK_ENABLE();

	GPIO_InitStruct.Pin = Extern8_Pin | Extern9_Pin | Extern10_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	SpiPlatformInit(SPI1);
}

void SpiExternalBaseDeinit(void) {
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	GPIO_InitStruct.Pin = Extern12_Pin | Extern13_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = Extern8_Pin | Extern9_Pin | Extern10_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	__HAL_RCC_SPI1_CLK_DISABLE();
}

__weak void SpiExternalInit(void) {
	SpiExternalBaseInit();
}

__weak void SpiExternalDeinit(void) {
	SpiExternalBaseDeinit();
}


void SpiExternalChipSelect(uint8_t chipSelect, bool selected) {
	GPIO_PinState state = GPIO_PIN_SET;
	if (selected) {
		state = GPIO_PIN_RESET;
	}
	if (chipSelect == 1) { //PA1
		HAL_GPIO_WritePin(Extern13_GPIO_Port, Extern13_Pin, state);
	}
	if (chipSelect == 2) { //PA2
		HAL_GPIO_WritePin(Extern12_GPIO_Port, Extern12_Pin, state);
	}
}

void SpiExternalTransferPolling(const uint8_t * dataOut, uint8_t * dataIn, size_t len, uint8_t chipSelect, bool resetChipSelect) {
	SpiExternalChipSelect(chipSelect, true);
	SpiGenericPolling(SPI1, dataOut, dataIn, len);
	if (resetChipSelect) {
		SpiExternalChipSelect(chipSelect, false);
	}
}

__weak void SpiExternalTransfer(const uint8_t * dataOut, uint8_t * dataIn, size_t len, uint8_t chipSelect, bool resetChipSelect) {
	SpiExternalTransferPolling(dataOut, dataIn, len, chipSelect, resetChipSelect);
}

void SpiExternalPrescaler(uint32_t prescaler) {
	SpiGenericPrescaler(SPI1, prescaler);
}
