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

/* Tests haben gezeigt, dass bei 1KB RAM rund 458 Byte frei sind. Diese können
  für Stack und malloc() verwendet werden. Bei 2KB steht entsprechend mehr zur
  Verfgung.
*/
#include "main.h"

#if modul_pxxo

struct xxo_spielfeldstruct{
u08 *address;
u08 sizex;
u08 sizey;
};

static u08 xxo_field_get(struct xxo_spielfeldstruct *spielfeld, u08 posx,
                         u08 posy) {
const u08 sizex = spielfeld->sizex;
const u08 sizey = spielfeld->sizey;
posx--;
posy--;
if ((posx < sizex) && (posy < sizey)) {
  return *(spielfeld->address+(sizex*posy+posx));
}
return 0xff;
}
static __inline__ u08 xxo_field_get_quick(struct xxo_spielfeldstruct *spielfeld,
       u08 posx, u08 posy) __attribute__((always_inline));
static __inline__ u08 xxo_field_get_quick(struct xxo_spielfeldstruct *spielfeld,
       u08 posx, u08 posy) {
const u08 sizex = spielfeld->sizex;
posx--;
posy--;
return *(spielfeld->address+(sizex*posy+posx));
}

static void xxo_field_set(struct xxo_spielfeldstruct *spielfeld, u08 posx,
                          u08 posy, u08 value) {
u08 *address;
const u08 sizex = spielfeld->sizex;
const u08 sizey = spielfeld->sizey;
posx--;
posy--;
if ((posx < sizex) && (posy < sizey)) {
  address = spielfeld->address+(spielfeld->sizex*posy+posx);
 *(address) = value;
} else {
#ifdef is_i386
  printf("xxo_field_set: Error: Invalid position write try\n");
#endif
}
}

static void xxo_drawmenu1(u08 wahl) {
//Größe des Spielfeldes Auswahl zeichnen
clear_screen();
if (wahl == 1) {
  draw_box(0,0,16,8,0x03,0x00);
}
if (wahl == 2) {
  draw_box(0,8,16,8,0x03,0x00);
}
draw_tinynumber(7,3,2,0x31);
draw_line(7,3,3,3,0x31,0);
draw_line(7,5,3,-3,0x31,0);
draw_tinynumber(6,11,2,0x31);
draw_tinynumber(10,-1,9,0x31); //-1 ist eigentlich etwas unsauber
draw_line(7,10,3,3,0x31,0);
draw_line(7,12,3,-3,0x31,0);
draw_tinynumber(10,9,9,0x31);
}

const char xxo_player1[] PROGMEM = "1P";
const char xxo_player2[] PROGMEM = "2P";

static void xxo_drawmenu2(u08 players) {
//Anzahl der Spieler auswahl zeichnen
clear_screen();
if (players == 1) {
  draw_box(0,0,16,8,0x03,0x00);
}
if (players == 2) {
  draw_box(0,8,16,8,0x03,0x00);
}
load_text(xxo_player1);
draw_string(3,1,0x31,0,0);
load_text(xxo_player2);
draw_string(3,9,0x31,0,0);
}

const char xxo_ai[] PROGMEM = "AI:";

static void xxo_drawmenu3(u08 wahl) {
//Ausgewählte KI Stärke zeichnen
clear_screen();
load_text(xxo_ai);
draw_string(0,1,0x31,0,0);
draw_line(7,14,2,0,0x20,0);
if (wahl > 0) {
  draw_line(6,13,4,0,0x30,0);
}
if (wahl > 1) {
  draw_line(5,12,6,0,0x31,0);
}
if (wahl > 2) {
  draw_line(4,11,8,0,0x32,0);
}
if (wahl > 3) {
  draw_line(3,10,10,0,0x12,0);
}
if (wahl > 4) {
  draw_line(2,9,12,0,0x03,0);
}
}

