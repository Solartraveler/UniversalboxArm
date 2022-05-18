#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Init sequence:
  LcdEnable();
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

void LcdWriteRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint8_t * data, size_t len);

void LcdWaitBackgroundDone(void);

bool KeyRightPressed(void);

bool KeyLeftPressed(void);

bool KeyUpPressed(void);

bool KeyDownPressed(void);

void Led1Red(void);

void Led1Green(void);

void Led1Yellow(void);

void Led1Off(void);

void Led2Red(void);

void Led2Green(void);

void Led2Yellow(void);

void Led2Off(void);