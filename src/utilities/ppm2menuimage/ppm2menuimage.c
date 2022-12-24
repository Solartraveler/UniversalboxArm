/* ppm2menuimage
(c) 2022 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause

Converts binary ppm images to 3bit per pixel (R, G, B) menu image graphics.
These can be compressed or uncompressed.

Known to work with ppm images generated by GIMP when choosing "Raw" and when
saving with KolourPaint.
Currently limited to images of up to 1MiB in size.
*/

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

typedef struct {
	const char * inputFilename;
	const char * outputFilename;
	bool compress;
	bool help;
	uint8_t redBits;
	uint8_t greenBits;
	uint8_t blueBits;
} settings_t;

size_t getLine(FILE * f, char * out, size_t outMax) {
	size_t i = 0;
	bool comment = false;
	while (i < (outMax - 1)) {
		char c = 0;

		size_t r = fread(&c, 1, 1, f);
		if (r != 1) {
			break;
		}
		if ((c == '#') && (i == 0)) {
			comment = true;
		}
		if ((c == '\n') || ((comment == false) && ((c == ' ') || (c == '\t') || (c == '\r')))) {
			if (i != 0) {
				break;
			}
		} else {
			out[i] = c;
			i++;
		}
	}
	out[i] = '\0';
	return i;
}

void ParamParse(int argc, char ** argv, settings_t * pS) {
	for (int i = 1; i < argc; i++) {
		if ((strcmp(argv[i], "--input") == 0) && ((i + 1) < argc)) {
			pS->inputFilename = argv[i + 1];
			i++;
		}
		if ((strcmp(argv[i], "--output") == 0) && ((i + 1) < argc)) {
			pS->outputFilename = argv[i + 1];
			i++;
		}
		if (strcmp(argv[i], "--compress") == 0) {
			pS->compress = true;
		}
		if (strcmp(argv[i], "--help") == 0) {
			pS->help = true;
		}
		if ((strcmp(argv[i], "--red") == 0) && ((i + 1) < argc)) {
			pS->redBits = atoi(argv[i + 1]);
			i++;
		}
		if ((strcmp(argv[i], "--green") == 0) && ((i + 1) < argc)) {
			pS->greenBits = atoi(argv[i + 1]);
			i++;
		}
		if ((strcmp(argv[i], "--blue") == 0) && ((i + 1) < argc)) {
			pS->blueBits = atoi(argv[i + 1]);
			i++;
		}


	}
}