static u08 xxo_selectmode(struct xxo_spielfeldstruct *spielfeld, u08 *aipower) {
u08 players, nunx,nuny;
//Spielfeldgröße auswählen: 7x6, 10x10
//Bei Größe 12x12 ist die KI einfach unerträglich lahm, aber möglich ist es
xxo_drawmenu1(1);
spielfeld->sizex = 7;
spielfeld->sizey = 6;
while (userin_press() == 0) {
  if (userin_up()) {
    xxo_drawmenu1(1);
    spielfeld->sizex = 7;
    spielfeld->sizey = 6;
  }
  if (userin_down()) {
    xxo_drawmenu1(2);
    spielfeld->sizex = 10;
    spielfeld->sizey = 10;
  }
}
//Speicher Reservieren
spielfeld->address = malloc(spielfeld->sizex*spielfeld->sizey*sizeof(u08));
if (spielfeld->address == NULL) { //Nicht ausreichend RAM vorhanden!
  return 0;
}
//Spielfeld leeren
for (nunx = 1; nunx <= spielfeld->sizex; nunx++) {
  for (nuny = 1; nuny <= spielfeld->sizey; nuny++) {
    xxo_field_set(spielfeld,nunx,nuny, 0);
  }
}
while (userin_press()); //Warte bis Taster losgelassen
userin_flush();
//Spielerzahl auswählen
players = 1;
xxo_drawmenu2(players);
while (userin_press() == 0) {
  if (userin_down()) {
    players = 2;
    xxo_drawmenu2(players);
  }
  if (userin_up()) {
    players = 1;
    xxo_drawmenu2(players);
  }
}
//AI Stärke auswählen
if (players == 1) { //AI Spielt mit
  *aipower = 3;
  while (userin_press()); //Warte bis Taster losgelassen
  userin_flush();
  xxo_drawmenu3(*aipower);
  while (userin_press() == 0) {
    if (userin_down()) { //Schwächer
      if (*aipower > 0) {
        (*aipower)--;
        xxo_drawmenu3(*aipower);
      }
    }
    if (userin_up()) { //Stärker
      if (spielfeld->sizex < 8) {
        *aipower = min((*aipower)+1, 5);
      } else {
        *aipower = min((*aipower)+1, 4);
      }
      xxo_drawmenu3(*aipower);
    }
  }
  //*aipower = 7; //Nur für PC benutzen, ein AVR wäre zu langsam
}
clear_screen();
return players;
}

static void xxo_drawgame(struct xxo_spielfeldstruct *spielfeld) {
u08 startx,starty;
u08 nunx,nuny;
u08 typ, color;
//Begrenzung zeichnen
startx = 8-spielfeld->sizex/2;
starty = 8-spielfeld->sizey/2;
draw_line(startx-1,starty-1,spielfeld->sizex+2,0,0x31,0);
draw_line(startx-1,starty+spielfeld->sizey,spielfeld->sizex+2,0,0x31,0);
draw_line(startx-1,starty-1,0,spielfeld->sizey+2,0x31,0);
draw_line(startx+spielfeld->sizex,starty-1,0,spielfeld->sizey+2,0x31,0);
//Inhalt des Spielfeldes zeichnen
for (nunx = 0; nunx < spielfeld->sizex; nunx++) {
  for (nuny = 0; nuny < spielfeld->sizey; nuny++) {
    typ = xxo_field_get(spielfeld,nunx+1,nuny+1);
    color = 0;
    if (typ == 1) {
      color = 0x30;  //Grün
    }
    if (typ == 2) {
      color = 0x03;  //Rot
    }
    pixel_set_safe(startx+nunx,starty+nuny,color);
  }
}
}

