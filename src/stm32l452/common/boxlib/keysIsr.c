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

#define KEY_RIGHT 0
#define KEY_LEFT 1
#define KEY_UP 2
#define KEY_DOWN 3


bool g_keysPressed[KEYS_NUM];

//internal ISR state variables
bool g_keysState[KEYS_NUM];
uint32_t g_keyDownTimestamp[KEYS_NUM];

void KeyUpdateState(GPIO_TypeDef * gpioPort, uint32_t gpioPin, uint8_t index) {
	bool oldState = g_keysState[index];
	if (__HAL_GPIO_EXTI_GET_IT(gpioPin)) {
		//If this is pending, it *must* always be cleared, otherwise we end up in endless interrupt
		__HAL_GPIO_EXTI_CLEAR_IT(gpioPin);
		bool pressed = false;
		/* By clearing the pending bit before reading the state, we make sure we do
		  not miss the very last down bounce. Because this will trigger in an additional
		  ISR and then with the correct pin state.
		*/
		if (HAL_GPIO_ReadPin(gpioPort, gpioPin) == GPIO_PIN_RESET) {
			pressed = true;
		}
		if ((oldState) && (!pressed)) { //key release
			uint32_t deltaTime = HAL_GetTick() - g_keyDownTimestamp[index];
			if (deltaTime > 10) { //10ms to make sure we don't trigger on debouncing keys
				g_keysPressed[index] = true;
				g_keysState[index] = false;
			}
		} else if ((!oldState) && (pressed)) {
			g_keyDownTimestamp[index] = HAL_GetTick();
			g_keysState[index] = true;
		}
	}
}

void EXTI9_5_IRQHandler(void) {
	KeyUpdateState(KeyRight_GPIO_Port, KeyRight_Pin, KEY_RIGHT);
	KeyUpdateState(KeyDown_GPIO_Port, KeyDown_Pin, KEY_DOWN);
}

void EXTI15_10_IRQHandler(void) {
	KeyUpdateState(KeyLeft_GPIO_Port, KeyLeft_Pin, KEY_LEFT);
	KeyUpdateState(KeyUp_GPIO_Port, KeyUp_Pin, KEY_UP);
}

void KeysInit(void) {
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();

	GPIO_InitTypeDef GPIO_InitStruct = {0};

	/* Rising and falling edge for Pin C13, C8, A15, B9 */
	GPIO_InitStruct.Pin = KeyUp_Pin | KeyDown_Pin;
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
	if (g_keysPressed[KEY_RIGHT]) {
		g_keysPressed[KEY_RIGHT] = false;
		return true;
	}
	return false;
}

bool KeyLeftPressed(void) {
	if (g_keysPressed[KEY_LEFT]) {
		g_keysPressed[KEY_LEFT] = false;
		return true;
	}
	return false;
}

bool KeyUpPressed(void) {
	if (g_keysPressed[KEY_UP]) {
		g_keysPressed[KEY_UP] = false;
		return true;
	}
	return false;
}

bool KeyDownPressed(void) {
	if (g_keysPressed[KEY_DOWN]) {
		g_keysPressed[KEY_DOWN] = false;
		return true;
	}
	return false;
}