int main(int argc, char ** argv) {
	settings_t s = {0};
	s.redBits = 1;
	s.greenBits = 1;
	s.blueBits = 1;
	ParamParse(argc, argv, &s);
	if (s.help) {
		printf("ppm2menuimage RGB converter\n");
		printf("(c) 2022 by Malte Marwedel, Version 1.1\n");
		printf("Give:\n  --input <ppmFilename>\n");
		printf("  --output <menuimageFilename>\n");
		printf("  --compress for compressed output\n");
		printf("  --red <bits> number of red output bits, default 1\n");
		printf("  --green <bits> number of green output bits, default 1\n");
		printf("  --blue <bits> number of blue output bits, default 1\n");
		printf("  The sum of red green and blue bits may not be larger than 8\n");
		return 0;
	}
	if ((!s.inputFilename) || (!s.outputFilename)) {
		printf("Error: Invalid parameters.\n");
		return 1;
	}
	uint8_t redBits = s.redBits;
	uint8_t greenBits = s.greenBits;
	uint8_t blueBits = s.blueBits;
	uint8_t colorBits = redBits + greenBits + blueBits;
	if (colorBits > 8) {
		printf("Error, this converter does not support more than 8 color bits\n");
		return 1;
	}
	unsigned int sx, sy, maxCol;
	FILE * f1 = fopen(s.inputFilename, "rb");
	if (!f1) {
		printf("Error, could not open input %s\n", s.inputFilename);
		return 1;
	}
	size_t inDataMax = 1024*1024;
	size_t inDataI = 0;
	uint8_t * inData = (uint8_t*)malloc(inDataMax);

	const size_t outDataMax = 1024*1024;
	size_t outDataI = 0;
	uint8_t * outData = (uint8_t*)malloc(outDataMax);
	if ((!outData) || (!inData)) {
		return 2;
	}
	char buffer[128];
	uint32_t mode = 0;
	while (mode <= 3) {
		if (getLine(f1, buffer, sizeof(buffer)) == 0) {
			printf("Error, input file too small\n");
			return 1;
		}
		if (buffer[0] != '#') {
			if (mode == 0) {
				if (strcmp(buffer, "P6")) {
					printf("Error, only P6 format is supported, this is >%s<\n", buffer);
					return 1;
				} else {
					mode = 1;
				}
			} else if (mode == 1) {
				if (sscanf(buffer, "%u", &sx) != 1) {
					printf("Error, could not get width from %s\n", buffer);
				} else {
					mode = 2;
				}
			} else if (mode == 2) {
				if (sscanf(buffer, "%u", &sy) != 1) {
					printf("Error, could not get height from %s\n", buffer);
				} else {
					mode = 3;
				}
			} else if (mode == 3) {
				if (sscanf(buffer, "%u", &maxCol) != 1) {
					printf("Error, could not get maximum color value from %s\n", buffer);
				} else {
					mode = 4;
				}
			}
		}
	}
	int num = fread(inData, 1, inDataMax, f1);
	if (num <= 0) {
		printf("Error, not enough data in file\n");
		return 1;
	}
	fclose(f1);
	inDataMax = num;
	if (inDataMax % 3 != 0) {
		printf("Error, image data is %u bytes, must be multiple of 3 for RGB\n", (unsigned int)inDataMax);
		return 1;
	}
	//convert
	uint8_t colorLast = 0;
	uint16_t lastRepeats = 0;
	uint8_t redBitsMissing = 8 - redBits;
	uint8_t greenBitsMissing = 8 - greenBits;
	uint8_t blueBitsMissing = 8 - blueBits;
	uint8_t compressionBits = 8 - colorBits;
	if (compressionBits == 0) {
		compressionBits = 8; //extra byte added
	}
	uint16_t supportedRepeats = 1 << compressionBits;

	for (inDataI = 0; inDataI < inDataMax; inDataI += 3) {
		uint8_t r = inData[inDataI] >> redBitsMissing;
		uint8_t g = inData[inDataI + 1] >> greenBitsMissing;
		uint8_t b = inData[inDataI + 2] >> blueBitsMissing;
		uint8_t color = (r << (blueBits + greenBits)) | (g << blueBits) | b;
		if ((color != colorLast) || (lastRepeats == supportedRepeats) || (s.compress == false)) {
			if (lastRepeats) {
				if (compressionBits != 8) {
					outData[outDataI] = colorLast | ((lastRepeats - 1) << colorBits);
					outDataI++;
				} else {
					outData[outDataI] = colorLast;
					outDataI++;
					outData[outDataI] = lastRepeats - 1;
					outDataI++;
				}
			}
			colorLast = color;
			lastRepeats = 1;
		} else {
			lastRepeats++;
		}
	}
	if (lastRepeats) {
		if (compressionBits != 8) {
			outData[outDataI] = colorLast | ((lastRepeats - 1) << colorBits);
			outDataI++;
		} else {
			outData[outDataI] = colorLast;
			outDataI++;
			outData[outDataI] = lastRepeats - 1;
			outDataI++;
		}
	}
	free(inData);
	//write output
	FILE * f2 = fopen(s.outputFilename, "wb");
	if (!f2) {
		printf("Error, could not open output %s\n", s.outputFilename);
		return 1;
	}
	if (fwrite(outData, 1, outDataI, f2) != outDataI) {
		printf("Error, writing output data failed\n");
		return 1;
	}
	fclose(f2);
	free(outData);
	return 0;
}