static u08 xxo_placestone_visual(struct xxo_spielfeldstruct *spielfeld,
                                 u08 playerturn, u08 posx) {
u08 posy = 1;
if (xxo_field_get(spielfeld,posx,1)) { //Wenn oberstes Feld bereits besetzt
  return 0; //Kann nicht platziert werden
}
while (1) {
  xxo_field_set(spielfeld, posx, posy, playerturn);
  xxo_drawgame(spielfeld);
  waitms(200);
  if ((posy < spielfeld->sizey) &&  //Wenn noch nicht am Boden
      (xxo_field_get(spielfeld, posx, posy+1) == 0)) { //Und untere Stelle frei
    xxo_field_set(spielfeld, posx, posy, 0); //Alten Stein entfernen
    posy++;
  } else {
    break;
  }
}
return posy; //Position, an der platziert wurde
}

static u08 xxo_placestone(struct xxo_spielfeldstruct *spielfeld, u08 playerturn,
                          u08 posx) {
u08 posy = 1;
//Tiefe ermitteln
for (posy = spielfeld->sizey; posy > 0; posy--) {
  if (posy) {
    if (xxo_field_get_quick(spielfeld, posx, posy) == 0) { //Wenn noch frei
      break;
    }
  }
}
if (posy != 0) { //0 = keine Platz, Platz muss aber sein
  xxo_field_set(spielfeld, posx, posy, playerturn);
}
return posy; //Position wo platziert wurde
}

static u08 xxo_movehuman(struct xxo_spielfeldstruct *spielfeld, u08 playerturn,
                         u08 playeroldpos) {
u08 startx, posx, posx_o = 0;
u08 posy;
posy = 8-spielfeld->sizey/2-2;
posx = playeroldpos;
startx = 8-spielfeld->sizex/2;
userin_flush();
while (1) {
  if (userin_press()) { //Wenn Tastendruck
    if (xxo_placestone_visual(spielfeld, playerturn, posx)) {
      break; //Platzierung erfolgreich
    }
  }
  if (userin.x > 50) { //Nach rechts
    posx = min(posx+1,spielfeld->sizex);
  }
  if (userin.x < -50) { //Nach links
    posx = max(posx-1,1);
  }
  if (posx_o != posx) { //Positionsänderung, neu zeichnen
    draw_line(0,posy,16,0,0x00,0); //Obere Zeile leeren
    if (playerturn == 1) { //Grüner Spieler
      pixel_set_safe(posx+startx-1,posy,0x30);
    } else {
      pixel_set_safe(posx+startx-1,posy,0x03); //Roter Spieler
    }
    waitms(550-abs(userin.x*3));
  }
  posx_o = posx;
}
return posx;  //Cursor Position zurückliefern
}

static void xxo_vote_inarow(u08 inarow1, u08 inarow2, u16 *vote1, u16 *vote2) {
//Wenn beide einen Stein dort haben, Bewertung 0
if ((inarow1 != 0) && (inarow2 != 0)) {
  return;
}
if (inarow1 == 4) { //Spieler 1 hat gewonnen
  *vote1 = 0xffff;
}
if (inarow2 == 4) { //Spieler 2 hat gewonnen
  *vote2 = 0xffff;
}
if ((*vote1 < 0xffff) && (*vote2 < 0xffff)) {
  if (inarow1 == 3) {
    *vote1 += 50;
  } else
  if (inarow2 == 3) {
    *vote2 += 50;
  } else
  if (inarow1 == 2) {
    *vote1 += 8;
    return;
  } else
  if (inarow2 == 2) {
    *vote2 += 8;
  } else
  if (inarow1 == 1) {
    *vote1 += 2;
  } else
  if (inarow2 == 1) {
    *vote2 += 2;
  }
}
}

