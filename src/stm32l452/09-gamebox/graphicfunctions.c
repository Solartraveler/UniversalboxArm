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

/*Graphic functions
Letzte Änderungen: Version 1.0: GPL Header eingefügt
                   Version 0.50: showbin() hierher verschoben

*/


#include "main.h"

#ifdef is_i386
  #include "graphicout.h"
  #include <time.h>
#endif
#ifdef is_avr
  #include "graphicint.h"
#endif

#include "graphicfunctions.h"
#include <inttypes.h>

//Universeller Speicher für eine Spalte/Zeile + 0 Zeichen
uint8_t linebuff[maxscreen+1];
uint8_t no_delays;         //Wenn 1, dann werden sämtliche Delays übersprungen

void clear_buff(void) {
uint8_t pos;
for (pos = 0; pos < maxscreen+1; pos++) {
  linebuff[pos] = 0;
}
}

void load_buff(PGM_VOID_P x) {
memcpy_P(linebuff,x,maxscreen);
}


/* Derzeit wird auf gdata noch direkt zugegriffen. Später soll
  dies mit den inline Funktionen pixel_set und pixel_get erfolgen,
  dies ist derzeit jedoch noch nicht der Fall, da der Compiler bei der
  Verwenung diese Funktionen größeren Code generiert. Möglicherweise
  liegt hier ein Compilerfehler in avr-gcc 3.4.1 vor.
  Siehe dazu auch: http://www.mikrocontroller.net/forum/read-2-127952.html
*/

void pixel_set_safe(uint8_t posx, uint8_t posy, uint8_t color) {
if ((posx < screenx) && (posy < screeny)) {
  gdata[posy][posx] = color;
}
}

void insert_buff_x(uint8_t y) {
uint8_t x;
for (x = 0; x < screenx; x++) {
  gdata[y][x] = linebuff[x];
  //pixel_set(x,y,linebuff[x]);
}
}

void insert_buff_y(uint8_t x) {
uint8_t y;
for (y = 0; y < screeny; y++) {
  gdata[y][x] = linebuff[y];
  //pixel_set(x,y,linebuff[y]);
}
}

void move_line_down (uint8_t x) { //Zeile um 1 Pixel nach unten
uint8_t y;
for (y = (screeny-1);y > 0;y--) {
gdata[y][x] = gdata[y-1][x];
//pixel_set(x,y,gdata[y-1][x]);
  if (y == 1) {
    gdata[0][x] = linebuff[x];
    //pixel_set(x,0,linebuff[x]);
  }
}
}

void move_down (void) { //Bild um 1 Pixel nach unten
uint8_t x;
for (x = 0;x < screenx;x++) {
  move_line_down(x);
}
}

void move_line_up (uint8_t x) { //Zeile um 1 Pixel nach oben
uint8_t y;
for (y = 0;y < (screeny-1);y++) {
  //pixel_set(x,y,gdata[y+1][x]);
  gdata[y][x] = gdata[y+1][x];
  if (y == (screeny-2)) {
    //pixel_set(x,y+1,linebuff[x]);
    gdata[y+1][x] = linebuff[x];
  }
}
}

void move_up (void) { //Bild um 1 Pixel nach oben
uint8_t x;
for (x = 0;x < screenx;x++) {
  move_line_up(x);
}
}

void move_line_right (uint8_t y) { //Zeile um 1 Pixel nach rechts
uint8_t x;
for (x = (screenx-1);x > 0;x--) {
  gdata[y][x] = gdata[y][x-1];
  if (x == 1) {
    gdata[y][x-1] = linebuff[y];
  }
}
}

void move_right (void) { //Bild um 1 Pixel nach rechts
uint8_t y;
for (y = 0;y <screeny;y++) {
  move_line_right(y);
}
}

void move_line_left (uint8_t y) { //Zeile um 1 Pixel nach links
uint8_t x;
for (x = 0;x < (screenx-1);x++) {
  gdata[y][x] = gdata[y][x+1];
  if (x == (screenx-2)) {
    gdata[y][x+1] = linebuff[y];
  }
}
}

void move_left (void) { //Bild um 1 Pixel nach links
uint8_t y;
for (y = 0;y < screeny;y++) {
  move_line_left(y);
}
}

