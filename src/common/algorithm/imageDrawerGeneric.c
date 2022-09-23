/*
(c) 2022 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause

*/

#include <stdint.h>
#include <ctype.h>
#include <stdlib.h>
#include <alloca.h>
#include <stddef.h>

#include "imageDrawer.h"

#include "utility.h"



#ifndef IMGTYPESDEFINED
#error "Do not directly compile this file. Instead compile imageDrawerLowres.c and/or imageDrawerHighres.c"
#endif



static int ImgOrderPixelsGeneric(const void * a, const void * b) {
	Img1BytePixelGeneric_t * ia = (Img1BytePixelGeneric_t *)a;
	Img1BytePixelGeneric_t * ib = (Img1BytePixelGeneric_t *)b;
	//y is the first criteria
	if (ia->y < ib->y) {
		return -1;
	}
	if (ia->y > ib->y) {
		return 1;
	}
	//if y are equal, x is the second criteria
	if (ia->x < ib->x) {
		return -1;
	}
	if (ia->x > ib->x) {
		return 1;
	}
	return 0; //if x and y are equal, noting can be sorted
}

uint8_t ImgCompressed1ByteGeneric(ImgResGeneric_t x, ImgResGeneric_t y, uint8_t colorBits,
              uint8_t * outBuffer, size_t outBufferLen, uint8_t backgroundColor,
              Img1BytePixelGeneric_t * drawPixels, uint16_t numPixel, size_t * outBufferUsed) {
	if ((outBuffer == NULL) || (colorBits > 8)) {
		return 2;
	}
	if ((drawPixels) && (numPixel)) {
		//sort the input
		qsort(drawPixels, numPixel, sizeof(Img1BytePixelGeneric_t), &ImgOrderPixelsGeneric);
		//merge duplicated pixels
		uint16_t wptr = 0;
		uint16_t rptr = 0;
		while (rptr < numPixel) {
			ImgResGeneric_t px = drawPixels[rptr].x;
			ImgResGeneric_t py = drawPixels[rptr].y;
			drawPixels[wptr].x = px;
			drawPixels[wptr].y = py;
			drawPixels[wptr].color = drawPixels[rptr].color;
			rptr++;
			while ((rptr < numPixel) && (px == drawPixels[rptr].x) && (py == drawPixels[rptr].y)) {
				drawPixels[wptr].color &= drawPixels[rptr].color;
				rptr++;
			}
			wptr++;
		}
		numPixel = wptr;
	}
	uint32_t generatedPixels = 0;
	ImgPixelsMax_t requiredPixels = x * y;
	size_t wptr = 0;
	uint16_t pixelsPerByte = 1 << (8 - colorBits);
	uint16_t inputIndex = 0;
	uint8_t lastColor = 0;
	uint8_t lastRepeats = 0;
	const uint8_t colorMask = (1 << colorBits) - 1;
	backgroundColor &= colorMask;
	while ((generatedPixels < requiredPixels) && (wptr < outBufferLen)) {
		ImgPixelsMax_t bgRepeats = requiredPixels - generatedPixels;
		if (inputIndex < numPixel) {
			uint32_t nextSpecial = drawPixels[inputIndex].y * x + drawPixels[inputIndex].x;
			bgRepeats = nextSpecial - generatedPixels;
		}
		if (bgRepeats) { //insert background pixels
			bgRepeats = MIN(bgRepeats, pixelsPerByte);
			if ((wptr > 0) && (lastColor == backgroundColor) && (lastRepeats < pixelsPerByte)) {
				//same color and not maximum pixel number, can be added to the previous byte
				bgRepeats = MIN(bgRepeats, pixelsPerByte - lastRepeats);
				lastRepeats += bgRepeats;
				outBuffer[wptr - 1] = backgroundColor | ((lastRepeats - 1) << colorBits);
				generatedPixels += bgRepeats;
			} else {
				//create a new byte in the output
				lastColor = backgroundColor;
				lastRepeats = bgRepeats;
				outBuffer[wptr] = backgroundColor | ((bgRepeats - 1) << colorBits);
				wptr++;
				generatedPixels += bgRepeats;
			}
		} else { //insert one pixel from the list
			if (inputIndex < numPixel) {
				uint8_t color = drawPixels[inputIndex].color & colorMask;
				if ((wptr > 0) && (lastColor == color) && (lastRepeats < pixelsPerByte)) {
					//same color and not maximum pixel number, can be added to the previous byte
					lastRepeats++;
					outBuffer[wptr - 1] = color | ((lastRepeats - 1) << colorBits);
				} else {
					//create a new byte in the output
					outBuffer[wptr] = color;
					lastRepeats = 1;
					lastColor = color;
					wptr++;
				}
			} else {
				outBuffer[wptr] = 0; //usually, this case should never happen, no need to update last variables
				wptr++;
			}
			inputIndex++;
			generatedPixels++;
		}
	}
	if (outBufferUsed) {
		*outBufferUsed = wptr;
	}
	//fixup, if the space was too short
	wptr = outBufferLen - 1;
	uint8_t result = 0; //ok
	while ((generatedPixels < requiredPixels) && (wptr)) {
		result = 1;
		uint8_t used = (outBuffer[wptr] >> colorBits) + 1;
		outBuffer[wptr] = backgroundColor | ((pixelsPerByte - 1) << colorBits);
		generatedPixels += pixelsPerByte - used; //update delta
		wptr--;
	}
	if ((wptr == 0) && (generatedPixels < requiredPixels)) {
		result = 2;
	}
	return result;
}