static s16 xxo_votegame(struct xxo_spielfeldstruct *spielfeld, u08 playerturn) {
u08 nunx,nuny, nunz;
s16 globalvote = 0;
u16 vote1 = 0, vote2 = 0; //Bewertung für Spieler 1 und Spieler 2
u08 inarow[4] = {0,0,0,0};
u08 sitting;
u08 freeline;
u08 freelines = 0;
u08 primefield;
const u08 sizey = spielfeld->sizey;

for (nuny = sizey; nuny > 0; nuny--) {
  freeline = 1;
  for (nunx = 1; nunx <= spielfeld->sizex; nunx++) {
    /* Es wird immer vier nach rechts, vier nach schräg unten und vier nach
       unten getestet. Dabei wird gezählt wie viele bereits in einer Reihe sind.
       Sind es 4, so hat ein Spieler bereits gewonnen.
       Bei 3 gibt es eine sehr hohe Bewertung.
       Bei 2 eine erhöhte
       Bei einem leicht erhöhte
       Allerdings ist die Bewertung immer 0, wenn der Gegner dazwischen sitzt
    */
    //Nach rechts sehen
    primefield = xxo_field_get_quick(spielfeld,nunx,nuny) & 0x03;
    if (primefield) {
      freeline = 0;
    }
    if (nunx < (spielfeld->sizex-2)) {
      inarow[0] = 0;
      inarow[1] = 0;
      inarow[2] = 0;
      inarow[primefield]++;
      for (nunz = 1; nunz < 4; nunz++) {
        sitting = xxo_field_get_quick(spielfeld,nunx+nunz,nuny) & 0x03;
        inarow[sitting]++;
      }
      if (inarow[0] != 4) {
        xxo_vote_inarow(inarow[1], inarow[2], &vote1,&vote2);//Resultat bewerten
      }
    }
    //Nach unten sehen
    if (nuny < (sizey-2)) {
      inarow[0] = 0;
      inarow[1] = 0;
      inarow[2] = 0;
      inarow[primefield]++;
      for (nunz = 1; nunz < 4; nunz++) {
        sitting = xxo_field_get_quick(spielfeld,nunx,nuny+nunz);
        inarow[sitting]++;
      }
      if (inarow[0] != 4) {
        xxo_vote_inarow(inarow[1], inarow[2], &vote1,&vote2);//Resultat bewerten
      }
    }
    //Nach rechts-unten sehen
    if ((nunx < (spielfeld->sizex-2)) && (nuny < (sizey-2))) {
      inarow[0] = 0;
      inarow[1] = 0;
      inarow[2] = 0;
      inarow[primefield]++;
      for (nunz = 1; nunz < 4; nunz++) {
        sitting = xxo_field_get_quick(spielfeld,nunx+nunz,nuny+nunz);
        inarow[sitting]++;
      }
    }
    if (inarow[0] != 4) {
        xxo_vote_inarow(inarow[1], inarow[2], &vote1,&vote2);//Resultat bewerten
      }
    //Nach links-unten sehen
    if ((nunx > 3) && (nuny < (sizey-2))) {
      inarow[0] = 0;
      inarow[1] = 0;
      inarow[2] = 0;
      inarow[primefield]++;
      for (nunz = 1; nunz < 4; nunz++) {
        sitting = xxo_field_get_quick(spielfeld,nunx-nunz,nuny+nunz);
        inarow[sitting]++;
      }
      if (inarow[0] != 4) {
        xxo_vote_inarow(inarow[1], inarow[2], &vote1,&vote2);//Resultat bewerten
      }
    }
  } //Ende x Schleife
  if (freeline) {
    freelines++;
  }
  if (freelines == 4) { //Darüber kommt auch nichts mehr
    break;
  }
} //Ende y Schleife
if (vote1 == 0xffff) {
  globalvote = 120;
}
if (vote2 == 0xffff) {
  globalvote = -120;
}
if (playerturn == 2) {
  globalvote *= (-1);
}
//Positiv: gut für den Spieler playerturn, negativ: schlecht für ihn
return globalvote;
}

