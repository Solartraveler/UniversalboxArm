#pragma once

//define maximum screen size in x direction
#define FB_SIZE_X 16
//define maximum screen size in y direction
#define FB_SIZE_Y 16

//define the datatype of which the color is delivered
#define FB_COLOR_IN_TYPE uint8_t

//define the datatype of which the color is needed for the display
#define FB_COLOR_OUT_TYPE uint16_t

//Index of the palette for the clear color, so the value must be
// below (1 << (FB_RED_IN_BITS +FB_GREEN_IN_BITS + FB_BLUE_IN_BITS)
#define FB_COLOR_CLEAR 0x0

//define the datatype of which the coordinates are delivered
#define FB_SCREENPOS_TYPE uint16_t

#define FB_RED_IN_BITS 2
#define FB_GREEN_IN_BITS 2
#define FB_BLUE_IN_BITS 0

#define FB_RED_OUT_BITS 5
#define FB_GREEN_OUT_BITS 6
#define FB_BLUE_OUT_BITS 5

//Depending of the target platform, its best to use 8, 16, 32 or 64 bit for the bitmask
#define FB_BITMAP_TYPE uint32_t
//Should be sizeof(FB_BITMAP_TYPE) * 8. As this is to be resolved by the preprocessor, sizeof can not be used here
#define FB_BITMAP_BITS 32

//Number of pixels which will be one pixel scaled
#define FB_OUTPUTBLOCK_PIXEL ((240 / FB_SIZE_Y) * (240 / FB_SIZE_Y))

//Should be enought to hold one line of pixels
#define FB_OUTPUTBLOCK_LINE 320

/*Define to transfer one output block by DMA, while another one is prepared.
  This doubles the required stack.
*/
#define FB_TWOBUFFERS
