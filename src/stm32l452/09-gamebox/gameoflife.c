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

/*Die Game-of-life Funktionen ben�tigen ungef�hr 386 Byte Flash
Um in der Demo immer die gleichen Resultate unabh�ngig der 'Aufl�sung' zu
erhalten verwende ich hier eine feste Gr��e von 16x16 Punkten und nicht
die durch screenx und screeny angegebene Gr��e.
Eine Anpassung an andere 'Aufl�sungen' d�rfe aber nicht schwierig sein.
Datum der letzten �nderung: 2006-11-03
2005-07-30: modul_demo Abfrage eingebaut
2005-08-19: Kommentare aktualisiert
2006-11-03: Tippfehler in den Kommentaren + Formatierungen korrigiert
            + GPL Text + internen Funktionen 'static' gemacht
*/


#include "main.h"

#if modul_demo

#include "gameoflife.h"

static u08 is_object(u08 px, u08 py) {
u08 the_data;

if ((px < 16) && (py < 16)) {
  the_data = gdata[py][px];
  the_data &= 0x30; //Extrahiere gr�ne Daten
  if (the_data == 0) {
    return 0;
  } else {
    return 1;
  }
}
return 0;
}

static u08 getneighbour(u08 posx, u08 posy){
u08 alive;
u08 nposx,nposy;
//Errechnet wieviele Nachbar Lebewesen leben.
 //Rechte Position
 nposx = posx+1;
 nposy = posy;
 alive = is_object(nposx,nposy);
 //Linke Position
 nposx = posx-1;
 nposy = posy;
 alive += is_object(nposx,nposy);
 //Obere Position
 nposx = posx;
 nposy = posy-1;
 alive += is_object(nposx,nposy);;
 //Untere Position
 nposx = posx;
 nposy = posy+1;
 alive += is_object(nposx,nposy);
 //Rechte obere Position
 nposx = posx+1;
 nposy = posy-1;
 alive += is_object(nposx,nposy);
 //Linke obere Position
 nposx = posx-1;
 nposy = posy-1;
 alive += is_object(nposx,nposy);
 //Rechte untere Position
 nposx = posx+1;
 nposy = posy+1;
 alive += is_object(nposx,nposy);
 //Linke untere Position
 nposx = posx-1;
 nposy = posy+1;
 alive += is_object(nposx,nposy);
return alive;
}

static u08 whattodo(u08 posx, u08 posy){
u08 usedfield;
usedfield = getneighbour(posx,posy);
  if (is_object(posx,posy) == 0) { //Frei
    if (usedfield == 3) { //Beginne zu leben
      return 0x40;//Schwaches gr�n - beginne zu leben
    } else {
      return 0x00; //Pixel aus - bleibe frei
    }
  } else {   //Ende wenn frei; Wenn bereits lebt
    if ((usedfield > 3) || (usedfield < 2)) {//Eenn zu viel oder zu wenig
      return 0x04; //Gestorben - schwaches rot
    } else {
      return 0xc0; //Bleibe am leben - starkes gr�n
    }
  }//Ende wenn bereits lebt
}

void gameoflife_step(void) {
/* 1: Basierend auf den gr�nen Inhalt erstelle neues Display in unbenutztem
      Speicherteil
   2: �berschreibe angezeige Daten mit dem unsichtbarem Teil
*/
u08 posx,posy;
u08 the_data;
//Die eigentlichen Berechungen
for (posx = 0; posx < 16; posx++) {
  for (posy = 0; posy < 16; posy++) {
    the_data = gdata[posy][posx]; //Lade Daten
    the_data &= 0x33;//L�sche unsichtbare Daten
    the_data |= whattodo(posx,posy);
    gdata[posy][posx] = the_data;
  }
}
//Kopieren der Daten vom unsichtbaren in den sichtbaren Speicherbereich
for (posx = 0; posx < 16; posx++) {
  for (posy = 0; posy < 16; posy++) {
    gdata[posy][posx] = gdata[posy][posx] >> 2;
  }
}
}

#endif
