/* Boxlib
(c) 2021, 2025 by Malte Marwedel

SPDX-License-Identifier: BSD-3-Clause
*/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "boxlib/lcd.h"

#include "boxlib/peripheral.h"

#include "main.h"

/*If the ST LCD is not used, dummy weak functions are there. This will
  save program memory in apps which do not support the LCD anyway.
  For some reasons, the driver for the ILI9341 is much smaller than the one for
  the ST7735. And the ILI9341 looks way better.
  For completeness, there are dummy functions for the ILI9341 too, so it could
  be disabled if only a ST7735 is used.
*/
#include "st7735/st7735.h"
#include "ili9341/ili9341.h"

bool g_lcdEnabled;

/*This variable makes sure multiple threads can share the same SPI, but it
is not enough to allow multiple threads to use the Lcd* functions.
*/
bool g_lcdTransferStarted;

uint16_t g_lcdPrescaler;


eDisplay_t g_lcdType;

//ST7735 weak functions

__attribute__((weak)) void st7735_Init(uint8_t width) {
	(void)width;
}

__attribute__((weak)) void st7735_WritePixel(uint16_t Xpos, uint16_t Ypos, uint16_t RGBCode) {
	(void)Xpos;
	(void)Ypos;
	(void)RGBCode;
}

__attribute__((weak)) void st7735_SetDisplayWindow(uint16_t Xpos, uint16_t Ypos, uint16_t Width, uint16_t Height) {
	(void)Xpos;
	(void)Ypos;
	(void)Width;
	(void)Height;
}

__attribute__((weak)) void st7735_DrawHLine(uint16_t RGBCode, uint16_t Xpos, uint16_t Ypos, uint16_t Length) {
	(void)RGBCode;
	(void)Xpos;
	(void)Ypos;
	(void)Length;
}

__attribute__((weak)) void st7735_DrawVLine(uint16_t RGBCode, uint16_t Xpos, uint16_t Ypos, uint16_t Length) {
	(void)RGBCode;
	(void)Xpos;
	(void)Ypos;
	(void)Length;
}

__attribute__((weak)) uint16_t st7735_GetLcdPixelWidth(void) {
	return 0;
}

__attribute__((weak)) uint16_t st7735_GetLcdPixelHeight(void) {
	return 0;
}

__attribute__((weak)) void st7735WriteArray(const uint8_t * data, size_t len) {
	(void)data;
	(void)len;
}

//ILI9341 weak functions

__attribute__((weak)) void ili9341_Init(void) {
}

__attribute__((weak)) uint16_t ili9341_GetLcdPixelWidth(void) {
	return 0;
}

__attribute__((weak)) uint16_t ili9341_GetLcdPixelHeight(void) {
	return 0;
}

__attribute__((weak)) void Ili9341SetWindowStart(uint16_t xStart, uint16_t xEnd, uint16_t yStart, uint16_t yEnd) {
	(void)xStart;
	(void)xEnd;
	(void)yStart;
	(void)yEnd;
}

__attribute__((weak)) void Ili9341WritePixel(uint16_t x, uint16_t y, uint16_t color) {
	(void)x;
	(void)y;
	(void)color;
}

__attribute__((weak)) void Ili9341WriteArray(const uint8_t * colors, uint16_t length) {
	(void)colors;
	(void)length;
}

//write to chip
void LcdCsOn(void) {
	if (g_lcdEnabled) {
		HAL_GPIO_WritePin(LcdCs_GPIO_Port, LcdCs_Pin, GPIO_PIN_RESET);
	}
}

//display will ignore the inputs
void LcdCsOff(void) {
	if (g_lcdEnabled) {
		HAL_GPIO_WritePin(LcdCs_GPIO_Port, LcdCs_Pin, GPIO_PIN_SET);
	}
}

#ifndef LcdReset_GPIO_Port
#define LcdReset_GPIO_Port GPIOC
#define LcdReset_Pin GPIO_PIN_2
#endif

void LcdResetOff(void) {
	HAL_GPIO_WritePin(LcdReset_GPIO_Port, LcdReset_Pin, GPIO_PIN_SET);
}

