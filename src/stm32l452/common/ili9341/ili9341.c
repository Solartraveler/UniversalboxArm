/*
(c) 2022 by Malte Marwedel

License: BSD-3-Clause
*/

#include <stdint.h>
#include <stddef.h>

#include "ili9341.h"

uint16_t ili9341_GetLcdPixelHeight(void) {
	return 240;
}

uint16_t ili9341_GetLcdPixelWidth(void) {
	return 320;
}

#define LcdSend(A, B, C) LcdCommandData(A, B, C, sizeof(B))

/* The application note gives some hints how to init the LCD:
  https://web.archive.org/web/20180508132839if_/http://www.wdflcd.com:80/xz/ILI9341_AN_V0.6_20110311.pdf
  Otherwise, the init sequence is a modified version copied from:
  https://vivonomicon.com/2018/06/17/drawing-to-a-small-tft-display-the-ili9341-and-stm32/

  TODO: Carefully remove commands which are just copied but turn out to be useless.
*/
void ili9341_Init(void) {

	const uint8_t a0[] = {};
	LcdSend(LCD_SWRESET, a0, NULL);

	LCD_Delay(6); //required time according to the datasheet is 5ms

	/* Nobody knows what the command 0xEF does. Just everybody is using 0xEF
	   or 0xCA. And everyone copying the working sequences from somewhere.
	   See guessing:
	   https://forums.adafruit.com/viewtopic.php?f=47&t=69805
	   But the init sequences from the application note don't use them.
	   Tests show this can be removed, so we disable it here.
	*/
	//const uint8_t a1[] = {0x03, 0x80, 0x02};
	//LcdSend(0xEF, a1, NULL);

	/* Compared to the default values, PCEQ for power saving is enabled.
	   Not really good documented at all.
	*/
	const uint8_t a2[] = {0x00, 0xC1, 0x30};
	LcdSend(LCD_POWERB, a2, NULL);

	/* Data equal to ST driver and application note.
	   Some start timings. Not much documented.
	*/
	const uint8_t a3[] = {0x64, 0x03, 0x12, 0x81};
	LcdSend(LCD_POWER_SEQ, a3, NULL);

	/* Compared to the default values:
	   Enables NOW, disable EQ, CR and sets precharging to two units.
	   The values in the application note are the same.
	*/
	const uint8_t a4[] = {0x85, 0x00, 0x78};
	LcdSend(LCD_DTCA, a4, NULL);

	/* Data are the same as the default values in the datasheet.
	   First three bytes seem just constants.
	   4. byte 0x4 -> 1.6V Vcore
	   5. byte DDVH = 5.6V
	*/
	const uint8_t a5[] = {0x39, 0x2C, 0x00, 0x34, 0x02};
	LcdSend(LCD_POWERA, a5, NULL);

	/* Pump ratio control.
	   0x20 represents DDVH = 2*VCI.
	   Value seems to be the default value.
	*/
	const uint8_t a6[] = {0x20};
	LcdSend(LCD_PRC, a6, NULL);

	/* Default values: 0x66 0x00
	   ST: Does not changes these values.
	   The values in the application note are the same.
	   Sets some timings from 1 to 0 units.
	*/
	const uint8_t a7[] = {0x00, 0x00};
	LcdSend(LCD_DTCB, a7, NULL);

	/* Reference GVDD (grayscale) level
	   Default 0x21 -> 4.5V
	   ST uses 0x10 -> 3.56V
	   0x23 -> 4.6V
	*/
	const uint8_t a8[] = {0x23};
	LcdSend(LCD_POWER1, a8, NULL);

	/* Default value is 0x10 too. The 0x10 bit is undocumented.
	   The lower bits correspond to DDVH = VCI * 2, VGH = VCI * 7, VGL = -VCI*4
	   Limits: DDVH must be <= 5.8V
	           VGH - VGHL <= 28V
	   ->VCI may only be a maximum of 2.54V
	*/
	const uint8_t a9[] = {0x10};
	LcdSend(LCD_POWER2, a9, NULL);

	/* Default: 0x31 0x37
	   ST uses: 0x45 0x15
	   0x3E -> VCOMH = 4.25V
	   0x28 -> VCOML = -1.5V
	   Values are only used if nVM is 0.
	   Propably this init command can be removed.
	*/
	const uint8_t a10[] = {0x3E, 0x28};
	LcdSend(LCD_VCOM1, a10, NULL);

	/* Default 0xC0
	   ST uses: 0x90
	   0x86 -> nVM = 1 -> VCOMH = VMH - 52, VCOML = VML - 52
	   The meaning of nVM is unclear in the datasheet. Because its declared
	   disabled by default. But nVM = 1 is the default value and this is described
	   as enabled.
	*/
	const uint8_t a11[] = {0x86};
	LcdSend(LCD_VCOM2, a11, NULL);

	/* Default after reset: 0x0
	   0x48: BGR color filter panel (0x8) + mirror X (0x40)
	   Other init sequence:
	   0xC8: BGR color filter panel (0x8) + mirror X (0x40) + mirror Y (0x80)
	   0xE8: BGR color filter panel (0x8) + mirror X (0x40) + mirror X (0x80) + swap X-Y (0x20)
	*/
	const uint8_t a12[] = {0x28};
	LcdSend(LCD_MAC, a12, NULL);

	/* Default after reset: 0x66 -> RGB and MCU interface uses 18Bit
	0x55: Both using 16it
	*/
	const uint8_t a14[] = {0x55};
	LcdSend(LCD_PIXEL_FORMAT, a14, NULL);

	/* Default value: 0x0, 0x1B
	Framerate = fosc / (clocks per line * division ratio * (lines + VBP + VFP))
	fosc = 600kHz
	1. 0x0 -> no divider for internal clock
	2. 0x1B -> 27 clocks per line. 0x18 -> 24 clocks per line

	VBP and VFP are not set, so they have their reset values:
	VBP = 0x02
	VFP = 0x02
	*/
	const uint8_t a15[] = {0x00, 0x18};
	LcdSend(LCD_FRMCTR1, a15, NULL);

	/* Needs 4 parameters, but this sequence only has 3 bytes. ST uses 4.
	 1. Byte: PTG (default 2) + PT (default 2) = 0
      PTG -> Scan mode in non display area to interval mode
	    PT -> Scan mode in partial display mode -> unimportant
	 2. Byte: REV (default 1) + GS (default 0) + SS (default 0)+ SM (default 0) + ISC (default 2)
	    0x82 is just the default value.
	 3. Byte: NL (default 0x27) = drive 320lines of the LCD
	 4. Byte: PCDIV (no default given)
	-> Most likely, this command can be removed
	*/
	const uint8_t a16[] = {0x08, 0x82, 0x27};
	LcdSend(LCD_DFC, a16, NULL);

	/* 3 gamma control disabled. Represents the default value after reset.
	   No further documentation present.
	   -> Most likely, this command can be removed
	   Tested: Works without this line, so disable it
	*/
	//const uint8_t a17[] = {0x00};
	//LcdSend(LCD_3GAMMA_EN, a17, NULL);

	/* Select gamma 2.2 curve. Up to 4 are available, but the other three are undefined
	*/
	const uint8_t a18[] = {0x01}; //eq
	LcdSend(LCD_GAMMA, a18, NULL);

	/* Settings are a copy from ILI9341 application note ILI9341_LG2.6_Initial
	*/
	const uint8_t a19[] = {0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00}; //diff
	LcdSend(LCD_PGAMMA, a19, NULL);

	/* Settings are a copy from ILI9341 application note ILI9341_LG2.6_Initial
	*/
	const uint8_t a20[] = {0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F}; //diff
	LcdSend(LCD_NGAMMA, a20, NULL);

	/* After reset, the display is in sleep mode. So this is a required command.
	*/
	const uint8_t a21[] = {};
	LcdSend(LCD_SLEEP_OUT, a21, NULL);

	/* Only 5ms according to the datasheet required, tested with 5ms and it works.
	   now using 10ms, to have some safety margin
	*/
	LCD_Delay(10);

	/* Just enables the display. Does nothing if already on. Default after reset
	   is off.
	*/
	const uint8_t a22[] = {};
	LcdSend(LCD_DISPLAY_ON, a22, NULL);

	//As compared to partial mode, in normal mode the whole screen is driven.
	//Should already be set after reset, so this is propably unneccessary.
	const uint8_t a23[] = {};
	LcdSend(LCD_NORMAL_MODE_ON, a23, NULL);

}

