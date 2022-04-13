/* MenuInterpreter
   (c) 2004-2006, 2020  by Malte Marwedel
   m DOT talk AT marwedels DOT de
   www.marwedels.de/malte
   menudesigner.sourceforge.net

   This Source Code Form is subject to the terms of the
   Mozilla Public License, v. 2.0.
   If a copy of the MPL was not distributed with this file,
   You can obtain one at https://mozilla.org/MPL/2.0/.
*/
//SPDX-License-Identifier: MPL-2.0

#ifndef MENU_TEXT_H
 #define MENU_TEXT_H

#include <stdint.h>

#include "menu-interpreter.h"
/* menu_draw_char does a range check of posx and posy before drawing. So even
negative values would be no problem.
transparency: if 0, background pixels are overwritten with the background color
              if 1, no modification of background pixels is done.
*/
uint8_t menu_char_draw(SCREENPOS posx, SCREENPOS posy, uint8_t font, char c, uint8_t transparency);

/*
Returns the height of the selected font in pixel
*/
uint8_t menu_font_heigth(uint8_t font);

#endif
