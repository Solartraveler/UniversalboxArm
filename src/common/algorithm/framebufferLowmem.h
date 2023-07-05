#pragma once

#include "framebufferConfig.h"

#include "framebuffer.h"

/*
Level until the sum of red + green + blue, shifted to the left to give an 8
bit value, is considered as front color. So anything in the range of 1 to 765
might give the best results.
*/
void menu_screen_frontlevel(uint16_t level);
