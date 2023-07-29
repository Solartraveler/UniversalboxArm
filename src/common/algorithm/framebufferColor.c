/* FramebufferColor
(c) 2022 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause

Implemets a framebuffer for devices with low memory.
Instead of storing the color information for every pixel, only a front and back
color can be defined. Then the front and back color can be set for every large
block.

This code implements the callbacks from menu-interpreter.h and forwards the
result to boxlib/lcd.h.
But other graphic libraries than then menu-interpreter can be used.
*/

#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include "framebufferColor.h"

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

Number of bits for red, green and blue color information, coming in
FB_RED_IN_BITS
FB_GREEN_IN_BITS
FB_BLUE_IN_BITS

Number of bits for red, green and blue color information, going out
FB_RED_OUT_BITS
FB_GREEN_OUT_BITS
FB_BLUE_OUT_BITS

*/

#include "framebufferConfig.h"

#define FB_MISSINGRED_IN   (8 - FB_RED_IN_BITS)
#define FB_MISSINGGREEN_IN (8 - FB_GREEN_IN_BITS)
#define FB_MISSINGBLUE_IN  (8 - FB_BLUE_IN_BITS)

#define FB_REDMASK_IN   ((1<< FB_RED_IN_BITS) - 1)
#define FB_GREENMASK_IN ((1<< FB_GREEN_IN_BITS) - 1)
#define FB_BLUEMASK_IN  ((1<< FB_BLUE_IN_BITS) - 1)

#define FB_GETRED_IN(color)    ((color >> (FB_BLUE_IN_BITS + FB_GREEN_IN_BITS) & (FB_REDMASK_IN))   << FB_MISSINGRED_IN)
#define FB_GETGREEN_IN(color) (((color >> (FB_BLUE_IN_BITS)                  ) & (FB_GREENMASK_IN)) << FB_MISSINGGREEN_IN)
#define FB_GETBLUE_IN(color)  (((color >> 0                                  ) & (FB_BLUEMASK_IN))  << FB_MISSINGBLUE_IN)

#define FB_COLOR_IN_BITS_USED (FB_RED_IN_BITS + FB_GREEN_IN_BITS + FB_BLUE_IN_BITS)

#define FB_PIXELS_IN_DATATYPE (FB_BITMAP_BITS / FB_COLOR_IN_BITS_USED)

_Static_assert(FB_PIXELS_IN_DATATYPE > 0, "FB_BITMAP_TYPE needs to provide storage for at least one pixel");
_Static_assert(FB_BITMAP_BITS == sizeof(FB_BITMAP_TYPE) * 8, "FB_BITMAP_BITS needs to be sizeof(FB_BITMAP_TYPE) * 8");

#define FB_MASK_IN_DATATYPE ((1<<FB_COLOR_IN_BITS_USED) - 1)

#define FB_ELEMENTS_X ((FB_SIZE_X + FB_PIXELS_IN_DATATYPE - 1) / FB_PIXELS_IN_DATATYPE)

#define FB_MISSINGRED_OUT   (8 - FB_RED_OUT_BITS)
#define FB_MISSINGGREEN_OUT (8 - FB_GREEN_OUT_BITS)
#define FB_MISSINGBLUE_OUT  (8 - FB_BLUE_OUT_BITS)

#define FB_SETRED_OUT(color) ((color >> FB_MISSINGRED_OUT) << (FB_GREEN_OUT_BITS + FB_BLUE_OUT_BITS))
#define FB_SETGREEN_OUT(color) ((color >> FB_MISSINGGREEN_OUT) << (FB_BLUE_OUT_BITS))
#define FB_SETBLUE_OUT(color) ((color >> FB_MISSINGBLUE_OUT) << 0)

FB_BITMAP_TYPE g_fbPixel[FB_ELEMENTS_X * FB_SIZE_Y];
FB_SCREENPOS_TYPE g_fbUseX = FB_SIZE_X;
FB_SCREENPOS_TYPE g_fbUseY = FB_SIZE_Y;

#define FB_OUTPUTBLOCKS_X (FB_SIZE_X / FB_OUTPUTBLOCK_X)
#define FB_OUTPUTBLOCKS_Y (FB_SIZE_Y / FB_OUTPUTBLOCK_Y)

/*This array is only used as performance booster.
  When no menu_screen_set is done between two menu_screen_clear, a write to the LCD
  can be skipped. Since we need to write on the So each pixel write sets the counter to 2. A flush must write
  the block if the counter is non 0 and dercreases the counter by 1. But on the very first
  flush, we have to write everything
*/
#define FB_WRITTENBLOCKS_PER_DATATYPE (FB_BITMAP_BITS / 2)

#define FB_WRITTENBLOCKS_X ((FB_OUTPUTBLOCKS_X + (FB_WRITTENBLOCKS_PER_DATATYPE - 1)) / FB_WRITTENBLOCKS_PER_DATATYPE)

#define FB_WRITTENMANAGED_BLOCKS_X (FB_WRITTENBLOCKS_X * FB_WRITTENBLOCKS_PER_DATATYPE)

