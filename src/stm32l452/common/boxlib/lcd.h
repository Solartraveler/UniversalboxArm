#pragma once

/* Init sequence:
  LcdEnable(4);
  LcdBacklightOn();
  LcdInit();
  then call LcdWritePixel or LcdTestpattern
  LcdBacklightOn can be reordered to any position in the sequence
*/

void LcdEnable(uint32_t clockPrescaler);

void LcdDisable(void);

void LcdBacklightOn(void);

void LcdBacklightOff(void);

typedef enum {
	NONE = 0,
	/* The controller used for the 128x128 and 160x128 LCD */
	ST7735_128 = 1,
	ST7735_160 = 2,
	/* The controller used for the 320x240 LCD */
	ILI9341 = 3
} eDisplay_t;

void LcdInit(eDisplay_t lcdType);

void LcdWritePixel(uint16_t x, uint16_t y, uint16_t color);

void LcdTestpattern(void);

void LcdCommandData(uint8_t command, const uint8_t * dataOut, uint8_t * dataIn, size_t len);


//TODO: Currently this function only works with a height = 1 rectangle.
void LcdWriteRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint8_t * data, size_t len);
