/* FramebufferLowmem
(c) 2022 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause

Implemets a framebuffer for devices with low memory.
Instead of storing the color information for every pixel, only a front and back
color can be defined. Then the front and back color can be set for every large
block.

This code implements the callbacks from menu-interpreter.h and forwards the
result to boxlib/lcd.h.
*/

#include <string.h>
#include <stdint.h>

#include "framebufferLowmem.h"

#include "menu-interpreter.h"

#include "boxlib/lcd.h"


/* Include from the project to get the proper configuration
The following defines are needed in the file:

For the size of the screen:
FB_SIZE_X
FB_SIZE_Y

For the granularity of the color information:
FB_COLOR_RES_X
FB_COLOR_RES_Y
Defining both to 1 would give a full color framebuffer, but this would waste
more memory than using a simple full color framebuffer.
In fact, this function starts saving memory when
(FB_COLOR_RES_X * FB_COLOR_RES_Y) >= 4 is true.

The integer type used for the color. Like uint16_t
FB_COLOR_TYPE

Number of bits for red color information
FB_RED_BITS
FB_GREEN_BITS
FB_BLUE_BITS

Level until the sum of red + green + blue, shifted to the left to give an 8
bit value, is considered as front color. So anything in the range of 1 to 765
might give the best results.
FB_FRONT_LEVEL
*/

#include "framebufferConfig.h"

#define FB_BLOCKS_X ((FB_SIZE_X + FB_COLOR_RES_X - 1) / FB_COLOR_RES_X)
#define FB_BLOCKS_Y ((FB_SIZE_Y + FB_COLOR_RES_Y - 1) / FB_COLOR_RES_Y)

#define FB_MISSINGRED (8 - FB_RED_BITS)
#define FB_MISSINGGREEN (8 - FB_GREEN_BITS)
#define FB_MISSINGBLUE (8 - FB_BLUE_BITS)

#define FB_REDMASK ((1<< FB_RED_BITS) - 1)
#define FB_GREENMASK ((1<< FB_GREEN_BITS) - 1)
#define FB_BLUEMASK ((1<< FB_GREEN_BITS) - 1)

#define FB_GETRED(color)    (color                                   & (FB_REDMASK   << FB_MISSINGRED))
#define FB_GETGREEN(color) ((color >> (FB_RED_BITS))                 & (FB_GREENMASK << FB_MISSINGGREEN))
#define FB_GETBLUE(color)  ((color >> (FB_RED_BITS + FB_GREEN_BITS)) & (FB_BLUEMASK  << FB_MISSINGBLUE))

#define FB_BYTES_X ((FB_SIZE_X + 7) / 8)

uint8_t g_fbFrontPixel[FB_BYTES_X * FB_SIZE_Y];
FB_COLOR_TYPE g_fbFront[FB_BLOCKS_X * FB_BLOCKS_Y];
FB_COLOR_TYPE g_fbBack[FB_BLOCKS_X * FB_BLOCKS_Y];

void menu_screen_set(SCREENPOS x, SCREENPOS y, SCREENCOLOR color) {
	if ((x < FB_SIZE_X) && (y < FB_SIZE_Y)) {
		uint8_t r = FB_GETRED(color);
		uint8_t g = FB_GETGREEN(color);
		uint8_t b = FB_GETBLUE(color);
		uint32_t bright = r + g + b;
		uint32_t index = x / 8 + y * FB_BYTES_X;
		uint32_t shift = x % 8;
		uint32_t indexBlock = (x / FB_COLOR_RES_X) + (y / FB_COLOR_RES_Y * FB_BLOCKS_X);
		if (sizeof(SCREENCOLOR) == 2) {
			color = (color << 8) | (color >> 8);
		}
		if (sizeof(SCREENCOLOR) == 3) {
			color = ((color & 0xFF) << 16) | (color >> 16) | (color & 0xFF00);
		}
		if (bright >= FB_FRONT_LEVEL) {
			g_fbFrontPixel[index] |= (1<<shift);
			g_fbFront[indexBlock] = color;
		} else {
			g_fbFrontPixel[index] &= ~(1<<shift);
			g_fbBack[indexBlock] = color;
		}
	}
}

void menu_screen_flush(void) {
	//TODO: Limit to actual screen size. Then send more than one line at a time.
	FB_COLOR_TYPE line[FB_SIZE_X];
	uint32_t colorIdxBase = 0;
	uint32_t colorIdxCntY = 0;
	for (uint32_t y = 0; y < FB_SIZE_Y; y++) {
		uint32_t bitmapIdxBase = y * FB_BYTES_X;
		uint32_t bitmapMask = 1;
		uint32_t bitmapIdxOffset = 0;
		uint32_t colorIdxOffset = 0;
		uint32_t colorIdxCntX = 0;
		for (uint32_t x = 0; x < FB_SIZE_X; x++) {
			uint32_t colorIdx = colorIdxBase + colorIdxOffset;
			uint8_t pixel8 = g_fbFrontPixel[bitmapIdxBase + bitmapIdxOffset];
			if (pixel8 & bitmapMask) {
				line[x] = g_fbFront[colorIdx];
			} else {
				line[x] = g_fbBack[colorIdx];
			}
			bitmapMask <<= 1;
			if (bitmapMask == 0x100) {
				bitmapMask = 1;
				bitmapIdxOffset++;
			}
			colorIdxCntX++;
			if (colorIdxCntX == FB_COLOR_RES_X) {
				colorIdxOffset++;
				colorIdxCntX = 0;
			}
		}
		LcdWriteRect(0, y, FB_SIZE_X, 1, (const uint8_t*)line, sizeof(line));
		colorIdxCntY++;
		if (colorIdxCntY == FB_COLOR_RES_Y) {
			colorIdxCntY = 0;
			colorIdxBase += FB_BLOCKS_X;
		}
	}
}

void menu_screen_clear(void) {
	memset(g_fbFrontPixel, 0xFF, (FB_BYTES_X * FB_SIZE_Y) * sizeof(uint8_t));
	memset(g_fbFront, 0xFF, (FB_BLOCKS_X * FB_BLOCKS_Y) * sizeof(FB_COLOR_TYPE));
	memset(g_fbBack, 0, FB_BLOCKS_X * FB_BLOCKS_Y  * sizeof(FB_COLOR_TYPE));
}
