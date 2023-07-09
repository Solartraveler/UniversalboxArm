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

#ifndef PREV_H
 #define PREV_H

//Spielfeld Größe
//Grenzen: 15x15 kann das Programm verwalten
//         12x12 kann mit den LEDs dargestellt werden
#define rev_x 8
#define rev_y 8

/* Interne als static deklarierte Funktionen:
static u08 rev_field_get(struct rev_spielfeldstruct *spielfeld, u08 posx,
                         u08 posy);
static __inline__ u08 rev_field_get_quick(struct rev_spielfeldstruct *spielfeld,
       u08 posx, u08 posy);
static void rev_field_set(struct rev_spielfeldstruct *spielfeld, u08 posx,
                          u08 posy, u08 value);
static void rev_drawmenu(u08 players);
static u08 rev_selectmode(struct rev_spielfeldstruct *spielfeld);
static void rev_drawgame(struct rev_spielfeldstruct *spielfeld);
static u08 rev_placestone_dir_test(struct rev_spielfeldstruct *spielfeld,
                           u08 playerturn, u08 posx, u08 posy, s08 dx, s08 dy);
static u08 rev_placestone_dir(struct rev_spielfeldstruct *spielfeld,
                           u08 playerturn, u08 posx, u08 posy, s08 dx, s08 dy);
static u08 rev_placestone_test(struct rev_spielfeldstruct *spielfeld,
                                 u08 playerturn, u08 posx, u08 posy);
static u08 rev_placestone(struct rev_spielfeldstruct *spielfeld,
                                 u08 playerturn, u08 posx, u08 posy);
static u08 rev_placestone_visual(struct rev_spielfeldstruct *spielfeld,
                                 u08 playerturn, u08 posx, u08 posy);
static u08 rev_movehuman(struct rev_spielfeldstruct *spielfeld, u08 playerturn,
                         u08 opos);
static s16 rev_votegame(struct rev_spielfeldstruct *spielfeld, u08 playerturn);
static u08 rev_pos_count(struct rev_spielfeldstruct *spielfeld, u08 playerturn);
static u16 rev_count_stones(struct rev_spielfeldstruct *spielfeld,
                            u08 playerturn);
static void rev_highlightwinner(struct rev_spielfeldstruct *spielfeld);
static void rev_fastcopy(struct rev_spielfeldstruct *target,
                         struct rev_spielfeldstruct *source);
static s16 rev_calcai(struct rev_spielfeldstruct *testfeld, u08 playerturn,
                      u08 oplayer, u08 remdepth);
static u08 rev_calcai_head(struct rev_spielfeldstruct *spielfeld,
                           u08 playerturn);
static void rev_moveai(struct rev_spielfeldstruct *spielfeld, u08 playerturn);
*/

void reversi_start(void);


#endif
