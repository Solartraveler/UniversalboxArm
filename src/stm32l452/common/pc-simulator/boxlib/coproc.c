/* Boxlib emulation
(c) 2022 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause
*/

#include <stdbool.h>
#include <unistd.h>

#include "coproc.h"

bool CoprocInGet(void) {
	return false;
	return true;
}

uint16_t CoprocSendCommand(uint8_t command, uint16_t data) {
	(void)command;
	(void)data;
	return 0;
}

uint16_t CoprocReadTestpattern(void) {
	return 0xF055;
}

uint16_t CoprocReadVersion(void) {
	return 0x0001;
}

uint16_t CoprocReadVcc(void) {
	return 3210;
}

void CoprocWriteReboot(uint8_t mode) {
	(void)mode;
}

void CoprocWatchdogCtrl(uint16_t timeout) {
	(void)timeout;
}

void CoprocWatchdogReset(void) {
}
