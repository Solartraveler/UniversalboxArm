/* Boxlib
(c) 2024 by Malte Marwedel

SPDX-License-Identifier: BSD-3-Clause
*/

#include <stdbool.h>

#include "boxlib/keys.h"

#include "main.h"

//If keysIsr.c is used, the init code is taken from there
__attribute__((weak)) void KeysInit(void) {
	__HAL_RCC_GPIOB_CLK_ENABLE(); //up
	__HAL_RCC_GPIOC_CLK_ENABLE(); //key left, down, right

	GPIO_InitTypeDef GPIO_InitStruct = {0};

	GPIO_InitStruct.Pin = KeyUp_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(KeyUp_GPIO_Port, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = KeyDown_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(KeyDown_GPIO_Port, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = KeyLeft_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(KeyLeft_GPIO_Port, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = KeyRight_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(KeyRight_GPIO_Port, &GPIO_InitStruct);
}

bool KeyRightPressed(void) {
	if (HAL_GPIO_ReadPin(KeyRight_GPIO_Port, KeyRight_Pin) == GPIO_PIN_RESET) {
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
