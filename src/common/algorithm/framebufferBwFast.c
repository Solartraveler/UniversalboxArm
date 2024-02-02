/* FramebufferBwFast
(c) 2022 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause

Implemets a framebuffer for devices which do not need color.
So 1Bit black-white per pixel.

This code implements the callbacks from menu-interpreter.h and forwards the
result to boxlib/lcd.h.
But other graphic libraries than then menu-interpreter can be used.
*/

#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include "framebufferBwFast.h"

#include "boxlib/lcd.h"

//For debug prints only:
//#include "main.h"


/* Include from the project to get the proper configuration
The following defines are needed in the file:

For the size of the screen:
FB_SIZE_X
FB_SIZE_Y

The integer type used for the color. Like uint16_t
FB_COLOR_TYPE

The integer type used for the coordinates. Like uint16_t
FB_SCREENPOS_TYPE

The default level between bright and dark color
FB_FRONT_LEVEL

//Writing the output in blocks is faster, as it saves transmitting the pixel
//position. This should fit on the stack twice.
FB_OUTPUTBLOCK_X
FB_OUTPUTBLOCK_Y

//The color which represents white in the output data
FB_COLOR_OUT_WHITE

*/

#include "framebufferConfig.h"

//number of FB_BITMAP_BITS
#define FB_ELEMENTS_X ((FB_SIZE_X + FB_BITMAP_BITS - 1) / FB_BITMAP_BITS)

FB_BITMAP_TYPE g_fbPixel[FB_ELEMENTS_X * FB_SIZE_Y];
FB_BITMAP_TYPE g_fbPixelPrevious[FB_ELEMENTS_X * FB_SIZE_Y];

FB_SCREENPOS_TYPE g_fbUseX = FB_SIZE_X;
FB_SCREENPOS_TYPE g_fbUseY = FB_SIZE_Y;

uint16_t g_fbFrontLevel = FB_FRONT_LEVEL;

//first time init? -> write everything
uint8_t g_fbWritten;

void menu_screen_set(FB_SCREENPOS_TYPE x, FB_SCREENPOS_TYPE y, FB_COLOR_IN_TYPE color) {
	if ((x < g_fbUseX) && (y < g_fbUseY)) {
		uint32_t index = x / FB_BITMAP_BITS + y * FB_ELEMENTS_X;
		uint32_t shift = x % FB_BITMAP_BITS;
		if (color >= g_fbFrontLevel) {
			g_fbPixel[index] |= (1<<shift);
		} else {
			g_fbPixel[index] &= ~(1<<shift);
		}
	}
}

//block must have FB_OUTPUTBLOCK_X * FB_OUTPUTBLOCK_Y elements
static void FbBlockFlush(const uint16_t startX, const uint16_t startY, FB_COLOR_OUT_TYPE * block) {
	uint32_t wptr = 0;
	FB_COLOR_OUT_TYPE colorOut = 0;
	for (uint32_t y = startY; y < (uint32_t)(startY + FB_OUTPUTBLOCK_Y); y++) {
		uint32_t bitmapIdxBase = y * FB_ELEMENTS_X + (startX / FB_BITMAP_BITS);
		FB_BITMAP_TYPE bitmapMask = 1;
		uint32_t bitmapIdxOffset = 0;
		FB_BITMAP_TYPE pixelData = g_fbPixel[bitmapIdxBase + bitmapIdxOffset];
		for (uint32_t x = startX; x < (uint32_t)(startX + FB_OUTPUTBLOCK_X); x++) {
			if (pixelData & bitmapMask) {
				colorOut = FB_COLOR_OUT_WHITE;
			} else {
				colorOut = 0;
			}
			block[wptr] = colorOut;
			wptr++;
			bitmapMask <<= 1;
			if (bitmapMask == 0) { //this is why there may not be any unused bits in FB_BITMAP_TYPE
				bitmapMask = 1;
				bitmapIdxOffset++;
				//we do not need data when its the last loop. Otherwise we would get a read-bufferoverflow
				if ((x + 1) < ((uint32_t)startX + FB_OUTPUTBLOCK_X)) {
					pixelData = g_fbPixel[bitmapIdxBase + bitmapIdxOffset];
				}
			}
		}
	}
	LcdWriteRect(startX, startY, FB_OUTPUTBLOCK_X, FB_OUTPUTBLOCK_Y, (const uint8_t*)block, FB_OUTPUTBLOCK_X * FB_OUTPUTBLOCK_Y * sizeof(FB_COLOR_OUT_TYPE));
}

