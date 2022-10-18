#pragma once

#include "lcd.h"

void LedsInit(void);

/* The LEDs are simulated by the LCD on the PC, but leds.c provides a dummy
   implementation as weak functions
*/

void Led1Red(void);

void Led1Green(void);

void Led1Yellow(void);

void Led1Off(void);

void Led2Red(void);

void Led2Green(void);

void Led2Yellow(void);

void Led2Off(void);
