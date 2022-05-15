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
#include <stdio.h>

#include "framebufferLowmem.h"

#include "boxlib/lcd.h"

//For debug prints only:
//#include "main.h"


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

#define FB_COLORBLOCKS_X ((FB_SIZE_X + FB_COLOR_RES_X - 1) / FB_COLOR_RES_X)
#define FB_COLORBLOCKS_Y ((FB_SIZE_Y + FB_COLOR_RES_Y - 1) / FB_COLOR_RES_Y)

//number of FB_BITMAP_BITS
#define FB_ELEMENTS_X ((FB_SIZE_X + FB_BITMAP_BITS - 1) / FB_BITMAP_BITS)

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

FB_BITMAP_TYPE g_fbFrontPixel[FB_ELEMENTS_X * FB_SIZE_Y];
fb_DoubleColor_t g_fbColor[FB_COLORBLOCKS_X * FB_COLORBLOCKS_Y];
FB_SCREENPOS_TYPE g_fbUseX = FB_SIZE_X;
FB_SCREENPOS_TYPE g_fbUseY = FB_SIZE_Y;
uint16_t g_fbFrontLevel = FB_FRONT_LEVEL;

/*This array is only used as performance booster.
  When no menu_screen_set is done between two menu_screen_clear, a write to the LCD
  can be skipped. Since we need to write on the So each pixel write sets the counter to 2. A flush must write
  the block if the counter is non 0 and dercreases the counter by 1. But on the very first
  flush, we have to write everything
*/
#define FB_OUTPUTBLOCKS_X (FB_SIZE_X / FB_OUTPUTBLOCK_X)
#define FB_OUTPUTBLOCKS_Y (FB_SIZE_Y / FB_OUTPUTBLOCK_Y)

#define FB_WRITTENBLOCKS_X ((FB_SIZE_X + FB_BITMAP_BITS - 1) / FB_BITMAP_BITS)
#define FB_WRITTENBLOCKS_Y ((FB_SIZE_Y + FB_BITMAP_BITS - 1) / FB_BITMAP_BITS)
FB_BITMAP_TYPE g_fbWrittenBlock[FB_WRITTENBLOCKS_X * FB_WRITTENBLOCKS_Y];
uint8_t g_fbWritten;


void menu_screen_set(FB_SCREENPOS_TYPE x, FB_SCREENPOS_TYPE y, FB_COLOR_IN_TYPE color) {
	if ((x < g_fbUseX) && (y < g_fbUseY)) {
		uint8_t r = FB_GETRED_IN(color);
		uint8_t g = FB_GETGREEN_IN(color);
		uint8_t b = FB_GETBLUE_IN(color);
		uint32_t bright = r + g + b;
		uint32_t index = x / FB_BITMAP_BITS + y * FB_ELEMENTS_X;
		uint32_t shift = x % FB_BITMAP_BITS;
		uint32_t indexBlock = (x / FB_COLOR_RES_X) + (y / FB_COLOR_RES_Y * FB_COLORBLOCKS_X);
		if (bright >= g_fbFrontLevel) {
			g_fbFrontPixel[index] |= (1<<shift);
			g_fbColor[indexBlock] = (color << FB_COLOR_IN_BITS_USED) | (g_fbColor[indexBlock] & FB_DOUBLECOLOR_BACK_MASK);
		} else {
			g_fbFrontPixel[index] &= ~(1<<shift);
			g_fbColor[indexBlock] = color | (g_fbColor[indexBlock] & FB_DOUBLECOLOR_FRONT_MASK);
		}
		//speed improvement
		uint32_t blockWritten = (x / FB_OUTPUTBLOCK_X) + (y / FB_OUTPUTBLOCK_Y) * FB_WRITTENBLOCKS_X;
		uint32_t indexWritten = blockWritten / (FB_BITMAP_BITS / 2);
		uint32_t offsetWritten = blockWritten % (FB_BITMAP_BITS / 2);
		FB_BITMAP_TYPE maskWrittenHigh = 2 << (offsetWritten * 2);
		g_fbWrittenBlock[indexWritten] |= maskWrittenHigh; //can now be 2 or 3. 3 is handled as it is a 2.
	}
}