static s16 xxo_voteplace_quick(struct xxo_spielfeldstruct *spielfeld, u08 posx,
                               u08 posy) {
u08 nun;
u08 playertype;
u08 place1, place2;
/* Diese Funktion achtet nicht sonderlich auf saubere Programmierung
  - hauptsache sie ist schnell!!!
  Die Bereichsüberprüfung übernimmt die xxo_field_get() Funktione, deshalb darf
  meistens nicht die xxo_field_get_quick() Funktion verwendet werden.
  Die Funktion sieht nach ob sich bei den übergebenen posx, posy Koordinaten
  vier in einer Reihe befinden. Dann wird 32000 zurückgegeben, ansonsten 0.
   */
playertype = xxo_field_get_quick(spielfeld,posx,posy);
if ((playertype != 1) && (playertype != 2)) { //is doch nix!
  return 0;
}
//Horizontal rechts
place1 = 0;
for (nun = 1; nun < 4; nun++) {
  if (xxo_field_get(spielfeld,posx+nun,posy) != playertype) {
    break;
  }
  place1++;
}
//Horizontal links
place2 = 0;
for (nun = 1; nun < 4; nun++) {
  if (xxo_field_get(spielfeld,posx-nun,posy) != playertype) {
    break;
  }
  place2++;
}
//Nachsehen ob horizontal vier in einer Reihe
if ((place1+place2) > 2) {
  return 120;
}
//Vertikal runter
place1 = 0;
for (nun = 1; nun < 4; nun++) {
  if (xxo_field_get(spielfeld,posx,posy+nun) != playertype) {
    break;
  }
  place1++;
}
//Vertikal hoch
place2 = 0;
for (nun = 1; nun < 4; nun++) {
  if (xxo_field_get(spielfeld,posx,posy-nun) != playertype) {
    break;
  }
  place2++;
}
//Nachsehen ob vertikal vier in einer Reihe
if ((place1+place2) > 2) {
  return 120;
}
//Schräg rechts unten
place1 = 0;
for (nun = 1; nun < 4; nun++) {
  if (xxo_field_get(spielfeld,posx+nun,posy+nun) != playertype) {
    break;
  }
  place1++;
}
//Schräg links oben
place2 = 0;
for (nun = 1; nun < 4; nun++) {
  if (xxo_field_get(spielfeld,posx-nun,posy-nun) != playertype) {
    break;
  }
  place2++;
}
//Nachsehen ob horizontal vier in einer Reihe
if ((place1+place2) > 2) {
  return 120;
}
//Schräg rechts oben
place1 = 0;
for (nun = 1; nun < 4; nun++) {
  if (xxo_field_get(spielfeld,posx+nun,posy-nun) != playertype) {
    break;
  }
  place1++;
}
//Schräg links unten
place2 = 0;
for (nun = 1; nun < 4; nun++) {
  if (xxo_field_get(spielfeld,posx-nun,posy+nun) != playertype) {
    break;
  }
  place2++;
}
//Nachsehen ob horizontal vier in einer Reihe
if ((place1+place2) > 2) {
  return 120;
}
return 0;
}

static u08 xxo_is_tie(struct xxo_spielfeldstruct *spielfeld) {
u08 nun;
for (nun = 1; nun <= spielfeld->sizex; nun++) {
  if (xxo_field_get(spielfeld, nun, 1) == 0) { //Ein Feld ohne Spielstein
    return 0; //Also kein Unentschieden, da weitergespielt werden kann
  }
}
return 1;
}

