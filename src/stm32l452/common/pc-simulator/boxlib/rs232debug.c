/* Boxlib emulation
(c) 2022 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause
*/

#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>

#include <poll.h>

#include "rs232debug.h"


#include "peripheral.h"

void Rs232Init(void) {
	PeripheralPowerOn();
}

void Rs232Stop(void) {
}

void Rs232Flush(void) {
	fflush(stdout);
}

void Rs232WriteString(const char * str) {
	printf("%s", str);
}

void Rs232WriteStringNoWait(const char * str) {
	printf("%s", str);
}

int printfNowait(const char * format, ...) {
	va_list args;
	va_start(args, format);
	char buffer[256];
	int params = vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);
	Rs232WriteStringNoWait(buffer);
	return params;
}

char Rs232GetChar(void) {
	struct pollfd fds;
	fds.fd = 0;
	fds.events = POLLIN;
	fds.revents = 0;

	char result = 0;
	system("stty raw");
	if (poll(&fds, 1, 1) > 0) {
		char val = getchar();
		if (isascii(val)) {
			result = val;
		}
	}
	system("stty cooked");
	return result;
}
