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

#include "main.h"


const char game_pts[] PROGMEM = "Pts:";

//Leichtes grün
const char intro1[16] PROGMEM = {0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,
                                 0x10,0x10,0x10,0x10,0x10,0x10,0x10};

u08 init_random_done = 0;

void fill_1(void) {
//Mit leichtem Grün füllen
u08 x,y;
for (x = 0; x < screenx; x++) {
  for (y = 0; y < screeny; y++) {
    gdata[x][y] = 0x10;//Grün füllen
  }
}
}

void fill_2(void) {
//Grün füllen, ping pong mäßig
u08 x,y;
for (x = 0; x < (screenx/2); x++) {
  for (y = 0; y < screeny; y++) {
    gdata[x*2][y] = 0x10;
    waitms(20);
  }
  for (y = screeny; y > 0; y--) {
    gdata[x*2+1][y-1] = 0x10;
    waitms(20);
  }
}
}

void fill_3(void) {
//Ausblenden nach links und rechts, auf Inhalt von linebuff achten!
u08 x,y;
for (x = 0; x < screenx; x++) {
  for (y = 0; y < screeny; y++) {
    if ((y % 2) == 0) {
     move_line_left(y);
   } else {
     move_line_right(y);
   }
   waitms(5); //1,28sec fürs Auflösen des Bildes
  }
}
}

void wordtostr(char *s, u16 nummer, u08 digits, u08 strposition) {
s += strposition;
while (digits > 0) {
  digits--;
  *(s+digits) = nummer%10+48;
  nummer = nummer / 10;
}
}

u08 max(u08 val1, u08 val2) {
if (val1 > val2) {
  return val1;
}
return val2;
}

u08 min(u08 val1, u08 val2) {
if (val1 > val2) {
  return val2;
}
return val1;
}
