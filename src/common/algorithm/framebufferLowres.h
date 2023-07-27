#pragma once

#include "framebufferConfig.h"

#include "framebuffer.h"

/*
Dummy function for compatibility with framebufferLowmem
*/
void menu_screen_frontlevel(uint16_t level);

void menu_screen_scale(FB_SCREENPOS_TYPE offsetX, FB_SCREENPOS_TYPE offsetY,
                       FB_SCREENPOS_TYPE scaleX, FB_SCREENPOS_TYPE scaleY);

void menu_screen_palette_set(FB_COLOR_IN_TYPE in, FB_COLOR_OUT_TYPE out);

/* Draws everything, which menu_screen_clear misses, so menu_screen_scale and
   should have been called first. sizeX and sizeY should be the real LCD pixel numbers.
*/
void menu_screen_colorize_border(FB_COLOR_OUT_TYPE color, FB_SCREENPOS_TYPE sizeX, FB_SCREENPOS_TYPE sizeY);
