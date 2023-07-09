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

/*Timer2 ist die Graphic Routine, wird ungefähr alle 1000 Take aufgerufen
 Bedeutet, die Routine wird ca. 4000 mal pro Sekunde aufgerufen
 Mit ungefähr 977Takten benötigt die Routine c.a. 50% der Rechenzeit

Datum der letzten Änderung: 2005-08-20
*/
#include "main.h"
#include "graphicint.h"
#include <inttypes.h>
#include <avr/interrupt.h>

#if (use_low_colors == 0)
//Welcher Durchlauf, wird bei use_low_colors nicht benötigt.
uint8_t volatile gdurchlauf;
#endif
uint8_t volatile gzeile;  //Welche Zeile gerade behandelt wird

/*256Byte für die Graphic Daten (12,5% des gesamten SRAMs bei screenx und
  screeny = 16 und 2KB RAM)
  Die Daten fürs Display[y;Zeile][x;Spalte]
*/
uint8_t volatile gdata[screeny][screenx];

void init_led_display(void) {
//Setzen der Data Direction Regiser auf Ausgang
LED_RESET_DDR |= (1<<LED_RESET_PIN);
LED_SELECT_DDR |= (1<<LED_SELECT_PIN);
LED_BRIGHT_DDR |= (1<<LED_BRIGHT_PIN);
LED_RED_DDR |= (1<<LED_RED_PIN);
LED_GREEN_DDR |= (1<<LED_GREEN_PIN);
LED_CLOCK_DDR |= (1<<LED_CLOCK_PIN);
TCNT2 = 0;                //Timer 2 Resetten
TIMSK |= 0x40;            //Timer 2 Overflow Interrupt aktiv
TCCR2 = 2;                //Prescaler = 8
LED_RESET_PORT |= (1<<LED_RESET_PIN); //Reset high
gzeile = 0;               //beginne mit Zeile 0
LED_RESET_PORT &= ~(1<<LED_RESET_PIN); //Reset low
LED_SELECT_PORT |= (1<<LED_SELECT_PIN); //Select auf high
}

void resync_led_display(void) {
//Setzt einmal Reset
cli();
LED_RESET_PORT |= (1<<LED_RESET_PIN); //Reset high
gzeile = 0;
LED_RESET_PORT &= ~(1<<LED_RESET_PIN); //Reset lo
sei();
}

#if (use_low_colors == 0)
//Verwende Variante mit 4Bit Farben (16 mögliche Farbtöne)

//4+24+19+(12+7+9)*16+7+5+4+27=538Takte pro Interrupt
ISR(TIMER2_OVF_vect) {
uint8_t gspalte;
uint8_t gtemp,gdbyte;
uint8_t gdurchlauf_t;
uint8_t gzeile_t;

TCNT2 = (uint8_t)timerset;
LED_BRIGHT_PORT |= (1<<LED_BRIGHT_PIN); //Bright auf high
//gdurchlauf_t und gzeile_t sind nicht volatile -> Speicher und Platz Ersparnis
gdurchlauf_t = gdurchlauf;
gzeile_t = gzeile;
gspalte = 0;
while (gspalte != screenx) {
  //Für die Gamebox wurde das Bild gekippt, also gzeile_t und gspalte vertauscht
  //Original: gdbyte = gdata[gzeile_t][gspalte];
  gdbyte = gdata[gspalte][screenx-1-gzeile_t]; //gdata ist volatile, gtemp nicht
  LED_RED_PORT &= ~(1<<LED_RED_PIN); //Rote LED Leitung sicher aus
  LED_GREEN_PORT &= ~(1<<LED_GREEN_PIN); //Grüne LED Leitung sicher aus
  /* Aus gtemp werden die für die rote LED wichtigen Bits extrahiert und die
     LED in Abhängikeit von gdurchlauf_t entweder ein oder Ausgeschaltet.
     So können durch schnelles Ein- und Ausschalten drei verschiedene
     Helligkeitsstufen (+ganz aus) angezeigt werden. */
  gtemp = gdbyte & 0x03; //Dies in der If Verzweigung ->16Bit ->langsamer+größer
  if (gdurchlauf_t < gtemp) {
    LED_RED_PORT |= (1<<LED_RED_PIN);//Rote LED an
  }
  gdbyte = (gdbyte>>4) & 0x03;
  if (gdurchlauf_t < gdbyte)  {
    LED_GREEN_PORT |= (1<<LED_GREEN_PIN);//Grüne LED an
  }
  //Clock Leitung auf high, Datenübernahme durch Takt
  LED_CLOCK_PORT |= (1<<LED_CLOCK_PIN);
  LED_CLOCK_PORT &= ~(1<<LED_CLOCK_PIN); //Clock Leitung auf low
  gspalte++;
}
LED_BRIGHT_PORT &= ~(1<<LED_BRIGHT_PIN); //Bright auf low
gzeile_t++;
if (gzeile_t == screeny) {
  gzeile_t = 0;
  gdurchlauf_t++;
}
if (gdurchlauf_t == 4) {
gdurchlauf_t = 0;
}
//Zurück in die volatile Variablen schreiben
gdurchlauf = gdurchlauf_t;
gzeile = gzeile_t;
#if modul_sram
//Stack-Heap Überwachung
if (minstack > SP) {
  minstack = SP;
}
if (maxheap < (u16)__brkval) {
  maxheap = (u16)__brkval;
}
#endif
}

