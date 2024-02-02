/* Boxlib
(c) 2022 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause
*/

#include "systickWithFreertos.h"

#include "main.h"

void SystickDisable(void) {
	__disable_irq();
}

void SystickForFreertosEnable(void) {

}
