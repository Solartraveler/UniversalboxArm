/*
   Gamebox
    Copyright (C) 2004-2006  by Malte Marwedel
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

#ifndef MAIN_H
 #define MAIN_H

#define is_avr

//Der eingestellte Takt
#define F_CPU 8000000
/*Falls der interne Oszillator verwendet wird, sollte mit dem Programmer einmal
das entsprechende Calibrierungsbyte ausgelesen und für osccaleradout
gesetzt werden. Dieser Wert kann sich von Chip zu Chip unterscheiden!
Bei falschen Werten läuft der Chip zu schnell oder zu langsam.
Der osccaleradout Wert wird dann beim Programmstart in das OSCCAL Register
geschrieben. Wird ein externer Takt verwendet, so hat dies keine Bedeutung.
*/
#define osccaleradout 0xad

/*Berechnung der Timer Geschwindigkeit.
Der Timer2 soll alle 4800mal pro Sekunde auslösen. (100Hz*16Zeilen*3Durchläufe)
Der Prescaler des Timers ist 8. Der Timer läuft bis 255 und ruft dann den
Interrupt auf. Daraus folgt:
4800 = (F_CPU/8)/(255-timerset)
umgestellt:
255-timerset = (F_CPU/8)/4800
255 = (FCPU/8)/4800)+timerset
255-((FCPU/8)/4800) = timerset
*/
#define timerset_test (255-(F_CPU/8/4800))

#if (timerset_test < 0)
#define timerset 0
#else
#define timerset (uint8_t)timerset_test
#endif

#if (timerset_test > 174)
#error "Der gewählte Takt ist zu langsam. 4MHZ sind Minnimum, 8MHZ werden empfohlen"
#endif

/* Berechnen des Wertes für waitms. Dieser Wert ist abhängig von F_CPU und
der Zeit die von der Interrupt Routine benötigt wird
Jeder Durchlauf durch die Interruptroutine benötigt rund 538 Takte,
Angenommen der Systemtakt liegt innerhalb von 3-11MHZ, so wird der Interrupt
4800 mal aufgerufen, was 2582400 Take pro Sekune sind.
eigentlich müsste F_CPU_msdelay als((F_CPU-(2582400))/4000) definiert sein.
Allerdings passt die Geschwindigkeit besser, wenn statt durch 4000 durch 5000
geteilt wird, da ich beim Schreiben der Demo die genaue Rechenzeit der Interrupt
Routine nicht genau ermittelt hatte und pauschal von 50% für die Interrupts
Routine ausging. Somit sind die verwendeten Delayzeiten in der Demos selbst
etwas zu groß.
*/
#define F_CPU_msdelay (uint16_t)((F_CPU-(2582400))/5000)

#define eeprom_data  __attribute__ ((section(".eeprom")))

//Externe Funktionen
#include <avr/io.h>
#include <inttypes.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <stdlib.h>
#include <math.h>

//Um zu wissen für was uint8_t u.s.w. steht siehe inttypes.h
typedef uint8_t  u08;
typedef int8_t   s08;
typedef uint16_t u16;
typedef int16_t  s16;
typedef uint32_t u32;
typedef int32_t  s32;

//Hier kann eingestellt werden welches Programmmodul mit kompiliert wird
//"1" bedeutet mit compilieren und  "0" weglassen
#define modul_demo 1
#define modul_calib_save 1
#define modul_sram 1
#define modul_highscore 1
#define modul_ptetris 1
#define modul_prace 1
#define modul_pxxo 1
#define modul_ppong 1
#define modul_prev 1
#define modul_psnake 1
#define modul_xmas 1

#include "timings.h"
#include "../other.h"
#include "graphicint.h"
#include "../graphicfunctions.h"
#include "../text.h"
#include "../highscore.h"
#if modul_demo
  #include "../gameoflife.h"
  #include "../demo.h"
#else
  #define play_demo menu_notcompiled
#endif
#include "userinput.h"
#include "../menu.h"
//Advanced config
#if modul_calib_save
  //nothing to include
#else
  #define calib_save menu_notcompiled
#endif
#if modul_sram
  #include "../sram.h"
#else
  #define ram_showfree menu_notcompiled
#endif
#if modul_highscore
  //nothing to include
#else
  #define highscore_clear menu_notcompiled
#endif
//Die Spiele
#if modul_ptetris
  #include "../ptetris.h"
#else
  #define tetris_start menu_notcompiled
#endif
#if modul_prace
  #include "../prace.h"
#else
  #define race_start menu_notcompiled
#endif
#if modul_pxxo
  #include "../pxxo.h"
#else
  #define xxo_start menu_notcompiled
#endif
#if modul_ppong
  #include "../ppong.h"
#else
  #define pong_start menu_notcompiled
#endif
#if modul_prev
  #include "../prev.h"
#else
  #define reversi_start menu_notcompiled
#endif
#if modul_psnake
  #include "../psnake.h"
#else
  #define snake_start menu_notcompiled
#endif
#if modul_xmas
  #include "../xmas.h"
#else
  #define xmas_start menu_notcompiled
#endif

//Funktionsprototypen:

/* Funktion ist als static deklariert:
static void init_io_pins(void);
*/
void init_random(void);
int main(void);

#endif
