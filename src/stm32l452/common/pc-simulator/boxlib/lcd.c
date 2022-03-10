/* Boxlib emulation
(c) 2022 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause
*/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "lcd.h"

#include "peripheral.h"
#include "main.h"

bool g_LcdEnabled;
eDisplay_t g_LcdType;
uint32_t g_LcdWidth;
uint32_t g_LcdHeight;

void LcdEnable(void) {
	PeripheralPowerOn();
	g_LcdEnabled = true;
}

void LcdDisable(void) {
	g_LcdEnabled = false;
}

void LcdBacklightOn(void) {
}

void LcdBacklightOff(void) {
}

void LcdInit(eDisplay_t lcdType) {
	g_LcdType = lcdType;
	if (g_LcdType == ST7735) {
		g_LcdWidth = 128;
		g_LcdHeight = 128;
	}
	if (g_LcdType == ILI9341) {
		g_LcdWidth = 320;
		g_LcdHeight = 240;
	}
	//TODO: Init
}

void LcdWritePixel(uint16_t x, uint16_t y, uint16_t color) {
	//TODO
}

void LcdDrawHLine(uint16_t color, uint16_t x, uint16_t y, uint16_t length) {
	for (uint32_t i = 0; i < length; i++) {
		LcdWritePixel(x + i, y, color);
	}
}

void LcdDrawVLine(uint16_t color, uint16_t x, uint16_t y, uint16_t length) {
	for (uint32_t i = 0; i < length; i++) {
		LcdWritePixel(x, y + i, color);
	}
}

//shows colored lines, a black square in the upper left and a box around it
void LcdTestpattern(void) {
	uint16_t height = g_LcdHeight;
	uint16_t width = g_LcdWidth;
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

