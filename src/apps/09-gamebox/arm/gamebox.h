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

#ifndef MAIN_H
 #define MAIN_H


//This is the original speed of the AVR, it is still used in the game and timer
#define F_CPU 8000000

//Externe Funktionen
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

typedef uint8_t  u08;
typedef int8_t   s08;
typedef uint16_t u16;
typedef int16_t  s16;
typedef uint32_t u32;
typedef int32_t  s32;


//AVR specific defines

#define PGM_VOID_P const void*
#define PROGMEM
#define memcpy_P memcpy

#define eeprom_data __attribute__ ((section(".eepromSimulation")))

//Set to 1 to add the module into the binary, set to 0 to skip it

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


#include "timing.h"
#include "../other.h"
#include "graphicout.h"
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

//Variablen & Konstanten
extern pthread_t avr_thread_id;

//Funktionsprototypen:
void sei(void);
void cli(void);
u08 pgm_read_byte(const u08 *data);
u16 pgm_read_word(const u16 *data);
u08 eeprom_read_byte(const u08 *addr);
u16 eeprom_read_word(const u16 *addr);
void eeprom_write_byte(u08 *addr, u08 value);
void eeprom_write_word(u16 *addr, u16 value);
void init_random(void);
void AppInit(void);
/* In pc-simulator/main.c als static deklariert:
static void *avr_thread(void * arg);
*/

#endif
