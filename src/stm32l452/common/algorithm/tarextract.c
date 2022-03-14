#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#define BLOCKSIZE 512

uint32_t TarParseOctal(const char * input, size_t len) {
	uint32_t res = 0;
	for (uint32_t i = 0; i < len; i++) {
		if (input[i]) {
			res <<= 3;
			if ((input[i] >= '0') && (input[i] <= '7')) {
				res += input[i] - '0';
			}
		}
	}
	return res;
}

bool TarFileStartGet(const char * filename, uint8_t * tarData, size_t tarLen,  uint8_t ** startAddress, size_t * fileLen, uint32_t * timestamp) {
	while (tarLen >= BLOCKSIZE) {
		size_t len = TarParseOctal((const char *)tarData + 124, 12);
		const char * name = (const char *)tarData;
		//printf("%s - %s - %u\n", name, (char *)tarData + 124, len);
		tarData += BLOCKSIZE;
		tarLen -= BLOCKSIZE;
		if (strcmp(filename, name) == 0) {
			*startAddress = tarData;
			*fileLen = len;
			if (timestamp) {
				*timestamp = TarParseOctal((const char *)tarData + 136, 12);
			}
			return true;
		} else {
			//printf("no match with >%s<\n", filename);
		}
		uint32_t allocated = (len + (BLOCKSIZE - 1)) & (~(BLOCKSIZE -1)); //round up to blocksize
		if (tarLen >= allocated) {
			tarData += allocated;
			tarLen -= allocated;
		} else {
			break;
		}
	}
	return false;
}
