/* Boxlib
(c) 2022 by Malte Marwedel

SPDX-License-Identifier: BSD-3-Clause
*/

#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include "boxlib/readLine.h"

#include "boxlib/rs232debug.h"

void ReadSerialLine(char * input, size_t len) {
	memset(input, 0, len);
	size_t i = 0;
	while (i < (len - 1)) {
		char c = Rs232GetChar();
		if (c != 0) {
			if ((c == '\r') || (c == '\n')) {
				break;
			}
			if ((c == '\b') && (i)) {
				i--;
				input[i] = '\0';
				printf("\b \b");
			} else if (isprint(c)) {
				printf("%c", c);
				input[i] = c;
				i++;
			}
		}
	}
}
