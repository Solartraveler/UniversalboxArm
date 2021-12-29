/* Boxlib
(c) 2021 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause
*/

#include <stdbool.h>
#include <stdint.h>

#include "lcd.h"

#include "peripheral.h"


#include "main.h"

#include "spi.h"

bool g_LcdEnabled;

static void LcdCsOn(void) {
	if (g_LcdEnabled) {
		HAL_GPIO_WritePin(LcdCs_GPIO_Port, LcdCs_Pin, GPIO_PIN_SET);
	}
}

static void LcdCsOff(void) {
	if (g_LcdEnabled) {
		HAL_GPIO_WritePin(LcdCs_GPIO_Port, LcdCs_Pin, GPIO_PIN_RESET);
	}
}

static void LcdA0On(void) {
	if (g_LcdEnabled) {
		HAL_GPIO_WritePin(LcdA0_GPIO_Port, LcdA0_Pin, GPIO_PIN_SET);
	}
}

static void LcdA0Off(void) {
	if (g_LcdEnabled) {
		HAL_GPIO_WritePin(LcdA0_GPIO_Port, LcdA0_Pin, GPIO_PIN_RESET);
	}
}

void LcdEnable(void) {
	PeripheralPowerOn();
	g_LcdEnabled = true;
	LcdCsOff();
}

void LcdDisable(void) {
	HAL_GPIO_WritePin(LcdCs_GPIO_Port, LcdCs_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LcdA0_GPIO_Port, LcdA0_Pin, GPIO_PIN_RESET);
	g_LcdEnabled = false;
}

void LcdBacklightOn(void) {
	HAL_GPIO_WritePin(LcdBacklight_GPIO_Port, LcdBacklight_Pin, GPIO_PIN_RESET);
}

void LcdBacklightOff(void) {
	HAL_GPIO_WritePin(LcdBacklight_GPIO_Port, LcdBacklight_Pin, GPIO_PIN_SET);
}

static void LcdTransfer(const uint8_t * dataOut, size_t len) {
	LcdCsOn();
	if (g_LcdEnabled) {
		HAL_SPI_Transmit(&hspi2, (uint8_t*)dataOut, len, 100);
	}
	LcdCsOff();
}

void LcdInit(void) {


}

void LcdTestpattern(void) {

}


