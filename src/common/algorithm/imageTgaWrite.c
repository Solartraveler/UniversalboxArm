/* Stateless TGA writer
(c) 2022 by Malte Marwedel

SPDX-License-Identifier: BSD-3-Clause

Version 1.0
*/


#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>

#include "imageTgaWrite.h"

#include "utility.h"

typedef struct {
	uint8_t idLength;
	uint8_t colorMapType;
	uint8_t imageType;
	uint16_t colorMapIndex;
	uint16_t colorMapLength;
	uint8_t colorMapEntrySize;
	uint16_t xOrigin;
	uint16_t yOrigin;
	uint16_t imageWidth;
	uint16_t imageHeight;
	uint8_t pixelDepth;
	uint8_t imageDescriptor;
} __attribute__((__packed__)) tgaHeader_t;

_Static_assert(sizeof(tgaHeader_t) == 18, "Error, fix alignment");



bool ImgTgaStart(uint16_t sizeX, uint16_t sizeY, uint16_t colorMapEntries, uint8_t colorBits,
                 bool compression, outputWriteFunc_t * pWriter, void * param) {
	tgaHeader_t tga;
	tga.idLength = 0;
	tga.xOrigin = 0;
	tga.yOrigin = 0;
	tga.imageWidth = sizeX;
	tga.imageHeight = sizeY;
	tga.imageDescriptor = 0x20; //top left to right bottom encoding
	tga.colorMapIndex = 0;
	if ((colorMapEntries > 0) && ((colorBits == 16) || (colorBits == 24))) {
		if (compression) {
			tga.imageType = 9; //compressed color mapped image
		} else {
			tga.imageType = 1; //uncompressed color mapped image
		}
		if (colorMapEntries <= 256) {
			tga.pixelDepth = 8;
		} else {
			//no program seems to support this mode, so we disable the support here too.
			//tga.pixelDepth = 16;
			return false;
		}
		tga.colorMapType = 1;
		tga.colorMapLength = colorMapEntries;
		tga.colorMapEntrySize = colorBits;
	} else if (colorBits == 8) {
		if (compression) {
			tga.imageType = 11; //compressed black white
		} else {
			tga.imageType = 3; //uncompressed black white
		}
		tga.pixelDepth = 8;
		tga.colorMapType = 0;
		tga.colorMapLength = 0;
		tga.colorMapEntrySize = 0;
	} else if ((colorBits == 16) || (colorBits == 24)) {
		if (compression) {
			tga.imageType = 10; //compressed true color image
		} else {
			tga.imageType = 2; //uncompressed true color image
		}
		if (colorBits == 16) {
			tga.imageDescriptor |= 0x1; //1 bit alpha - so 15 bits for color left
		}
		tga.pixelDepth = colorBits;
		tga.colorMapType = 0;
		tga.colorMapLength = 0;
		tga.colorMapEntrySize = 0;
	} else {
		return false; //unsupported combination
	}
	return pWriter(&tga, sizeof(tga), param);
}

bool ImgTgaColormap16(uint8_t redBits, uint8_t greenBits, uint8_t blueBits,
                    outputWriteFunc_t * pWriter, void * param) {
	if ((redBits > 5) || (greenBits > 5) || (blueBits > 5)) {
		return false;
	}
	uint8_t redSpaceBits = 5 - redBits;
	uint8_t greenSpaceBits = 5 - greenBits;
	uint8_t blueSpaceBits = 5 - blueBits;
	uint16_t redColors = 1 << redBits;
	uint16_t greenColors = 1 << greenBits;
	uint16_t blueColors = 1 << blueBits;
	for (uint8_t r = 0; r < redColors; r++) {
		uint16_t redMask;
		if (r < (redColors - 1)) {
			redMask = (r << (redSpaceBits + 10));
		} else {
			redMask = 0x1F << 10;
		}
		for (uint8_t g = 0; g < greenColors; g++) {
			uint16_t greenMask;
			if (g < (greenColors - 1)) {
				greenMask = (g << (greenSpaceBits + 5));
			} else {
				greenMask = 0x1F << 5;
			}
			for (uint8_t b = 0; b < blueColors; b++) {
				uint16_t blueMask;
				if (b < (blueColors - 1)) {
					blueMask = b << blueSpaceBits;
				} else {
					blueMask = 0x1F;
				}
				uint16_t color = redMask | greenMask | blueMask;
				if (pWriter(&color, sizeof(uint16_t), param) == false) {
					return false;
				}
			}
		}
	}
	return true;
}

