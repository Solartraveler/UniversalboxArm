
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "../imageTgaWrite.h"

const char * g_outputDir;


typedef struct outBuffer {
	uint8_t * memory;
	size_t written;
	size_t len;
} outData_t;

bool OutputWriteFunc(const void * buffer, size_t len, void * param) {
	outData_t * pOut = (outData_t *)param;
	size_t required = pOut->written + len;
	if (required > pOut->len) {
		pOut->memory = realloc(pOut->memory, required);
		if (!pOut->memory) {
			return false;
		}
		pOut->len = required;
	}
	memcpy(pOut->memory + pOut->written, buffer, len);
	pOut->written = required;
	return true;
}

bool OutputToFile(outData_t * pOut, const char * filename) {
	bool success = false;
	char buffer[512];
	if (g_outputDir) {
		snprintf(buffer, sizeof(buffer), "%s/%s", g_outputDir, filename);
	} else {
		snprintf(buffer, sizeof(buffer), "%s", filename);
	}
	FILE * f = fopen(buffer, "wb");
	if (f) {
		fwrite(pOut->memory, pOut->written, 1, f);
		fclose(f);
		success = true;
	}
	free(pOut->memory);
	pOut->memory = NULL;
	return success;
}

bool TestAppendLines1B(const uint8_t * data, size_t bytes, bool compressed, uint32_t repeats, outData_t * pOut) {
	bool success = true;
	for (uint32_t i = 0; i < repeats; i++) {
		if (compressed) {
			success &= ImgTgaAppendCompress1Byte(data, bytes, &OutputWriteFunc, pOut);
		} else {
			success &= ImgTgaAppendDirect(data, bytes, &OutputWriteFunc, pOut);
		}
	}
	return success;
}

bool TestAppendLines2B(const uint16_t * data, size_t elements, bool compressed, uint32_t repeats, outData_t * pOut) {
	bool success = true;
	for (uint32_t i = 0; i < repeats; i++) {
		if (compressed) {
			success &= ImgTgaAppendCompress2Byte(data, elements, &OutputWriteFunc, pOut);
		} else {
			success &= ImgTgaAppendDirect(data, elements * sizeof(uint16_t), &OutputWriteFunc, pOut);
		}
	}
	return success;
}

bool TestAppendLines4B(const uint32_t * data, size_t elements, bool compressed, uint32_t repeats, outData_t * pOut) {
	bool success = true;
	for (uint32_t i = 0; i < repeats; i++) {
		if (compressed) {
			success &= ImgTgaAppendCompress3Byte(data, elements, &OutputWriteFunc, pOut);
		} else {
			success &= ImgTgaAppend32To24(data, elements, &OutputWriteFunc, pOut);
		}
	}
	return success;
}

void TestMemset2B(uint16_t * data, uint16_t pattern, size_t elements) {
	for (size_t i = 0; i < elements; i++) {
		data[i] = pattern;
	}
}

void TestMemset4B(uint32_t * data, uint32_t pattern, size_t elements) {
	for (size_t i = 0; i < elements; i++) {
		data[i] = pattern;
	}
}

int TestGreyscaleDirect8bppX(bool compressed, const char * filename) {
	const uint32_t sizeX = 256;
	const uint32_t sizeY = 60;
	outData_t out;
	memset(&out, 0, sizeof(out));
	bool success = ImgTgaStart(sizeX, sizeY, 0, 8, compressed, &OutputWriteFunc, &out);
	//increment color gradient
	uint8_t colorLine[sizeX];
	for (uint32_t i = 0; i < sizeX; i++) {
		colorLine[i] = i;
	}
	success &= TestAppendLines1B(colorLine, sizeX, compressed, sizeY, &out);
	success &= OutputToFile(&out, filename);
	if (success) {
		return 0;
	}
	return 1;
}

int TestGreyscaleDirect8bpp(void) {
	return TestGreyscaleDirect8bppX(false, "testDirect08bpp----.tga");
}

int TestGreyscaleDirect8bppCompressed(void) {
	return TestGreyscaleDirect8bppX(true, "testDirect08bppComp.tga");
}


int TestRgbIndexed8WithX(uint8_t paletteSize, bool compressed, const char * filename) {
	const uint32_t sizeX = 256;
	const uint32_t sizeY = 60;
	outData_t out;
	memset(&out, 0, sizeof(out));
	bool success = ImgTgaStart(sizeX, sizeY, 8, paletteSize, compressed, &OutputWriteFunc, &out);
	if (paletteSize == 16) {
		success &= ImgTgaColormap16(1, 1, 1, &OutputWriteFunc, &out);
	} else {
		success &= ImgTgaColormap24(1, 1, 1, &OutputWriteFunc, &out);
	}
	uint8_t colorLine[sizeX];
	//1-3 line red
	memset(colorLine, 4, sizeX);
	success &= TestAppendLines1B(colorLine, sizeX, compressed, 10, &out);
	//4-6 line green
	memset(colorLine, 2, sizeX);
	success &= TestAppendLines1B(colorLine, sizeX, compressed, 10, &out);
	//7-9 line green
	memset(colorLine, 1, sizeX);
	success &= TestAppendLines1B(colorLine, sizeX, compressed, 10, &out);
	//10-12 line mixed
	for (uint32_t i = 0; i < sizeX; i++) {
		colorLine[i] = i & 0x7;
	}
	success &= TestAppendLines1B(colorLine, sizeX, compressed, 30, &out);
	success &= OutputToFile(&out, filename);
	if (success) {
		return 0;
	}
	return 1;
}

