/* FramebufferLowres
(c) 2023 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause

Implemets a framebuffer for devices with low memory.
Instead of storing the color information for every pixel, only a front and back
color can be defined. Then the front and back color can be set for every large
block.

This code implements the callbacks from menu-interpreter.h and forwards the
result to boxlib/lcd.h.
But other graphic libraries than then menu-interpreter can be used.

This variant does upscaling (without interpolating) and uses a palette.

*/

#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include "framebufferLowres.h"

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

#define FB_COLORS (1 << FB_COLOR_IN_BITS_USED)

FB_COLOR_OUT_TYPE g_fbPalette[FB_COLORS];

//size of the input data
FB_SCREENPOS_TYPE g_fbUseX = FB_SIZE_X;
FB_SCREENPOS_TYPE g_fbUseY = FB_SIZE_Y;

FB_SCREENPOS_TYPE g_fbScaleX = 1;
FB_SCREENPOS_TYPE g_fbScaleY = 1;

FB_SCREENPOS_TYPE g_fbOffsetX = 0;
FB_SCREENPOS_TYPE g_fbOffsetY = 0;


/*This array is only used as performance booster.
  When no menu_screen_set is done between two menu_screen_clear, a write to the LCD
  can be skipped. Since we need to write on the So each pixel write sets the counter to 2. A flush must write
  the block if the counter is non 0 and dercreases the counter by 1. But on the very first
  flush, we have to write everything
*/
#define FB_WRITTENPIXELS_PER_DATATYPE (FB_BITMAP_BITS / 2)

#define FB_WRITTENPIXELS_X ((FB_SIZE_X + (FB_WRITTENPIXELS_PER_DATATYPE - 1)) / FB_WRITTENPIXELS_PER_DATATYPE)

#define FB_WRITTENMANAGED_PIXELS_X (FB_WRITTENPIXELS_X * FB_WRITTENPIXELS_PER_DATATYPE)

#define FB_WRITTENPIXELS_Y FB_SIZE_Y

FB_BITMAP_TYPE g_fbWrittenPixel[FB_WRITTENPIXELS_X * FB_WRITTENPIXELS_Y];
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
		uint32_t pixelWritten = x  + y * FB_WRITTENMANAGED_PIXELS_X;

		uint32_t indexWritten = pixelWritten / (FB_BITMAP_BITS / 2);
		uint32_t offsetWritten = pixelWritten % (FB_BITMAP_BITS / 2);
		FB_BITMAP_TYPE maskWrittenHigh = (FB_BITMAP_TYPE)2 << (offsetWritten * 2);
		g_fbWrittenPixel[indexWritten] |= maskWrittenHigh; //can now be 2 or 3. 3 is handled as it is a 2.
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


static void FbBlockFlush(const uint16_t x, const uint16_t y,  FB_COLOR_OUT_TYPE * block) {
	//1. get the color
	uint32_t index = x / FB_PIXELS_IN_DATATYPE + y * FB_ELEMENTS_X;

	FB_BITMAP_TYPE shift = (x % FB_PIXELS_IN_DATATYPE) * FB_COLOR_IN_BITS_USED;
	FB_BITMAP_TYPE colorIn = (g_fbPixel[index] >> shift) & (FB_BITMAP_TYPE)FB_MASK_IN_DATATYPE;
	FB_COLOR_OUT_TYPE colorOut = g_fbPalette[colorIn];
	//2. create array
	for (uint32_t i = 0; i < g_fbScaleX * g_fbScaleY; i++) {
		block[i] = colorOut;
	}
	//3. write array to display
	uint16_t posX = g_fbOffsetX + g_fbScaleX * x;
	uint16_t posY = g_fbOffsetY + g_fbScaleY * y;
	LcdWriteRect(posX, posY, g_fbScaleX, g_fbScaleY, (const uint8_t*)block, g_fbScaleX * g_fbScaleY * sizeof(FB_COLOR_OUT_TYPE));
}

