#pragma once

/*This overwrites the normal backlight on/off functionality, so run after
  the normal LCD init functions are done.
  Timer1 is used in PWM mode.
*/
void LcdBacklightInit(void);

//value is in the range 0 (off) to 1000 (maximum brightness)
void LcdBacklightSet(uint16_t level);

