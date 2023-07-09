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

/*Die Demo gibt es auch als Stand-alone Version auf meiner Homepage.
Die Stand-alone Version (Endlosschleife) läuft auch
auf einem Mega8 und benötigt keine Eingabegeräte.
*/

#include "main.h"

#if modul_demo

const char text1[] PROGMEM = "A";
const char text2[] PROGMEM = "Demo";
const char text3[] PROGMEM = "by";
const char text4[] PROGMEM = "Malte";
const char text5[] PROGMEM = "This is the house of santaclaus";
const char text6[] PROGMEM = "Let's play the game of life ";
const char text7[] PROGMEM = "Some patterns";
const char text8[] PROGMEM = "Now use some trigometric functions";
const char text9[] PROGMEM = "Some animations";
const char text10[] PROGMEM = "Restart demo with full speed of the";
const char text11[] PROGMEM = "MCU";


static void demo_intro(void) {
u08 nun;
//Mit leichtem Grün füllen
fill_1();
load_text(text1); //A
draw_string(6, 1, 0x0f,0,0);
load_text(text2); //Demo
scrolltext (9, 0x0f,0x10,120);
waitms(300);
load_buff(intro1); //Lade leichtes Grün in den Puffer
//Schiebe Text nach oben
for (nun = 0; nun < 9; nun++) {
  move_up();
  waitms(40);
}
load_text(text3); //by
draw_string(3, 9, 0x0f,0,1);
waitms(300);
for (nun = 0; nun < 9; nun++) {
  move_up();
  waitms(40);
}
waitms(300);
flip_color();
waitms(200);
load_text(text4); //Malte
scrolltext (9, 0xf0,0x01,150);
waitms(300);
}

static void house(void) {
u08 nun;
//grün füllen
fill_2();
waitms(200);
load_text(text5); //This is the house of santaclaus
scrolltext (4, 0x0f,0x10,100);
for (nun = 0; nun < 16; nun++) {
  move_left();
  waitms(75);
}
waitms(500);
//Das Haus vom Nikolaus
draw_line( 1,15,+0,-7,0x01,1);// |
draw_line( 2,15,+0,-9,0x01,1);// |
draw_line( 3,15,+0,-9,0x01,1);// |
draw_line( 4,15,+0,-8,0x01,1);// |
draw_line( 5,15,+0,-7,0x01,1);// |
draw_line( 0,15,+0,-6,0x0f,0);// Das
waitms(300);
draw_line( 0, 9,+3,-3,0x0f,0);// ist
waitms(300);
draw_line( 3, 6,+3,+3,0x0f,0);// das
waitms(300);
draw_line( 6, 9,+0,+6,0x0f,0);// Haus
waitms(300);
draw_line( 6,15,-6,+0,0x0f,0);// vom
waitms(300);
draw_line( 0,15,+6,-6,0x0f,0);// Ni-
waitms(300);
draw_line( 6, 9,-6,+0,0x0f,0);// ko-
waitms(300);
draw_line( 0, 9,+6,+6,0x0f,0);// laus
waitms(300);
//Ein Baum
draw_line(10,15, 0,-9,0x0f,1);// |
waitms(400);
draw_line(11,15, 0,-9,0x0f,1);// |
waitms(400);
draw_line( 9, 7, 4, 0,0xf0,1);// _
waitms(400);
draw_line( 7, 6, 8, 0,0xf0,0);// _
waitms(400);
draw_line( 6, 5,10, 0,0xf0,0);// _
waitms(400);
draw_line( 6, 4,10, 0,0xf0,0);// _
waitms(400);
draw_line( 7, 3, 8, 0,0xf0,0);// _
waitms(400);
draw_line( 8, 2, 6, 0,0xf0,0);// _
waitms(800);
waitms(800);
flip_color();
waitms(800);
clear_buff();
fill_3();
}

