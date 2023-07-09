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

/* Reversi
0 = Feld ist frei
1 = Auf Feld ist Spieler 1
2 = Auf Feld ist spieler 2
*/
#include "main.h"

#if modul_prev

struct rev_spielfeldstruct{
u08 *address;
};

u32 aivotedops;

const char rev_memerror[] PROGMEM = "Need more RAM!";

static u08 rev_field_get(struct rev_spielfeldstruct *spielfeld, u08 posx,
                         u08 posy) {
posx--;
posy--;
if ((posx < rev_x) && (posy < rev_y)) {
  return *(spielfeld->address+(rev_x*posy+posx));
}
#ifdef is_i386
  printf("rev_field_get: Error: Invalid position request\n");
#endif
return 0xff;
}

static __inline__ u08 rev_field_get_quick(struct rev_spielfeldstruct *spielfeld,
       u08 posx, u08 posy) {
posx--;
posy--;
return *(spielfeld->address+(rev_x*posy+posx));
}

static void rev_field_set(struct rev_spielfeldstruct *spielfeld, u08 posx,
                          u08 posy, u08 value) {
u08 *address;
posx--;
posy--;
if ((posx < rev_x) && (posy < rev_y)) {
  address = spielfeld->address+(rev_x*posy+posx);
 *(address) = value;
} else {
#ifdef is_i386
  printf("rev_field_set: Error: Invalid position write try\n");
#endif
}
}

const char rev_player1[] PROGMEM = "1P";
const char rev_player2[] PROGMEM = "2P";

static void rev_drawmenu(u08 players) {
//Anzahl der Spieler auswahl zeichnen
clear_screen();
if (players == 1) {
  draw_box(0,0,16,8,0x03,0x00);
}
if (players == 2) {
  draw_box(0,8,16,8,0x03,0x00);
}
load_text(rev_player1);
draw_string(3,1,0x31,0,0);
load_text(rev_player2);
draw_string(3,9,0x31,0,0);
}

static u08 rev_selectmode(struct rev_spielfeldstruct *spielfeld) {
//Returns: Anzahl der Spieler
u08 players, nunx,nuny;
//Speicher Reservieren
spielfeld->address = malloc(rev_x*rev_y*sizeof(u08));
if (spielfeld->address == NULL) { //Nicht ausreichend RAM vorhanden!
  return 0;
}
//Spielfeld leeren
for (nunx = 1; nunx <= rev_x; nunx++) {
  for (nuny = 1; nuny <= rev_y; nuny++) {
    rev_field_set(spielfeld,nunx,nuny, 0);
  }
}
//Start Spielfeld
rev_field_set(spielfeld,rev_x/2,rev_y/2, 1);
rev_field_set(spielfeld,rev_x/2,rev_y/2+1, 2);
rev_field_set(spielfeld,rev_x/2+1,rev_y/2, 2);
rev_field_set(spielfeld,rev_x/2+1,rev_y/2+1, 1);
while (userin_press()); //Warte bis Taster losgelassen
userin_flush();
//Spielerzahl ausw�len
players = 1;
rev_drawmenu(players);
while (userin_press() == 0) {
  if (userin_down()) {
    players = 2;
    rev_drawmenu(players);
  }
  if (userin_up()) {
    players = 1;
    rev_drawmenu(players);
  }
}
clear_screen();
return players;
}

static void rev_drawgame(struct rev_spielfeldstruct *spielfeld) {
u08 startx,starty;
u08 nunx,nuny;
u08 typ, color;
//Begrenzung zeichnen
startx = 8-rev_x/2;
starty = 8-rev_y/2;
draw_line(startx-1,starty-1,rev_x+2,0,0x31,0);
draw_line(startx-1,starty+rev_y,rev_x+2,0,0x31,0);
draw_line(startx-1,starty-1,0,rev_y+2,0x31,0);
draw_line(startx+rev_x,starty-1,0,rev_y+2,0x31,0);
//Inhalt des Spielfeldes zeichnen
for (nunx = 0; nunx < rev_x; nunx++) {
  for (nuny = 0; nuny < rev_y; nuny++) {
    typ = rev_field_get(spielfeld,nunx+1,nuny+1);
    color = 0;
    if (typ == 1) {
      color = 0x30;  //Grn
    }
    if (typ == 2) {
      color = 0x03;  //Rot
    }
    pixel_set_safe(startx+nunx,starty+nuny,color);
  }
}
}

