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

#ifndef PTETRIS_H
 #define PTETRIS_H

/* Static Funktionen in ptetris.c:
static void tetris_newblock(struct tetris_blockstruct *theblock);
static u08 tetris_checkcollide(struct tetris_blockstruct *oldblock,
                 struct tetris_blockstruct *newblock);
static u08 tetris_moveblock(struct tetris_blockstruct *theblock, s08 movex,
                            s08 movey);
static void tetris_removeline(u08 liney);
static u08 tetris_checkline(void);
static void tetris_rotateblock(struct tetris_blockstruct *theblock);
*/

void tetris_start(void);

#endif
