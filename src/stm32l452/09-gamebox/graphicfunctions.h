/*
   Gamebox
    Copyright (C) 2004-2006  by Malte Marwedel
    m.marwedel AT onlinehome dot de

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

#ifndef GRAPHICFUNCTIONS_H
 #define GRAPHICFUNCTIONS_H


/* Wenn auf 1 gesetzt, so wird bei jedem verwenden von pixel_set eine
 Bereichsüberprüfung vorgenommen
Nachteil: Mehr Code und langsamer
Allerdings wird derzeit pixel_set nicht immer verwendet,
näheres dazu siehe graphicfunctions.c
*/
#define pixel_set_always_safe 0

//Universeller Speicher für eine Spalte/Zeile + 0 Zeichen
extern uint8_t linebuff[maxscreen+1];
extern uint8_t no_delays;


void clear_buff(void);
void load_buff(PGM_VOID_P x);

#if (pixel_set_always_safe == 0)
static __inline__ void pixel_set(uint8_t posx, uint8_t posy, uint8_t color) {
  gdata[posy][posx] = color;
}
#else
#define pixel_set pixel_set_safe
#endif

/* pixel_get ist leider nicht so kompakt wie das direkte verwenden
   von gdata[y][x] im Programmcode
   Daher verwende ich diese Makro nich immer
*/
static __inline__ uint8_t pixel_get(uint8_t posx, uint8_t posy) {
  return gdata[posy][posx];
}

void pixel_set_safe(uint8_t posx, uint8_t posy, uint8_t color);
void insert_buff_x(uint8_t y);
void insert_buff_y(uint8_t x);
void move_line_down (uint8_t x);
void move_down (void);
void move_line_up (uint8_t x);
void move_up (void);
void move_line_right (uint8_t y);
void move_right (void);
void move_line_left (uint8_t y);
void move_left (void);
void draw_line (uint8_t posx, uint8_t posy, int8_t lengthx, int8_t lengthy,
                uint8_t color, uint8_t overlay);
void flip_color(void);
void draw_box (uint8_t startx, uint8_t starty, uint8_t lengthx, uint8_t lengthy,
               uint8_t outercolor, uint8_t innercolor);
void clear_screen(void);
void showbin(u08 posx, u16 value, u08 oncolor);

#endif