static void xxo_highlightwinner(struct xxo_spielfeldstruct *spielfeld) {
u08 nunx,nuny = 0, nunz;
u08 inarow1 = 0, inarow2 = 0;
s08 directionx = 0, directiony = 0;
u08 sitting;
u08 nun;
u08 startx, starty;
for (nunx = 1; nunx <= spielfeld->sizex; nunx++) {
  for (nuny = 1; nuny <= spielfeld->sizey; nuny++) {
    /* Es wird immer vier nach rechts, vier nach schräg unten und vier nach
       unten getestet. Dabei wird gezählt wie viele bereits in einer Reihe sind.
       Sind es 4, so hat ein Spieler bereits gewonnen und dies wird gelb
       gefärbt
    */
    //Nach rechts sehen
    if (nunx < (spielfeld->sizex-2)) {
      inarow1 = 0;
      inarow2 = 0;
      directionx = 4;
      directiony = 0;
      for (nunz = 0; nunz < 4; nunz++) {
        sitting = xxo_field_get(spielfeld,nunx+nunz,nuny);
        if (sitting == 1) {
          inarow1++;
        }
        if (sitting == 2) {
          inarow2++;
        }
      }
      if ((inarow1 == 4) || (inarow2 == 4)) {
        break;
      }
    }
    //Nach unten sehen
    if (nuny < (spielfeld->sizey-2)) {
      inarow1 = 0;
      inarow2 = 0;
      directionx = 0;
      directiony = 4;
      for (nunz = 0; nunz < 4; nunz++) {
        sitting = xxo_field_get(spielfeld,nunx,nuny+nunz);
        if (sitting == 1) {
          inarow1++;
        }
        if (sitting == 2) {
          inarow2++;
        }
      }
      if ((inarow1 == 4) || (inarow2 == 4)) {
        break;
      }
    }
    //Nach rechts-unten sehen
    if ((nunx < (spielfeld->sizex-2)) && (nuny < (spielfeld->sizey-2))) {
      inarow1 = 0;
      inarow2 = 0;
      directionx = 4;
      directiony = 4;
      for (nunz = 0; nunz < 4; nunz++) {
        sitting = xxo_field_get(spielfeld,nunx+nunz,nuny+nunz);
        if (sitting == 1) {
          inarow1++;
        }
        if (sitting == 2) {
          inarow2++;
        }
      }
      if ((inarow1 == 4) || (inarow2 == 4)) {
        break;
      }
    }
    //Nach links-unten sehen
    if ((nunx > 3) && (nuny < (spielfeld->sizey-2))) {
      inarow1 = 0;
      inarow2 = 0;
      directionx = -4;
      directiony = 4;
      for (nunz = 0; nunz < 4; nunz++) {
        sitting = xxo_field_get(spielfeld,nunx-nunz,nuny+nunz);
        if (sitting == 1) {
          inarow1++;
        }
        if (sitting == 2) {
          inarow2++;
        }
      }
      if ((inarow1 == 4) || (inarow2 == 4)) {
        break;
      }
    }
  }//Ende der y Schleife
  if ((inarow1 == 4) || (inarow2 == 4)) {
    break;
  }
}//Ende der x Schleife
//Wir haben Koordinaten und Richtung des Gewinners
startx = 8-spielfeld->sizex/2-1;
starty = 8-spielfeld->sizey/2-1;
for (nun = 0; nun < 4; nun++) {
  //Normales Spielfeld anzeigen
  xxo_drawgame(spielfeld);
  waitms(350);
  //Gewinnreihe gelb aufblinken lassen
  draw_line(startx+nunx, starty+nuny, directionx, directiony, 0x32, 0);
  waitms(350);
}
}

