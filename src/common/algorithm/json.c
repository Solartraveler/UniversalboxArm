/* json
(c) 2022 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause

Easy usage of jsmn parser. Suitable for small files with up to 15 key/value
elements. For larger files a more efficient method should be used than parsing
the file for every key/value to retrieve.
*/

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "json.h"

#include "jsmn.h"

bool JsonValueGet(const uint8_t * jsonStart, size_t jsonLen, const char * key, char * valueOut, size_t valueMax) {
	jsmn_parser p;
	jsmntok_t t[32];
	jsmn_init(&p);
	int elems = jsmn_parse(&p, (const char *)jsonStart, jsonLen, t, sizeof(t) / sizeof(t[0]));
	if (elems < 0) {
		printf("Error, parsing json failed. Code: %i\r\n", elems);
		return false;
	}
	size_t keyLen = strlen(key);
	for (int i = 1; i < elems; i += 2) {
		size_t elemLen = t[i].end - t[i].start;
		if ((t[i].type == JSMN_STRING) && (elemLen == keyLen)) {
			if (memcmp(jsonStart + t[i].start, key, keyLen) == 0) {
				size_t valueLen = t[i + 1].end - t[i + 1].start;
				if (valueLen < valueMax) {
					memcpy(valueOut, jsonStart + t[i + 1].start, valueLen);
					valueOut[valueLen] = '\0';
					return true;
				}
			}
		}
	}
	return false;
}
