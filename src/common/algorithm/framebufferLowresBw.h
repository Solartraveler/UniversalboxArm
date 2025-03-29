#pragma once

#include "framebufferConfig.h"

#include "framebuffer.h"

/*
Dummy function for compatibility with framebufferLowmem
*/
void menu_screen_frontlevel(uint16_t level);

void menu_screen_scale(uint16_t offsetX, uint16_t offsetY,
                       uint16_t scaleX, uint16_t scaleY);

/* Draws everything, which menu_screen_clear misses, so menu_screen_scale and
   should have been called first. sizeX and sizeY should be the real LCD pixel numbers.
*/
void menu_screen_colorize_border(FB_COLOR_OUT_TYPE color, uint16_t sizeX, uint16_t sizeY);
