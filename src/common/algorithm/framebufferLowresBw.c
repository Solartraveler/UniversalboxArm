/* FramebufferLowresBw
(c) 2025 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause

Implements a framebuffer for devices with low memory.
This framebuffer is only black-white and the screen can be scaled to further
reduce the memory required. To further reduce memory, no buffer is used
to skip block writes, so a flush always writes the whole screen.

This code implements the callbacks from menu-interpreter.h and forwards the
result to boxlib/lcd.h.
But other graphic libraries than then menu-interpreter can be used.
*/

#include <alloca.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include "framebufferLowresBw.h"

#include "boxlib/lcd.h"

//For debug prints only:
//#include "main.h"


/*Includes from the project framebufferConfig.h to get the proper configuration.
The following defines are needed in the file:

For the size of the screen:
FB_SIZE_X
FB_SIZE_Y

Preferred data type for the architecture to use. eg uint32_t:
FB_BITMAP_TYPE

Number of bits in FB_BITMAP_TYPE eg 32:
FB_BITMAP_BITS

The integer type used for the coordinates. Like uint16_t:
FB_SCREENPOS_TYPE

Number of input pixels written to the LCD in once call, eg 16 and 8.
The number of output pixels written is this value multiplied with the settings
of g_fbScaleX and g_fbScaleY.
FB_OUTPUTBLOCK_X
FB_OUTPUTBLOCK_Y

Data type written to the LCD. Like uint16_t:
FB_COLOR_OUT_TYPE

Color for an input 1 at the LCD, eg 0x0 for black:
FB_COLOR_OUT_SET

Color for an input 0 at the LCD, eg 0xFFFF for white if FB_COLOR_OUT_TYPE uses 16 bit:
FB_COLOR_OUT_CLEAR

Number of elements in X direction in the used LCD, eg 320:
FB_OUTPUTBLOCK_LINE

Optional define
FB_TWOBUFFERS
to speed up the output if DMA is in use.

The approximate stack requirement of menu_screen_flush is without FB_TWOBUFFERS:
sizeof(FB_BITMAP_TYPE) * FB_OUTPUTBLOCK_X * g_fbScaleX * FB_OUTPUTBLOCK_Y * g_fbScaleY.
With FB_TWOBUFFERS defined, this value is doubled.

The approximate stack requirement of menu_screen_colorize_border is:
sizeof(FB_BITMAP_TYPE) * FB_OUTPUTBLOCK_LINE
*/

#include "framebufferConfig.h"

#define FB_ELEMENTS_X ((FB_SIZE_X + FB_BITMAP_BITS - 1) / FB_BITMAP_BITS)

static FB_BITMAP_TYPE g_fbPixel[FB_ELEMENTS_X * FB_SIZE_Y];

//size of the input data
static FB_SCREENPOS_TYPE g_fbUseX = FB_SIZE_X;
static FB_SCREENPOS_TYPE g_fbUseY = FB_SIZE_Y;

static uint16_t g_fbScaleX = 1;
static uint16_t g_fbScaleY = 1;

static uint16_t g_fbOffsetX = 0;
static uint16_t g_fbOffsetY = 0;


void menu_screen_set(FB_SCREENPOS_TYPE x, FB_SCREENPOS_TYPE y, FB_COLOR_IN_TYPE color) {
	if ((x < g_fbUseX) && (y < g_fbUseY)) {
		uint32_t index = x / FB_BITMAP_BITS + y * FB_ELEMENTS_X;
		uint32_t shift = x % FB_BITMAP_BITS;
		if (color) {
			g_fbPixel[index] &= ~(1 << shift);
		} else {
			g_fbPixel[index] |= (1 << shift);
		}
	}
}

FB_COLOR_IN_TYPE menu_screen_get(FB_SCREENPOS_TYPE x, FB_SCREENPOS_TYPE y) {
	if ((x < g_fbUseX) && (y < g_fbUseY)) {
		uint32_t index = x / FB_BITMAP_BITS + y * FB_ELEMENTS_X;
		uint32_t shift = x % FB_BITMAP_BITS;
		if (g_fbPixel[index] & (1 << shift)) {
			return 1;
		}
	}
	return 0;
}

