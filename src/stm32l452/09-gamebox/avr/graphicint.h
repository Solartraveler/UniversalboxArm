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

//Funktionsprototypen für Graphic
#ifndef GRAPHICINT_H
 #define GRAPHICINT_H


//Pin Belegung des LED Displays

//Die Reset Leitung
#define LED_RESET_PORT  PORTC
#define LED_RESET_DDR   DDRC
#define LED_RESET_PIN   3
//Die Select Leitung
#define LED_SELECT_PORT PORTC
#define LED_SELECT_DDR  DDRC
#define LED_SELECT_PIN  4
//Die Bright Leitung
#define LED_BRIGHT_PORT PORTC
#define LED_BRIGHT_DDR  DDRC
#define LED_BRIGHT_PIN  5
//Die rote LED Datenleitung
#define LED_RED_PORT    PORTC
#define LED_RED_DDR     DDRC
#define LED_RED_PIN     0
//Die grüne LED Datenleitung
#define LED_GREEN_PORT  PORTC
#define LED_GREEN_DDR   DDRC
#define LED_GREEN_PIN   1
//Die Clock Leitung
#define LED_CLOCK_PORT  PORTC
#define LED_CLOCK_DDR   DDRC
#define LED_CLOCK_PIN   2

//Konstanten, ändern nur mit Vorsicht

/*Definieren der LED Feld Größe.
Achtung: Alle Funktionen wurden nur mit screenx = 16 und screeny = 16 getestet.
Manche Funktionen sind nicht explizit dafür ausgelegt, mit größeren
'Auflösungen' zu funktionieren. Mit 'Auflösungen' die mehr als 254 Pixel
Kantenläge haben, werden die Funktionieren nicht funktionieren.
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


/* Wird use_low_colors auf eins gesetzt, so wird kein Software PWM zum steuern
   der LED Helligkeit verwendet. Die Farbtiefe wird folglich von 4 auf 2 Bit
   reduziert. Die Demo sieht mit 2Bit Farben doch recht langweilig aus.
*/
#define use_low_colors 0

extern uint8_t volatile gdurchlauf;   //welcher Durchlauf
extern uint8_t volatile gzeile;       //Welche Zeile gerade behandelt wird
//Die Daten fürs Display[y;Zeile][x;Spalte]
extern uint8_t volatile gdata[screeny][screenx];


void init_led_display(void); //Initialisiert das LED Display
void resync_led_display(void); //Kurz ein Reset bei eventuellen Fehlern
ISR(TIMER2_OVF_vect); //Schreibt die Display Daten

#endif
