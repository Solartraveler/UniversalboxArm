/* Boxlib
(c) 2021-2022 by Malte Marwedel

SPDX-License-Identifier: BSD-3-Clause
*/

#include <stdbool.h>

#include "boxlib/ir.h"

#include "main.h"

void IrInit(void) {
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	__HAL_RCC_GPIOB_CLK_ENABLE(); //In pin
	__HAL_RCC_GPIOC_CLK_ENABLE(); //Power pin

	GPIO_InitStruct.Pin = IrIn_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = IrPower_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

void IrOn(void) {
	HAL_GPIO_WritePin(IrPower_GPIO_Port, IrPower_Pin, GPIO_PIN_SET);
}

void IrOff(void) {
	HAL_GPIO_WritePin(IrPower_GPIO_Port, IrPower_Pin, GPIO_PIN_RESET);
}

bool IrPinSignal(void) {
	if (HAL_GPIO_ReadPin(IrIn_GPIO_Port, IrIn_Pin) == GPIO_PIN_RESET) {
		return true;
	}
	return false;

}

