#pragma once

#include "framebufferConfig.h"

//Callback used by menuInterpreter
void menu_screen_set(FB_SCREENPOS_TYPE x, FB_SCREENPOS_TYPE y, FB_COLOR_IN_TYPE color);

//Optional readback function, interesting to take a screenshot or similar things
FB_COLOR_IN_TYPE menu_screen_get(FB_SCREENPOS_TYPE x, FB_SCREENPOS_TYPE y);

//Callback used by menuInterpreter
void menu_screen_flush(void);

//Callback used by menuInterpreter
void menu_screen_clear(void);

//using a smaller size than the maxmimum speeds things up when calling menu_screen_flush
//The values may not be larger than FB_SIZE_X, FB_SIZE_Y.
void menu_screen_size(FB_SCREENPOS_TYPE x, FB_SCREENPOS_TYPE y);

//Returns the values set by menu_screen_size
void menu_screen_size_get(FB_SCREENPOS_TYPE * pX, FB_SCREENPOS_TYPE * pY);