#else
/*Verwende Variante mit 2Bit Farben (4 mögliche Farbtöne)
  2Bit Farben ist das, was das LED Panel eigentlich nur darstellen kann und
  deshalb nicht durch Software PWM simuliert werden muss. Allerdings sieht die
  Demo dann nicht annähernd so gut aus. Da Software PWM weg fällt, könnte der
  MCU erheblich im Bezug auf Rechenleistung entlastet werden und so problemlos
  mehrere LED Panels ansteuern. Diese Möglichkeit der reduzierten Rechenleistung
  wurde hier jedoch nicht realisiert. Je nachdem auf welchen Wert gdurchlauf_t
  gesetzt wurde, werden die dunklen Farben mit maximaler Helligkeit angezeigt
  oder ganz ausgelassen!. gdurchlauf_t = 0 zeigt auch die dunkelste Farbe an,
  gdurchlauf_t = 3 nur die allerhellsten.
*/

SIGNAL(SIG_OVERFLOW2) {
uint8_t gspalte;
uint8_t gtemp,gdbyte;
const uint8_t gdurchlauf_t = 1;
uint8_t gzeile_t;

TCNT2 = (uint8_t)timerset;
LED_BRIGHT_PORT |= (1<<LED_BRIGHT_PIN); //Bright auf high
//gdurchlauf_t und gzeile_t sind nicht volatile -> Speicher und Platz Ersparnis
gzeile_t = gzeile;
gspalte = 0;
while (gspalte != screenx) {
  //Original: gdbyte = gdata[gzeile_t][gspalte];
  gdbyte = gdata[gspalte][screenx-1-gzeile_t]; //gdata ist volatile, gtemp nicht
  LED_RED_PORT &= ~(1<<LED_RED_PIN); //Rote LED Leitung sicher aus
  LED_GREEN_PORT &= ~(1<<LED_GREEN_PIN); //Grüne LED Leitung sicher auss
  /* Aus gtemp werden die für die rote LED wichtigen Bits extrahiert und die
     LED in Abhängikeit von gdurchlauf_t entweder ein oder Ausgeschaltet.
   */
  gtemp = gdbyte & 0x03; //Dies in der If Verzweigung ->16Bit ->langsamer+größer
  if (gdurchlauf_t < gtemp) {
    LED_RED_PORT |= (1<<LED_RED_PIN);//Rote LED an
  }
  gdbyte = (gdbyte>>4) & 0x03;
  if (gdurchlauf_t < gdbyte)  {
    LED_GREEN_PORT |= (1<<LED_GREEN_PIN);//Grüne LED an
  }
  //Clock Leitung auf high, Datenübernahme durch Takt
  LED_CLOCK_PORT |= (1<<LED_CLOCK_PIN);
  LED_CLOCK_PORT &= ~(1<<LED_CLOCK_PIN); //Clock Leitung auf low
  gspalte++;
}
LED_BRIGHT_PORT &= ~(1<<LED_BRIGHT_PIN); //Bright auf low
gzeile_t++;
if (gzeile_t == screeny) {
  gzeile_t = 0;
}
//Zurück in die volatile Variablen schreiben
gzeile = gzeile_t;
#if modul_sram
//Stack-Heap Überwachung
if (minstack > SP) {
  minstack = SP;
}
if (maxheap < (u16)__brkval) {
  maxheap = (u16)__brkval;
}
#endif
}

#endif
