#pragma once

/* Init sequence:
  LcdEnable();
  LcdBacklightOn();
  LcdInit();
  then call LcdWritePixel or LcdTestpattern
  LcdBacklightOn can be reordered to any position in the sequence
*/

void LcdEnable(void);

void LcdDisable(void);

void LcdBacklightOn(void);

void LcdBacklightOff(void);

typedef enum {
	/* The controller used for the 128x128 and 160x128 LCD */
	ST7735 = 1,
	/* The controller used for the 320x240 LCD */
	ILI9341 = 2
} eDisplay_t;

void LcdInit(eDisplay_t lcdType);

void LcdWritePixel(uint16_t x, uint16_t y, uint16_t color);

void LcdTestpattern(void);

void LcdCommandData(uint8_t command, const uint8_t * dataOut, uint8_t * dataIn, size_t len);
