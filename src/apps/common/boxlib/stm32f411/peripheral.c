/* Boxlib
(c) 2021 by Malte Marwedel

SPDX-License-Identifier: BSD-3-Clause
*/

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "boxlib/peripheral.h"

#include "main.h"

#include "boxlib/lcd.h"
#include "boxlib/flash.h"

/*
Requirements:
1. Pins should be available on CN5, CN6, CN8 or CN9 connector for easy jumper settings
2. Pins should not collide with the on board nucleo button, LED and serial rx/tx
3. SPI2 and SPI3 only support 25MHz -> slows down the refresh rate

-> SPI5: Only SPI fulfilling most of the requirements (MISO on other header, but
   this is only needed if a flash is connected)
*/

SPI_HandleTypeDef g_hspi5;

void PeripheralBaseInit(void) {
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	__HAL_RCC_GPIOA_CLK_ENABLE(); //SPI5 MISO, MOSI pins
	__HAL_RCC_GPIOB_CLK_ENABLE(); //Lcd A0, LcdCs, LcdReset, SPI CLK pin

	HAL_GPIO_WritePin(GPIOB, LcdA0_Pin | LcdCs_Pin, GPIO_PIN_RESET);

	GPIO_InitStruct.Pin = LcdA0_Pin | LcdCs_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	__HAL_RCC_SPI5_CLK_ENABLE();

	GPIO_InitStruct.Pin = PerSpiSck_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF6_SPI5;
	HAL_GPIO_Init(PerSpiSck_GPIO_Port, &GPIO_InitStruct);

	//MISO is not needed for the LCD

	GPIO_InitStruct.Pin = PerSpiMosi_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF6_SPI5;
	HAL_GPIO_Init(PerSpiMosi_GPIO_Port, &GPIO_InitStruct);

	//Copied from the STM cube generator output:
	g_hspi5.Instance = SPI5;
	g_hspi5.Init.Mode = SPI_MODE_MASTER;
	g_hspi5.Init.Direction = SPI_DIRECTION_2LINES;
	g_hspi5.Init.DataSize = SPI_DATASIZE_8BIT;
	g_hspi5.Init.CLKPolarity = SPI_POLARITY_LOW;
	g_hspi5.Init.CLKPhase = SPI_PHASE_1EDGE;
	g_hspi5.Init.NSS = SPI_NSS_SOFT;
	//The scaler is of no real importance here, as it is set before every access anyway
	g_hspi5.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
	g_hspi5.Init.FirstBit = SPI_FIRSTBIT_MSB;
	g_hspi5.Init.TIMode = SPI_TIMODE_DISABLE;
	g_hspi5.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	g_hspi5.Init.CRCPolynomial = 7;
	HAL_SPI_Init(&g_hspi5);
}

__weak void PeripheralInit(void) {
	PeripheralBaseInit();
}

void PeripheralPowerOn(void) {
}

void PeripheralPowerOff(void) {
	LcdDisable();
	FlashDisable();

	__HAL_RCC_SPI5_CLK_DISABLE();
}

__weak void PeripheralTransfer(const uint8_t * dataOut, uint8_t * dataIn, size_t len) {
	if ((dataIn) && (dataOut)) {
		HAL_SPI_TransmitReceive(&g_hspi5, (uint8_t*)dataOut, dataIn, len, 100);
	} else if (dataOut) {
		HAL_SPI_Transmit(&g_hspi5, (uint8_t*)dataOut, len, 100);
	} else if (dataIn) {
		HAL_SPI_Receive(&g_hspi5 ,dataIn, len, 100);
	}
}

__weak void PeripheralTransferBackground(const uint8_t * dataOut, uint8_t * dataIn, size_t len) {
	PeripheralTransfer(dataOut, dataIn, len);
}

void PeripheralPrescaler(uint32_t prescaler) {
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
	uint32_t reg = READ_REG(g_hspi5.Instance->CR1);
	reg &= ~SPI_CR1_BR_Msk;
	reg |= bits;
	WRITE_REG(g_hspi5.Instance->CR1, reg);
}

__weak void PeripheralTransferWaitDone(void) {
}

__weak void PeripheralLockMt(void) {
}

__weak void PeripheralUnlockMt(void) {
}