static u08 rev_placestone_dir_test(struct rev_spielfeldstruct *spielfeld,
                           u08 playerturn, u08 posx, u08 posy, s08 dx, s08 dy) {
u08 data, valid = 0;
//Return: 0: OK, 1: Fehler
//Prüfe Bewegungsrichtung
if ((dx < -1) || (dx > 1) || (dy < -1) || (dy > 1) ||
    ((dx == 0) && (dy == 0))) {
  return 1;
}
//Teste ob bisherige Position frei
data = rev_field_get_quick(spielfeld,posx,posy);
if (data != 0) { //Platz ist nicht frei
  return 1;
}
//Muss fremde Steine beinhalten
while (valid < 2) {
  posx += dx;
  posy += dy;
  if ((posx == 0) || (posx > rev_x) || (posy == 0) || (posy > rev_y)) { //Rand
    return 1;
  }
  data = rev_field_get_quick(spielfeld,posx,posy);
  if (data == 0) { //Lücke, breche ab
    return 1;
  }
  if ((3-playerturn) == data) { //Ein gegnerischer Stein
    valid = 1;
  }
  if (playerturn == data) {
    if (valid == 1) {
      valid = 2;
    } else {
      return 1;
    }
  }
}
return 0;
}

static u08 rev_placestone_dir(struct rev_spielfeldstruct *spielfeld,
                           u08 playerturn, u08 posx, u08 posy, s08 dx, s08 dy) {
u08 data;
//Retun: 0: Es werden Steine umgedreht, 1: Keine Steine zum umdrehen
//Prüfe Bewegungsrichtung
if ((dx < -1) || (dx > 1) || (dy < -1) || (dy > 1) ||
    ((dx == 0) && (dy == 0))) {
  return 1;
}
//Ersetze fremde Steine (die Gültigkeit dafür muss vorher geprüft werden)
while (1) {
  posx += dx;
  posy += dy;
  if ((posx == 0) || (posx > rev_x) || (posy == 0) || (posy > rev_y)) { //Rand
    return 0;
  }
  data = rev_field_get_quick(spielfeld,posx,posy);
  if ((3-playerturn) == data) { //Ein gegnerischer Stein
    rev_field_set(spielfeld, posx, posy, playerturn);
  } else
    return 0;
}
}

static u08 rev_placestone_test(struct rev_spielfeldstruct *spielfeld,
                                 u08 playerturn, u08 posx, u08 posy) {
//Directions hat einen offset von 1, die oberen 4 Bit sind y, die unteren x
u08 directions[8] = {0x10, 0x12, 0x01, 0x21, 0x00, 0x22, 0x02, 0x20};
u08 nun;
s08 dx,dy;
//Returns: 0: nicht gültig, ansonsten Richtungen passend zu directions als Bits
//Testet ob der Zug gegnerische Steine umdreht
u08 accepted_dir = 0;
for (nun = 0; nun < 8; nun++) {
  dx = (s08)(directions[nun]%16)-1;
  dy = (s08)(directions[nun]/16)-1;
  accepted_dir <<= 1;
  if (rev_placestone_dir_test(spielfeld, playerturn, posx, posy, dx, dy) == 0) {
    accepted_dir |= 1;
  }
}
return accepted_dir;
}

static u08 rev_placestone(struct rev_spielfeldstruct *spielfeld,
                                 u08 playerturn, u08 posx, u08 posy) {
//Returns: 0: valid, 1: out of range
u08 directions[8] = {0x10, 0x12, 0x01, 0x21, 0x00, 0x22, 0x02, 0x20};
s08 dx,dy;
u08 nun;
u08 accepted_dir;
accepted_dir = rev_placestone_test(spielfeld,playerturn,posx,posy);
if (accepted_dir == 0) {
  return 1; //Kann nicht platziert werden
}
//Erobere umliegende fremde Steine
for (nun = 0; nun < 8; nun++) {
  dx = (s08)(directions[nun]%16)-1;
  dy = (s08)(directions[nun]/16)-1;
  if (accepted_dir & 0x80) {
    rev_placestone_dir(spielfeld, playerturn, posx, posy, dx, dy);
  }
  accepted_dir <<= 1;
}
//Platziere am Ursprung (muss nach rev_placestone_dir() erfolgen)
rev_field_set(spielfeld, posx, posy, playerturn); //Setze an Ursprung
return 0;
}

