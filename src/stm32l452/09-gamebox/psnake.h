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

#ifndef PSNAKE_H
 #define PSNAKE_H

/* Interne Funktionen, deklariert als static
static void snake_fifo_init(struct snake_fifostruct *fifo);
static s08 snake_fifo_put(struct snake_fifostruct *fifo, u08 value);
static u08 snake_fifo_get(struct snake_fifostruct *fifo);
static void snake_placestart(u08 position);
static void snake_place(u08 position);
static void snake_placeend(u08 position);
static void snake_placeclear(u08 position);
static u08 snake_placeapple(void);
static s16 snake_move(u08 position, u08 direction);
*/

void snake_start(void);

#endif
