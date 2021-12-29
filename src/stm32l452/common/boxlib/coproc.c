/* Boxlib
(c) 2021 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause
*/

#include <stdbool.h>

#include "coproc.h"

#include "main.h"


bool CoprocInGet(void) {
	if (HAL_GPIO_ReadPin(AvrSpiMiso_GPIO_Port, AvrSpiMiso_Pin) == GPIO_PIN_RESET) {
		return false;
	}
	return true;
}

