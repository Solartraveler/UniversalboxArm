/* FramebufferLowmem
(c) 2022 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause

Implemets a framebuffer for devices with low memory.
Instead of storing the color information for every pixel, only a front and back
color can be defined. Then the front and back color can be set for every large
block.

This code implements the callbacks from menu-interpreter.h and forwards the
result to boxlib/lcd.h.
But other graphic libraries than then menu-enterpreter can be used.
*/

#include <string.h>
#include <stdint.h>

#include "framebufferLowmem.h"

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

The integer type used for the coordinates. Like uint16_t
FB_SCREENPOS_TYPE

Number of bits for red, green and blue color information, coming in
FB_RED_IN_BITS
FB_GREEN_IN_BITS
FB_BLUE_IN_BITS

Number of bits for red, green and blue color information, going out
FB_RED_OUT_BITS
FB_GREEN_OUT_BITS
FB_BLUE_OUT_BITS


The default level between background and front color
FB_FRONT_LEVEL
*/

#include "framebufferConfig.h"

#define FB_BLOCKS_X ((FB_SIZE_X + FB_COLOR_RES_X - 1) / FB_COLOR_RES_X)
#define FB_BLOCKS_Y ((FB_SIZE_Y + FB_COLOR_RES_Y - 1) / FB_COLOR_RES_Y)

#define FB_BYTES_X ((FB_SIZE_X + 7) / 8)

#define FB_MISSINGRED_IN   (8 - FB_RED_IN_BITS)
#define FB_MISSINGGREEN_IN (8 - FB_GREEN_IN_BITS)
#define FB_MISSINGBLUE_IN  (8 - FB_BLUE_IN_BITS)

#define FB_REDMASK_IN   ((1<< FB_RED_IN_BITS) - 1)
#define FB_GREENMASK_IN ((1<< FB_GREEN_IN_BITS) - 1)
#define FB_BLUEMASK_IN  ((1<< FB_BLUE_IN_BITS) - 1)

#define FB_GETRED_IN(color)    ((color                                         & (FB_REDMASK_IN))   << FB_MISSINGRED_IN)
#define FB_GETGREEN_IN(color) (((color >> (FB_RED_IN_BITS))                    & (FB_GREENMASK_IN)) << FB_MISSINGGREEN_IN)
#define FB_GETBLUE_IN(color)  (((color >> (FB_RED_IN_BITS + FB_GREEN_IN_BITS)) & (FB_BLUEMASK_IN))  << FB_MISSINGBLUE_IN)

#define FB_COLOR_IN_BITS_USED (FB_RED_IN_BITS + FB_GREEN_IN_BITS + FB_BLUE_IN_BITS)

#if (FB_COLOR_IN_BITS_USED * 2 <= 8)
typedef uint8_t fb_DoubleColor_t;
#elif (FB_COLOR_IN_BITS_USED * 2 <= 16)
typedef uint16_t fb_DoubleColor_t;
#elif ((FB_COLOR_IN_BITS_USED * 2) <= 32)
typedef uint32_t fb_DoubleColor_t;
#elif ((FB_COLOR_IN_BITS_USED * 2) <= 64)
typedef uint64_t fb_DoubleColor_t;
#else
#error "Input color bits not supported"
#endif

#define FB_DOUBLECOLOR_BACK_MASK ((1 << FB_COLOR_IN_BITS_USED) - 1)
#define FB_DOUBLECOLOR_FRONT_MASK (FB_DOUBLECOLOR_BACK_MASK << FB_COLOR_IN_BITS_USED)

#define FB_MISSINGRED_OUT   (8 - FB_RED_OUT_BITS)
#define FB_MISSINGGREEN_OUT (8 - FB_GREEN_OUT_BITS)
#define FB_MISSINGBLUE_OUT  (8 - FB_BLUE_OUT_BITS)

#define FB_SETRED_OUT(color) (color >> FB_MISSINGRED_OUT)
#define FB_SETGREEN_OUT(color) ((color >> FB_MISSINGGREEN_OUT) << (FB_RED_OUT_BITS))
#define FB_SETBLUE_OUT(color) ((color >> FB_MISSINGBLUE_OUT) << (FB_GREEN_OUT_BITS + FB_RED_OUT_BITS))

