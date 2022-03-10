/* Boxlib emulation
(c) 2022 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause
*/

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "esp.h"

void EspEnable(void) {
}

void EspStop(void) {
}

char EspGetChar(void) {
	return 0;
}

void EspSendString(const char * str) {
	printf("Sending >%s< to ESP\n", str);
}

uint32_t EspCommand(const char * command, char * response, size_t maxResponse, uint32_t timeout) {
	(void)command;
	if ((maxResponse > 0) && (response)) {
		*response = '\0';
	}
	(void)timeout;
	return 0;
}
