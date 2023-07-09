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

#if modul_prace

const char race_text1[] PROGMEM = "Avoid red, collect green";

u08 race_showintro = 1;

static void race_newtrack(void) {
u16 value;
u08 nun,color;
value = rand() & rand() & rand();
for (nun = 2;nun <= 13;nun++) { //Rote Punkte zeichnen
  if (value & 1) {
    color = 0x03;
  } else {
    color = 0;
  }
  pixel_set(nun,0,color);
  value = value >> 1;
}
//Einen grünen Punkt zeichnen
value = rand();
if ((value & 0x03) == 0x03) { //25% wahrscheinlich einen grünen Punkt
  value >>= 2;
  value %= 12;
  value += 2;
  pixel_set_safe(value,0,0x30);
}
}

void race_start(void) {
u08 lifes, carpos, carpos_old, level, moved;
u08 steps;
u08 nun;
s16 glsteps;
if (race_showintro) {
  load_text(race_text1);
  scrolltext(0,0x32,0,120);
  race_showintro = 0;
}
waitms(500);
for (nun = 0; nun <= 7; nun++) { //Text ausblenden
  move_up();
  waitms(50);
}
draw_line(1,0,0,16,0x01,0); //Linke Begrenzung
draw_line(14,0,0,16,0x01,0); //Rechte Begrenzung
lifes = 3;
carpos_old = 0;
level = 1;
steps = 0;
moved = 0;
glsteps = -8;
//Timer1 wird für das Timing verwendet, 31,25KHZ Takt
TCNT1 = 0; //Reset Timer
TCCR1B = (1<<CS12); //Prescaler: 256
//Zufallsgenerator initialisieren
init_random();
while (lifes > 0) { //Solange noch Leben vorhanden
  //Zeichnen der Level
  draw_line(0,15,0,-level,0x13,0); //Rot-oranger Balken
  draw_line(0,0,0,16-level,0x00,0); //Dunkler Balken
  //Zeichnen der Leben
  draw_line(15,15,0,-lifes,0x13,0); //Rot-oranger Balken
  draw_line(15,0,0,16-lifes,0x00,0); //Dunkler Balken
  //Auto Position berechnen
  carpos = 7+userin.x/21;
  if (carpos < 2) {
    carpos = 2;
  }
  if (carpos > 13) {
    carpos = 13;
  }
  //Falls Auto die Spur gewechselt hat
  if (carpos_old != carpos) { //bei Positionsveränderung
    moved = 1;
    //Auto an alter Position entfernen
    pixel_set_safe(carpos_old,15, 0x00);
  }
  //Bonus und Verlust bewerten
  if (moved) {
    moved = 0;
    if (pixel_get(carpos,15) == 0x03) { //Roter Punkt an neuer Position
      lifes--;
    }
  //Grüner Punkt an neuer Position
    if ((pixel_get(carpos,15) == 0x30) && (lifes <= 15)) {
      lifes++;
    }
    //Auto zeichnen
    pixel_set_safe(carpos,15, pixel_get(carpos,15) | 0x31);
  }
  //Spielfeld einen Schritt herunter bewegen
  if (TCNT1 > (F_CPU/1280)) { //Mehr als 200ms sind vergangen; (8M/1280) = 6250
    moved = 1; //Neue Berührungen überprüfen
    TCNT1 = (level-1)*(F_CPU/26229); //(8M/26229) = 305
    for (nun = 2;nun <= 13;nun++) { //Spielfeld um 1 nach unten
      move_line_down(nun);
    }
    race_newtrack();
    //Spielgeschwindigkeit
    steps++;
    if (steps == 35) {
      steps = 0;
      if (level < 16) {
        level++;
      }
    }
    //Punkte
    if (glsteps < 32000) {
      glsteps++;
    }
  }
  //Alte Position merken
  carpos_old = carpos;
}
waitms(1000);
TCCR1B = 0; //Stopp Timer1
//Bildschrim leeren
for (nun = 0; nun < 16;nun++) {
  move_up();
  waitms(50);
}
//Punkte anzeigen
draw_gamepoints(glsteps/2, RACE_ID);
}

#endif
