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

#ifndef OTHER_H
 #define OTHER_H

//Variablen & Konstanten
extern const char game_pts[] PROGMEM;
extern const char intro1[16] PROGMEM;
extern u08 init_random_done;

//Funktionsprototypen:
void fill_1(void);
void fill_2(void);
void fill_3(void);
void wordtostr(char *s, u16 nummer, u08 digits, u08 strposition);
u08 max(u08 val1, u08 val2);
u08 min(u08 val1, u08 val2);

#endif
