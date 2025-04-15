/* Boxlib
(c) 2022 by Malte Marwedel

SPDX-License-Identifier: BSD-3-Clause

This is an alternate implementation for keys.c, choose one, do not include both.
Unlike the keys.c implementation, this supports checking release of a key, and
press down.
By using interrupts, the release event is latched, so it works even with
prolonged polling intervals.

The implementation currently does not support other external interrupts on pins
5-15 used side by side.
*/

#include <stdbool.h>

#include "boxlib/keysIsr.h"

#include "main.h"

#define KEYS_NUM 4

#define KEY_RIGHT 0
#define KEY_LEFT 1
#define KEY_UP 2
#define KEY_DOWN 3


//sets to true once a press->release change is detected, false on second request
bool g_keysReleased[KEYS_NUM];

//internal ISR state variables, non ISR only read this variable
bool g_keysPressed[KEYS_NUM];
uint32_t g_keyDownTimestamp[KEYS_NUM];

void KeyUpdateState(GPIO_TypeDef * gpioPort, uint32_t gpioPin, uint8_t index) {
	bool oldState = g_keysPressed[index];
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
				g_keysReleased[index] = true;
				g_keysPressed[index] = false;
			}
		} else if ((!oldState) && (pressed)) {
			g_keyDownTimestamp[index] = HAL_GetTick();
			g_keysPressed[index] = true;
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

	HAL_NVIC_SetPriority(EXTI9_5_IRQn, 8, 0);
	HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

	HAL_NVIC_SetPriority(EXTI15_10_IRQn, 8, 0);
	HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

bool KeyRightReleased(void) {
	if (g_keysReleased[KEY_RIGHT]) {
		g_keysReleased[KEY_RIGHT] = false;
		return true;
	}
	return false;
}

bool KeyLeftReleased(void) {
	if (g_keysReleased[KEY_LEFT]) {
		g_keysReleased[KEY_LEFT] = false;
		return true;
	}
	return false;
}

bool KeyUpReleased(void) {
	if (g_keysReleased[KEY_UP]) {
		g_keysReleased[KEY_UP] = false;
		return true;
	}
	return false;
}

bool KeyDownReleased(void) {
	if (g_keysReleased[KEY_DOWN]) {
		g_keysReleased[KEY_DOWN] = false;
		return true;
	}
	return false;
}

