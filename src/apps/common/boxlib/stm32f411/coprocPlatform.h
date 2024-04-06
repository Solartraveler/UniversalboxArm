/* Boxlib
(c) 2024 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause

This file should only be included by coproc.c. Including it multiple times
will not work.
*/

#include <stdbool.h>

#include "boxlib/coproc.h"

void CoprocInit(void) {
}

bool CoprocInGet(void) {
	return false;
}

uint16_t CoprocSendCommand(uint8_t command, uint16_t data) {
	(void)command;
	(void)data;
	return 0;
}