void LcdResetOn(void) {
	HAL_GPIO_WritePin(LcdReset_GPIO_Port, LcdReset_Pin, GPIO_PIN_RESET);
}

static void LcdA0On(void) {
	if (g_lcdEnabled) {
		HAL_GPIO_WritePin(LcdA0_GPIO_Port, LcdA0_Pin, GPIO_PIN_SET);
	}
}

static void LcdA0Off(void) {
	if (g_lcdEnabled) {
		HAL_GPIO_WritePin(LcdA0_GPIO_Port, LcdA0_Pin, GPIO_PIN_RESET);
	}
}

void LcdEnable(uint32_t clockPrescaler) {
	g_lcdPrescaler = clockPrescaler;

	PeripheralPowerOn();

	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = LcdReset_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(LcdReset_GPIO_Port, &GPIO_InitStruct);

	LcdResetOn();
	g_lcdEnabled = true;
	LcdCsOff();
	HAL_Delay(50);
	LcdResetOff();
	HAL_Delay(50);
}

void LcdDisable(void) {
	HAL_GPIO_WritePin(LcdReset_GPIO_Port, LcdReset_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LcdCs_GPIO_Port, LcdCs_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LcdA0_GPIO_Port, LcdA0_Pin, GPIO_PIN_RESET);
	g_lcdEnabled = false;
}

void LcdBacklightOn(void) {
	HAL_GPIO_WritePin(LcdBacklight_GPIO_Port, LcdBacklight_Pin, GPIO_PIN_RESET);
}

void LcdBacklightOff(void) {
	HAL_GPIO_WritePin(LcdBacklight_GPIO_Port, LcdBacklight_Pin, GPIO_PIN_SET);
}

void LcdInit(eDisplay_t lcdType) {
	if (!g_lcdEnabled) {
		return;
	}
	PeripheralLockMt();
	g_lcdType = lcdType;
	PeripheralPrescaler(g_lcdPrescaler);
	if (g_lcdType == ST7735_128) {
		st7735_Init(128);
	}
	if (g_lcdType == ST7735_160) {
		st7735_Init(160);
	}
	if (g_lcdType == ILI9341) {
		ili9341_Init();
	}
	PeripheralUnlockMt();
}

void LcdDelay(uint32_t delay) {
	HAL_Delay(delay);
}

void LcdWriteMultipleDataBackground(const uint8_t *pData, size_t len) {
	LcdA0On();
	PeripheralTransferBackground(pData, NULL, len);
}

void LcdWriteMultipleData(const uint8_t * dataOut, size_t len) {
	LcdA0On();
	PeripheralTransfer(dataOut, NULL, len);
}

void LcdWriteReg(uint8_t Reg) {
	LcdA0Off();
	PeripheralTransfer(&Reg, NULL, 1);
	LcdA0On();
}

void LcdCommand(uint8_t command) {
	LcdCsOn();
	LcdWriteReg(command); //the command, leaves A0 on -> data next
	LcdCsOff();
}

void LcdData(const uint8_t * dataOut, size_t len) {
	LcdCsOn();
	PeripheralTransfer(dataOut, NULL, len);
	LcdCsOff();
}

void LcdCommandData(uint8_t command, const uint8_t * dataOut, uint8_t * dataIn, size_t len) {
	LcdCsOn();
	LcdWriteReg(command); //the command, leaves A0 on -> data next
	PeripheralTransfer(dataOut, dataIn, len);
	LcdCsOff();
}

void LcdCommandDataBackground(uint8_t command, const uint8_t * dataOut, uint8_t * dataIn, size_t len) {
	LcdWaitBackgroundDone();
	LcdCsOn();
	LcdWriteReg(command); //the command, leaves A0 on -> data next
	PeripheralTransferBackground(dataOut, dataIn, len);
}

void LcdWaitBackgroundDone(void) {
	PeripheralTransferWaitDone();
	LcdCsOff();
}

void LcdWaitBackgroundDoneRelease(void) {
	LcdWaitBackgroundDone();
	if (g_lcdTransferStarted) {
		PeripheralUnlockMt();
		g_lcdTransferStarted = false;
	}
}

