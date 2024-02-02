#pragma once

#include "menu-interpreter.h"

//define maximum screen size in x direction
#define FB_SIZE_X MENU_SCREEN_X
//define maximum screen size in x direction
#define FB_SIZE_Y MENU_SCREEN_Y

//define how many pixels in each direction share the same front and back color
#define FB_COLOR_RES_X 4
#define FB_COLOR_RES_Y 4


//define the datatype of which the color is delivered
#define FB_COLOR_IN_TYPE SCREENCOLOR

//define the datatype of which the color is needed for the display
#define FB_COLOR_OUT_TYPE uint16_t

//define the datatype of which the coordinates are delivered
#define FB_SCREENPOS_TYPE SCREENPOS

//default level at which a color should be at the front
#define FB_FRONT_LEVEL 100

#ifdef MENU_SCREEN_COLORCUSTOM1
#define FB_RED_IN_BITS MENU_COLOR_CUSTOM1_RED_BITS
#define FB_GREEN_IN_BITS MENU_COLOR_CUSTOM1_GREEN_BITS
#define FB_BLUE_IN_BITS MENU_COLOR_CUSTOM1_BLUE_BITS
#endif

#ifdef MENU_SCREEN_COLORCUSTOM2
#define FB_RED_IN_BITS MENU_COLOR_CUSTOM2_RED_BITS
#define FB_GREEN_IN_BITS MENU_COLOR_CUSTOM2_GREEN_BITS
#define FB_BLUE_IN_BITS MENU_COLOR_CUSTOM2_BLUE_BITS
#endif

#ifdef MENU_SCREEN_COLORCUSTOM3
#define FB_RED_IN_BITS MENU_COLOR_CUSTOM3_RED_BITS
#define FB_GREEN_IN_BITS MENU_COLOR_CUSTOM3_GREEN_BITS
#define FB_BLUE_IN_BITS MENU_COLOR_CUSTOM3_BLUE_BITS
#endif

#define FB_RED_OUT_BITS 5
#define FB_GREEN_OUT_BITS 6
#define FB_BLUE_OUT_BITS 5

//Depending of the target platform, its best to use 8, 16, 32 or 64 bit for the bitmask
#define FB_BITMAP_TYPE uint32_t
//How many bits should be saved in FB_BITMAP_TYPE? There may *not* be bits left unused!
#define FB_BITMAP_BITS 32

/*Must be a fraction of FB_SIZEX, FB_SIZEY
  This block is transferred to the LCD as one write. It needs to be able to be
  stored on the stack. And can be skipped if it is just the background color and
  was so already for the previous frame
*/
#define FB_OUTPUTBLOCK_X 32
#define FB_OUTPUTBLOCK_Y 16

/*Define to transfer one output block by DMA, while another one is prepared.
  This doubles the required stack.
*/
#define FB_TWOBUFFERS
