/* Boxlib
(c) 2021 by Malte Marwedel

SPDX-License-Identifier: BSD-3-Clause
*/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>

#include "boxlib/peripheral.h"

#include "main.h"

#include "boxlib/lcd.h"
#include "boxlib/flash.h"
#include "boxlib/spiGeneric.h"
#include "spiPlatform.h"

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
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	SpiPlatformInit(SPI2);
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

void PeripheralTransferPolling(const uint8_t * dataOut, uint8_t * dataIn, size_t len) {
	SpiGenericPolling(SPI2, dataOut, dataIn, len);
}

__weak void PeripheralTransfer(const uint8_t * dataOut, uint8_t * dataIn, size_t len) {
	PeripheralTransferPolling(dataOut, dataIn, len);
}

__weak void PeripheralTransferBackground(const uint8_t * dataOut, uint8_t * dataIn, size_t len) {
	PeripheralTransferPolling(dataOut, dataIn, len);
}

void PeripheralPrescaler(uint32_t prescaler) {
	SpiGenericPrescaler(SPI2, prescaler);
}

__weak void PeripheralTransferWaitDone(void) {
}

__weak void PeripheralLockMt(void) {
}

__weak void PeripheralUnlockMt(void) {
}


