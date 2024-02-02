/*
   Gamebox
    Copyright (C) 2023 by Malte Marwedel
    m.talk AT marwedels dot de

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "main.h"

#include "FreeRTOS.h"

#include "framebufferLowres.h"

uint8_t volatile gdata[screeny][screenx];

uint8_t gdataShadow[screeny][screenx];

void resync_led_display(void) {
}


static void GraphicDrawPixel(uint16_t x, uint16_t y, uint8_t color) {
	uint8_t red = color & 0x03;
	uint8_t green = (color & 0x30) >> 4;
	uint8_t colorNew = (red << 2) | green;
	menu_screen_set(x, y, colorNew);
}

bool GraphicUpdate(void) {
	bool changed = false;
	for (uint16_t py = 0; py < screeny; py++) {
		for (uint16_t px = 0; px < screenx; px++) {
			uint8_t color = gdata[py][px];
			if (color != gdataShadow[py][px]) {
				gdataShadow[py][px] = color;
				changed = true;
				GraphicDrawPixel(px, py, color);
			}
		}
	}
	if (changed) {
		menu_screen_flush();
	}
	return changed;
}
