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

#if modul_sram

u16 volatile minstack = 0xffff;
u16 volatile maxheap;

const char ram_overflow[] PROGMEM = "Stack-Heap overlap possilbe!";
const char ram_free[] PROGMEM = "Min free RAM Bytes";

void ram_showfree(void) {
u16 volatile freeram;
cli();
if (minstack > maxheap) { //alles in Ordnung
  freeram = minstack-maxheap;
  sei();
  load_text(ram_free);
  scrolltext(1, 0x30,0x00,140);
  draw_tinynumber(freeram, 0, 10, 0x31);
} else { //overflow possible
  sei();
  load_text(ram_overflow);
  scrolltext(1, 0x03,0x00,140);
}
userin_flush();
while (userin_press() == 0); //Warten auf Tastendruck
}

#endif
