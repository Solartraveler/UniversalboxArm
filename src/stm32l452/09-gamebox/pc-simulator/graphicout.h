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

//Funktionsprototypen f�r Graphic
#ifndef GRAPHICINT_H
 #define GRAPHICINT_H


//Konstanten, �ndern nur mit Vorsicht

/*Definieren der LED Feld Gr��e.
Achtung: Alle Funktionen wurden nur mit screenx = 16 und screeny = 16 getestet.
Manche Funktionen sind nicht explizit daf�r ausgelegt, mit gr��eren
'Aufl�sungen' zu funktionieren. Mit 'Aufl�sungen' die mehr als 254 Pixel
Kantenl�nge haben, werden die Funktionieren nicht funktionieren.
*/
#define screenx 16
#define screeny 16

#if (screenx > screeny)
#define maxscreen screenx
#else
#define maxscreen screeny
#endif

//'Aufl�sungen' unterhalb 16x16 w�rden in der Demo Fehler verursachen
#if ((screenx < 16) || (screeny < 16))
#error "Mit screenx oder screeny kleiner als 16 kann die Demo nicht funktionieren!"
#endif


/* Wird use_low_colors auf eins gesetzt, so wird kein Software PWM zum steuern
   der LED Helligkeit verwendet. Die Farbtiefe wird folglich von 4 auf 2 Bit
   reduziert. Die Demo sieht mit 2Bit Farben doch recht langweilig aus.
*/
#define use_low_colors 0

//Die Daten f�rs Display[y;Zeile][x;Spalte]
extern uint8_t volatile gdata[screeny][screenx];
void resync_led_display(void);
void init_window(void);

/* In i386/graphicout.c als static deklariert:
static void drawboard(void);
static void redraw(int param);
static void update_window_size(int width, int height);
*/

#endif