static void life(void) {
u16 nun;
//The game of life
load_text(text6); //Let's play the game of life
scrolltext (4, 0x31,0x00,100);
for (nun = 0; nun < 280;nun++) {
  gameoflife_step();
  waitms(100);
  //Glider in game of life einfügen
  if (nun == 48) {
    gdata[13][6] = 0x31;
    gdata[13][5] = 0x31;
    gdata[14][6] = 0x31;
    gdata[14][4] = 0x31;
    gdata[15][6] = 0x31;
    waitms(1000);
  }
  //Spaceship in game of life einfügen
  if (nun == 90) {
    gdata[0][3] = 0x31;
    gdata[1][4] = 0x31;
    gdata[2][0] = 0x31;
    gdata[2][4] = 0x31;
    gdata[3][1] = 0x31;
    gdata[3][2] = 0x31;
    gdata[3][3] = 0x31;
    gdata[3][4] = 0x31;
    waitms(1000);
  }
}
}

static void patterns(void) {
u08 x,y,nun;
//Einige einfache Muster zeigen
load_text(text7);//Some patterns
scrolltext (4, 0x13,0x00,100);
waitms(500);
draw_box(0,0,16,16,0x01,0);
waitms(500);
draw_box(1,1,14,14,0x02,0);
waitms(500);
draw_box(2,2,12,12,0x11,0);
waitms(500);
draw_box(3,3,10,10,0x12,0);
waitms(500);
draw_box(4,4, 8, 8,0x03,0);
waitms(500);
draw_box(5,5, 6, 6,0x13,0);
waitms(500);
draw_box(6,6, 4, 4,0x23,0);
waitms(500);
draw_box(7,7, 2, 2,0x33,0);
waitms(500);
flip_color();
waitms(1000);
draw_box(5,5, 6, 6,0x03,0x30);
waitms(300);
draw_box(2 , 2, 4, 4,0x33,0x33);
draw_box(10, 2, 4, 4,0x33,0x33);
draw_box( 2,10, 4, 4,0x33,0x33);
draw_box(10,10, 4, 4,0x33,0x33);
waitms(300);
draw_box(0, 0, 3,3,0x11,0x11);
draw_box(13,0, 3,3,0x11,0x11);
draw_box(0, 13,3,3,0x11,0x11);
draw_box(13,13,3,3,0x11,0x11);
waitms(300);
draw_box(3, 0,10,2,0x10,0x10);
draw_box(3,14,10,2,0x10,0x10);
draw_box(0, 3,2,10,0x10,0x10);
draw_box(14,3,2,10,0x10,0x10);
waitms(300);
draw_box(2 ,6,3,4,0,0);
draw_box(11,6,3,4,0,0);
draw_box(6, 2,4,3,0,0);
draw_box(6,11,4,3,0,0);
waitms(3000);
for (x = 0; x < 16;x++) {
  for (y = 0; y < 16; y++) {
    gdata[x][y] = (x % 4) + (((15-y) % 4)<<4);
  }
}
waitms(3000);
for (x = 0; x < 16;x++) {
  for (y = 0; y < 16; y++) {
    gdata[x][y] = (x / 4) + (((15-y) / 4)<<4);
  }
}
waitms(3000);
for (x = 0; x < 16;x++) {
  for (y = 0; y < 16; y++) {
    gdata[x][y] = (x / 4) + (((15-x) / 4)<<4);
  }
}
waitms(3000);
draw_box(0 , 0,6,6,0x20,0x30);
draw_box(10, 0,6,6,0x20,0x30);
draw_box(0 ,10,6,6,0x20,0x30);
draw_box(10,10,6,6,0x20,0x30);
waitms(500);
draw_line( 0, 6,16,0,0x03,0);
draw_line( 0, 9,16,0,0x03,0);
draw_line( 6, 0,0,16,0x03,0);
draw_line( 9, 0,0,16,0x03,0);
waitms(500);
draw_box(0, 7, 6,2,0x01,0x01);
draw_box(10,7, 6,2,0x01,0x01);
draw_box(7, 0, 2,6,0x01,0x01);
draw_box(7, 10,2,6,0x01,0x01);
draw_box(7,7,2,2,0x33,0x33);
waitms(3000);
for (nun = 0; nun < 8;nun++) {
  draw_line(0,     nun<<1,16,0,0,0);
  draw_line(nun<<1,0,    0, 16,0,0);
  waitms(100);
}
for (nun = 0; nun < 8;nun++) {
  draw_line(0,     (nun<<1)+1,16,0,0x30,1);
  draw_line((nun<<1)+1,0,    0, 16,0x02,1);
  waitms(100);
}
waitms(3000);
}