static s16 xxo_calcai(struct xxo_spielfeldstruct *testfeld, u08 playerturn,
                      u08 oplayer, u08 remdepth) {

const u08 sizex = testfeld->sizex;
s16 currvote[sizex];
s16 bestvote, worstvote;
u08 nun;
u08 testplacey;
for (nun = 1; nun <= sizex; nun++) { //Wir bewerten alle Plätze (max 12)
  testplacey = xxo_placestone(testfeld, playerturn, nun);
  if (testplacey) { //Wenn platziert werden konnte
    //Bewertung einholen
    currvote[nun-1] = xxo_voteplace_quick(testfeld,nun,testplacey);
    if (playerturn != oplayer) { //Wenn Zug von gegner Bewertet wurde
      //Was für den Gegner gut ist für einen selber schlecht:
      currvote[nun-1] *= (-1);
    }
    if (abs(currvote[nun-1]) < 120) { //Spiel läuft noch
      if (remdepth) {  //Wenn noch tiefer und nicht entschieden
        currvote[nun-1] = xxo_calcai(testfeld, 3-playerturn,
        oplayer, remdepth-1);
      }
    }
    xxo_field_set(testfeld, nun, testplacey,0); //Testplatzierung lï¿½chen
  } else {
    currvote[nun-1] = -125; //nicht möglich, da Spalte voll
  }
} //Ende for Schleife

/*So, wir haben jetzt eine Bewertung für jede mögliche Position
  Wenn oplayer == playerturn, so wird die beste Bewertung zurückgeliefert
  Wenn oplayer != playerturn, so wird die schlechteste zurückgeliefert
*/
bestvote = -127;
worstvote = 127;
for (nun = 0; nun < sizex; nun++) { //Die besten und schlechtesten Punkte
  if (currvote[nun] > -125) { //-125 steht für Wand und somit egal
    if (currvote[nun] > bestvote) { //Bessere Bewertung
      bestvote = currvote[nun];
    }
    if (currvote[nun] < worstvote) { //Schlechtere Bewertung
      worstvote = currvote[nun];
    }
  }
}
//Bewertung zurückgeben
if (playerturn == oplayer) { //KI am Zug
  if (bestvote >= 10) {
    bestvote -= 10;
  }
  return bestvote;
} else {
  if (worstvote <= -10) {
    worstvote += 10;
  }
  return worstvote;
}
}


static s16 xxo_calcai_head(struct xxo_spielfeldstruct *testfeld, u08 playerturn,
                      u08 oplayer, u08 remdepth) {

const u08 sizex = testfeld->sizex;
s16 currvote[sizex];
s16 bestvote, worstvote;
u08 bestplace;
u08 nun;
u08 testplacey;
for (nun = 1; nun <= sizex; nun++) { //Wir bewerten alle Plätze (max 12)
  testplacey = xxo_placestone(testfeld, playerturn, nun);
  if (testplacey) { //Wenn platziert werden konnte
    //Bewertung einholen
    currvote[nun-1] = xxo_voteplace_quick(testfeld,nun,testplacey);
    if (playerturn != oplayer) { //Wenn Zug von Gegner bewertet wurde
      //Was für den Gegner gut ist für einen selber schlecht:
      currvote[nun-1] *= (-1);
    }
    if (abs(currvote[nun-1]) < 120) { //Spiel läuft noch
      if (remdepth) {  //Wenn noch tiefer und nicht entschieden
        currvote[nun-1] = xxo_calcai(testfeld, 3-playerturn,
        oplayer, remdepth-1);
      }
    }
    xxo_field_set(testfeld, nun, testplacey,0); //Testplatzierung löschen
  } else {
    currvote[nun-1] = -125; //nicht möglich, da Spalte voll
  }
  //Statusanzeige
  if (playerturn == 2) { //Rote Statusanzeige
    pixel_set_safe(nun+8-testfeld->sizex/2-1,8-testfeld->sizey/2-2,0x03);
  } else { //Grüne Statusanzeige
    pixel_set_safe(nun+8-testfeld->sizex/2-1,8-testfeld->sizey/2-2,0x30);
  }
} //Ende for Schleife

/*So, wir haben jetzt eine Bewertung für jede mögliche Position
  Wir geben die Position mit dem besten Wert zurück
*/
bestvote = -127;
worstvote = 125;
bestplace = 3;
for (nun = 0; nun < sizex; nun++) { //Die besten und schlechtesten Punkte
  if (currvote[nun] > -125) { //-125 steht für Wand und somit eher egal
    if (currvote[nun] == bestvote) { //Gleichwertig
      if ((rand() & 3) == 3) { //25% Wahrscheinlichkeit
        bestvote = currvote[nun];
        bestplace = nun+1;
      }
    }
    if (currvote[nun] > bestvote) { //Bessere Bewertung
      bestvote = currvote[nun];
      bestplace = nun+1;
    }
  }
  if (currvote[nun] < worstvote) { //Schlechtere Bewertung
    worstvote = currvote[nun]; //Hier nur für Mitte setzen wichtig
  }
/*
#ifdef is_i386
  printf("Depth: %i: Place %i: %i\n",remdepth,nun+1,currvote[nun]);
#endif
*/
}
//Zurückgeben der Werte
if (bestvote <= -110) { //Verloren, platziere irgendwo
  //Suche nächsten freien Platz
  bestplace = rand() % sizex; //Zufallsgenerator
  for (nun = 1; nun <= sizex; nun++) {
    bestplace %= sizex;
    bestplace++;
    if (xxo_field_get(testfeld, bestplace, 1) == 0) {
      break;
    }
  }
}
if ((bestvote == 0) && (worstvote == 0))  { //Wenn absolut keine Ahnung
  bestplace = (sizex+1) / 2; //Dann ist die Mitte meist am besten
}
return bestplace;
}

