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

void LcdInit(void);

void LcdWritePixel(uint16_t x, uint16_t y, uint16_t color);

void LcdTestpattern(void);