static u08 rev_placestone_visual(struct rev_spielfeldstruct *spielfeld,
                                 u08 playerturn, u08 posx, u08 posy) {
//Returns: 0: valid, 1: invalid
u08 res;
res = rev_placestone(spielfeld, playerturn, posx, posy);
//Zeichne
rev_drawgame(spielfeld);
return res;
}

static u08 rev_movehuman(struct rev_spielfeldstruct *spielfeld, u08 playerturn,
                         u08 opos) {
//Returns: Cursor Position
u08 posx, posy;
u08 oposx = 16, oposy = 16;
u08 wallx = 8-(rev_x/2)-2;
u08 wally = 8-(rev_y/2)-2;

posx = opos % 16;
posy = opos / 16;
userin_flush();
while (1) {
  if (userin_press()) { //Wenn Tastendruck
    if (rev_placestone_visual(spielfeld, playerturn, posx, posy) == 0) {
      break; //Platzierung erfolgreich
    }
  }
  if (userin.x > 50) { //Nach rechts
    posx = min(posx+1,rev_x);
  }
  if (userin.x < -50) { //Nach links
    posx = max(posx-1,1);
  }
  if (userin.y > 50) { //Nach unten
    posy = min(posy+1,rev_y);
  }
  if (userin.y < -50) { //Nach open
    posy = max(posy-1,1);
  }
  if (oposx != posx) { //X-Positions�derung, neu zeichnen
    draw_line(0,wally,16,0,0x00,0); //Obere Zeile leeren
    if (playerturn == 1) { //Grner Spieler
      pixel_set_safe(posx+wallx+1,wally,0x30);
    } else {
      pixel_set_safe(posx+wallx+1,wally,0x03); //Roter Spieler
    }
    waitms(550-abs(userin.x*3));
  }
  if (oposy != posy) { //Y-Positions�derung, neu zeichnen
    draw_line(wallx,0,0,16,0x00,0); //Obere Zeile leeren
    if (playerturn == 1) { //Grner Spieler
      pixel_set_safe(wallx,posy+wally+1,0x30);
    } else {
      pixel_set_safe(wallx,posy+wally+1,0x03); //Roter Spieler
    }
    waitms(550-abs(userin.y*3));
  }
  oposx = posx;
  oposy = posy;
}
return ((posy<<4)+posx);  //Cursor Position zurckliefern
}

static s16 rev_votegame(struct rev_spielfeldstruct *spielfeld, u08 playerturn) {
/* Jeder Punkt für den Spieler zählt +1, jeder für den Gegner -1
   Steine am Rand zählen +3 b.z. -3
   Steine in der Ecke zählen +20, b.z. -20
*/
u08 nunx, nuny, state, value;
s16 globalvote = 0;
u16 vote1 = 0, vote2 = 0; //Bewertung für Spieler 1 und Spieler 2
for (nuny = 1; nuny <= rev_y; nuny++) {
  for (nunx = 1; nunx < rev_x; nunx++) {
    if ((nunx == 1) || (nunx == rev_x) || (nuny == 1) || (nuny == rev_y)) {
      value = 3;
    } else
    if (((nunx == 1) || (nunx == rev_x)) && ((nuny == 1) || (nuny == rev_y))) {
       value = 10;
    } else
      value = 1;
    state = rev_field_get_quick(spielfeld,nunx,nuny) & 0x03;
    if (state == 1) {
      vote1 += value;
    }
    if (state == 2) {
      vote2 += value;
    }
  } //Ende x Schleife
} //Ende y Schleife
globalvote = vote1-vote2;
if (playerturn == 2) {
  globalvote *= (-1);
}
aivotedops++;
//Positiv: gut für den Spieler playerturn, negativ: schlecht für ihn
return globalvote;
}

