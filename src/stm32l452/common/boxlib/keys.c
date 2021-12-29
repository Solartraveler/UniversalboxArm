/* Boxlib
(c) 2021 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause
*/

#include <stdbool.h>

#include "keys.h"

#include "main.h"


bool KeyRighPressed(void) {
	if (HAL_GPIO_ReadPin(KeyRight_GPIO_Port, KeyRight_Pin) == GPIO_PIN_RESET)
	{
		return true;
	}
	return false;
}

bool KeyLeftPressed(void) {
	if (HAL_GPIO_ReadPin(KeyLeft_GPIO_Port, KeyLeft_Pin) == GPIO_PIN_RESET) {
		return true;
	}
	return false;
}

bool KeyUpPressed(void) {
	if (HAL_GPIO_ReadPin(KeyUp_GPIO_Port, KeyUp_Pin) == GPIO_PIN_RESET) {
		return true;
	}
	return false;
}

bool KeyDownPressed(void) {
	if (HAL_GPIO_ReadPin(KeyDown_GPIO_Port, KeyDown_Pin) == GPIO_PIN_RESET) {
		return true;
	}
	return false;
}