//block must have FB_OUTPUTBLOCK_X * FB_OUTPUTBLOCK_Y elements
static void FbBlockFlush(const uint16_t startX, const uint16_t startY, FB_COLOR_OUT_TYPE * block) {
	uint32_t wptr = 0;
	uint32_t colorIdxBase = (startX / FB_COLOR_RES_X) + (startY / FB_COLOR_RES_X) * (FB_SIZE_X / FB_COLOR_RES_X);
	uint32_t colorIdxCntY = 0;
	FB_COLOR_IN_TYPE colorIn;
	FB_COLOR_IN_TYPE colorInLast = 0;
	FB_COLOR_OUT_TYPE colorOut = 0;
	for (uint32_t y = startY; y < (startY + FB_OUTPUTBLOCK_Y); y++) {
		uint32_t bitmapIdxBase = y * FB_ELEMENTS_X + (startX / FB_BITMAP_BITS);
		FB_BITMAP_TYPE bitmapMask = 1;
		uint32_t bitmapIdxOffset = 0;
		uint32_t colorIdxOffset = 0;
		uint32_t colorIdxCntX = 0;
		FB_BITMAP_TYPE pixelData = g_fbFrontPixel[bitmapIdxBase + bitmapIdxOffset];
		for (uint32_t x = startX; x < (startX + FB_OUTPUTBLOCK_X); x++) {
			uint32_t colorIdx = colorIdxBase + colorIdxOffset;
			if (pixelData & bitmapMask) {
				colorIn = g_fbColor[colorIdx] >> FB_COLOR_IN_BITS_USED;
			} else {
				colorIn = g_fbColor[colorIdx] & FB_DOUBLECOLOR_BACK_MASK;
			}
			if (colorIn != colorInLast) {
				/*The bit calculations take quite a lot CPU time, so 'caching' the
				  previous converted color nearly doubles the speed of this function
				  (not counting the LcdWriteRect call). */
				colorInLast = colorIn;
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
				colorOut = (FB_SETRED_OUT(r)) | (FB_SETGREEN_OUT(g)) | (FB_SETBLUE_OUT(b));
				//due to little endian, we need to swap the bytes for the transfer later.
				if (sizeof(FB_COLOR_OUT_TYPE) == 2) {
					colorOut = (colorOut >> 8) | (colorOut << 8);
				}
				//TODO: How do we need to swap for 3byte output data?
			}
			block[wptr] = colorOut;
			wptr++;
			bitmapMask <<= 1;
			if (bitmapMask == 0) { //thats why there may not be any unused bits in FB_BITMAP_TYPE
				bitmapMask = 1;
				bitmapIdxOffset++;
				pixelData = g_fbFrontPixel[bitmapIdxBase + bitmapIdxOffset];
			}
			colorIdxCntX++;
			if (colorIdxCntX == FB_COLOR_RES_X) {
				colorIdxOffset++;
				colorIdxCntX = 0;
			}
		}
		colorIdxCntY++;
		if (colorIdxCntY == FB_COLOR_RES_Y) {
			colorIdxCntY = 0;
			colorIdxBase += FB_COLORBLOCKS_X;
		}
	}
	LcdWriteRect(startX, startY, FB_OUTPUTBLOCK_X, FB_OUTPUTBLOCK_Y, (const uint8_t*)block, FB_OUTPUTBLOCK_X * FB_OUTPUTBLOCK_Y * sizeof(FB_COLOR_OUT_TYPE));
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
	for (uint32_t y = 0; y < yMax; y++) {
		for (uint32_t x = 0; x < xMax; x++) {
			uint32_t blockWritten = x + y * FB_OUTPUTBLOCKS_X;
			uint32_t indexWritten = blockWritten / (FB_BITMAP_BITS / 2);
			uint32_t offsetWritten = blockWritten % (FB_BITMAP_BITS / 2);
			FB_BITMAP_TYPE maskWrittenHigh = 2 << (offsetWritten * 2);
			FB_BITMAP_TYPE maskWrittenLow = 1 << (offsetWritten * 2);
			FB_BITMAP_TYPE bitsWritten = g_fbWrittenBlock[indexWritten];
			if (((maskWrittenHigh | maskWrittenLow) & bitsWritten)) {
				FbBlockFlush(x * FB_OUTPUTBLOCK_X, y * FB_OUTPUTBLOCK_Y, block);
				if (bitsWritten & maskWrittenHigh) { //if 2 -> 1, if 3 -> 1
					bitsWritten = (bitsWritten & ~maskWrittenHigh) | maskWrittenLow;
				} else { //if 1 -> 0
					bitsWritten = bitsWritten & ~maskWrittenLow;
				}
				g_fbWrittenBlock[indexWritten] = bitsWritten;
#ifdef FB_TWOBUFFERS
				toggle = 1 - toggle;
				block = blocks[toggle];
#endif
			}
		}
	}
	LcdWaitDmaDone(); //is an empty function if DMA is not used
	//uint32_t timeStop = HAL_GetTick();
	//printf("Redraw took %uticks\r\n", (unsigned int)(timeStop - timeStart));
}

void menu_screen_size(SCREENPOS x, SCREENPOS y) {
	if (x <= FB_SIZE_X) {
		g_fbUseX = x;
	}
	if (y <= FB_SIZE_Y) {
		g_fbUseY = y;
	}
}

void menu_screen_frontlevel(uint16_t level) {
	g_fbFrontLevel = level;
}

void menu_screen_clear(void) {
	memset(g_fbFrontPixel, 0xFF, (FB_ELEMENTS_X * FB_SIZE_Y) * sizeof(FB_BITMAP_TYPE));
	memset(g_fbColor, 0xFF, (FB_COLORBLOCKS_X * FB_COLORBLOCKS_Y) * sizeof(fb_DoubleColor_t));
	if (g_fbWritten == 0) {
		//0x55 -> write each block 1x
		memset(g_fbWrittenBlock, 0x55, (FB_WRITTENBLOCKS_X * FB_WRITTENBLOCKS_Y) * sizeof(FB_BITMAP_TYPE));
		g_fbWritten = 1; //first frame needs to be written completely
	}
}
