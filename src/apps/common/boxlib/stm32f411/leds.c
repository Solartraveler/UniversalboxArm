/* Boxlib
(c) 2024 by Malte Marwedel

SPDX-License-Identifier: BSD-3-Clause
*/

#include "boxlib/leds.h"

#include "main.h"

void LedsInit(void) {
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	__HAL_RCC_GPIOA_CLK_ENABLE(); //Led1Green

	HAL_GPIO_WritePin(Led1Green_GPIO_Port, Led1Green_Pin, GPIO_PIN_RESET);

	GPIO_InitStruct.Pin = Led1Green_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(Led1Green_GPIO_Port, &GPIO_InitStruct);
}

void Led1Red(void) {
}

void Led1Green(void) {
	HAL_GPIO_WritePin(Led1Green_GPIO_Port, Led1Green_Pin, GPIO_PIN_SET);
}

void Led1Yellow(void) {
	HAL_GPIO_WritePin(Led1Green_GPIO_Port, Led1Green_Pin, GPIO_PIN_SET);
}

void Led1Off(void) {
	HAL_GPIO_WritePin(Led1Green_GPIO_Port, Led1Green_Pin, GPIO_PIN_RESET);
}

void Led2Red(void) {
}

void Led2Green(void) {
}

void Led2Yellow(void) {
}

void Led2Off(void) {
}
