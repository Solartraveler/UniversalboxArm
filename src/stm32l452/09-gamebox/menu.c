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

/* Menstruktur

#0 Games -> #5
#1 Advanced_Config -> #11
#2 Eingabe Kalibrieren -> input_calib()
#3 Demo Abspielen -> play_demo()
#4 Xmas -> xmas_start()

##5 Tetris -> tetris_start()
##6 Racer -> racer_start()
##7 XXO -> xxo_start()
##8 Pong -> pong_start()
##9 Reversi -> reversi_start()
##10 Snake -> snake_start()

##11 Eingabe -> input_select()
##12 Kalibrierung Speichern -> calib_save()
##13 Freien SRAM anzeigen -> ram_showfree()
##14 Highscore zurcksetzen -> highscore_clear()

Menü Animation:
Schwaches grn im Hintergrund
Text während des Einblendens schwach rot
Sobald Text an Position starkes rot

*/


#include "main.h"


#define menuentries_nr 15

struct menupoint{
u08 prev;
u08 next;
u08 up;
u08 down;
char text[5];
void (*execute) (void);
};

/* Neue Menü Nummer 255 bedeutet, es wird die angegebene Routine ausgeführt.
   Ansonsten werden alle Nummern größer gleich menuentries_nr ignoriert, so
   dass die Zahl 254 dafür verwendet wird, wenn eine Bedienrichtung keine
   Funktion haben soll.
*/


/*Die Warnung darüber, dass menu_notcompiled nicht benutzt wird, kann problemlos
  ignoriert werden
*/
const char menu_gamemissing[] PROGMEM = "Not compiled in";

static void menu_notcompiled(void) {
load_text(menu_gamemissing);
scrolltext(4,0x02,0x00,110);
waitms(500);
}

const struct menupoint mainmenu[menuentries_nr] PROGMEM = {
  {254,5,4,1,"Game\0"},			//0
  {254,11,0,2,"AdvC\0"},		//1
  {254,255,1,3,"Cali\0",input_calib},	//2
  {254,255,2,4,"Demo\0",play_demo},	//3
  {254,255,3,0,"Xmas\0",xmas_start},		//4

  {0,255,10,6,"Tetr\0",tetris_start},	//5
  {0,255,5,7,"Race\0",race_start},	//6
  {0,255,6,8,"XXO\0",xxo_start},	//7
  {0,255,7,9,"Pong\0",pong_start},	//8
  {0,255,8,10,"Reve\0",reversi_start},	//9
  {0,255,9,5,"Snak\0",snake_start},	//10

  {1,255,14,12,"Inp\0",input_select},	//11
  {1,255,11,13,"CalS\0",calib_save},	//12
  {1,255,12,14,"RAM\0",ram_showfree},	//13
  {1,255,13,11,"Rese\0",highscore_clear}//14
};

static void menu_preparemove(u08 newnumber) {
load_buff(intro1); //Leichtes grn laden
load_text(mainmenu[newnumber].text);
}

static void menu_move_next(u08 newnumber) {
u08 nun;
menu_preparemove(newnumber);
for (nun = 0; nun < (screenx*3/2); nun++) {
  move_left();
  draw_string(screenx*3/2-nun-1,3,0x01,0,1);
  waitms(75);
}
draw_string(0,3,0x03,0,1);
userin_flush();
}

static void menu_move_prev(u08 newnumber) {
u08 nun;
menu_preparemove(newnumber);
for (nun = 0; nun < (screenx*3/2); nun++) {
  move_right();
  draw_string((-(screenx*3/2))+nun+1,3,0x01,0,1);
  waitms(75);
}
draw_string(0,3,0x03,0,1);
userin_flush();
}

static void menu_move_up(u08 newnumber) {
u08 nun;
menu_preparemove(newnumber);
for (nun = 0; nun < (screeny*3/2); nun++) {
  move_down();
  draw_string(0,(-(screeny*3/2))+nun+4,0x01,0,1);
  waitms(75);
}
draw_string(0,3,0x03,0,1);
userin_flush();
}

static void menu_move_down(u08 newnumber) {
u08 nun;
menu_preparemove(newnumber);
for (nun = 0; nun < (screeny*3/2); nun++) {
  move_up();
  draw_string(0,(screeny*3/2)-nun+2,0x01,0,1);
  waitms(75);
}
draw_string(0,3,0x03,0,1);
userin_flush();
}

void menu_start(void) {
u08 menupos = 0;
u08 wishmenu;
void (*subprog) (void);
menu_move_next(menupos);
for (;;) {
  if (userin_left()) { //Zurck im Menu
    wishmenu = pgm_read_byte(&mainmenu[menupos].prev);
    if (wishmenu <  menuentries_nr) {
      menupos = pgm_read_byte(&mainmenu[menupos].prev);
      menu_move_prev(menupos);
    }
  }
  if (userin_right()) { //Vorwï¿½ts im Men
    wishmenu = pgm_read_byte(&mainmenu[menupos].next);
    if (wishmenu == 255) { //Funktion aufrufen
      clear_buff();
      fill_3(); //auflï¿½en des Mens
      userin_flush();
      resync_led_display(); //falls Fehler auftraten
      memcpy_P(&subprog,&mainmenu[menupos].execute,sizeof(subprog));
      subprog(); //Sprung zum passendem Unterprogramm
      menu_move_prev(menupos);
      userin_flush();
      resync_led_display(); //falls Fehler auftraten
    } else {
      if (wishmenu <  menuentries_nr) {
        menupos = pgm_read_byte(&mainmenu[menupos].next);
        menu_move_next(menupos);
      }
    }
  }
  if (userin_up()) { //nach oben im Men
    wishmenu = pgm_read_byte(&mainmenu[menupos].up);
    if (wishmenu <  menuentries_nr) {
      menupos = pgm_read_byte(&mainmenu[menupos].up);
      menu_move_up(menupos);
    }
  }
  if (userin_down()) { //nach unten im Men
    wishmenu = pgm_read_byte(&mainmenu[menupos].down);
    if (wishmenu <  menuentries_nr) {
      menupos = pgm_read_byte(&mainmenu[menupos].down);
      menu_move_down(menupos);
    }
  }

}
}
