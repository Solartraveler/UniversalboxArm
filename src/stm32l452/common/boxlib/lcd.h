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

void LcdWriteRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint8_t * data, size_t len);

/*If LcdWriteRect is using dma, the buffer data must be valid until this function
  returns. LcdWriteRect does the waiting internally, so there is no need to call
  LcdWaitDmaDone before a next LcdWriteRect call. Other functions however need a call
  before too. If DMA is not used by the implementation, the function may be left empty.
*/
void LcdWaitDmaDone(void);
