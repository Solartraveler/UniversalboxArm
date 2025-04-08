#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "../locklessfifo.h"

FifoState_t g_fifo;

uint8_t FifoGetData(void) {
	return FifoDataGet(&g_fifo);
}

size_t FifoFree(void) {
	return FifoDataFree(&g_fifo);
}

bool FifoPutData(uint8_t data) {
	return FifoDataPut(&g_fifo, data);
}

void FifoPutBuffer(const uint8_t * data, size_t dataLen) {
	FifoBufferPut(&g_fifo, data, dataLen);
}

#define TASS(is, should) if ((is) != (should)) {printf("Error in line %u, should %u, is %u\n", (unsigned int)__LINE__, (unsigned int)should, (unsigned int)is); exit(1);}

int main(void) {
	uint8_t buffer[9];
	FifoInit(&g_fifo, buffer, sizeof(buffer));
	TASS(FifoFree(), sizeof(buffer) - 1);
	TASS(FifoGetData(), 0);
	TASS(FifoFree(), sizeof(buffer) - 1);
	TASS(FifoPutData(42), true);
	TASS(FifoFree(), sizeof(buffer) - 2);
	TASS(FifoGetData(), 42);
	TASS(FifoFree(), sizeof(buffer) - 1);
	uint8_t manyBytes[8] = {0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11};
	FifoPutBuffer((uint8_t *)&manyBytes, sizeof(manyBytes));
	TASS(FifoFree(), 0);
	TASS(FifoPutData(42), false);
	TASS(FifoGetData(), 0x88);
	TASS(FifoFree(), sizeof(buffer) - 8);
	TASS(FifoGetData(), 0x77);
	TASS(FifoGetData(), 0x66);
	TASS(FifoGetData(), 0x55);
	TASS(FifoPutData(0xAA), true);
	TASS(FifoPutData(0xBB), true);
	TASS(FifoGetData(), 0x44);
	TASS(FifoGetData(), 0x33);
	TASS(FifoGetData(), 0x22);
	TASS(FifoGetData(), 0x11);
	TASS(FifoGetData(), 0xAA);
	TASS(FifoGetData(), 0xBB);
	TASS(FifoFree(), sizeof(buffer) - 1);
}
