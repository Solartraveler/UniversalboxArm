/* Boxlib
(c) 2021 by Malte Marwedel

SPDX-License-Identifier: BSD-3-Clause
*/

#include "leds.h"

#include "main.h"


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