static bool FbBlockCopyChanged(uint16_t startX, uint16_t startY) {
	bool changed = false;
	const size_t len = FB_OUTPUTBLOCK_X / FB_BITMAP_BITS * sizeof(FB_BITMAP_TYPE);
	for (uint32_t y = startY; y < (uint32_t)(startY + FB_OUTPUTBLOCK_Y); y++) {
		uint32_t bitmapIdxBase = y * FB_ELEMENTS_X + (startX / FB_BITMAP_BITS);
		if (memcmp(g_fbPixel + bitmapIdxBase, g_fbPixelPrevious + bitmapIdxBase, len)) {
			memcpy(g_fbPixelPrevious + bitmapIdxBase, g_fbPixel + bitmapIdxBase, len);
			changed = true;
		}
	}
	return changed;
}

void menu_screen_flush(void) {
	//uint32_t timeStart = HAL_GetTick();
	uint16_t xMax = g_fbUseX / FB_OUTPUTBLOCK_X;
	uint16_t yMax = g_fbUseY / FB_OUTPUTBLOCK_Y;
#ifdef FB_TWOBUFFERS
	FB_COLOR_OUT_TYPE blocks[2][FB_OUTPUTBLOCK_X * FB_OUTPUTBLOCK_Y];
	FB_COLOR_OUT_TYPE * block = blocks[0];
	uint8_t toggle = 0;
#else
	FB_COLOR_OUT_TYPE block[FB_OUTPUTBLOCK_X * FB_OUTPUTBLOCK_Y];
#endif
	const size_t elem = (FB_OUTPUTBLOCK_X * FB_OUTPUTBLOCK_Y) / FB_BITMAP_BITS;
	size_t offset = 0;
	for (uint32_t y = 0; y < yMax; y++) {
		for (uint32_t x = 0; x < xMax; x++) {
			if (FbBlockCopyChanged(x * FB_OUTPUTBLOCK_X, y * FB_OUTPUTBLOCK_Y) || (g_fbWritten == 0)) {
				FbBlockFlush(x * FB_OUTPUTBLOCK_X, y * FB_OUTPUTBLOCK_Y, block);
#ifdef FB_TWOBUFFERS
				toggle = 1 - toggle;
				block = blocks[toggle];
#endif
			}
			offset += elem;
		}
	}
	g_fbWritten = 1;
	LcdWaitBackgroundDoneRelease();
	//uint32_t timeStop = HAL_GetTick();
	//printf("Redraw took %uticks\r\n", (unsigned int)(timeStop - timeStart));
}

void menu_screen_size(FB_SCREENPOS_TYPE x, FB_SCREENPOS_TYPE y) {
	if (x <= FB_SIZE_X) {
		g_fbUseX = x;
	}
	if (y <= FB_SIZE_Y) {
		g_fbUseY = y;
	}
}

void menu_screen_size_get(FB_SCREENPOS_TYPE * pX, FB_SCREENPOS_TYPE * pY) {
	if (pX) {
		*pX = g_fbUseX;
	}
	if (pY) {
		*pY = g_fbUseY;
	}
}

void menu_screen_frontlevel(uint16_t level) {
	g_fbFrontLevel = level;
}

void menu_screen_clear(void) {
	memset(g_fbPixel, 0xFF, (FB_ELEMENTS_X * FB_SIZE_Y) * sizeof(FB_BITMAP_TYPE));
}