bool ImgTgaColormap24(uint8_t redBits, uint8_t greenBits, uint8_t blueBits,
                    outputWriteFunc_t * pWriter, void * param) {
	if ((redBits > 8) || (greenBits > 8) || (blueBits > 8)) {
		return false;
	}
	uint8_t redSpaceBits = 8 - redBits;
	uint8_t greenSpaceBits = 8 - greenBits;
	uint8_t blueSpaceBits = 8 - blueBits;
	uint16_t redColors = 1 << redBits;
	uint16_t greenColors = 1 << greenBits;
	uint16_t blueColors = 1 << blueBits;
	for (uint8_t r = 0; r < redColors; r++) {
		uint32_t redMask;
		if (r < (redColors - 1)) {
			redMask = (r << (redSpaceBits + 16));
		} else {
			redMask = 0xFF << 16;
		}
		for (uint8_t g = 0; g < greenColors; g++) {
			uint32_t greenMask;
			if (g < (greenColors - 1)) {
				greenMask = (g << (greenSpaceBits + 8));
			} else {
				greenMask = 0xFF << 8;
			}
			for (uint8_t b = 0; b < blueColors; b++) {
				uint32_t blueMask;
				if (b < (blueColors - 1)) {
					blueMask = b << blueSpaceBits;
				} else {
					blueMask = 0xFF;
				}
				uint32_t color = redMask | greenMask | blueMask;
				if (ImgTgaAppend32To24(&color, 1, pWriter, param) == false) {
					return false;
				}
			}
		}
	}
	return true;
}


bool ImgTgaAppendDirect(const void * data, size_t bytes, outputWriteFunc_t * pWriter, void * param) {
	return pWriter(data, bytes, param);
}

bool ImgTgaAppend32To24(const uint32_t * data, size_t elements, outputWriteFunc_t * pWriter, void * param) {
	bool success = true;
	for (size_t i = 0; i < elements; i++) {
		uint8_t temp[3];
		temp[0] = data[i] & 0xFF;
		temp[1] = (data[i] >> 8) & 0xFF;
		temp[2] = (data[i] >> 16) & 0xFF;
		success &= pWriter(temp, sizeof(temp), param);
	}
	return success;
}

bool ImgTgaAppendCompress1Byte(const uint8_t * data, size_t bytes, outputWriteFunc_t * pWriter, void * param) {
	bool success = true;
	size_t i = 0;
	while (i < bytes) {
		uint16_t equals = 1;
		for (size_t j = i + 1; j < MIN(bytes, i + 128); j++) {
			if (data[i] == data[j]) {
				equals++;
			} else {
				break;
			}
		}
		if (equals > 1) {
			//ok, add start a compression packet
			uint8_t rlp = 0x80 | (equals - 1); //run length packet
			success &= pWriter(&rlp, 1, param);
			success &= pWriter(data + i, 1, param);
			i += equals;
		} else {
			//add a raw packet
			uint16_t noEquals = 1;
			for (size_t j = i + 1; j < MIN(bytes - 1, i + 128); j++) {
				if (data[j] != data[j + 1]) {
					noEquals++;
				} else {
					break;
				}
			}
			uint8_t rp = noEquals - 1;
			success &= pWriter(&rp, 1, param);
			success &= pWriter(data + i, noEquals, param);
			i += noEquals;
		}
	}
	return success;
}

/*Of course, the three compress functions are very similar and could be unified,
but this would end up in one more complex function, needing more space.
And usually a program needs only one of the three compression functions, so it
would be a waste of progam space there.
*/
bool ImgTgaAppendCompress2Byte(const uint16_t * data, size_t elements, outputWriteFunc_t * pWriter, void * param) {
	bool success = true;
	size_t i = 0;
	while (i < elements) {
		uint16_t equals = 1;
		for (size_t j = i + 1; j < MIN(elements, i + 128); j++) {
			if (data[i] == data[j]) {
				equals++;
			} else {
				break;
			}
		}
		if (equals > 1) {
			//ok, add start a compression packet
			uint8_t rlp = 0x80 | (equals - 1); //run length packet
			success &= pWriter(&rlp, 1, param);
			success &= pWriter(data + i, sizeof(uint16_t), param);
			i += equals;
		} else {
			//add a raw packet
			uint16_t noEquals = 1;
			for (size_t j = i + 1; j < MIN(elements - 1, i + 128); j++) {
				if (data[j] != data[j + 1]) {
					noEquals++;
				} else {
					break;
				}
			}
			uint8_t rp = noEquals - 1;
			success &= pWriter(&rp, 1, param);
			success &= pWriter(data + i, noEquals * sizeof(uint16_t), param);
			i += noEquals;
		}
	}
	return success;
}

bool ImgTgaAppendCompress3Byte(const uint32_t * data, size_t elements, outputWriteFunc_t * pWriter, void * param) {
	bool success = true;
	size_t i = 0;
	while (i < elements) {
		uint16_t equals = 1;
		for (size_t j = i + 1; j < MIN(elements, i + 128); j++) {
			if (data[i] == data[j]) {
				equals++;
			} else {
				break;
			}
		}
		if (equals > 1) {
			//ok, add start a compression packet
			uint8_t rlp = 0x80 | (equals - 1); //run length packet
			success &= pWriter(&rlp, 1, param);
			success &= ImgTgaAppend32To24(data + i, 1, pWriter, param);
			i += equals;
		} else {
			//add a raw packet
			uint16_t noEquals = 1;
			for (size_t j = i + 1; j < MIN(elements - 1, i + 128); j++) {
				if (data[j] != data[j + 1]) {
					noEquals++;
				} else {
					break;
				}
			}
			uint8_t rp = noEquals - 1;
			success &= pWriter(&rp, 1, param);
			success &= ImgTgaAppend32To24(data + i, noEquals, pWriter, param);
			i += noEquals;
		}
	}
	return success;
}