static void dotrigometric(void) {
u08 ta,tb;
float nun;
u08 nun08;
u08 color, choose;
//Nun ein paar trigometrische Funktionen
load_text(text8);//Now use some trigometric functions
scrolltext ( 4, 0x33,0x10,100);
//Koordinatensystem zeichnen
fill_3();//Bildschirm leeren
draw_line(0,7,16,0,0x32,0);
draw_line(7,0,0,16,0x32,0);
draw_line(14,6,0,3,0x32,0);
draw_line(6, 1,3,0,0x32,0);
nun = 0;
//Sinus und cosinus Zeichen
while (nun < 17) {
  nun08 = (u08)nun;
  if (nun08 < 16) {
    ta = ((u08)(sin((2*M_PI)/16*nun)*8))+8;
    if (ta < 16) {
      gdata[ta][nun08] = 0x03;
    }
    tb = ((u08)(cos((2*M_PI)/16*nun)*8))+8;
    if (tb < 16) {
      gdata[tb][nun08] = 0x30;
    }
  }
  nun += 0.1;
}
waitms(1000);
//Bunte Kreise zeichnen
nun08 = 0;
while (nun08 < 12) {
  nun = 0;
  choose = nun08 % 4;
  color = 0;
  if (choose == 0) {
    color = 0x03;
  }
  if (choose == 1) {
    color = 0x30;
  }
  if (choose == 2) {
    color = 0x32;
  }
  while (nun < (2*M_PI)) {
    ta = ((u08)(sin(nun)*nun08)+7.5);
    tb = ((u08)(cos(nun-M_PI)*nun08)+7.5);
    if ((ta < 16) && (tb < 16)) {
      gdata[ta][tb] = color;
    }
    nun += 0.09;
  }
  nun08++;
}
waitms(5000);
}

static void doanimations(void) {
u08 x,y,red,green;
u16 nun;
s08 x2,y2;
//Und jetzt ein paar Animationen
load_text(text9);//Some animations
draw_line(0,3,16,0,0x23,1);
draw_line(0,11,16,0,0x23,1);
scrolltext( 4, 0x30,0x23,100);
waitms(200);
//Farbwellen
for (x = 0; x < 16; x++) {
  red = (u08)(sin((2*M_PI)/8*x)*1.745 + 1.745);
  green = (u08)(cos((2*M_PI)/8*x)*1.745 + 1.745);
  move_right();
  for (y = 0; y < 16; y++) {
    gdata[y][0] = red+(green<<4);
  }
  waitms(150);
}
waitms(1000);
//Bunten Uhrzeiger
for (nun = 0; nun < 750; nun++) {
  red =   (u08)(sin((2*M_PI)/11*nun)*1.745 + 1.745);
  green = (u08)(cos((2*M_PI)/11*nun)*1.745 + 1.745);
  x2 =    (s08)(sin((2*M_PI)/80*nun)*16);
  y2 =    (s08)(cos((2*M_PI)/80*nun)*16);
  draw_line(8,8,x2,y2,red+(green<<4),0);
  waitms(15);
}
//Bildschirm löschen
for (y = 0; y < 16; y++) {
  draw_line(0,15,16,-y,0x00,0);
  waitms(75);
}
for (x = 0; x <= 16; x++) {
  draw_line(0,15,16-x,-16,0x00,0);
  waitms(75);
}

}

void play_demo(void) {
for (no_delays = 0; no_delays <= 1; no_delays++) {
  demo_intro();
  house();
  life();
  patterns();
  dotrigometric();
  doanimations();
  if (no_delays == 0) {
    load_text(text10);//Restart demo with full speed of the
    scrolltext (0, 0x03,0x00,100);
    load_text(text11);//MCU
    draw_string(0, 8, 0x13,0,1);
    waitms(1000);
  }
}
no_delays = 0; //Normal speed
}

#endif
