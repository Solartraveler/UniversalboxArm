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

#include "main.h"


#if modul_highscore

u16 sessionscores[scoregames];
u16 globalscores[scoregames] eeprom_data;

const char highscore_clear_save1[] PROGMEM = "Clear all highscores?";
const char highscore_clear_no[] PROGMEM = "No";
const char highscore_clear_yes[] PROGMEM = "Yes";
const char highscore_clear_save2[] PROGMEM = "Cleared";

void highscore_clear(void) {
u08 accepted = 0;
u08 nun;
clear_screen();
load_text(highscore_clear_save1);
scrolltext(0,0x03,0,120);
load_text(highscore_clear_no);
draw_string(1,8,0x31,0,1);
while (userin_press() == 0) { //Warte auf Tastendruck
  if ((userin_right()) && (accepted == 0)) { //Yes
    accepted = 1;
    load_text(highscore_clear_yes);
    draw_box(0,8,16,8,0x00,0x00);
  }
  if (userin_left() || userin_up() || userin_down()) { //Nein
    accepted = 0;
    load_text(highscore_clear_no);
    draw_box(0,8,16,8,0x00,0x00);
  }
  draw_string(1,8,0x31,0,1);
}
if (accepted == 1) { // Highscores löschen
  for (nun = 0; nun <scoregames; nun++) {
    eeprom_write_word(&globalscores[nun],0); //EEProm löschen
    sessionscores[nun] = 0; //RAM löschen
  }
  load_text(highscore_clear_save2);
  scrolltext(3,0x13,0,120);
  waitms(500);
  userin_flush();
}
}

const char highscore_points_best[] PROGMEM = "Best points global/session/game";
const char highscore_points_newglobal[] PROGMEM = "New best global points!";
const char highscore_points_newsession[] PROGMEM = "New best session points!";

u08 highscore_showtext = 1;

void draw_gamepoints(u16 points, u08 game_id) {
u16 globalbestscore;
u08 nun;
u08 leftpress = 0;
load_text(game_pts);
scrolltext(9,0x31,0x00,100);
waitms(200);
for (nun = 0; nun < 9; nun++) {
  move_up();
  waitms(70);
}
waitms(150);
if (points > 9999) { //mehr als 4 Stellen lassen sich nicht anzeigen
  points = 9999;
}
draw_tinynumber(points, 0, 9, 0x30);
//nun der erweiterte Teil
//warte bis Taster irgendwie bewegt wird
userin_flush();
while ((leftpress == 0) && (userin_right() == 0) && (userin_up() == 0) &&
       (userin_down() == 0) && (userin_press() == 0)) {
  leftpress = userin_left();
}
//lade beste Punkte
globalbestscore = eeprom_read_word(&globalscores[game_id]);
if (globalbestscore > 9999) { //Fehler im Speicher, setze auf Null
  globalbestscore = 0;
}
//das Aktualisieren des EEProms
if (points > globalbestscore) {
  eeprom_write_word(&globalscores[game_id],points);
}
if (leftpress == 0) { //wenn erweiterte Highscore angezeigt werden soll
  if (highscore_showtext) {
    highscore_showtext = 0;
    draw_line(0,8,16,0,0x01,0);
    load_text(highscore_points_best);
    scrolltext(9,0x30,0x01,100);
    waitms(250);
  }
  clear_screen();
  waitms(170);
  draw_tinynumber(globalbestscore, 0, 0, 0x30);
  waitms(170);
  draw_tinynumber(sessionscores[game_id], 0, 5, 0x33);
  waitms(170);
  draw_tinynumber(points, 0, 10, 0x03);
  if (points > sessionscores[game_id]) { //neue Highscore
    userin_flush();
    while (userin_press() == 0); //Warten auf Tastendruck
    if (points > globalbestscore) {
      load_text(highscore_points_newglobal);
    } else {
      load_text(highscore_points_newsession);
    }
    draw_box(0,0,16,9,0x02,0x02);
    draw_line(0,9,16,0,0x00,0);
    scrolltext(1,0x30,0x02,110);
  }
  userin_flush();
  while (userin_press() == 0); //Warten auf Tastendruck
}
//das aktualisieren der Session Highscore
if (points > sessionscores[game_id]) {
  sessionscores[game_id] = points;
}
}

#else
//simples Anzeigen der Punkte

void draw_gamepoints(u16 points, u08 game_id) {
u08 nun;
load_text(game_pts);
scrolltext(9,0x31,0x00,100);
waitms(200);
for (nun = 0; nun < 9; nun++) {
  move_up();
  waitms(70);
}
waitms(150);
if (points > 9999) { //mehr als 4 Stellen lassen sich nicht anzeigen
  points = 9999;
}
draw_tinynumber(points, 0, 9, 0x30);
userin_flush();
while (userin_press() == 0); //Warten auf Tastendruck
}

#endif
