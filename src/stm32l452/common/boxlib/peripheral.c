/* Boxlib
(c) 2021 by Malte Marwedel

SPDX-License-Identifier: BSD-3-Clause
*/

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "peripheral.h"

#include "main.h"

#include "lcd.h"
#include "flash.h"

SPI_HandleTypeDef g_hspi2;

void PeripheralBaseInit(void) {
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	__HAL_RCC_GPIOA_CLK_ENABLE(); //Backlight, power off pin
	__HAL_RCC_GPIOB_CLK_ENABLE(); //SPI2 pins
	__HAL_RCC_GPIOC_CLK_ENABLE(); //FlashCs, Lcd A0, LcdCs

	HAL_GPIO_WritePin(GPIOC, FlashCs_Pin | LcdA0_Pin | LcdCs_Pin, GPIO_PIN_RESET);

	HAL_GPIO_WritePin(LcdBacklight_GPIO_Port, LcdBacklight_Pin, GPIO_PIN_SET);

	GPIO_InitStruct.Pin = FlashCs_Pin | LcdA0_Pin | LcdCs_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = LcdBacklight_Pin;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	__HAL_RCC_SPI2_CLK_ENABLE();

	GPIO_InitStruct.Pin = PerSpiSck_Pin | PerSpiMosi_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = PerSpiMiso_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_PULLDOWN;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	//Copied from the STM cube generator output:
	g_hspi2.Instance = SPI2;
	g_hspi2.Init.Mode = SPI_MODE_MASTER;
	g_hspi2.Init.Direction = SPI_DIRECTION_2LINES;
	g_hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
	g_hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
	g_hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
	g_hspi2.Init.NSS = SPI_NSS_SOFT;
	//The scaler is of no real importance here, as it is set before every access anyway
	g_hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
	g_hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
	g_hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
	g_hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	g_hspi2.Init.CRCPolynomial = 7;
	g_hspi2.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
	g_hspi2.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
	HAL_SPI_Init(&g_hspi2);


}

__weak void PeripheralInit(void) {
	PeripheralBaseInit();
}

void PeripheralPowerOn(void) {
	//active low enables the LCD, external flash and RS232
	GPIO_InitTypeDef pinState = {0};
	pinState.Mode = GPIO_MODE_INPUT;
	pinState.Pull = GPIO_PULLDOWN;
	pinState.Pin = PeripheralNPower_Pin;
	HAL_GPIO_Init(PeripheralNPower_GPIO_Port, &pinState);
}

void PeripheralPowerOff(void) {
	LcdDisable();
	FlashDisable();

	__HAL_RCC_SPI2_CLK_DISABLE();

	//disables the LCD, external flash and RS232
	GPIO_InitTypeDef pinState = {0};
	pinState.Mode = GPIO_MODE_INPUT;
	pinState.Pull = GPIO_PULLUP;
	pinState.Pin = PeripheralNPower_Pin;
	HAL_GPIO_Init(PeripheralNPower_GPIO_Port, &pinState);
}

__weak void PeripheralTransfer(const uint8_t * dataOut, uint8_t * dataIn, size_t len) {
	if ((dataIn) && (dataOut)) {
		HAL_SPI_TransmitReceive(&g_hspi2, (uint8_t*)dataOut, dataIn, len, 100);
	} else if (dataOut) {
		HAL_SPI_Transmit(&g_hspi2, (uint8_t*)dataOut, len, 100);
	} else if (dataIn) {
		HAL_SPI_Receive(&g_hspi2 ,dataIn, len, 100);
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
	uint32_t reg = READ_REG(g_hspi2.Instance->CR1);
	reg &= ~SPI_CR1_BR_Msk;
	reg |= bits;
	WRITE_REG(g_hspi2.Instance->CR1, reg);
}

__weak void PeripheralTransferWaitDone(void) {

}

