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

#if modul_psnake

/* Der zu löschende Punkt wird in einem FIFO (256 Byte)gespeichert,
 Die Kollisionserkennung erfolgt mittles des Bildspeichers
*/

struct snake_fifostruct{
u08 *address;
u16 writepoint;
u16 readpoint;
u16 free;
u16 size;
};

const char snake_memerror[] PROGMEM = "Need more RAM!";
const char snake_perfect[] PROGMEM = "WOW";

static void snake_fifo_init(struct snake_fifostruct *fifo) {
fifo->size = 256;
fifo->address = malloc(sizeof(u08) * fifo->size);
fifo->writepoint = 0;
fifo->readpoint = 0;
if (fifo->address != NULL) {
  fifo->free = fifo->size;
} else {
  fifo->free = 0;
}
}

static s08 snake_fifo_put(struct snake_fifostruct *fifo, u08 value) {
if (fifo->address == NULL) {
  return -1; //Kein FIFO vorhanden !!!!!!!!!!!!!!!!!!!!!
}
if (fifo->free == 0) {
  return 0; //Nada, nix, niete, kein Speicher frei
}
fifo->free--;
*(fifo->address+fifo->writepoint) = value;
fifo->writepoint++;
if (fifo->writepoint >= fifo->size) {
  fifo->writepoint = 0;
}
return 1; //OK, wurde ausgeführt
}

static u08 snake_fifo_get(struct snake_fifostruct *fifo) {
u08 value = 0;
value = *(fifo->address+fifo->readpoint);
if (fifo->free < fifo->size) { //Ja, es waren auch Daten da
  fifo->free++;
  fifo->readpoint++;
  if (fifo->readpoint >= fifo->size) {
    fifo->readpoint = 0;
  }
}
return value;
}

static void snake_placestart(u08 position) {
pixel_set_safe(position>>4,position&0x0f,0x31);
}

static void snake_place(u08 position) {
pixel_set_safe(position>>4,position&0x0f,0x30);
}

static void snake_placeend(u08 position) {
pixel_set_safe(position>>4,position&0x0f,0x10);
}

static void snake_placeclear(u08 position) {
pixel_set_safe(position>>4,position&0x0f,0x00);
}

static u08 snake_placeapple(void) {
u08 value;
u16 security;
for (security = 0;security < 100;security++) {
  value = rand() & 0xff;
  if (pixel_get(value>>4, value&0x0f) == 0) { //Feld ist frei
    pixel_set_safe(value>>4,value&0x0f,0x03);
    return 1;
  }
}
//Schlange sehr lang, bei 100 Versuchen keine Stelle -> systematisch versuchen
for (security = 0;security < 256;security++) {
  if (pixel_get(security>>4, security&0x0f) == 0) { //Feld ist frei
    pixel_set_safe(security>>4,security&0x0f,0x03);
    return 1;
  }
}
return 0;
}

static s16 snake_move(u08 position, u08 direction) {
u08 invalid = 0;
if (direction == 0) { //Hoch
  if ((position & 0x0f) == 0x00) { //Wand getroffen
    invalid = 1;
  }
  position -= 0x01;
}
if (direction == 1) { //Runter
  if ((position & 0x0f) == 0x0f) { //Wand getroffen
    invalid = 1;
  }
  position += 0x01;
}
if (direction == 2) { //Links
  if ((position & 0xf0) == 0x00) { //Wand getroffen
    invalid = 1;
  }
  position -= 0x10;
}
if (direction == 3) { //Rechts
  if ((position & 0xf0) == 0xf0) { //Wand getroffen
    invalid = 1;
  }
  position += 0x10;
}
if ((pixel_get(position>>4,position&0x0f) & 0xf0) != 0) { //Schlange vorhanden!
  invalid = 1;
}
if (direction > 3) { //Fehlerhafter Parameter
  invalid = 1;
}
if (invalid) {
  return -1;
}
return position;
}

