/* Boxlib
(c) 2021 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause
*/

#include <stdbool.h>

#include "relays.h"

#include "main.h"

#define BOOLTOPIN(state) state ? GPIO_PIN_SET : GPIO_PIN_RESET

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