void Ili9341SetWindowStart(uint16_t xStart, uint16_t xEnd, uint16_t yStart, uint16_t yEnd) {
	//x
	static uint16_t xStartLast = 0xFFFF;
	static uint16_t xEndLast = 0xFFFF;
	if ((xStartLast != xStart) || (xEndLast != xEnd)) {
		uint8_t ax[4];
		ax[0] = xStart >> 8;
		ax[1] = xStart & 0xFF;
		ax[2] = xEnd >> 8;
		ax[3] = xEnd & 0xFF;
		LcdSend(LCD_COLUMN_ADDR, ax, NULL);
		xStartLast = xStart;
		xEndLast = xEnd;
	}
	//y
	static uint16_t yStartLast = 0xFFFF;
	static uint16_t yEndLast = 0xFFFF;
	if ((yStartLast != yStart) || (yEndLast != yEnd)) {
		uint8_t ay[4];
		ay[0] = yStart >> 8;
		ay[1] = yStart & 0xFF;
		ay[2] = yEnd >> 8;
		ay[3] = yEnd & 0xFF;
		LcdSend(LCD_PAGE_ADDR, ay, NULL);
		yStartLast = yStart;
		yEndLast = yEnd;
	}
}

void Ili9341WriteArray(const uint8_t * colors, uint16_t length) {
	LcdCommandData(LCD_GRAM, colors, NULL, length);
}

void Ili9341WriteColor(uint16_t color, uint16_t length) {
	LcdCsOn();
	LCD_IO_WriteReg(LCD_GRAM);
	uint8_t bytes[2];
	bytes[0] = color >> 8;
	bytes[1] = color & 0xFF;
	for (uint16_t i = 0; i < length; i++) {
		PeripheralTransfer(bytes, NULL, sizeof(bytes));
	}
	LcdCsOff();
}


void Ili9341WritePixel(uint16_t x, uint16_t y, uint16_t color) {
	Ili9341SetWindowStart(x, x+1, y, y + 1);
	Ili9341WriteColor(color, 1);
}

void Ili9341DrawHLine(uint16_t color, uint16_t x, uint16_t y, uint16_t length) {
	Ili9341SetWindowStart(x, x + length, y, y);
	Ili9341WriteColor(color, length);
}

void Ili9341DrawVLine(uint16_t color, uint16_t x, uint16_t y, uint16_t length) {
	Ili9341SetWindowStart(x, x, y, y + length);
	Ili9341WriteColor(color, length);
}