void snake_start(void) {
u16 points = 0;
u32 time = 0;
u08 direction = 0; //0 = hoch, 1 = runter, 2 = links, 3 = rechts
u08 life = 1;
u08 snakeend;
u08 snakestart;
u08 waittime;
u08 waitcount = 0;
s16 tmppos;
u08 xabs;
u08 yabs;
struct snake_fifostruct fifo;
//Erzeuge FIFO
snake_fifo_init(&fifo);
if (fifo.address != NULL) {
  //Zufallsgenerator initialisieren
  init_random();
  //Schlange erzeugen
  //Schlangenende
  snakeend = 0xef;
  snake_placeend(snakeend);
  //Schlangenmitte
  snake_fifo_put(&fifo, 0xee);
  snake_place(0xee);
  //Schlangenkopf
  snakestart = 0xed;
  snake_placestart(snakestart);
  snake_fifo_put(&fifo, snakestart);
  //Apfel setzen
  snake_placeapple();
  //Timer1 wird für das Timing verwendet, 31,25KHZ Takt
  TCNT1 = 0; //Reset Timer
  TCCR1B = (1<<CS12); //Prescaler: 256
  userin_flush();
  while(life == 1) {
    if (TCNT1 > (F_CPU/25641)) { //Wenn 10ms vergangen, bei 8MHZ TCNT1 > 312
      TCNT1 = 0;
      time++;
      waitcount++;
      //Benutzereingabe bearbeiten - Geschwindigkeit
      xabs = abs(userin.x);
      yabs = abs(userin.y);
      if (yabs > xabs) { //yabs wird beachtet
        waittime = 100-yabs*2/3;
      } else {
        waittime = 100-xabs*2/3;
      }
      if (waitcount > waittime) { //Nächste Schritt
        waitcount = 0;
        //Benutzereingabe bearbeiten - Richtung
        if (yabs > xabs) { //yabs wird beachtet
          if ((userin.y > 50) && (direction != 0)) { //Unten
            direction = 1;
          }
          if ((userin.y < -50) && (direction != 1)) { //Oben
            direction = 0;
          }
        } else {  //xabs wird beachtet
          if ((userin.x > 50) && (direction != 2)) { //Nach rechts
            direction = 3;
          }
          if ((userin.x < -50) && (direction != 3)) { //Nach links
            direction = 2;
          }
        }
        //Zeichne alte Position in normalem Grün
        snake_place(snakestart);
        //Neue Position berechnen, auf Schlange oder Wand überprüfen
        tmppos = snake_move(snakestart,direction);
        if (tmppos < 0) { //Game over
          life = 0;
          break;
        }
        snakestart = tmppos;
        snake_fifo_put(&fifo, snakestart);
        //Sehe nach ob an neuer Position ein Apfel
        if (pixel_get(snakestart>>4,snakestart&0x0f) == 0x03) {
          points++;
          snake_placestart(snakestart);
          if (snake_placeapple() == 0) { //Konnte keinen Apfel platzieren
            life = 2; //Wow, perfekt durchgespielt!
          }
        } else { //Aktualisiere Schlangenende
          snake_placestart(snakestart);
          snake_placeclear(snakeend);
          snakeend = snake_fifo_get(&fifo);
          snake_placeend(snakeend);
        } //Ende aktualisiere Schlangeende
      } //Ende nächster Schritt
    } //Ende 10ms Durchlauf
  } //Ende while Schleife
  free(fifo.address);
  TCCR1B = 0; //Timer stoppen
  waitms(600);
  points *= 10;
  time /= 100;
  if (time < points) {
    points -= time;
  } else {
    points = 0;
  }
  if (points > 9999) {
    points = 9999;
  }
  clear_screen();
  if (life == 2) { //Perfect game (function never tested)
    load_text(snake_perfect);
    draw_string(0, 1, 0x03,0,1);
    waitms(300);
    flip_color();
    waitms(300);
    flip_color();
    waitms(300);
  }
  draw_gamepoints(points, SNAKE_ID);
} else { //Out of RAM!
  clear_screen();
  load_text(snake_memerror);
  scrolltext(4,0x03,0x00,100);
  userin_flush();
  while (userin_press() == 0); //Warten auf Tastendruck
}
}

#endif
