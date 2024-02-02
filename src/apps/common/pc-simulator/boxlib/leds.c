/* Boxlib emulation
(c) 2022 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause
*/

#include "leds.h"

void LedsInit(void) {
}

/* Non weak function can be found in lcd.c
   so the LEDs will be only visualized if an LCD is initialized.
   Of course, the real hardware would display the LEDs without a LCD.
*/

__attribute__((weak)) void Led1Red(void) {
}

__attribute__((weak)) void Led1Green(void) {
}

__attribute__((weak)) void Led1Yellow(void) {
}

__attribute__((weak)) void Led1Off(void) {
}

__attribute__((weak)) void Led2Red(void) {
}

__attribute__((weak)) void Led2Green(void) {
}

__attribute__((weak)) void Led2Yellow(void) {
}

__attribute__((weak)) void Led2Off(void) {
}