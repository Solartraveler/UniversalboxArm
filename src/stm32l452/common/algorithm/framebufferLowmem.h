#pragma once

#include "framebufferConfig.h"

//Callback used by menuInterpreter
void menu_screen_set(FB_SCREENPOS_TYPE x, FB_SCREENPOS_TYPE y, FB_COLOR_IN_TYPE color);

//Callback used by menuInterpreter
void menu_screen_flush(void);

//Callback used by menuInterpreter
void menu_screen_clear(void);

//using a smaller size than the maxmimum speeds things up when calling menu_screen_flush
//The values may not be larger than FB_SIZE_X, FB_SIZE_Y.
void menu_screen_size(FB_SCREENPOS_TYPE x, FB_SCREENPOS_TYPE y);

/*
Level until the sum of red + green + blue, shifted to the left to give an 8
bit value, is considered as front color. So anything in the range of 1 to 765
might give the best results.
*/
void menu_screen_frontlevel(uint16_t level);