void LcdWritePixel(uint16_t x, uint16_t y, uint16_t color) {
	PeripheralLockMt();
	PeripheralPrescaler(g_lcdPrescaler);
	if ((g_lcdType == ST7735_128) || (g_lcdType == ST7735_160)) {
		st7735_WritePixel(x, y, color);
	}
	if (g_lcdType == ILI9341) {
		Ili9341WritePixel(x, y, color);
	}
	PeripheralUnlockMt();
}

void LcdWriteRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint8_t * data, size_t len) {
	if (!g_lcdEnabled) {
		return;
	}
	LcdWaitBackgroundDoneRelease();
	PeripheralLockMt();
	g_lcdTransferStarted = true;
	PeripheralPrescaler(g_lcdPrescaler);
	if ((g_lcdType == ST7735_128) || (g_lcdType == ST7735_160)) {
		uint16_t h = st7735_GetLcdPixelHeight();
		uint16_t w = st7735_GetLcdPixelWidth();
		if (((x + width) <= w) && ((y + height) <= h)) {
			st7735_SetDisplayWindow(x, y, width, height);
			st7735WriteArray(data, len);
		}
	} else if (g_lcdType == ILI9341) {
		uint16_t h = ili9341_GetLcdPixelHeight();
		uint16_t w = ili9341_GetLcdPixelWidth();
		if (((x + width) <= w) && ((y + height) <= h)) {
			Ili9341SetWindowStart(x, x + width - 1, y, y + height);
			Ili9341WriteArray(data, len);
		}
	}
}

void LcdDrawHLine(uint16_t color, uint16_t x, uint16_t y, uint16_t length) {
	PeripheralLockMt();
	PeripheralPrescaler(g_lcdPrescaler);
	if ((g_lcdType == ST7735_128) || (g_lcdType == ST7735_160)) {
		st7735_DrawHLine(color, x, y, length);
	}
	if (g_lcdType == ILI9341) {
		Ili9341DrawHLine(color, x, y, length);
	}
	PeripheralUnlockMt();
}

void LcdDrawVLine(uint16_t color, uint16_t x, uint16_t y, uint16_t length) {
	PeripheralLockMt();
	PeripheralPrescaler(g_lcdPrescaler);
	if ((g_lcdType == ST7735_128) || (g_lcdType == ST7735_160)) {
		st7735_DrawVLine(color, x, y, length);
	}
	if (g_lcdType == ILI9341) {
		Ili9341DrawVLine(color, x, y, length);
	}
	PeripheralUnlockMt();
}

//shows colored lines, a black square in the upper left and a box around it
void LcdTestpattern(void) {
	uint16_t height = 0;
	uint16_t width = 0;
	if ((g_lcdType == ST7735_128) || (g_lcdType == ST7735_160)) {
		height = st7735_GetLcdPixelHeight();
		width = st7735_GetLcdPixelWidth();
	}
	if (g_lcdType == ILI9341) {
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
	//fading bars
	for (uint16_t i = 0; i < 64; i++) {
		uint16_t r = (i >> 1) << 11; //5bit red
		uint16_t g = (i >> 0) << 5; //6bit green
		uint16_t b = (i >> 1); //5bit red
		LcdDrawVLine(r, 2 + i, 48, 4);
		LcdDrawVLine(g, 2 + i, 52, 4);
		LcdDrawVLine(b, 2 + i, 56, 4);
	}
	//white-dark green border marks the outer layer
	LcdDrawHLine(0xFFFF, 0, 0, width);
	LcdDrawHLine(0xFFFF, 0, height - 1, width);
	LcdDrawVLine(0xFFFF, 0, 0, height);
	LcdDrawVLine(0xFFFF, width - 1, 0, height);
	LcdDrawHLine(0x07E0, 1, 1, width - 2);
	LcdDrawHLine(0x07E0, 1, height - 2, width - 2);
	LcdDrawVLine(0x07E0, 1, 1, height - 2);
	LcdDrawVLine(0x07E0, width - 2, 1, height - 2);
}

