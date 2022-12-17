#pragma once

/* Init sequence:
  LcdEnable(4);
  LcdBacklightOn();
  LcdInit();
  then call LcdWritePixel or LcdTestpattern
  LcdBacklightOn can be reordered to any position in the sequence

Thread safetyness is only provided if peripheralMt is used. The thread safety
is *not* given for all LCD functions from different threads - especially the
init and deinit functions are not thread safe.

It however makes it safe to use the LCD from one thread and the flash from
another thread while both use the same SPI hardware.
*/

//Not thread safe
void LcdEnable(uint32_t clockPrescaler);

//Not thread safe
void LcdDisable(void);

//Thread safe
void LcdBacklightOn(void);

//Thread safe
void LcdBacklightOff(void);

typedef enum {
	NONE = 0,
	/* The controller used for the 128x128 and 160x128 LCD */
	ST7735_128 = 1,
	ST7735_160 = 2,
	/* The controller used for the 320x240 LCD */
	ILI9341 = 3
} eDisplay_t;

//Thread safe if peripheralMt.c is used
void LcdInit(eDisplay_t lcdType);

//Thread safe if peripheralMt.c is used
void LcdWritePixel(uint16_t x, uint16_t y, uint16_t color);

//Thread safe if peripheralMt.c is used
void LcdTestpattern(void);

//Thread safe, no hardware usage
void LcdDelay(uint32_t delay);

//Background operation, chip select has to be done by the caller.
//Thread safetyness has to be managed by the caller.
void LcdWriteMultipleDataBackground(const uint8_t * dataOut, size_t len);

//Sends data, chip select has to be done by the caller.
//Thread safetyness has to be managed by the caller.
void LcdWriteMultipleData(const uint8_t * dataOut, size_t len);

//Only sets one register, chip select has to be done by the caller.
//Thread safetyness has to be managed by the caller.
void LcdWriteReg(uint8_t Reg);

//Sets one register, controls chip select.
//Thread safetyness has to be managed by the caller.
void LcdCommand(uint8_t command);

//Sends one data, controls chip select.
//Thread safetyness has to be managed by the caller.
void LcdData(const uint8_t * dataOut, size_t len);

//Sets one command with chip select.
//Thread safetyness has to be managed by the caller.
void LcdCommandData(uint8_t command, const uint8_t * dataOut, uint8_t * dataIn, size_t len);

/*Background operation if spiDma.c is used.
  Needs a LcdWaitBackgroundDone call before the next call to any Lcd function, except LcdCommandDataBackground.
  Thread safetyness has to be managed by the caller.
*/
void LcdCommandDataBackground(uint8_t command, const uint8_t * dataOut, uint8_t * dataIn, size_t len);

//Background operation if spiDma.c is used.
//Waits for previous transfers to finish by calling LcdWaitBackgroundDone
//Thread safe, if peripheralMt.c is used.
//The lock is only taken, a release is done by a call to LcdWaitBackgroundDone
void LcdWriteRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint8_t * data, size_t len);

/*If LcdWriteRect is using DMA, the buffer data must be valid until this function
  returns. LcdWriteRect does the waiting internally, so there is no need to call
  LcdWaitDmaDone before a next LcdWriteRect call. Other functions however need a call
  before too. If DMA and threadsafetyness is not used by the implementation,
  the function may be left empty.
  After disabling chip select, this releases the threadsafe lock.
*/
void LcdWaitBackgroundDone(void);
