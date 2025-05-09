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
#include "boxlib/spiGeneric.h"
#include "spiPlatform.h"

/*
Requirements:
1. Pins should be available on CN5, CN6, CN8 or CN9 connector for easy jumper settings
2. Pins should not collide with the on board nucleo button, LED and serial rx/tx
3. SPI2 and SPI3 only support 25MHz -> slows down the refresh rate

-> SPI5: Only SPI fulfilling most of the requirements (MISO on other header, but
   this is only needed if a flash is connected)
*/

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

	SpiPlatformInit(SPI5);
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

void PeripheralTransferPolling(const uint8_t * dataOut, uint8_t * dataIn, size_t len) {
	SpiGenericPolling(SPI5, dataOut, dataIn, len);
}

__weak void PeripheralTransfer(const uint8_t * dataOut, uint8_t * dataIn, size_t len) {
	PeripheralTransferPolling(dataOut, dataIn, len);
}

__weak void PeripheralTransferBackground(const uint8_t * dataOut, uint8_t * dataIn, size_t len) {
	PeripheralTransferPolling(dataOut, dataIn, len);
}

void PeripheralPrescaler(uint32_t prescaler) {
	SpiGenericPrescaler(SPI5, prescaler);
}

__weak void PeripheralTransferWaitDone(void) {
}

__weak void PeripheralLockMt(void) {
}

__weak void PeripheralUnlockMt(void) {
}


