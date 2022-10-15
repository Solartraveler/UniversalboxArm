/* Boxlib
(c) 2021 by Malte Marwedel

SPDX-License-Identifier: BSD-3-Clause
*/

#include <stdbool.h>

#include "keys.h"

#include "main.h"


void KeysInit(void) {
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();

	GPIO_InitTypeDef GPIO_InitStruct = {0};

	/* Connected to Pin C13, C8, A15, B9 */
	GPIO_InitStruct.Pin = KeyUp_Pin | KeyDown_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

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