static void FbBlockFlush(const uint16_t startX, const uint16_t startY, FB_COLOR_OUT_TYPE * pBlock) {
	FB_COLOR_OUT_TYPE * pBlockWp = pBlock;
	size_t lineItems = FB_OUTPUTBLOCK_X * g_fbScaleX;
	for (uint32_t y = 0; y < FB_OUTPUTBLOCK_Y; y++) {
		uint32_t bitmapIdxBase = (startY + y) * FB_ELEMENTS_X;
		FB_BITMAP_TYPE bitmapMask = 1 << (startX % FB_BITMAP_BITS);
		uint32_t bitmapIdxOffset = startX / FB_BITMAP_BITS;
		FB_BITMAP_TYPE pixelData = g_fbPixel[bitmapIdxBase + bitmapIdxOffset];
		for (uint32_t x = 0; x < FB_OUTPUTBLOCK_X; x++) {
			FB_COLOR_OUT_TYPE pixel;
			if (pixelData & bitmapMask) {
				pixel = FB_COLOR_OUT_SET;
			} else {
				pixel = FB_COLOR_OUT_CLEAR;
			}
			for (uint32_t z = 0; z < g_fbScaleX; z++) {
				*pBlockWp = pixel;
				pBlockWp++;
			}
			bitmapMask <<= 1;
			if (bitmapMask == 0) {
				bitmapMask = 1;
				bitmapIdxOffset++;
				//we do not need data when its the last loop. Otherwise we would get a read-bufferoverflow
				if ((x + 1) < FB_OUTPUTBLOCK_X) {
					pixelData = g_fbPixel[bitmapIdxBase + bitmapIdxOffset];
				}
			}
		}
		//now simply duplicate the line we have just calculated
		for (uint32_t z = 1; z < g_fbScaleY; z++) {
			memcpy(pBlockWp, pBlockWp - lineItems, lineItems * sizeof(FB_COLOR_OUT_TYPE));
			pBlockWp += lineItems;
		}
	}
	size_t len = g_fbScaleX * g_fbScaleY * FB_OUTPUTBLOCK_X * FB_OUTPUTBLOCK_Y * sizeof(FB_COLOR_OUT_TYPE);
	LcdWriteRect(g_fbOffsetX + startX * g_fbScaleX, g_fbOffsetY + startY * g_fbScaleY, g_fbScaleX * FB_OUTPUTBLOCK_X, g_fbScaleY * FB_OUTPUTBLOCK_Y, (const uint8_t*)pBlock, len);
}

void menu_screen_flush(void) {
	//uint32_t timeStart = HAL_GetTick();
	uint16_t xMax = g_fbUseX / FB_OUTPUTBLOCK_X;
	uint16_t yMax = g_fbUseY / FB_OUTPUTBLOCK_Y;
	size_t bytes = FB_OUTPUTBLOCK_X * FB_OUTPUTBLOCK_Y * sizeof(FB_COLOR_OUT_TYPE) * g_fbScaleX * g_fbScaleY;
#ifdef FB_TWOBUFFERS
	FB_COLOR_OUT_TYPE * blocks[2];
	blocks[0] = alloca(bytes);
	blocks[1] = alloca(bytes);
	FB_COLOR_OUT_TYPE * block = blocks[0];
	uint8_t toggle = 0;
#else
	FB_COLOR_OUT_TYPE * block = alloca(bytes);
#endif
	for (uint32_t y = 0; y < yMax; y++) {
		for (uint32_t x = 0; x < xMax; x++) {
			FbBlockFlush(x * FB_OUTPUTBLOCK_X, y * FB_OUTPUTBLOCK_Y, block);
#ifdef FB_TWOBUFFERS
			toggle = 1 - toggle;
			block = blocks[toggle];
#else
			LcdWaitBackgroundDone();
#endif
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
	memset(g_fbPixel, 0, (FB_ELEMENTS_X * FB_SIZE_Y) * sizeof(FB_BITMAP_TYPE));
}

void menu_screen_scale(uint16_t offsetX, uint16_t offsetY,
                       uint16_t scaleX, uint16_t scaleY) {
	g_fbScaleX = scaleX;
	g_fbScaleY = scaleY;
	g_fbOffsetX = offsetX;
	g_fbOffsetY = offsetY;
}

void menu_screen_colorize_border(FB_COLOR_OUT_TYPE color, uint16_t sizeX, uint16_t sizeY) {
	if (sizeX > FB_OUTPUTBLOCK_LINE) {
		return; //error, FB_OUTPUTBLOCK_MAX not large enough
	}
	FB_COLOR_OUT_TYPE block[FB_OUTPUTBLOCK_LINE];
	//TODO: How do we need to swap for 3byte output data?
	color = (color >> 8) | (color << 8);
	for (uint16_t i = 0; i < FB_OUTPUTBLOCK_LINE; i++) {
		block[i] = color;
	}
	//left side
	if (g_fbOffsetX > 0) {
		for (uint16_t y = 0; y < sizeY; y++) {
			LcdWriteRect(0, y, g_fbOffsetX, 1, (const uint8_t*)block, g_fbOffsetX * sizeof(FB_COLOR_OUT_TYPE));
		}
	}
	//right side
	uint16_t borderMiddle = g_fbScaleX * g_fbUseX;
	uint16_t endRight = g_fbOffsetX + borderMiddle;
	if (endRight < sizeX) {
		uint16_t borderRight = sizeX - endRight;
		for (uint16_t y = 0; y < sizeY; y++) {
			LcdWriteRect(endRight, y, borderRight, 1, (const uint8_t*)block, borderRight * sizeof(FB_COLOR_OUT_TYPE));
		}
	}
	//top side
	if (g_fbOffsetY > 0) {
		for (uint16_t y = 0; y < g_fbOffsetY; y++) {
			LcdWriteRect(g_fbOffsetX, y, borderMiddle, 1, (const uint8_t*)block, borderMiddle * sizeof(FB_COLOR_OUT_TYPE));
		}
	}
	//bottom side
	uint16_t endBottom = g_fbOffsetY + g_fbScaleY * g_fbUseY;
	if (endBottom < sizeY) {
		for (uint16_t y = endBottom; y < sizeY; y++) {
			LcdWriteRect(g_fbOffsetX, y, borderMiddle, 1, (const uint8_t*)block, borderMiddle * sizeof(FB_COLOR_OUT_TYPE));
		}
	}
	LcdWaitBackgroundDoneRelease();
}




