#pragma once

#include <assert.h>
#include <stdint.h>

#define SIGNATUREBYTES 4
#define RIFFIDBYTES 4

#define METAHEADER (SIGNATUREBYTES + sizeof(uint32_t))

typedef struct {
	char signature[SIGNATUREBYTES]; //must be "RIFF"
	uint32_t filesize;
	char id[RIFFIDBYTES]; //must be "WAVE"
} riffHeader_t;

_Static_assert(sizeof(riffHeader_t) == 12, "Fix the struct size for your platform");

typedef struct {
	char signature[SIGNATUREBYTES]; //must be "fmt "
	uint32_t headerSize;
	uint16_t format;
	uint16_t channels;
	uint32_t sampleRate;
	uint32_t bytesPerSecond;
	uint16_t blockAlign;
	uint16_t bitsPerSample;
} fmtHeader_t;

_Static_assert(sizeof(fmtHeader_t) == 24, "Fix the struct size for your platform");

typedef struct {
	char signature[SIGNATUREBYTES]; //must be "data"
	uint32_t blockSize;
} dataHeader_t;

_Static_assert(sizeof(dataHeader_t) == 8, "Fix the struct size for your platform");
