/* Boxlib emulation
(c) 2022 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause
*/

#include <stdbool.h>

#include "keys.h"

/* Non weak function can be found in lcd.c
   so the keys will be only visualized if an LCD is initialized.
   Of course, the real hardware can use the keys without a LCD.
*/

__attribute__((weak)) void KeysInit(void) {
}

__attribute__((weak)) bool KeyRightPressed(void) {
	return false;
}

__attribute__((weak)) bool KeyLeftPressed(void) {
	return false;
}

__attribute__((weak)) bool KeyUpPressed(void) {
	return false;
}

__attribute__((weak)) bool KeyDownPressed(void) {
	return false;
}

__attribute__((weak)) bool KeyRightReleased(void) {
	return false;
}

__attribute__((weak)) bool KeyLeftReleased(void) {
	return false;
}

__attribute__((weak)) bool KeyUpReleased(void) {
	return false;
}

__attribute__((weak)) bool KeyDownReleased(void) {
	return false;
}