static u08 rev_pos_count(struct rev_spielfeldstruct *spielfeld,
                         u08 playerturn) {
//Returns: Anzahl der Möglichkeiten
u08 nunx, nuny, index = 0;
for (nunx = 1; nunx <= rev_x; nunx++) {
  for (nuny = 1; nuny <= rev_y; nuny++) {
    if (rev_placestone_test(spielfeld, playerturn, nunx, nuny)) {
      index++;
    }
  }
}
return index;
}

static u16 rev_count_stones(struct rev_spielfeldstruct *spielfeld,
                            u08 playerturn) {
/* Jeder Punkt für den Spieler zählt +1, jeder für den Gegner -1
   Steine am Rand zählen +3 b.z. -3
   Steine in der Ecke zählen +20, b.z. -20
*/
u08 nunx, nuny, state;
u16 globalvote = 0;
for (nuny = 1; nuny <= rev_y; nuny++) {
  for (nunx = 1; nunx < rev_x; nunx++) {
    state = rev_field_get_quick(spielfeld,nunx,nuny) & 0x03;
    if (state == playerturn) {
      globalvote++;
    }
  } //Ende x Schleife
} //Ende y Schleife
return globalvote;
}

static void rev_highlightwinner(struct rev_spielfeldstruct *spielfeld) {
u16 stones1, stones2;
u08 winner = 1;
u08 nunx, nuny, nun = 0, color;
u08 startx,starty;
stones1 = rev_count_stones(spielfeld, 1);
stones2 = rev_count_stones(spielfeld, 2);
startx = 8-rev_x/2;
starty = 8-rev_y/2;
if (stones2 > stones1) {
  winner = 2;
}
while (nun < 6) {
  if (winner == 1) {
    color = 0x31;
  } else
    color = 0x13;
  for (nunx = 1; nunx <= rev_x; nunx++) {
    for (nuny = 1; nuny <= rev_y; nuny++) {
      if (rev_field_get(spielfeld, nunx, nuny) == winner) {
        pixel_set_safe(startx+nunx-1,starty+nuny-1,color); //Gelb
      }
    }
  }
  waitms(500);
  rev_drawgame(spielfeld);
  nun++;
  if (stones1 == stones2) {
    winner = 3-winner;		//Unentschieden, beide blinken
  } else {
    waitms(500);
    nun++;
  }
}
}

static void rev_fastcopy(struct rev_spielfeldstruct *target,
                         struct rev_spielfeldstruct *source) {
u16 cp;
if (target != 0) {
  for (cp= 0; cp < rev_x*rev_y*sizeof(u08); cp++) {
    *(target->address+cp) = *(source->address+cp);
   }
}
}

static s16 rev_calcai(struct rev_spielfeldstruct *testfeld, u08 playerturn,
                      u08 oplayer, u08 remdepth) {
u08 nunx, nuny;
s16 points, bestpoints = -10000, worstpoints = 1000;
u16 stones1, stones2;
struct rev_spielfeldstruct subtestfeld;
subtestfeld.address = malloc(rev_x*rev_y*sizeof(u08));
if (subtestfeld.address > 0) { //Ohne freien RAM wird einfach direkt bewertet
  for (nunx = 1; nunx <= rev_x; nunx++) {
    for (nuny = 1; nuny <= rev_y; nuny++) {
      if (rev_placestone_test(testfeld, playerturn, nunx, nuny)) {
        //Kopiere Daten
        rev_fastcopy(&subtestfeld, testfeld);
        rev_placestone(&subtestfeld, playerturn, nunx, nuny);
        if (remdepth == 0) {
          points = rev_votegame(&subtestfeld, oplayer);
        } else
          points = rev_calcai(&subtestfeld, 3-playerturn, oplayer, remdepth-1);
        if (points > bestpoints)
          bestpoints = points;
        if (points < worstpoints)
          worstpoints = points;
       }
    }
  }
  free(subtestfeld.address);
}
if (bestpoints == -10000) { //Keine Änderung, wahscheinlich keine freie Position
  stones1 = rev_count_stones(testfeld, playerturn);
  stones2 = rev_count_stones(testfeld, 3-playerturn);
  if (stones1 == stones2) {
    bestpoints = 100;
    worstpoints = 100;
  } else
  if (stones1 > stones2) { //Aktueller Spieler hat gewonnen
    bestpoints = 500;
    worstpoints = 500;
  } else {
    bestpoints = -500;
    worstpoints = -500;
  }
}
if (playerturn == oplayer) {
  return bestpoints;
} else
  return worstpoints;
}