uint16_t ImgInterpolateToPixelsGeneric(const uint16_t * dataIn, uint16_t dataPointsIn,
               Img1BytePixelGeneric_t * drawPixels, uint16_t maxPixels, uint8_t color) {
	uint16_t wptr = 0;
	if (dataPointsIn > 1) {
		for (uint16_t i = 0; i < dataPointsIn - 1; i++) {
			uint16_t start = dataIn[i];
			uint16_t end = dataIn[i + 1];
			do {
				if (wptr < maxPixels) {
					drawPixels[wptr].x = i;
					drawPixels[wptr].y = start;
					drawPixels[wptr].color = color;
					wptr++;
				}
				if (start < end) {
					start++;
				} else if (start > end) {
					start--;
				}
			} while (start != end);
		}
	}
	//append last (or only) pixel
	if ((dataPointsIn > 0) && (wptr < maxPixels)) {
		drawPixels[wptr].x = dataPointsIn - 1;
		drawPixels[wptr].y = dataIn[dataPointsIn - 1];
		drawPixels[wptr].color = color;
		wptr++;
	}
	return wptr;
}

uint8_t ImgCreateLineGfx1BitGeneric(const uint16_t * data, uint16_t dataPoints, ImgResGeneric_t x, ImgResGeneric_t y, bool flip,
              uint8_t * compressedGfxOut, size_t outGfxLen, size_t * outGfxUsed, void * tempBuffer, size_t tempBufferLen) {
	const uint8_t lineColor = 0;  //dark on white background
	const uint16_t * pData[1];
	pData[0] = data;
	return ImgCreateLinesGfxGeneric(pData, dataPoints, &lineColor, 1, x, y, 1, flip, compressedGfxOut, outGfxLen, outGfxUsed, tempBuffer, tempBufferLen);
}

uint8_t ImgCreateLinesGfxGeneric(const uint16_t ** data, uint16_t dataPoints, const uint8_t * colors, uint8_t lines,
              ImgResGeneric_t x, ImgResGeneric_t y, uint8_t colorBits, bool flip,
              uint8_t * compressedGfxOut, size_t outGfxLen, size_t * outGfxUsed, void * tempBuffer, size_t tempBufferLen) {
	const uint8_t bgColor = (1 << colorBits) - 1; //white background
	uint16_t drawPixelsNum = 0;
	uint16_t * lineData = alloca(x * sizeof(uint16_t));
	Img1BytePixelGeneric_t * drawPixels = NULL;
	if ((lineData) && (dataPoints) && (lines)) {
		uint16_t maxPixels;
		if (tempBuffer) {
			maxPixels = tempBufferLen / sizeof(Img1BytePixelGeneric_t);
			drawPixels = (Img1BytePixelGeneric_t *)tempBuffer;
		} else {
			maxPixels = x * lines * 10; //10 is just a guess
			drawPixels = (Img1BytePixelGeneric_t *)alloca(maxPixels * sizeof(Img1BytePixelGeneric_t));
		}
		for (uint8_t i = 0; i < lines; i++) {
			ImgInterpolateLines(*data, dataPoints, lineData, x);
			ImgScale2Byte(y - 1, lineData, x, 0, 0, flip, NULL, NULL);
			if ((drawPixels) && (drawPixelsNum < maxPixels)) {
				drawPixelsNum += ImgInterpolateToPixelsGeneric(lineData, x, drawPixels + drawPixelsNum, maxPixels - drawPixelsNum, *colors);
			}
			data++;
			colors++;
		}
	}
	return ImgCompressed1ByteGeneric(x, y, colorBits, compressedGfxOut, outGfxLen, bgColor, drawPixels, drawPixelsNum, outGfxUsed);
}
