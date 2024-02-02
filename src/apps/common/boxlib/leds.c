/* Boxlib
(c) 2021 by Malte Marwedel

SPDX-License-Identifier: BSD-3-Clause
*/

#include "leds.h"

#include "main.h"

void LedsInit(void) {
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	__HAL_RCC_GPIOA_CLK_ENABLE(); //Led2Red, Led2Green
	__HAL_RCC_GPIOB_CLK_ENABLE(); //Led1Red
	__HAL_RCC_GPIOD_CLK_ENABLE(); //Led1Green

	HAL_GPIO_WritePin(GPIOA, Led2Red_Pin | Led2Green_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(Led1Red_GPIO_Port, Led1Red_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(Led1Green_GPIO_Port, Led1Green_Pin, GPIO_PIN_RESET);

	GPIO_InitStruct.Pin = Led2Red_Pin | Led2Green_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = Led1Red_Pin;
	HAL_GPIO_Init(Led1Red_GPIO_Port, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = Led1Green_Pin;
	HAL_GPIO_Init(Led1Green_GPIO_Port, &GPIO_InitStruct);
}

void Led1Red(void) {
	HAL_GPIO_WritePin(Led1Red_GPIO_Port,   Led1Red_Pin,   GPIO_PIN_SET);
	HAL_GPIO_WritePin(Led1Green_GPIO_Port, Led1Green_Pin, GPIO_PIN_RESET);
}

void Led1Green(void) {
	HAL_GPIO_WritePin(Led1Red_GPIO_Port,   Led1Red_Pin,   GPIO_PIN_RESET);
	HAL_GPIO_WritePin(Led1Green_GPIO_Port, Led1Green_Pin, GPIO_PIN_SET);
}

void Led1Yellow(void) {
	HAL_GPIO_WritePin(Led1Red_GPIO_Port,   Led1Red_Pin,   GPIO_PIN_SET);
	HAL_GPIO_WritePin(Led1Green_GPIO_Port, Led1Green_Pin, GPIO_PIN_SET);
}

void Led1Off(void) {
	HAL_GPIO_WritePin(Led1Red_GPIO_Port,   Led1Red_Pin,   GPIO_PIN_RESET);
	HAL_GPIO_WritePin(Led1Green_GPIO_Port, Led1Green_Pin, GPIO_PIN_RESET);
}

void Led2Red(void) {
	HAL_GPIO_WritePin(Led2Red_GPIO_Port,   Led2Red_Pin,   GPIO_PIN_SET);
	HAL_GPIO_WritePin(Led2Green_GPIO_Port, Led2Green_Pin, GPIO_PIN_RESET);
}

void Led2Green(void) {
	HAL_GPIO_WritePin(Led2Red_GPIO_Port,   Led2Red_Pin,   GPIO_PIN_RESET);
	HAL_GPIO_WritePin(Led2Green_GPIO_Port, Led2Green_Pin, GPIO_PIN_SET);
}

void Led2Yellow(void) {
	HAL_GPIO_WritePin(Led2Red_GPIO_Port,   Led2Red_Pin,   GPIO_PIN_SET);
	HAL_GPIO_WritePin(Led2Green_GPIO_Port, Led2Green_Pin, GPIO_PIN_SET);
}

void Led2Off(void) {
	HAL_GPIO_WritePin(Led2Red_GPIO_Port,   Led2Red_Pin,   GPIO_PIN_RESET);
	HAL_GPIO_WritePin(Led2Green_GPIO_Port, Led2Green_Pin, GPIO_PIN_RESET);
}