static void xxo_moveai(struct xxo_spielfeldstruct *spielfeld, u08 playerturn,
                u08 aipower) {
u08 bestpos;
u08 color;
draw_line(0,8-spielfeld->sizey/2-2,16,0,0x00,0); //Obere Zeile leeren
bestpos = xxo_calcai_head(spielfeld, playerturn, playerturn, aipower);
if (playerturn == 1) {
  color = 0x30;
} else {
  color = 0x03;
}
draw_line(8-spielfeld->sizex/2,8-spielfeld->sizey/2-2,spielfeld->sizex,
          0,color,0); //Obere Zeile füllen
xxo_placestone_visual(spielfeld, playerturn, bestpos);
}

const char xxo_memerror[] PROGMEM = "Need more RAM!";

void xxo_start(void) {
u08 players, playerturn;
u08 gameinprogress = 1;
struct xxo_spielfeldstruct spielfeld;
u08 playeroldpos[2];  //Cursor für beide Spieler
u08 aipower = 0; //wird durch xxo_selectmode initialisiert falls AI verwendet

players = xxo_selectmode(&spielfeld,&aipower);
init_random(); //Zufallsgenerator für KI
playeroldpos[0] = (spielfeld.sizex+1)/2;
playeroldpos[1] = (spielfeld.sizex+1)/2;
if (players) {
  xxo_drawgame(&spielfeld);
  waitms(500);
  playerturn = 1+(rand() & 1); //Entweder Spieler 1 oder Spieler 2 beginnt
  while (gameinprogress) {
    if ((players == 1) && (playerturn == 2)) { //KI ist am Zug
      xxo_moveai(&spielfeld, playerturn, aipower);
      waitms(1000);
    } else {  //Benutzereingabe bearbeiten
      playeroldpos[playerturn-1] = xxo_movehuman(&spielfeld, playerturn,
                                                 playeroldpos[playerturn-1]);
    }
    playerturn = 3-playerturn; //Anderer Spieler
    if (abs(xxo_votegame(&spielfeld,playerturn)) >= 120) {
      gameinprogress = 0;
    }
    if (xxo_is_tie(&spielfeld)) { //Es kann kein Stein mehr gesetzt werden
      gameinprogress = 0;
    }
  } //Ende Spiel läuft
  if (xxo_is_tie(&spielfeld) == 0) { //Wenn jemand gewonnen hat
    xxo_highlightwinner(&spielfeld); //Die gewonnenen 4 aufblinken lassen
  }
  free(spielfeld.address);
} else { //Out of RAM!
  clear_screen();
  load_text(xxo_memerror);
  scrolltext(4,0x03,0x00,100);
}
userin_flush();
while (userin_press() == 0); //Warten auf Tastendruck
}

#endif
