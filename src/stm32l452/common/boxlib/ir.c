/* Boxlib
(c) 2021 by Malte Marwedel

SPDX-License-Identifier: BSD-3-Clause
*/

#include <stdbool.h>

#include "ir.h"

#include "main.h"

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

