/* Boxlib
(c) 2022 by Malte Marwedel

SPDX-License-Identifier: BSD-3-Clause

This is an alternate implementation for keys.c, choose one, do not include both.
Unlike the keys.c implementation, this only reacts on the release of a key,
but it is using interrupts, so it works even with prolonged polling intervals.

The implementation currently does not support other external interrupts on pins
5-15 used side by side.
*/

#include <stdbool.h>

#include "keys.h"

#include "main.h"

#define KEYS_NUM 4

uint8_t g_keysPressed[KEYS_NUM];

//internal ISR state variables
bool g_keysState[KEYS_NUM];
uint32_t g_keyDownTimestamp[KEYS_NUM];


void KeyUpdateState(uint32_t gpioPort, uint32_t gpioPin, uint8_t index) {
	bool oldState = g_keysState[index];
	bool pressed = false;
	if (HAL_GPIO_ReadPin(gpioPort, gpioPin) == GPIO_PIN_RESET) {
		pressed = true;
	}
	if ((oldState) & (!pressed)) { //key release
		uint32_t deltaTime = HAL_GetTick() - g_keyDownTimestamp[index];
		if (deltaTime > 20) { //20ms to make sure we dont trigger on debouncing keys
			g_keysPressed[index] = 1;
		}
		g_keysState[index] = false;
		__HAL_GPIO_EXTI_CLEAR_IT(gpioPin);
	} else if ((!oldState) & (pressed)) { //key pressed
		g_keyDownTimestamp[index] = HAL_GetTick();
		g_keysState[index] = true;
		__HAL_GPIO_EXTI_CLEAR_IT(gpioPin);
	}
}

void EXTI9_5_IRQHandler(void) {
	KeyUpdateState(KeyRight_GPIO_Port, KeyRight_Pin, 0);
	KeyUpdateState(KeyDown_GPIO_Port, KeyDown_Pin, 3);
}

void EXTI15_10_IRQHandler(void) {
	KeyUpdateState(KeyLeft_GPIO_Port, KeyLeft_Pin, 1);
	KeyUpdateState(KeyUp_GPIO_Port, KeyUp_Pin, 2);
}

void KeysInit(void) {
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	/* Rising and falling edge for Pin C13, C8, A15, B9 */
	GPIO_InitStruct.Pin = KeyUp_Pin|KeyDown_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = KeyLeft_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(KeyLeft_GPIO_Port, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = KeyRight_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(KeyRight_GPIO_Port, &GPIO_InitStruct);

	HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

	HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

bool KeyRightPressed(void) {
	if (g_keysPressed[0]) {
		g_keysPressed[0] = 0;
		return true;
	}
	return false;
}

bool KeyLeftPressed(void) {
	if (g_keysPressed[1]) {
		g_keysPressed[1] = 0;
		return true;
	}
	return false;
}

bool KeyUpPressed(void) {
	if (g_keysPressed[2]) {
		g_keysPressed[2] = 0;
		return true;
	}
	return false;
}

bool KeyDownPressed(void) {
	if (g_keysPressed[3]) {
		g_keysPressed[3] = 0;
		return true;
	}
	return false;
}
