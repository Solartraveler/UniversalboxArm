/* Boxlib
(c) 2021 by Malte Marwedel

SPDX-License-Identifier: BSD-3-Clause
*/

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "boxlib/spiExternal.h"

#include "main.h"

SPI_HandleTypeDef g_hspiExternal;

void SpiExternalBaseInit(void) {
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	__HAL_RCC_GPIOA_CLK_ENABLE(); //for two CS pins
	__HAL_RCC_GPIOB_CLK_ENABLE(); //SPI1 pins

	HAL_GPIO_WritePin(Extern12_GPIO_Port, Extern12_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(Extern13_GPIO_Port, Extern13_Pin, GPIO_PIN_SET);

	GPIO_InitStruct.Pin = Extern12_Pin | Extern13_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	__HAL_RCC_SPI1_CLK_ENABLE();

	GPIO_InitStruct.Pin = Extern8_Pin | Extern9_Pin | Extern10_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	//Copied from the STM cube generator output:
	g_hspiExternal.Instance = SPI1;
	g_hspiExternal.Init.Mode = SPI_MODE_MASTER;
	g_hspiExternal.Init.Direction = SPI_DIRECTION_2LINES;
	g_hspiExternal.Init.DataSize = SPI_DATASIZE_8BIT;
	g_hspiExternal.Init.CLKPolarity = SPI_POLARITY_LOW;
	g_hspiExternal.Init.CLKPhase = SPI_PHASE_1EDGE;
	g_hspiExternal.Init.NSS = SPI_NSS_SOFT;
	//The scaler is of no real importance here, as it should be set by SpiExternalPrescaler at least once
	g_hspiExternal.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
	g_hspiExternal.Init.FirstBit = SPI_FIRSTBIT_MSB;
	g_hspiExternal.Init.TIMode = SPI_TIMODE_DISABLE;
	g_hspiExternal.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	g_hspiExternal.Init.CRCPolynomial = 7;
	g_hspiExternal.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
	g_hspiExternal.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
	HAL_SPI_Init(&g_hspiExternal);
}

__weak void SpiExternalInit(void) {
	SpiExternalBaseInit();
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

__weak void SpiExternalTransfer(const uint8_t * dataOut, uint8_t * dataIn, size_t len, uint8_t chipSelect, bool resetChipSelect) {
	SpiExternalChipSelect(chipSelect, true);
	if ((dataIn) && (dataOut)) {
		HAL_SPI_TransmitReceive(&g_hspiExternal, (uint8_t*)dataOut, dataIn, len, 100);
	} else if (dataOut) {
		HAL_SPI_Transmit(&g_hspiExternal, (uint8_t*)dataOut, len, 100);
	} else if (dataIn) {
		HAL_SPI_Receive(&g_hspiExternal ,dataIn, len, 100);
	}
	if (resetChipSelect) {
		SpiExternalChipSelect(chipSelect, false);
	}
}

void SpiExternalPrescaler(uint32_t prescaler) {
	uint32_t bits;
	if (prescaler <= 2) {
		bits = SPI_BAUDRATEPRESCALER_2;
	} else if (prescaler <= 4) {
		bits = SPI_BAUDRATEPRESCALER_4;
	} else if (prescaler <= 8) {
		bits = SPI_BAUDRATEPRESCALER_8;
	} else if (prescaler <= 16) {
		bits = SPI_BAUDRATEPRESCALER_16;
	} else if (prescaler <= 32) {
		bits = SPI_BAUDRATEPRESCALER_32;
	} else if (prescaler <= 64) {
		bits = SPI_BAUDRATEPRESCALER_64;
	} else if (prescaler <= 128) {
		bits = SPI_BAUDRATEPRESCALER_128;
	} else {
		bits = SPI_BAUDRATEPRESCALER_256;
	}
	uint32_t reg = READ_REG(g_hspiExternal.Instance->CR1);
	reg &= ~SPI_CR1_BR_Msk;
	reg |= bits;
	WRITE_REG(g_hspiExternal.Instance->CR1, reg);
}


