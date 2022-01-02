/* Boxlib
(c) 2021 by Malte Marwedel

SPDX-License-Identifier: BSD-3-Clause
*/

#include <stdbool.h>
#include <stdint.h>

#include "lcd.h"

#include "peripheral.h"


#include "main.h"

#include "spi.h"

#include "st7735/st7735.h"

bool g_LcdEnabled;

//write to chip
static void LcdCsOn(void) {
	if (g_LcdEnabled) {
		HAL_GPIO_WritePin(LcdCs_GPIO_Port, LcdCs_Pin, GPIO_PIN_RESET);
	}
}

//display will ignore the inputs
static void LcdCsOff(void) {
	if (g_LcdEnabled) {
		HAL_GPIO_WritePin(LcdCs_GPIO_Port, LcdCs_Pin, GPIO_PIN_SET);
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
	st7735_Init();
}

void LcdWritePixel(uint16_t x, uint16_t y, uint16_t color) {
	st7735_WritePixel(x, y, color);
}

//shows colored lines, a black square in the upper left and a box around it
void LcdTestpattern(void) {
	uint16_t height = st7735_GetLcdPixelHeight();
	uint16_t width = st7735_GetLcdPixelWidth();
	for (uint16_t y = 0; y < height; y++) {
		uint16_t rgb = 1 << (y % 16);
		st7735_DrawHLine(rgb, 0, y, width);
	}
	//black square marks the top left
	for (uint16_t y = 0; y < 48; y++) {
		st7735_DrawHLine(0, 0, y, 48);
	}
	//white-dark green border marks the outer layer
	st7735_DrawHLine(0xFFFF, 0, 0, width);
	st7735_DrawHLine(0xFFFF, 0, height - 1, width);
	st7735_DrawVLine(0xFFFF, 0, 0, height);
	st7735_DrawVLine(0xFFFF, width - 1, 0, height);
	st7735_DrawHLine(0x07E0, 1, 1, width - 2);
	st7735_DrawHLine(0x07E0, 1, height - 2, width - 2);
	st7735_DrawVLine(0x07E0, 1, 1, height - 2);
	st7735_DrawVLine(0x07E0, width - 2, 1, height - 2);
}

//Interface implemenation of st7735.c:
void LCD_IO_Init(void) {
}

void LCD_IO_WriteMultipleData(uint8_t *pData, uint32_t Size) {
	LcdA0On();
	LcdTransfer(pData, Size);
}

void LCD_IO_WriteReg(uint8_t Reg) {
	LcdA0Off();
	LcdTransfer(&Reg, 1);
	LcdA0On();
}

void LCD_Delay(uint32_t delay) {
	HAL_Delay(delay);
}