void menu_screen_flush(void) {
	//uint32_t timeStart = HAL_GetTick();
#ifdef FB_TWOBUFFERS
	FB_COLOR_OUT_TYPE blocks[2][FB_OUTPUTBLOCK_PIXEL];
	FB_COLOR_OUT_TYPE * block = blocks[0];
	uint8_t toggle = 0;
#else
	FB_COLOR_OUT_TYPE block[FB_OUTPUTBLOCK_PIXEL];
#endif
	for (uint32_t y = 0; y < g_fbUseY; y++) {
		for (uint32_t x = 0; x < g_fbUseX; x++) {
			uint32_t pixelWritten = x + y * FB_WRITTENMANAGED_PIXELS_X;
			uint32_t indexWritten = pixelWritten / (FB_BITMAP_BITS / 2);
			uint32_t offsetWritten = pixelWritten % (FB_BITMAP_BITS / 2);
			FB_BITMAP_TYPE maskWrittenHigh = (FB_BITMAP_TYPE)2 << (offsetWritten * 2);
			FB_BITMAP_TYPE maskWrittenLow = 1 << (offsetWritten * 2);
			FB_BITMAP_TYPE bitsWritten = g_fbWrittenPixel[indexWritten];
			if (((maskWrittenHigh | maskWrittenLow) & bitsWritten)) {
				FbBlockFlush(x, y, block);
				if (bitsWritten & maskWrittenHigh) { //if 2 -> 1, if 3 -> 1
					bitsWritten = (bitsWritten & ~maskWrittenHigh) | maskWrittenLow;
				} else { //if 1 -> 0
					bitsWritten = bitsWritten & ~maskWrittenLow;
				}
				g_fbWrittenPixel[indexWritten] = bitsWritten;
#ifdef FB_TWOBUFFERS
				toggle = 1 - toggle;
				block = blocks[toggle];
#endif
			}
		}
	}
	LcdWaitBackgroundDone(); //is an empty function if DMA is not used
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
	memset(g_fbPixel, FB_COLOR_CLEAR, (FB_ELEMENTS_X * FB_SIZE_Y) * sizeof(FB_BITMAP_TYPE));
	if (g_fbWritten == 0) {
		//0x55 -> write each block 1x
		memset(g_fbWrittenPixel, 0x55, (FB_WRITTENPIXELS_X * FB_WRITTENPIXELS_Y) * sizeof(FB_BITMAP_TYPE));
		g_fbWritten = 1; //first frame needs to be written completely
	}
}

void menu_screen_scale(FB_SCREENPOS_TYPE offsetX, FB_SCREENPOS_TYPE offsetY,
                       FB_SCREENPOS_TYPE scaleX, FB_SCREENPOS_TYPE scaleY) {
	g_fbScaleX = scaleX;
	g_fbScaleY = scaleY;
	g_fbOffsetX = offsetX;
	g_fbOffsetY = offsetY;
}

void menu_screen_palette_set(FB_COLOR_IN_TYPE in, FB_SCREENPOS_TYPE out) {
	if (in < FB_COLORS) {
		//due to little endian, we need to swap the bytes for the transfer later.
		//TODO: How do we need to swap for 3byte output data?
		out = (out >> 8) | (out << 8);
		g_fbPalette[in] = out;
	}
}

void menu_screen_colorize_border(FB_COLOR_OUT_TYPE color, FB_SCREENPOS_TYPE sizeX, FB_SCREENPOS_TYPE sizeY) {
	if (sizeX > FB_OUTPUTBLOCK_LINE) {
		return; //error, FB_OUTPUTBLOCK_MAX not large enough
	}
	FB_COLOR_OUT_TYPE block[FB_OUTPUTBLOCK_LINE];
	//TODO: How do we need to swap for 3byte output data?
	color = (color >> 8) | (color << 8);
	for (FB_SCREENPOS_TYPE i = 0; i < FB_OUTPUTBLOCK_PIXEL; i++) {
		block[i] = color;
	}
	//left side
	if (g_fbOffsetX > 0) {
		for (FB_SCREENPOS_TYPE y = 0; y < sizeY; y++) {
			LcdWriteRect(0, y, g_fbOffsetX, 1, (const uint8_t*)block, g_fbOffsetX * sizeof(FB_COLOR_OUT_TYPE));
		}
	}
	//right side
	FB_SCREENPOS_TYPE borderMiddle = g_fbScaleX * g_fbUseX;
	FB_SCREENPOS_TYPE endRight = g_fbOffsetX + borderMiddle;
	if (endRight < sizeX) {
		FB_SCREENPOS_TYPE borderRight = sizeX - endRight;
		for (FB_SCREENPOS_TYPE y = 0; y < sizeY; y++) {
			LcdWriteRect(endRight, y, borderRight, 1, (const uint8_t*)block, borderRight * sizeof(FB_COLOR_OUT_TYPE));
		}
	}
	//top side
	if (g_fbOffsetY > 0) {
		for (FB_SCREENPOS_TYPE y = 0; y < g_fbOffsetY; y++) {
			LcdWriteRect(g_fbOffsetX, y, borderMiddle, 1, (const uint8_t*)block, borderMiddle * sizeof(FB_COLOR_OUT_TYPE));
		}
	}
	//bottom side
	FB_SCREENPOS_TYPE endBottom = g_fbOffsetY + g_fbScaleY * g_fbUseY;
	if (endBottom < sizeY) {
		for (FB_SCREENPOS_TYPE y = endBottom; y < sizeY; y++) {
			LcdWriteRect(g_fbOffsetX, y, borderMiddle, 1, (const uint8_t*)block, borderMiddle * sizeof(FB_COLOR_OUT_TYPE));
		}
	}
	LcdWaitBackgroundDone();
}