//Zeichnet eine Linie, benötigt 340 Byte Flash
void draw_line (uint8_t posx, uint8_t posy, int8_t lengthx, int8_t lengthy,
                uint8_t color, uint8_t overlay) {
uint8_t rundung,pixelx,pixely;
int16_t steigung;
int8_t nun;
uint16_t temp;
//Bei overlay ist die Farbe nicht überall die gleiche; kostet: 4 Byte
uint8_t ncolor;
  if (lengthx != 0) {
    steigung = lengthy*256 / lengthx;
    nun = 0;
    while (nun != lengthx) {
      temp = steigung*nun; //Das einmalige Berechnen spart 58 Byte
      if (((temp) % 256) > 127) {
        rundung = 1;
      } else {
        rundung = 0;
      }
      pixelx = posx+nun;
      pixely = posy+rundung+(temp / 256);
      ncolor = color;
      if (overlay != 0) { //Überlappen durch OR Verknüpfung
        ncolor |= gdata[pixely][pixelx];
      }
      pixel_set_safe(pixelx,pixely,ncolor);
      if (lengthx < 0) {
        nun--;
      } else {
       nun++;
      }
    } //Ende der Schleife
  } //Ende wenn lengthx = 0
  if (lengthy != 0) {
    steigung = lengthx*256 / lengthy;
    nun = 0;
    while (nun != lengthy) {
      temp = steigung*nun; //Das einmalige Berechnen spart 58 Byte Flash
      if (((temp) % 256) > 127) {
        rundung = 1;
      } else {
        rundung = 0;
      }
      pixelx = posx+rundung+(temp) / 256;
      pixely = posy+nun;
      ncolor = color;
      if (overlay != 0) { // Überlappen durch OR Verknüpfung
        ncolor |= gdata[pixely][pixelx];
      }
      pixel_set_safe(pixelx,pixely,ncolor);
      if (lengthy < 0) {
        nun--;
      } else {
        nun++;
      }//Ende der Schleife
    }
  }//Ende wenn lengthy = 0
}

void flip_color(void) {
uint8_t x,y, durchlauf;
uint8_t temp;
uint8_t ocolor,ncolor;
//Zuerst Abbild des akutellen Inhalts erstellen
for (x = 0;x <screenx;x++) {
  for (y = 0;y <screeny;y++) {
    temp = gdata[y][x];//temp ist nicht volatile
    temp &= 0x33;      //Restlichen Bits auf 0 setzen
    temp |= temp*4;    //In die auf 0 gesetzten Bits den anderen Inhalt kopieren
    gdata[y][x] = temp;//Zurückschreiben
  }
}
//Nun die eigentliche Flip Funktion
for (durchlauf = 3; durchlauf > 0;durchlauf--) {
  for (x = 0;x <screenx;x++) {
    for (y = 0;y <screeny;y++) {
      temp = gdata[y][x];
      //Rote Pixel
      ocolor = temp & 0x03;//Aktuelles Rot
      ncolor = temp >>6;//Zukünftiges Rot
      if (ocolor > ncolor) {
        ocolor--;
      }
      if (ocolor < ncolor) {
        ocolor++;
      }
      temp = (temp & 0xfc) | ocolor;
      //Grüne Pixel
      ocolor = (temp & 0x30)>>4;//Aktuelles Grün
      ncolor = (temp & 0x0c)>>2;//Zukünftiges Grün
      if (ocolor > ncolor) {
        ocolor--;
      }
      if (ocolor < ncolor) {
        ocolor++;
      }
      temp = (temp & 0xcf) | (ocolor<<4);
      gdata[y][x] = temp;
    }
  }
  waitms(100);
}
}

void draw_box (uint8_t startx, uint8_t starty, uint8_t lengthx, uint8_t lengthy,
               uint8_t outercolor, uint8_t innercolor) { //zeichnet eine Box

uint8_t nunx,nuny;
const uint8_t endx = startx+lengthx-1;
const uint8_t endy = starty+lengthy-1;
uint8_t color;
if ((lengthx != 0) && (lengthy != 0)) {
  for (nunx = startx; nunx <= endx; nunx++) {
    for (nuny = starty; nuny <= endy; nuny++) {
      if ((nunx == startx) || (nunx == endx) || (nuny == starty) ||
         (nuny == endy)) {
        color = outercolor;
      } else {
        color = innercolor;
      }
      pixel_set_safe(nunx,nuny,color);
    }
  }
}
}

void clear_screen(void) {
  draw_box(0,0,screenx,screeny,0x00,0x00);
}

void showbin(u08 posx, u16 value, u08 oncolor) {
u08 nun,color;
for (nun = 0; nun < 16; nun++) {
  if ((value & 1) == 0) { //Wenn Bit eine 0
    color = 0; //Keine Farbe
  } else {
    color = oncolor; //Kräftiges rot
  }
  value = value >> 1; //Rechts Shift
  pixel_set_safe(15-nun,posx,color);
}
}