int TestRgbIndexed8With16(void) {
	return TestRgbIndexed8WithX(16, false, "testIndexed08with16bppA---.tga");
}

int TestRgbIndexed8With24(void) {
	return TestRgbIndexed8WithX(24, false, "testIndexed08with24bppA---.tga");
}

int TestRgbIndexed8With16Compressed(void) {
	return TestRgbIndexed8WithX(16, true, "testIndexed08with16bppComp.tga");
}

int TestRgbIndexed8With24Compressed(void) {
	return TestRgbIndexed8WithX(24, true, "testIndexed08with24bppComp.tga");
}

int TestRgbIndexed8WithXb(uint8_t paletteSize, bool compressed, const char * filename) {
	const uint32_t sizeX = 256;
	const uint32_t sizeY = 60;
	outData_t out;
	memset(&out, 0, sizeof(out));
	bool success = ImgTgaStart(sizeX, sizeY, 64, paletteSize, compressed, &OutputWriteFunc, &out);
	if (paletteSize == 16) {
		success &= ImgTgaColormap16(2, 2, 2, &OutputWriteFunc, &out);
	} else {
		success &= ImgTgaColormap24(2, 2, 2, &OutputWriteFunc, &out);
	}
	uint8_t colorLine[sizeX];
	//1-3 line red
	memset(colorLine, 3<<4, sizeX);
	success &= TestAppendLines1B(colorLine, sizeX, compressed, 10, &out);
	//4-6 line green
	memset(colorLine, 3<<2, sizeX);
	success &= TestAppendLines1B(colorLine, sizeX, compressed, 10, &out);
	//7-9 line green
	memset(colorLine, 3, sizeX);
	success &= TestAppendLines1B(colorLine, sizeX, compressed, 10, &out);
	//10-12 line mixed
	for (uint32_t i = 0; i < sizeX; i++) {
		colorLine[i] = i & 0x3F;
	}
	success &= TestAppendLines1B(colorLine, sizeX, compressed, 30, &out);
	success &= OutputToFile(&out, filename);
	if (success) {
		return 0;
	}
	return 1;
}

int TestRgbIndexed8With16b(void) {
	return TestRgbIndexed8WithXb(16, false, "testIndexed08with16bppB---.tga");
}

int TestRgbIndexed8With24b(void) {
	return TestRgbIndexed8WithXb(24, false, "testIndexed08with24bppB---.tga");
}

#if 0
//No program supports this mode...
int TestRgbIndexed12With24(void) {
	const uint32_t sizeX = 256;
	const uint32_t sizeY = 60;
	const uint32_t colors = 1<<12;
	outData_t out;
	memset(&out, 0, sizeof(out));
	bool success = ImgTgaStart(sizeX, sizeY, colors, 24, false, &OutputWriteFunc, &out);
	success &= ImgTgaColormap24(4, 4, 4, &OutputWriteFunc, &out);
	uint16_t colorLine[sizeX];
	//1-3 line red
	TestMemset2B(colorLine, 0xF << 8, sizeX);
	success &= TestAppendLines2B(colorLine, sizeX, false, 10, &out);
	//4-6 line green
	TestMemset2B(colorLine, 0xF << 4, sizeX);
	success &= TestAppendLines2B(colorLine, sizeX, false, 10, &out);
	//7-9 line blue
	TestMemset2B(colorLine, 0xF, sizeX);
	success &= TestAppendLines2B(colorLine, sizeX, false, 10, &out);
	//10. line
	for (uint32_t i = 0; i < sizeX; i++) {
		colorLine[i] = (i >> 4) << 8; //red
	}
	success &= TestAppendLines2B(colorLine, sizeX, false, 10, &out);
	//11. line
	for (uint32_t i = 0; i < sizeX; i++) {
		colorLine[i] = (i >> 4) << 4; //green
	}
	success &= TestAppendLines2B(colorLine, sizeX, false, 10, &out);
	//12. line
	for (uint32_t i = 0; i < sizeX; i++) {
		colorLine[i] = (i >> 4); //blue
	}
	success &= TestAppendLines2B(colorLine, sizeX, false, 10, &out);
	success &= OutputToFile(&out, "testIndexed12with24bpp----.tga");
	if (success) {
		return 0;
	}
	return 1;
}

