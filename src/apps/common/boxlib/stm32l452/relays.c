/* Boxlib
(c) 2021 by Malte Marwedel

SPDX-License-Identifier: BSD-3-Clause
*/

#include <stdbool.h>

#include "boxlib/relays.h"

#include "main.h"

#define BOOLTOPIN(state) state ? GPIO_PIN_SET : GPIO_PIN_RESET


void RelaysInit(void) {
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	__HAL_RCC_GPIOA_CLK_ENABLE(); //Relay1, Relay4
	__HAL_RCC_GPIOB_CLK_ENABLE(); //Relay2
	__HAL_RCC_GPIOC_CLK_ENABLE(); //Relay3

	GPIO_InitStruct.Pin = Relay1_Pin | Relay4_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = Relay2_Pin;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = Relay3_Pin;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

void Relay1Set(bool state) {
	GPIO_PinState ps = BOOLTOPIN(state);
	HAL_GPIO_WritePin(Relay1_GPIO_Port, Relay1_Pin, ps);
}

void Relay2Set(bool state) {
	GPIO_PinState ps = BOOLTOPIN(state);
	HAL_GPIO_WritePin(Relay2_GPIO_Port, Relay2_Pin, ps);
}

void Relay3Set(bool state) {
	GPIO_PinState ps = BOOLTOPIN(state);
	HAL_GPIO_WritePin(Relay3_GPIO_Port, Relay3_Pin, ps);
}

void Relay4Set(bool state) {
	GPIO_PinState ps = BOOLTOPIN(state);
	HAL_GPIO_WritePin(Relay4_GPIO_Port, Relay4_Pin, ps);
}