#define FB_WRITTENBLOCKS_Y FB_OUTPUTBLOCKS_Y

FB_BITMAP_TYPE g_fbWrittenBlock[FB_WRITTENBLOCKS_X * FB_WRITTENBLOCKS_Y];
uint8_t g_fbWritten;

void menu_screen_set(FB_SCREENPOS_TYPE x, FB_SCREENPOS_TYPE y, FB_COLOR_IN_TYPE color) {
	if ((x < g_fbUseX) && (y < g_fbUseY)) {
		uint32_t index = x / FB_PIXELS_IN_DATATYPE + y * FB_ELEMENTS_X;

		FB_BITMAP_TYPE shift = (x % FB_PIXELS_IN_DATATYPE) * FB_COLOR_IN_BITS_USED;
		FB_BITMAP_TYPE mask = (FB_BITMAP_TYPE)FB_MASK_IN_DATATYPE << shift;
		FB_BITMAP_TYPE maskKeep = ~mask;
		g_fbPixel[index] &= maskKeep;
		g_fbPixel[index] |= (FB_BITMAP_TYPE)color << shift;

		//speed improvement
		uint32_t blockWritten = (x / FB_OUTPUTBLOCK_X) + (y / FB_OUTPUTBLOCK_Y) * FB_WRITTENMANAGED_BLOCKS_X;

		uint32_t indexWritten = blockWritten / (FB_BITMAP_BITS / 2);
		uint32_t offsetWritten = blockWritten % (FB_BITMAP_BITS / 2);
		FB_BITMAP_TYPE maskWrittenHigh = (FB_BITMAP_TYPE)2 << (offsetWritten * 2);
		g_fbWrittenBlock[indexWritten] |= maskWrittenHigh; //can now be 2 or 3. 3 is handled as it is a 2.
	}
}

FB_COLOR_IN_TYPE menu_screen_get(FB_SCREENPOS_TYPE x, FB_SCREENPOS_TYPE y) {
	if ((x < g_fbUseX) && (y < g_fbUseY)) {
		uint32_t index = x / FB_PIXELS_IN_DATATYPE + y * FB_ELEMENTS_X;
		FB_BITMAP_TYPE shift = (x % FB_PIXELS_IN_DATATYPE) * FB_COLOR_IN_BITS_USED;
		return (g_fbPixel[index] >> shift) & (FB_BITMAP_TYPE)FB_MASK_IN_DATATYPE;
	}
	return 0;
}

//block must have FB_OUTPUTBLOCK_X * FB_OUTPUTBLOCK_Y elements
static void FbBlockFlush(const uint16_t startX, const uint16_t startY, FB_COLOR_OUT_TYPE * block) {
	uint32_t wptr = 0;
	FB_BITMAP_TYPE colorData = 0; //contains data of 1..n pixels
	FB_BITMAP_TYPE colorIn; //contains data of 1 pixel
	FB_BITMAP_TYPE colorInLast = 0;
	FB_COLOR_OUT_TYPE colorOut = 0; //easy default, color in 0 is color out 0.

	uint32_t startCounter = (startX % FB_PIXELS_IN_DATATYPE);
	uint32_t startIndex = startX / FB_PIXELS_IN_DATATYPE;
	for (uint32_t y = startY; y < (uint32_t)(startY + FB_OUTPUTBLOCK_Y); y++) {
		uint32_t index = startIndex + y * FB_ELEMENTS_X;
		uint32_t counter = startCounter;
		if (counter != 0) { //first pixel is not at the beginning of the datatype
			colorData = g_fbPixel[index];
			colorData >>= FB_COLOR_IN_BITS_USED * (counter - 1);
		}
		for (uint32_t x = startX; x < (uint32_t)(startX + FB_OUTPUTBLOCK_X); x++) {
			if (counter == 0) {
				colorData = g_fbPixel[index];
			} else {
				colorData >>= FB_COLOR_IN_BITS_USED;
			}
			counter++;
			if (counter == FB_PIXELS_IN_DATATYPE) {
				counter = 0;
				index++;
			}

			colorIn = colorData & FB_MASK_IN_DATATYPE;
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
			uint32_t blockWritten = x + y * FB_WRITTENMANAGED_BLOCKS_X;
			uint32_t indexWritten = blockWritten / (FB_BITMAP_BITS / 2);
			uint32_t offsetWritten = blockWritten % (FB_BITMAP_BITS / 2);
			FB_BITMAP_TYPE maskWrittenHigh = (FB_BITMAP_TYPE)2 << (offsetWritten * 2);
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
	(void)level;
}

void menu_screen_clear(void) {
	memset(g_fbPixel, 0xFF, (FB_ELEMENTS_X * FB_SIZE_Y) * sizeof(FB_BITMAP_TYPE));
	if (g_fbWritten == 0) {
		//0x55 -> write each block 1x
		memset(g_fbWrittenBlock, 0x55, (FB_WRITTENBLOCKS_X * FB_WRITTENBLOCKS_Y) * sizeof(FB_BITMAP_TYPE));
		g_fbWritten = 1; //first frame needs to be written completely
	}
}