static u08 rev_calcai_head(struct rev_spielfeldstruct *spielfeld,
                           u08 playerturn) {
u08 nunx, nuny;
s16 points, bestpoints = -10000;
struct rev_spielfeldstruct testfeld;
u08 bestposlist[rev_x*rev_y];
u08 bestposidx = 0;
u08 bestpos;

bestposlist[0] = 0x11;
testfeld.address = malloc(rev_x*rev_y*sizeof(u08));
aivotedops = 0;
if (testfeld.address > 0) {
  //Teste alle Positionen und bestimme die Beste
  for (nunx = 1; nunx <= rev_x; nunx++) {
    for (nuny = 1; nuny <= rev_y; nuny++) {
      if (rev_placestone_test(spielfeld, playerturn, nunx, nuny)) {
        //Kopiere Daten
        rev_fastcopy(&testfeld, spielfeld);
        rev_placestone(&testfeld, playerturn, nunx, nuny);
        points = rev_calcai(&testfeld, 3-playerturn, playerturn, 2);
        if (points == bestpoints) {
          bestposlist[bestposidx] = (nuny<<4)+nunx;
          bestposidx++;
        }
        if (points > bestpoints) {
          bestpoints = points;
          bestposlist[0] = (nuny<<4)+nunx;
          bestposidx = 1;
        }
      showbin(14, aivotedops>>16, 0x03);
      showbin(15, aivotedops & 0xFFFF, 0x03);
      }
    }
  }
  free(testfeld.address);
} else {
  clear_screen();
  load_text(rev_memerror);
  scrolltext(4,0x03,0x00,100);
}
if (bestposidx > 0) {
  bestpos = bestposlist[rand() % bestposidx];
} else {
  bestpos = 0x11;
  #ifdef is_i386
    printf("rev_calcai_head: Error: No position found\n");
  #endif
}
//aivotedops entfernen
draw_line(0,14,16,0,0x00,0);
draw_line(0,15,16,0,0x00,0);
return bestpos;
}

static void rev_moveai(struct rev_spielfeldstruct *spielfeld, u08 playerturn) {
u08 bestpos;
//Ermittelt die Beste Lage
bestpos = rev_calcai_head(spielfeld, playerturn);
rev_placestone_visual(spielfeld, playerturn, bestpos%16, bestpos / 16);
}

void reversi_start(void) {
u08 players, playerturn;
u08 gameinprogress = 1;
struct rev_spielfeldstruct spielfeld;
u08 playeroldpos[2];  //Cursor für beide Spieler

players = rev_selectmode(&spielfeld);
init_random(); //Zufallsgenerator für KI
playeroldpos[0] = (rev_x/2)+((rev_y/2)<<4);
playeroldpos[1] = (rev_x/2)+((rev_y/2)<<4);
if (players) {
  rev_drawgame(&spielfeld);
  waitms(500);
  playerturn = 1+(rand() & 1); //Entweder Spieler 1 oder Spieler 2 beginnt
  while (gameinprogress) {
    if ((players == 1) && (playerturn == 2)) { //KI ist am Zug
      rev_moveai(&spielfeld, playerturn);
      waitms(700);
    } else {  //Benutzereingabe bearbeiten
      playeroldpos[playerturn-1] = rev_movehuman(&spielfeld, playerturn,
                                                 playeroldpos[playerturn-1]);
    }
    playerturn = 3-playerturn; //Anderer Spieler
    if (rev_pos_count(&spielfeld, playerturn) == 0) {
      playerturn = 3-playerturn; //Spieler überspringen
      if (rev_pos_count(&spielfeld, playerturn) == 0) { //Auch er kann nicht
        gameinprogress = 0;
      }
    }
  } //Ende Spiel l�ft
  rev_highlightwinner(&spielfeld); //Die gewonnenen aufblinken lassen
  free(spielfeld.address);
} else { //Out of RAM!
  clear_screen();
  load_text(rev_memerror);
  scrolltext(4,0x03,0x00,100);
}
userin_flush();
while (userin_press() == 0); //Warten auf Tastendruck
}

#endif