uint8_t g_fbFrontPixel[FB_BYTES_X * FB_SIZE_Y];
fb_DoubleColor_t g_fbColor[FB_BLOCKS_X * FB_BLOCKS_Y];
FB_SCREENPOS_TYPE g_fbUseX = FB_SIZE_X;
FB_SCREENPOS_TYPE g_fbUseY = FB_SIZE_Y;
uint16_t g_fbFrontLevel = FB_FRONT_LEVEL;

void menu_screen_set(FB_SCREENPOS_TYPE x, FB_SCREENPOS_TYPE y, FB_COLOR_IN_TYPE color) {
	if ((x < FB_SIZE_X) && (y < FB_SIZE_Y)) {
		uint8_t r = FB_GETRED_IN(color);
		uint8_t g = FB_GETGREEN_IN(color);
		uint8_t b = FB_GETBLUE_IN(color);
		uint32_t bright = r + g + b;
		uint32_t index = x / 8 + y * FB_BYTES_X;
		uint32_t shift = x % 8;
		uint32_t indexBlock = (x / FB_COLOR_RES_X) + (y / FB_COLOR_RES_Y * FB_BLOCKS_X);
		if (bright >= g_fbFrontLevel) {
			g_fbFrontPixel[index] |= (1<<shift);
			g_fbColor[indexBlock] = (color << FB_COLOR_IN_BITS_USED) | (g_fbColor[indexBlock] & FB_DOUBLECOLOR_BACK_MASK);
		} else {
			g_fbFrontPixel[index] &= ~(1<<shift);
			g_fbColor[indexBlock] = color | (g_fbColor[indexBlock] & FB_DOUBLECOLOR_FRONT_MASK);
		}
	}
}

void menu_screen_flush(void) {
	FB_COLOR_OUT_TYPE line[FB_SIZE_X];
	uint32_t colorIdxBase = 0;
	uint32_t colorIdxCntY = 0;
	for (uint32_t y = 0; y < g_fbUseY; y++) {
		uint32_t bitmapIdxBase = y * FB_BYTES_X;
		uint32_t bitmapMask = 1;
		uint32_t bitmapIdxOffset = 0;
		uint32_t colorIdxOffset = 0;
		uint32_t colorIdxCntX = 0;
		for (uint32_t x = 0; x < g_fbUseX; x++) {
			uint32_t colorIdx = colorIdxBase + colorIdxOffset;
			uint8_t pixel8 = g_fbFrontPixel[bitmapIdxBase + bitmapIdxOffset];
			FB_COLOR_IN_TYPE colorIn;
			if (pixel8 & bitmapMask) {
				colorIn = g_fbColor[colorIdx] >> FB_COLOR_IN_BITS_USED;
			} else {
				colorIn = g_fbColor[colorIdx] & FB_DOUBLECOLOR_BACK_MASK;
			}
			uint8_t r = FB_GETRED_IN(colorIn);
			uint8_t g = FB_GETGREEN_IN(colorIn);
			uint8_t b = FB_GETBLUE_IN(colorIn);
			if (r & 0x80) {
				r |= ((1<<FB_MISSINGRED_IN) - 1);
			}
			if (g & 0x80) {
				g |= ((1<<FB_MISSINGGREEN_IN) - 1);
			}
			if (b & 0x80) {
				b |= ((1<<FB_MISSINGBLUE_IN) - 1);
			}
			line[x] = (FB_SETRED_OUT(r)) | (FB_SETGREEN_OUT(g)) | (FB_SETBLUE_OUT(b));
			//due to little endian, we need to swap the bytes for the transfer later.
			if (sizeof(FB_COLOR_OUT_TYPE) == 2) {
				line[x] = (line[x] >> 8) | (line[x] << 8);
			}
			//TODO: How do we need to swap for 3byte output data?
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
		LcdWriteRect(0, y, g_fbUseX, 1, (const uint8_t*)line, sizeof(line));
		colorIdxCntY++;
		if (colorIdxCntY == FB_COLOR_RES_Y) {
			colorIdxCntY = 0;
			colorIdxBase += FB_BLOCKS_X;
		}
	}
}

void menu_screen_size(SCREENPOS x, SCREENPOS y) {
	g_fbUseX = x;
	g_fbUseY = y;
}

void menu_screen_frontlevel(uint16_t level) {
	g_fbFrontLevel = level;
}

void menu_screen_clear(void) {
	memset(g_fbFrontPixel, 0xFF, (FB_BYTES_X * FB_SIZE_Y) * sizeof(uint8_t));
	memset(g_fbColor, 0xFF, (FB_BLOCKS_X * FB_BLOCKS_Y) * sizeof(fb_DoubleColor_t));
}