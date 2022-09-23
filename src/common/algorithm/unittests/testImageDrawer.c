/*
(c) 2022 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause
*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../imageDrawer.h"
#include "../imageDrawerHighres.h"
#include "../imageDrawerLowres.h"
#include "../utility.h"

#ifdef TESTHIGHRES

#define ImgOrderPixelsGeneric ImgOrderPixelsHighres
#define ImgCompressed1ByteGeneric ImgCompressed1ByteHighres
#define ImgInterpolateToPixelsGeneric ImgInterpolateToPixelsHighres
#define ImgCreateLineGfx1BitGeneric ImgCreateLineGfx1BitHighres
#define ImgCreateLinesGfxGeneric ImgCreateLinesGfxHighres

typedef Img1BytePixelHighres_t Img1BytePixelGeneric_t;

#endif

#ifdef TESTLOWRES

#define ImgOrderPixelsGeneric ImgOrderPixelsLowres
#define ImgCompressed1ByteGeneric ImgCompressed1ByteLowres
#define ImgInterpolateToPixelsGeneric ImgInterpolateToPixelsLowres
#define ImgCreateLineGfx1BitGeneric ImgCreateLineGfx1BitLowres
#define ImgCreateLinesGfxGeneric ImgCreateLinesGfxLowres

typedef Img1BytePixelLowres_t Img1BytePixelGeneric_t;

#endif

#if !defined(TESTHIGHRES) && !defined(TESTLOWRES)
#error "TESTHIGHRES or TESTLOWRES must be defined while compiling"
#endif

#define TASSH(X, Y) if (!(X)) {return Y;}


int Empty20x10x1xA(void) {
	//input
	uint8_t bgColor = 1;
	uint8_t bits = 1;
	uint16_t x = 20;
	uint16_t y = 10;
	//test function
	uint8_t buffer[8];
	size_t lenUsed;
	size_t used = 0;
	uint8_t result = ImgCompressed1ByteGeneric(x, y, bits, buffer, sizeof(buffer), bgColor, NULL, 0, &used);
	//compare result
	uint8_t expected[2] = {0xFF, 0x8F};
	TASSH(result == 0, 1);
	TASSH(used == 2, 2);
	TASSH(memcmp(buffer, expected, 2) == 0, 3);
	return 0;
}

int Empty20x10x1xB(void) {
	//input
	uint8_t bgColor = 0;
	uint8_t bits = 1;
	uint16_t x = 20;
	uint16_t y = 10;
	//test function
	uint8_t buffer[8] = {0};
	size_t lenUsed;
	size_t used = 0;
	uint8_t result = ImgCompressed1ByteGeneric(x, y, bits, buffer, sizeof(buffer), bgColor, NULL, 0, &used);
	//compare result
	uint8_t expected[2] = {0xFE, 0x8E};
	TASSH(result == 0, 1);
	TASSH(used == 2, 2);
	TASSH(memcmp(buffer, expected, used) == 0, 3);
	return 0;
}

int Empty20x10x2xA(void) {
	//input
	uint8_t bgColor = 2;
	uint8_t bits = 2;
	uint16_t x = 20;
	uint16_t y = 10;
	//test function
	uint8_t buffer[8] = {0};
	size_t lenUsed;
	size_t used = 0;
	uint8_t result = ImgCompressed1ByteGeneric(x, y, bits, buffer, sizeof(buffer), bgColor, NULL, 0, &used);
	//compare result
	uint8_t expected[4] = {0xFE, 0xFE, 0xFE, 0x1E};
	TASSH(result == 0, 1);
	TASSH(used == 4, 2);
	TASSH(memcmp(buffer, expected, used) == 0, 3);
	return 0;
}

int Empty20x10x8xA(void) {
	//input
	uint8_t bgColor = 0x65;
	uint8_t bits = 8;
	uint16_t x = 20;
	uint16_t y = 10;
	//test function
	uint8_t buffer[200] = {0};
	size_t lenUsed;
	size_t used = 0;
	uint8_t result = ImgCompressed1ByteGeneric(x, y, bits, buffer, sizeof(buffer), bgColor, NULL, 0, &used);
	//compare result
	uint8_t expected[200];
	memset(expected, bgColor, sizeof(expected));
	TASSH(result == 0, 1);
	TASSH(used == 200, 2);
	TASSH(memcmp(buffer, expected, used) == 0, 3);
	return 0;
}

int Empty20x10x8xTooSmall(void) {
	//input
	uint8_t bgColor = 0x65;
	uint8_t bits = 8;
	uint16_t x = 20;
	uint16_t y = 10;
	//test function
	uint8_t buffer[199] = {0};
	size_t lenUsed;
	size_t used = 0;
	uint8_t result = ImgCompressed1ByteGeneric(x, y, bits, buffer, sizeof(buffer), bgColor, NULL, 0, &used);
	//compare result
	TASSH(result == 2, 1);
	TASSH(used == 199, 2);
	return 0;
}

//one pixel at the beginning
int Pixels10x5x4xA(void) {
	//input
	uint8_t bgColor = 0x01;
	uint8_t bits = 4;
	uint16_t x = 10;
	uint16_t y = 5;
	Img1BytePixelGeneric_t pixels[1] = {0, 0, 0x4};
	size_t numPixels = sizeof(pixels)/sizeof(Img1BytePixelGeneric_t);
	//test function
	uint8_t buffer[50] = {0};
	size_t lenUsed;
	size_t used = 0;
	uint8_t result = ImgCompressed1ByteGeneric(x, y, bits, buffer, sizeof(buffer), bgColor, pixels, numPixels, &used);
	//compare result
	uint8_t expected[50] = {0x4, 0xF1, 0xF1, 0xF1, 0x1};
	TASSH(result == 0, 1);
	TASSH(used == 5, 2);
	TASSH(memcmp(buffer, expected, used) == 0, 3);
	return 0;
}

//one pixel not at the beginning
int Pixels10x5x4xB(void) {
	//input
	uint8_t bgColor = 0x01;
	uint8_t bits = 4;
	uint16_t x = 10;
	uint16_t y = 5;
	Img1BytePixelGeneric_t pixels[1] = {{1, 0, 0x4}};
	size_t numPixels = sizeof(pixels)/sizeof(Img1BytePixelGeneric_t);
	//test function
	uint8_t buffer[50] = {0};
	size_t lenUsed;
	size_t used = 0;
	uint8_t result = ImgCompressed1ByteGeneric(x, y, bits, buffer, sizeof(buffer), bgColor, pixels, numPixels, &used);
	//compare result
	uint8_t expected[50] = {0x01, 0x4, 0xF1, 0xF1, 0xF1};
	TASSH(result == 0, 1);
	TASSH(used == 5, 2);
	TASSH(memcmp(buffer, expected, used) == 0, 3);
	return 0;
}

//two pixels, need merge
int Pixels10x5x4xC(void) {
	//input
	uint8_t bgColor = 0x01;
	uint8_t bits = 4;
	uint16_t x = 10;
	uint16_t y = 5;
	Img1BytePixelGeneric_t pixels[2] = {{1, 0, 0x4}, {2, 0, 0x4}};
	size_t numPixels = sizeof(pixels)/sizeof(Img1BytePixelGeneric_t);
	//test function
	uint8_t buffer[50] = {0};
	size_t lenUsed;
	size_t used = 0;
	uint8_t result = ImgCompressed1ByteGeneric(x, y, bits, buffer, sizeof(buffer), bgColor, pixels, numPixels, &used);
	//compare result
	uint8_t expected[50] = {0x01, 0x14, 0xF1, 0xF1, 0xE1};
	TASSH(result == 0, 1);
	TASSH(used == 5, 2);
	TASSH(memcmp(buffer, expected, used) == 0, 3);
	return 0;
}

//two pixels, can not merge
int Pixels10x5x4xD(void) {
	//input
	uint8_t bgColor = 0x01;
	uint8_t bits = 4;
	uint16_t x = 10;
	uint16_t y = 5;
	Img1BytePixelGeneric_t pixels[2] = {{1, 0, 0x4}, {2, 0, 0x5}};
	size_t numPixels = sizeof(pixels)/sizeof(Img1BytePixelGeneric_t);
	//test function
	uint8_t buffer[50] = {0};
	size_t lenUsed;
	size_t used = 0;
	uint8_t result = ImgCompressed1ByteGeneric(x, y, bits, buffer, sizeof(buffer), bgColor, pixels, numPixels, &used);
	//compare result
	uint8_t expected[50] = {0x01, 0x04, 0x05, 0xF1, 0xF1, 0xE1};
	TASSH(result == 0, 1);
	TASSH(used == 6, 2);
	TASSH(memcmp(buffer, expected, used) == 0, 3);
	return 0;
}

//three pixels, can not merge
int Pixels10x5x4xE(void) {
	//input
	uint8_t bgColor = 0x01;
	uint8_t bits = 4;
	uint16_t x = 10;
	uint16_t y = 5;
	Img1BytePixelGeneric_t pixels[3] = {{1, 0, 0x4}, {2, 0, 0x5}, {0, 1, 0x6}};
	size_t numPixels = sizeof(pixels)/sizeof(Img1BytePixelGeneric_t);
	//test function
	uint8_t buffer[50] = {0};
	size_t lenUsed;
	size_t used = 0;
	uint8_t result = ImgCompressed1ByteGeneric(x, y, bits, buffer, sizeof(buffer), bgColor, pixels, numPixels, &used);
	//compare result
	uint8_t expected[50] = {0x01, 0x04, 0x05, 0x61, 0x6, 0xF1, 0xF1, 0x61};
	TASSH(result == 0, 1);
	TASSH(used == 8, 2);
	TASSH(memcmp(buffer, expected, used) == 0, 3);
	return 0;
}

//three pixels, can not merge, need to be sorted first
int Pixels10x5x4xF(void) {
	//input
	uint8_t bgColor = 0x01;
	uint8_t bits = 4;
	uint16_t x = 10;
	uint16_t y = 5;
	Img1BytePixelGeneric_t pixels[3] = {{0, 1, 0x6}, {2, 0, 0x5}, {1, 0, 0x4}};
	size_t numPixels = sizeof(pixels)/sizeof(Img1BytePixelGeneric_t);
	//test function
	uint8_t buffer[50] = {0};
	size_t lenUsed;
	size_t used = 0;
	uint8_t result = ImgCompressed1ByteGeneric(x, y, bits, buffer, sizeof(buffer), bgColor, pixels, numPixels, &used);
	//compare result
	uint8_t expected[50] = {0x01, 0x04, 0x05, 0x61, 0x6, 0xF1, 0xF1, 0x61};
	TASSH(result == 0, 1);
	TASSH(used == 8, 2);
	TASSH(memcmp(buffer, expected, used) == 0, 3);
	return 0;
}

//three pixels, one can be canceled out because it has the background color
int Pixels10x5x4xG(void) {
	//input
	uint8_t bgColor = 0x01;
	uint8_t bits = 4;
	uint16_t x = 10;
	uint16_t y = 5;
	Img1BytePixelGeneric_t pixels[3] = {{1, 0, 0x4}, {2, 0, 0x5}, {0, 1, 0x1}};
	size_t numPixels = sizeof(pixels)/sizeof(Img1BytePixelGeneric_t);
	//test function
	uint8_t buffer[50] = {0};
	size_t lenUsed;
	size_t used = 0;
	uint8_t result = ImgCompressed1ByteGeneric(x, y, bits, buffer, sizeof(buffer), bgColor, pixels, numPixels, &used);
	//compare result
	uint8_t expected[50] = {0x01, 0x04, 0x05, 0xF1, 0xF1, 0xE1};
	TASSH(result == 0, 1);
	TASSH(used == 6, 2);
	TASSH(memcmp(buffer, expected, used) == 0, 3);
	return 0;
}

//nothing to do...
int Scale1ManualA(void) {
	uint16_t data[1] = {10};
	uint16_t rangeOut = 20;
	uint16_t rangeMin = 0;
	uint16_t rangeMax = 20;
	//test function
	ImgScale2Byte(rangeOut, data, sizeof(data)/sizeof(uint16_t), rangeMin, rangeMax, false, NULL, NULL);
	//compare result
	TASSH(data[0] == 10, 1);
	return 0;
}

//scale to half the size
int Scale1ManualB(void) {
	uint16_t data[1] = {10};
	uint16_t rangeOut = 10;
	uint16_t rangeMin = 0;
	uint16_t rangeMax = 20;
	//test function
	uint16_t outMin = 0;
	uint16_t outMax = 0;
	ImgScale2Byte(rangeOut, data, sizeof(data)/sizeof(uint16_t), rangeMin, rangeMax, false, &outMin, &outMax);
	//compare result
	TASSH(data[0] == 5, 1);
	TASSH(outMin == 5, 2);
	TASSH(outMax == 5, 3);
	return 0;
}

//scale to twiche the size
int Scale1ManualC(void) {
	uint16_t data[1] = {10};
	uint16_t rangeOut = 40;
	uint16_t rangeMin = 0;
	uint16_t rangeMax = 20;
	//test function
	uint16_t outMin = 0;
	uint16_t outMax = 0;
	ImgScale2Byte(rangeOut, data, sizeof(data)/sizeof(uint16_t), rangeMin, rangeMax, false, &outMin, &outMax);
	//compare result
	TASSH(data[0] == 20, 1);
	TASSH(outMin == 20, 2);
	TASSH(outMax == 20, 3);
	return 0;
}

//scale to twiche the size, 4 data points
int Scale1ManualD(void) {
	uint16_t data[4] = {5, 10, 20, 10};
	uint16_t rangeOut = 40;
	uint16_t rangeMin = 0;
	uint16_t rangeMax = 20;
	//test function
	uint16_t outMin = 0;
	uint16_t outMax = 0;
	ImgScale2Byte(rangeOut, data, sizeof(data)/sizeof(uint16_t), rangeMin, rangeMax, false, &outMin, &outMax);
	//compare result
	TASSH(data[0] == 10, 1);
	TASSH(data[1] == 20, 2);
	TASSH(data[2] == 40, 3);
	TASSH(data[3] == 20, 4);
	TASSH(outMin == 10, 5);
	TASSH(outMax == 40, 6);
	return 0;
}

//scale to twiche the size, 4 data points, crop the too high and low ones away
int Scale1ManualE(void) {
	uint16_t data[4] = {5, 10, 20, 25};
	uint16_t rangeOut = 40;
	uint16_t rangeMin = 10;
	uint16_t rangeMax = 20;
	//test function
	uint16_t outMin = 0;
	uint16_t outMax = 0;
	ImgScale2Byte(rangeOut, data, sizeof(data)/sizeof(uint16_t), rangeMin, rangeMax, false, &outMin, &outMax);
	//compare result
	TASSH(data[0] == 0, 1);
	TASSH(data[1] == 0, 2);
	TASSH(data[2] == 40, 3);
	TASSH(data[3] == 40, 4);
	TASSH(outMin == 0, 5);
	TASSH(outMax == 40, 6);
	return 0;
}

//scale to twiche the size, 4 data points
int Scale1AutoA(void) {
	uint16_t data[4] = {5, 10, 20, 11};
	uint16_t rangeOut = 40;
	uint16_t rangeMin = 0;
	uint16_t rangeMax = 0;
	//test function
	uint16_t outMin = 0;
	uint16_t outMax = 0;
	ImgScale2Byte(rangeOut, data, sizeof(data)/sizeof(uint16_t), rangeMin, rangeMax, false, &outMin, &outMax);
	//compare result
	TASSH(data[0] == 0, 1);
	TASSH(data[1] == 13, 2); //properly rounded down
	TASSH(data[2] == 40, 3);
	TASSH(data[3] == 16, 4); //properly rounded up
	TASSH(outMin == 0, 5);
	TASSH(outMax == 40, 6);
	return 0;
}

//scale to twiche the size, 4 data points, and flip
int Scale1AutoB(void) {
	uint16_t data[4] = {5, 10, 20, 11};
	uint16_t rangeOut = 40;
	uint16_t rangeMin = 0;
	uint16_t rangeMax = 0;
	//test function
	uint16_t outMin = 0;
	uint16_t outMax = 0;
	ImgScale2Byte(rangeOut, data, sizeof(data)/sizeof(uint16_t), rangeMin, rangeMax, true, &outMin, &outMax);
	//compare result
	TASSH(data[0] == 40, 1);
	TASSH(data[1] == 27, 2);
	TASSH(data[2] == 0, 3);
	TASSH(data[3] == 24, 4);
	TASSH(outMin == 0, 5);
	TASSH(outMax == 40, 6);
	return 0;
}

//impossible to scale, as it is just a line. should end in the middle
int Scale1AutoC(void) {
	uint16_t data[2] = {5, 5};
	uint16_t rangeOut = 4;
	uint16_t rangeMin = 0;
	uint16_t rangeMax = 0;
	//test function
	uint16_t outMin = 0;
	uint16_t outMax = 0;
	ImgScale2Byte(rangeOut, data, sizeof(data)/sizeof(uint16_t), rangeMin, rangeMax, false, &outMin, &outMax);
	//compare result
	TASSH(data[0] == 2, 1); //0...4 -> middle is 2
	TASSH(data[1] == 2, 2);
	TASSH(outMin == 2, 3);
	TASSH(outMax == 2, 4);
	return 0;
}


//horizontal line
int InterpolateA(void) {
	uint16_t start = 5;
	uint16_t end = 5;
	//test function
	uint16_t data[5] = {0};
	ImgInterpolateSingleLine(start, end, data, sizeof(data)/sizeof(uint16_t));
	//compare result
	TASSH(data[0] == 5, 1);
	TASSH(data[1] == 5, 2);
	TASSH(data[2] == 5, 3);
	TASSH(data[3] == 5, 4);
	TASSH(data[4] == 5, 5);
	return 0;
}

//just two pixel, basically reproducing the input
int InterpolateB(void) {
	uint16_t start = 5;
	uint16_t end = 6;
	//test function
	uint16_t data[2] = {0};
	ImgInterpolateSingleLine(start, end, data, sizeof(data)/sizeof(uint16_t));
	//compare result
	TASSH(data[0] == 5, 1);
	TASSH(data[1] == 6, 2);
	return 0;
}

//line going up with an elevation of ~0.555
int InterpolateC(void) {
	uint16_t start = 5;
	uint16_t end = 10;
	//test function
	uint16_t data[10] = {0};
	ImgInterpolateSingleLine(start, end, data, sizeof(data)/sizeof(uint16_t));
	//compare result
	TASSH(data[0] == 5, 1);
	TASSH(data[1] == 6, 2);
	TASSH(data[2] == 6, 3);
	TASSH(data[3] == 7, 4);
	TASSH(data[4] == 7, 5);
	TASSH(data[5] == 8, 6);
	TASSH(data[6] == 8, 7);
	TASSH(data[7] == 9, 8);
	TASSH(data[8] == 9, 9);
	TASSH(data[9] == 10, 10);
	return 0;
}

//line going down with an elevation of 2.5
int InterpolateD(void) {
	uint16_t start = 20;
	uint16_t end = 10;
	//test function
	uint16_t data[5] = {0};
	ImgInterpolateSingleLine(start, end, data, sizeof(data)/sizeof(uint16_t));
	//compare result
	TASSH(data[0] == 20, 1);
	TASSH(data[1] == 18, 2);
	TASSH(data[2] == 15, 3);
	TASSH(data[3] == 13, 4);
	TASSH(data[4] == 10, 5);
	return 0;
}

//condense two points
int InterpolateE(void) {
	uint16_t start = 20;
	uint16_t end = 10;
	//test function
	uint16_t data[1] = {0};
	ImgInterpolateSingleLine(start, end, data, sizeof(data)/sizeof(uint16_t));
	//compare result
	TASSH(data[0] == 15, 1);
	return 0;
}

//just two pixel, basically reproducing the input
int InterpolateLinesA(void) {
	uint16_t dataIn[2] = {5, 6};
	//test function
	uint16_t data[2] = {0};
	ImgInterpolateLines(dataIn, sizeof(dataIn)/sizeof(uint16_t), data, sizeof(data)/sizeof(uint16_t));
	//compare result
	TASSH(data[0] == 5, 1);
	TASSH(data[1] == 6, 2);
	return 0;
}

//create 3 pixel out of 2
int InterpolateLinesB(void) {
	uint16_t dataIn[2] = {5, 7};
	//test function
	uint16_t data[3] = {0};
	ImgInterpolateLines(dataIn, sizeof(dataIn)/sizeof(uint16_t), data, sizeof(data)/sizeof(uint16_t));
	//compare result
	TASSH(data[0] == 5, 1);
	TASSH(data[1] == 6, 2);
	TASSH(data[2] == 7, 3);
	return 0;
}


//create 5 pixel out of 3
int InterpolateLinesC(void) {
	uint16_t dataIn[3] = {5, 7, 5};
	//test function
	uint16_t data[5] = {0};
	ImgInterpolateLines(dataIn, sizeof(dataIn)/sizeof(uint16_t), data, sizeof(data)/sizeof(uint16_t));
	//compare result
	TASSH(data[0] == 5, 1);
	TASSH(data[1] == 6, 2);
	TASSH(data[2] == 7, 3);
	TASSH(data[3] == 6, 4);
	TASSH(data[4] == 5, 5);
	return 0;
}

//create 7 pixel out of 4
int InterpolateLinesD(void) {
	uint16_t dataIn[4] = {5, 7, 5, 7};
	//test function
	uint16_t data[7] = {0};
	ImgInterpolateLines(dataIn, sizeof(dataIn)/sizeof(uint16_t), data, sizeof(data)/sizeof(uint16_t));
	//compare result
	TASSH(data[0] == 5, 1);
	TASSH(data[1] == 6, 2);
	TASSH(data[2] == 7, 3);
	TASSH(data[3] == 6, 4);
	TASSH(data[4] == 5, 5);
	TASSH(data[5] == 6, 6);
	TASSH(data[6] == 7, 7);
	return 0;
}

//create 9 pixel out of 5
int InterpolateLinesE(void) {
	uint16_t dataIn[5] = {5, 7, 5, 7, 5};
	//test function
	uint16_t data[9] = {0};
	ImgInterpolateLines(dataIn, sizeof(dataIn)/sizeof(uint16_t), data, sizeof(data)/sizeof(uint16_t));
	//compare result
	TASSH(data[0] == 5, 1);
	TASSH(data[1] == 6, 2);
	TASSH(data[2] == 7, 3);
	TASSH(data[3] == 6, 4);
	TASSH(data[4] == 5, 5);
	TASSH(data[5] == 6, 6);
	TASSH(data[6] == 7, 7);
	TASSH(data[7] == 6, 8);
	TASSH(data[8] == 5, 9);
	return 0;
}

//create 9 pixel out of 1
int InterpolateLinesF(void) {
	uint16_t dataIn[1] = {5};
	//test function
	uint16_t data[9] = {0};
	ImgInterpolateLines(dataIn, sizeof(dataIn)/sizeof(uint16_t), data, sizeof(data)/sizeof(uint16_t));
	//compare result
	TASSH(data[0] == 5, 1);
	TASSH(data[1] == 5, 2);
	TASSH(data[2] == 5, 3);
	TASSH(data[3] == 5, 4);
	TASSH(data[4] == 5, 5);
	TASSH(data[5] == 5, 6);
	TASSH(data[6] == 5, 7);
	TASSH(data[7] == 5, 8);
	TASSH(data[8] == 5, 9);
	return 0;
}

//create 9 pixel out of 0
int InterpolateLinesG(void) {
	//test function
	uint16_t data[9];
	memset(data, 1, sizeof(data)); //not start with the desired output...
	ImgInterpolateLines(NULL, 0, data, sizeof(data)/sizeof(uint16_t));
	//compare result
	TASSH(data[0] == 0, 1);
	TASSH(data[1] == 0, 2);
	TASSH(data[2] == 0, 3);
	TASSH(data[3] == 0, 4);
	TASSH(data[4] == 0, 5);
	TASSH(data[5] == 0, 6);
	TASSH(data[6] == 0, 7);
	TASSH(data[7] == 0, 8);
	TASSH(data[8] == 0, 9);
	return 0;
}


//create horizontal bar of 3 pixels
int InterpolatePixelsA(void) {
	uint16_t dataIn[3] = {5, 5, 5};
	uint8_t color = 0x12;
	//test function
	Img1BytePixelGeneric_t pixels[5] = {0};
	uint16_t numPixels = ImgInterpolateToPixelsGeneric(dataIn, sizeof(dataIn)/sizeof(uint16_t), pixels, sizeof(pixels)/sizeof(Img1BytePixelGeneric_t), color);
	//compare result
	TASSH(numPixels == 3, 1);
	for (uint16_t i = 0; i < numPixels; i++) {
		TASSH(pixels[i].x == i, 100 * i + 101);
		TASSH(pixels[i].y == 5, 100 * i + 102);
		TASSH(pixels[i].color == color, 100 * i + 103);
	}
	return 0;
}

//line down
int InterpolatePixelsB(void) {
	uint16_t dataIn[2] = {0, 5};
	uint8_t color = 0x12;
	//test function
	Img1BytePixelGeneric_t pixels[20] = {0};
	uint16_t numPixels = ImgInterpolateToPixelsGeneric(dataIn, sizeof(dataIn)/sizeof(uint16_t), pixels, sizeof(pixels)/sizeof(Img1BytePixelGeneric_t), color);
	//compare result
	TASSH(numPixels == 6, 1);
	for (uint16_t i = 0; i < 5; i++) {
		TASSH(pixels[i].x == 0,     100 * i + 101);
		TASSH(pixels[i].y == i, 100 * i + 102);
		TASSH(pixels[i].color == color, 100 * i + 103);
	}
	TASSH(pixels[5].x == 1, 1001);
	TASSH(pixels[5].y == 5, 1002);
	TASSH(pixels[5].color == color, 1003);
	return 0;
}


//rectangle
int InterpolatePixelsC(void) {
	uint16_t dataIn[3] = {5, 0, 0};
	uint8_t color = 0x12;
	//test function
	Img1BytePixelGeneric_t pixels[20] = {0};
	uint16_t numPixels = ImgInterpolateToPixelsGeneric(dataIn, sizeof(dataIn)/sizeof(uint16_t), pixels, sizeof(pixels)/sizeof(Img1BytePixelGeneric_t), color);
	//compare result
	TASSH(numPixels == 7, 1);
	for (uint16_t i = 0; i < 5; i++) {
		TASSH(pixels[i].x == 0,     100 * i + 101);
		TASSH(pixels[i].y == 5 - i, 100 * i + 102);
		TASSH(pixels[i].color == color, 100 * i + 103);
	}
	TASSH(pixels[5].x == 1, 1001);
	TASSH(pixels[5].y == 0, 1002);
	TASSH(pixels[5].color == color, 1003);
	TASSH(pixels[6].x == 2, 2001);
	TASSH(pixels[6].y == 0, 2002);
	TASSH(pixels[6].color == color, 2003);
	return 0;
}

void PlotGfx1Bit(const uint8_t * data, uint16_t x, uint16_t y) {
	uint16_t rptr = 0;
	uint8_t color = 0;
	uint8_t left = 0;
	for (uint16_t j = 0; j < y; j++) {
		for (uint16_t i = 0; i < x; i++) {
			if (left == 0) {
				uint8_t in = data[rptr];
				rptr++;
				left = (in >> 1) + 1;
				color = in & 1;
			}
			if (color == 0) {
				printf("#");
			} else {
				printf(".");
			}
			left--;
		}
		printf("\n");
	}
}

//get a 10x5 pixel graphic out of 3 data points, zero is up
int CreateLineGfx1BitA(void) {
	uint16_t dataIn[3] = {10, 20, 10};
	uint16_t x = 10;
	uint16_t y = 5;
	//test function
	uint8_t dataGraphic[50] = {0};
	size_t dataUsed = 0;
	uint8_t result = ImgCreateLineGfx1BitGeneric(dataIn, sizeof(dataIn)/sizeof(uint16_t), x, y, false,
	                   dataGraphic, sizeof(dataGraphic), &dataUsed, NULL, 0);
	//compare result
	const uint8_t expected[] = {0x00, 0x0F, 0x00, 0x01, 0x00,
	                            0x0B, 0x00, 0x05, 0x00, 0x07,
	                            0x00, 0x09, 0x00, 0x03, 0x00,
	                            0x0D, 0x02, 0x07};
	//printf("Used: %u\n", dataUsed);
	//PrintHex(dataGraphic, 18);
	//PlotGfx1Bit(dataGraphic, x, y);
	TASSH(result == 0, 1);
	TASSH(dataUsed == 18, 2);
	TASSH(memcmp(dataGraphic, expected, 18) == 0, 3);
	return 0;
}

//get a 10x5 pixel graphic out of 3 data points, zero is down
int CreateLineGfx1BitB(void) {
	uint16_t dataIn[3] = {10, 20, 10};
	uint16_t x = 10;
	uint16_t y = 5;
	//test function
	uint8_t dataGraphic[50] = {0};
	size_t dataUsed = 0;
	uint8_t result = ImgCreateLineGfx1BitGeneric(dataIn, sizeof(dataIn)/sizeof(uint16_t), x, y, true,
	                   dataGraphic, sizeof(dataGraphic), &dataUsed, NULL, 0);
	//compare result
	const uint8_t expected[] = {0x07, 0x02, 0x0D, 0x00, 0x03, 0x00, 0x09, 0x00,
	                            0x07, 0x00, 0x05, 0x00, 0x0B, 0x00, 0x01, 0x00,
	                            0x0F, 0x00};
	//PlotGfx1Bit(dataGraphic, x, y);
	TASSH(result == 0, 1);
	TASSH(dataUsed == 18, 2);
	TASSH(memcmp(dataGraphic, expected, 18) == 0, 3);
	return 0;
}

//get a 10x5 pixel graphic out of 1 data points, zero is down
int CreateLineGfx1BitC(void) {
	uint16_t dataIn[1] = {10};
	uint16_t x = 10;
	uint16_t y = 5;
	//test function
	uint8_t dataGraphic[50] = {0};
	size_t dataUsed = 0;
	uint8_t result = ImgCreateLineGfx1BitGeneric(dataIn, sizeof(dataIn)/sizeof(uint16_t), x, y, true,
	                   dataGraphic, sizeof(dataGraphic), &dataUsed, NULL, 0);
	//compare result
	const uint8_t expected[] = {0x27, 0x12, 0x27};
	//PrintHex(dataGraphic, 3);
	//PlotGfx1Bit(dataGraphic, x, y);
	TASSH(result == 0, 1);
	TASSH(dataUsed == 3, 2);
	TASSH(memcmp(dataGraphic, expected, 3) == 0, 3);
	return 0;
}

//get a 10x5 pixel graphic out of 0 data points, just an empty rectangel
int CreateLineGfx1BitD(void) {
	uint16_t dataIn[1] = {10};
	uint16_t x = 10;
	uint16_t y = 5;
	//test function
	uint8_t dataGraphic[50] = {0};
	size_t dataUsed = 0;
	uint8_t result = ImgCreateLineGfx1BitGeneric(dataIn, 0, x, y, false,
	                   dataGraphic, sizeof(dataGraphic), &dataUsed, NULL, 0);
	//compare result
	const uint8_t expected[] = {0x63};
	//PlotGfx1Bit(dataGraphic, x, y);
	TASSH(result == 0, 1);
	TASSH(dataUsed == 1, 2);
	TASSH(memcmp(dataGraphic, expected, 1) == 0, 3);
	return 0;
}

void PlotGfx2Bit(const uint8_t * data, uint16_t x, uint16_t y) {
	uint16_t rptr = 0;
	uint8_t color = 0;
	uint8_t left = 0;
	for (uint16_t j = 0; j < y; j++) {
		for (uint16_t i = 0; i < x; i++) {
			if (left == 0) {
				uint8_t in = data[rptr];
				rptr++;
				left = (in >> 2) + 1;
				color = in & 3;
			}
			if (color == 0) {
				printf("#");
			} else if (color == 1) {
				printf("*");
			} else if (color == 2) {
				printf("+");
			} else {
				printf(".");
			}
			left--;
		}
		printf("\n");
	}
}


//get a 10x8 pixel graphic out of 2x4 data points
int CreateLinesGfx2BitA(void) {
	uint16_t dataIn1[4] = {5, 10, 10, 15};
	uint16_t dataIn2[4] = {20, 0, 0, 10};
	uint8_t colors[2] = {0x1, 0x2};
	const uint16_t * dataIn[2];
	dataIn[0] = dataIn1;
	dataIn[1] = dataIn2;
	uint16_t x = 20;
	uint16_t y = 8;
	uint8_t colorBits = 2;
	uint8_t lines = 2;
	//test function
	uint8_t dataGraphic[50] = {0};
	size_t dataUsed = 0;
	uint8_t result = ImgCreateLinesGfxGeneric(dataIn, sizeof(dataIn1) / sizeof(uint16_t),
	                   colors, lines, x, y, colorBits, true, dataGraphic, sizeof(dataGraphic), &dataUsed, NULL, 0);
	//compare result
	const uint8_t expected[39] = {
	0x02, 0x47, 0x01, 0x03, 0x02, 0x37, 0x09, 0x0B,
	0x02, 0x2F, 0x01, 0x1B, 0x02, 0x07, 0x21, 0x0F,
	0x02, 0x0B, 0x02, 0x03, 0x01, 0x2F, 0x02, 0x0F,
	0x01, 0x00, 0x2B, 0x06, 0x0B, 0x05, 0x07, 0x02,
	0x1F, 0x06, 0x0F, 0x01, 0x13, 0x1E, 0x17};
	//PrintHex(dataGraphic, dataUsed);
	//PlotGfx2Bit(dataGraphic, x, y);
	TASSH(result == 0, 1);
	TASSH(dataUsed == sizeof(expected), 2);
	TASSH(memcmp(dataGraphic, expected, sizeof(expected)) == 0, 3);
	return 0;
}


typedef int (*test_t)(void);

test_t g_tests[] = {
	&Empty20x10x1xA,
	&Empty20x10x1xB,
	&Empty20x10x2xA,
	&Empty20x10x8xA,
	&Empty20x10x8xTooSmall,
	&Pixels10x5x4xA,
	&Pixels10x5x4xB,
	&Pixels10x5x4xD,
	&Pixels10x5x4xE,
	&Pixels10x5x4xF,
	&Pixels10x5x4xG,
	&Scale1ManualA,
	&Scale1ManualB,
	&Scale1ManualC,
	&Scale1ManualD,
	&Scale1ManualE,
	&Scale1AutoA,
	&Scale1AutoB,
	&Scale1AutoC,
	&InterpolateA, //test 20
	&InterpolateB,
	&InterpolateC,
	&InterpolateD,
	&InterpolateE,
	&InterpolateLinesA, //test 25
	&InterpolateLinesB,
	&InterpolateLinesC,
	&InterpolateLinesD,
	&InterpolateLinesE,
	&InterpolateLinesF,
	&InterpolateLinesG,
	&InterpolatePixelsA, //test 32
	&InterpolatePixelsB,
	&InterpolatePixelsC,
	&CreateLineGfx1BitA, //test 35
	&CreateLineGfx1BitB,
	&CreateLineGfx1BitC,
	&CreateLineGfx1BitD,
	&CreateLinesGfx2BitA, //test 39
};

int main(void) {
	int result = 0;
	int entries = sizeof(g_tests) / sizeof(test_t);
	for (int i = 0; i < entries; i++) {
		int e = g_tests[i]();
		if (e != 0) {
			printf("Error, test %i has failed with code %i\n", i + 1, e);
			result = -1;
		}
	}
	return result;
}
