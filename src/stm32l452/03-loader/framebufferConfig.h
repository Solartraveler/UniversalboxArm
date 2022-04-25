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

