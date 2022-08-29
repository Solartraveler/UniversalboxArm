/* utility
(c) 2022 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause
*/

#include <stdint.h>
#include <ctype.h>
#include <stdio.h>

#include "utility.h"

uint32_t AsciiScanHex(const char * string) {
	uint32_t out = 0;
	uint32_t parsed = 0;
	while (*string) {
		char c = *string;
		if ((parsed == 0) && (c == ' ')) {
			string++;
			continue;
		}
		uint32_t fourbit = 0;
		if (c == 'x') {
			out = 0;
		} else if (isdigit(c)) {
			fourbit = c - '0';
		} else if ((c >= 'a') && (c <= 'f')) {
			fourbit = c - 'a' + 10;
		} else if ((c >= 'A') && (c <= 'F')) {
			fourbit = c - 'A' + 10;
		} else {
			break;
		}
		out *= 16;
		out += fourbit;
		string++;
		parsed++;
	}
	return out;
}


uint32_t AsciiScanDec(const char * string) {
	uint32_t out = 0;
	uint32_t parsed = 0;
	while (*string) {
		if ((parsed == 0) && (*string == ' ')) {
			string++;
			continue;
		}
		if (isdigit(*string)) {
			out *= 10;
			out += *string - '0';
			parsed++;
			string++;
			continue;
		}
		break;
	}
	return out;
}

void PrintHex(const uint8_t * data, size_t len) {
	for (uint32_t i = 0; i < len; i++) {
		printf("%02X ", data[i]);
		if (((i % 8) == 7) || (i == len -1)) {
			printf("\r\n");
		}
	}
}
