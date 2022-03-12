/* Boxlib
(c) 2021 by Malte Marwedel

SPDX-License-Identifier: BSD-3-Clause
*/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "lcd.h"

#include "peripheral.h"


#include "main.h"

#include "spi.h"

#include "st7735/st7735.h"
#include "ili9341/ili9341.h"

bool g_LcdEnabled;
bool g_lcdPrescaler;
eDisplay_t g_LcdType;

//write to chip
void LcdCsOn(void) {
	if (g_LcdEnabled) {
		HAL_GPIO_WritePin(LcdCs_GPIO_Port, LcdCs_Pin, GPIO_PIN_RESET);
	}
}

//display will ignore the inputs
void LcdCsOff(void) {
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

void LcdEnable(uint32_t clockPrescaler) {
	g_lcdPrescaler = clockPrescaler;
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
	if (g_LcdType == ST7735) {
		LcdCsOn();
	}
	if (g_LcdEnabled) {
		HAL_SPI_Transmit(&hspi2, (uint8_t*)dataOut, len, 100);
	}
	if (g_LcdType == ST7735) {
		LcdCsOff();
	}
}

void LcdInit(eDisplay_t lcdType) {
	if (!g_LcdEnabled) {
		return;
	}
	g_LcdType = lcdType;
	PeripheralPrescaler(g_lcdPrescaler);
	if (g_LcdType == ST7735) {
		st7735_Init();
	}
	if (g_LcdType == ILI9341) {
		ili9341_Init();
	}
}

void LcdWritePixel(uint16_t x, uint16_t y, uint16_t color) {
	PeripheralPrescaler(g_lcdPrescaler);
	if (g_LcdType == ST7735) {
		st7735_WritePixel(x, y, color);
	}
	if (g_LcdType == ILI9341) {
		Ili9341WritePixel(x, y, color);
	}
}

void LcdDrawHLine(uint16_t color, uint16_t x, uint16_t y, uint16_t length) {
	PeripheralPrescaler(g_lcdPrescaler);
	if (g_LcdType == ST7735) {
		st7735_DrawHLine(color, x, y, length);
	}
	if (g_LcdType == ILI9341) {
		Ili9341DrawHLine(color, x, y, length);
	}
}

void LcdDrawVLine(uint16_t color, uint16_t x, uint16_t y, uint16_t length) {
	PeripheralPrescaler(g_lcdPrescaler);
	if (g_LcdType == ST7735) {
		st7735_DrawVLine(color, x, y, length);
	}
	if (g_LcdType == ILI9341) {
		Ili9341DrawVLine(color, x, y, length);
	}
}

//shows colored lines, a black square in the upper left and a box around it
void LcdTestpattern(void) {
	uint16_t height = 0;
	uint16_t width = 0;
	if (g_LcdType == ST7735) {
		height = st7735_GetLcdPixelHeight();
		width = st7735_GetLcdPixelWidth();
	}
	if (g_LcdType == ILI9341) {
		height = ili9341_GetLcdPixelHeight();
		width = ili9341_GetLcdPixelWidth();
	}
	for (uint16_t y = 0; y < height; y++) {
		uint16_t rgb = 1 << (y % 16);
		LcdDrawHLine(rgb, 0, y, width);
	}
	//black square marks the top left
	for (uint16_t y = 0; y < 48; y++) {
		LcdDrawHLine(0, 0, y, 48);
	}
	//next to it a red sqare
	for (uint16_t y = 0; y < 48; y++) {
		LcdDrawHLine(0xF800, 48, y, 16);
	}
	//a green one
	for (uint16_t y = 0; y < 48; y++) {
		LcdDrawHLine(0x07E0, 64, y, 16);
	}
	//a blue one
	for (uint16_t y = 0; y < 48; y++) {
		LcdDrawHLine(0x001F, 80, y, 16);
	}
	//	white-dark green border marks the outer layer
	LcdDrawHLine(0xFFFF, 0, 0, width);
	LcdDrawHLine(0xFFFF, 0, height - 1, width);
	LcdDrawVLine(0xFFFF, 0, 0, height);
	LcdDrawVLine(0xFFFF, width - 1, 0, height);
	LcdDrawHLine(0x07E0, 1, 1, width - 2);
	LcdDrawHLine(0x07E0, 1, height - 2, width - 2);
	LcdDrawVLine(0x07E0, 1, 1, height - 2);
	LcdDrawVLine(0x07E0, width - 2, 1, height - 2);
}

//Interface implemenation of st7735.c
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

//only used by ILI9341
void LcdCommandData(uint8_t command, const uint8_t * dataOut, uint8_t * dataIn, size_t len) {
	LcdCsOn();
	LCD_IO_WriteReg(command); //the command, leaves A0 on -> data next
	PeripheralTransfer(dataOut, dataIn, len);
	LcdCsOff();
}

void ili9341_WriteData(uint16_t data) {
	uint8_t bytes = data & 0xFF;
	LCD_IO_WriteMultipleData(&bytes, 1);
}

