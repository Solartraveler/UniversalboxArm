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

#ifndef PXXO_H
 #define PXXO_H

/* Interne als static deklarierte Funktionen:
static u08 xxo_field_get(struct xxo_spielfeldstruct *spielfeld, u08 posx,
                         u08 posy);
static __inline__ u08 xxo_field_get_quick(struct xxo_spielfeldstruct *spielfeld,
       u08 posx, u08 posy);
static void xxo_field_set(struct xxo_spielfeldstruct *spielfeld, u08 posx,
                          u08 posy, u08 value);
static void xxo_drawmenu1(u08 wahl);
static void xxo_drawmenu2(u08 players);
static void xxo_drawmenu3(u08 wahl);
static u08 xxo_selectmode(struct xxo_spielfeldstruct *spielfeld, u08 *aipower);
static void xxo_drawgame(struct xxo_spielfeldstruct *spielfeld);
static u08 xxo_placestone_visual(struct xxo_spielfeldstruct *spielfeld,
                                 u08 playerturn, u08 posx);
static u08 xxo_placestone(struct xxo_spielfeldstruct *spielfeld, u08 playerturn,
                          u08 posx);
static u08 xxo_movehuman(struct xxo_spielfeldstruct *spielfeld, u08 playerturn,
                         u08 playeroldpos);
static void xxo_vote_inarow(u08 inarow1, u08 inarow2, u16 *vote1, u16 *vote2);
static s16 xxo_votegame(struct xxo_spielfeldstruct *spielfeld, u08 playerturn);
static s16 xxo_voteplace_quick(struct xxo_spielfeldstruct *spielfeld, u08 posx,
                               u08 posy);
static u08 xxo_is_tie(struct xxo_spielfeldstruct *spielfeld);
static void xxo_highlightwinner(struct xxo_spielfeldstruct *spielfeld);
static u16 xxo_calcai(struct xxo_spielfeldstruct *testfeld, u08 playerturn,
                      u08 oplayer, u08 remdepth, u08 firstcall);
static void xxo_moveai(struct xxo_spielfeldstruct *spielfeld, u08 playerturn,
                u08 aipower);
*/

void xxo_start(void);


#endif