#endif

int TestRgbDirect16bppX(bool compressed, const char * filename) {
	const uint32_t sizeX = 256;
	const uint32_t sizeY = 60;
	outData_t out;
	memset(&out, 0, sizeof(out));
	bool success = ImgTgaStart(sizeX, sizeY, 0, 16, compressed, &OutputWriteFunc, &out);
	uint16_t colorLine[sizeX];
	//1-3 line red
	TestMemset2B(colorLine, 0x1F << 10, sizeX);
	success &= TestAppendLines2B(colorLine, sizeX, compressed, 10, &out);
	//4-6 line green
	TestMemset2B(colorLine, 0x1F << 5, sizeX);
	success &= TestAppendLines2B(colorLine, sizeX, compressed, 10, &out);
	//7-9 line blue
	TestMemset2B(colorLine, 0x1F, sizeX);
	success &= TestAppendLines2B(colorLine, sizeX, compressed, 10, &out);
	//10. line
	for (uint32_t i = 0; i < sizeX; i++) {
		colorLine[i] = (i >> 3) << 10; //red
	}
	success &= TestAppendLines2B(colorLine, sizeX, compressed, 10, &out);
	//11. line
	for (uint32_t i = 0; i < sizeX; i++) {
		colorLine[i] = (i >> 3) << 5; //green
	}
	success &= TestAppendLines2B(colorLine, sizeX, compressed, 10, &out);
	//12. line
	for (uint32_t i = 0; i < sizeX; i++) {
		colorLine[i] = i >> 3; //blue
	}
	success &= TestAppendLines2B(colorLine, sizeX, compressed, 10, &out);
	success &= OutputToFile(&out, filename);
	if (success) {
		return 0;
	}
	return 1;
}

int TestRgbDirect16bpp(void) {
	return TestRgbDirect16bppX(false, "testDirect16bpp----.tga");
}

int TestRgbDirect16bppCompressed(void) {
	return TestRgbDirect16bppX(true, "testDirect16bppComp.tga");
}


int TestRgbDirect24bppX(bool compressed, const char * filename) {
	const uint32_t sizeX = 256;
	const uint32_t sizeY = 60;
	outData_t out;
	memset(&out, 0, sizeof(out));
	bool success = ImgTgaStart(sizeX, sizeY, 0, 24, compressed, &OutputWriteFunc, &out);
	uint32_t colorLine[sizeX];
	//1-3 line red
	TestMemset4B(colorLine, 0xFF << 16, sizeX);
	success &= TestAppendLines4B(colorLine, sizeX, compressed, 10, &out);
	//4-6 line green
	TestMemset4B(colorLine, 0xFF << 8, sizeX);
	success &= TestAppendLines4B(colorLine, sizeX, compressed, 10, &out);
	//7-9 line blue
	TestMemset4B(colorLine, 0xFF << 0, sizeX);
	success &= TestAppendLines4B(colorLine, sizeX, compressed, 10, &out);
	//10. line
	for (uint32_t i = 0; i < sizeX; i++) {
		colorLine[i] = i << 16; //red
	}
	success &= TestAppendLines4B(colorLine, sizeX, compressed, 10, &out);
	//11. line
	for (uint32_t i = 0; i < sizeX; i++) {
		colorLine[i] = i << 8; //green
	}
	success &= TestAppendLines4B(colorLine, sizeX, compressed, 10, &out);
	//12. line
	for (uint32_t i = 0; i < sizeX; i++) {
		colorLine[i] = i; //blue
	}
	success &= TestAppendLines4B(colorLine, sizeX, compressed, 10, &out);
	success &= OutputToFile(&out, filename);
	if (success) {
		return 0;
	}
	return 1;
}

int TestRgbDirect24bpp(void) {
	return TestRgbDirect24bppX(false, "testDirect24bpp----.tga");
}

int TestRgbDirect24bppCompressed(void) {
	return TestRgbDirect24bppX(true, "testDirect24bppComp.tga");
}


int main(int argc, char ** argv) {
	if (argc == 2) {
		g_outputDir = argv[1];
	}
	int error = 0;
	error |= TestGreyscaleDirect8bpp();
	error |= TestRgbDirect16bpp();
	error |= TestRgbDirect24bpp();
	error |= TestRgbIndexed8With16();
	error |= TestRgbIndexed8With16b();
	error |= TestRgbIndexed8With24();
	error |= TestRgbIndexed8With24b();
	//error |= TestRgbIndexed12With24();
	error |= TestGreyscaleDirect8bppCompressed();
	error |= TestRgbDirect16bppCompressed();
	error |= TestRgbDirect24bppCompressed();
	error |= TestRgbIndexed8With16Compressed();
	error |= TestRgbIndexed8With24Compressed();
	return error;
}
