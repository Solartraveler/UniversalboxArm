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

#include "spi.h"

SPI_TypeDef * g_spi = (SPI_TypeDef *)SPI2_BASE;

__weak void PeripheralInit(void) {
}

void PeripheralPowerOn(void) {
	//active low enables the LCD, external flash and RS232
	GPIO_InitTypeDef pinState = {0};
	pinState.Mode = GPIO_MODE_INPUT;
	pinState.Pull = GPIO_PULLDOWN;
	pinState.Pin = PeripheralNPower_Pin;
	HAL_GPIO_Init(PeripheralNPower_GPIO_Port, &pinState);
	HAL_SPI_MspInit(&hspi2);
}

void PeripheralPowerOff(void) {
	LcdDisable();
	FlashDisable();

	HAL_SPI_MspDeInit(&hspi2);

	//disables the LCD, external flash and RS232
	GPIO_InitTypeDef pinState = {0};
	pinState.Mode = GPIO_MODE_INPUT;
	pinState.Pull = GPIO_PULLUP;
	pinState.Pin = PeripheralNPower_Pin;
	HAL_GPIO_Init(PeripheralNPower_GPIO_Port, &pinState);
}

__weak void PeripheralTransfer(const uint8_t * dataOut, uint8_t * dataIn, size_t len) {
	if ((dataIn) && (dataOut)) {
		HAL_SPI_TransmitReceive(&hspi2, (uint8_t*)dataOut, dataIn, len, 100);
	} else if (dataOut) {
		HAL_SPI_Transmit(&hspi2, (uint8_t*)dataOut, len, 100);
	} else if (dataIn) {
		HAL_SPI_Receive(&hspi2 ,dataIn, len, 100);
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
	uint32_t reg = READ_REG(hspi2.Instance->CR1);
	reg &= ~SPI_CR1_BR_Msk;
	reg |= bits;
	WRITE_REG(hspi2.Instance->CR1, reg);
}

__weak void PeripheralTransferWaitDone(void) {

}

