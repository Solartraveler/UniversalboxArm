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

#ifndef USERINPUTPC_H
 #define USERINPUTPC_H

struct userinputstruct{
s08 volatile x;
s08 volatile y;
s08 volatile z;
u08 volatile left;
u08 volatile right;
u08 volatile up;
u08 volatile down;
u08 volatile press;
};

struct userinputcalibstruct{
s08 volatile min;
u16 volatile zero;
s08 volatile max;
};

extern struct userinputstruct userin;
extern struct userinputcalibstruct calib_x;
extern struct userinputcalibstruct calib_y;

#if modul_calib_save
void calib_load(void);
void calib_save(void);
#endif
/* Funktion ist static
static void showbin(u08 posx, u16 value, u08 oncolor);
*/
void input_calib(void);
void input_select(void);
void input_key_key (unsigned char key, int x, int y);
void input_key_cursor (int key, int x, int y);
void input_mouse_key(int button, int state, int x, int y);
void input_mouse_move(int x, int y);
u08 userin_left(void);
u08 userin_right(void);
u08 userin_up(void);
u08 userin_down(void);
u08 userin_press(void);
void userin_flush(void);

#endif
