/* Boxlib
(c) 2021 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause
*/

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "peripheral.h"

#include "main.h"

#include "lcd.h"
#include "flash.h"

#include "spi.h"

void PeripheralPowerOn(void) {
	//active low enables the LCD, external flash and RS232
	GPIO_InitTypeDef state = {0};
	state.Mode = GPIO_MODE_INPUT;
	state.Pull = GPIO_PULLDOWN;
	state.Pin = PeripheralNPower_Pin;
	HAL_GPIO_Init(PeripheralNPower_GPIO_Port, &state);
}

void PeripheralPowerOff(void) {
	LcdDisable();
	FlashDisable();
	//disables the LCD, external flash and RS232
	GPIO_InitTypeDef state = {0};
	state.Mode = GPIO_MODE_INPUT;
	state.Pull = GPIO_PULLUP;
	state.Pin = PeripheralNPower_Pin;
	HAL_GPIO_Init(PeripheralNPower_GPIO_Port, &state);
}

void PeripheralTransfer(const uint8_t * dataOut, uint8_t * dataIn, size_t len) {
	if ((dataIn) && (dataOut)) {
		HAL_SPI_TransmitReceive(&hspi2, (uint8_t*)dataOut, dataIn, len, 100);
	} else if (dataOut) {
		HAL_SPI_Transmit(&hspi2, (uint8_t*)dataOut, len, 100);
	} else if (dataIn) {
		HAL_SPI_Receive(&hspi2 ,dataIn, len, 100);
	}
}
