/*
   Gamebox
    Copyright (C) 2004-2006, 2023  by Malte Marwedel
    m.talk AT marwedels dot de

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

#pragma once

#include <stdint.h>
#include <stdbool.h>

/*Definieren der LED Feld Größe.
Achtung: Alle Funktionen wurden nur mit screenx = 16 und screeny = 16 getestet.
Manche Funktionen sind nicht explizit dafür ausgelegt, mit größeren
'Auflösungen' zu funktionieren. Mit 'Auflösungen' die mehr als 254 Pixel
Kantenlänge haben, werden die Funktionieren nicht funktionieren.
*/
#define screenx 16
#define screeny 16

#if (screenx > screeny)
#define maxscreen screenx
#else
#define maxscreen screeny
#endif

//'Auflösungen' unterhalb 16x16 würden in der Demo Fehler verursachen
#if ((screenx < 16) || (screeny < 16))
#error "Mit screenx oder screeny kleiner als 16 kann die Demo nicht funktionieren!"
#endif


//Die Daten fürs Display[y;Zeile][x;Spalte]
extern uint8_t volatile gdata[screeny][screenx];

//dummy function
void resync_led_display(void);

//sizeX and sizeY should be the LCD screen resolution
bool GraphicUpdate(void);
