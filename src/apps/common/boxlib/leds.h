#pragma once

/*Call to configure GPIOs, otherwise all other functions may be called but have
  no effect. After init, all LEDs are off.

For the PC simulation:
The LEDs are simulated by the LCD on the PC, but leds.c provides a dummy
implementation as weak functions.
*/
void LedsInit(void);

void Led1Red(void);

void Led1Green(void);

void Led1Yellow(void);

void Led1Off(void);

void Led2Red(void);

void Led2Green(void);

void Led2Yellow(void);

void Led2Off(void);
