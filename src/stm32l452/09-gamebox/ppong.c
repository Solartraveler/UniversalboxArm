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

#if modul_ppong

struct pong_ballstruct{
s08 speedx;
s08 speedy;
u08 posx;
u08 posy;
};

struct pong_shotstruct{
s08 movey;
u08 posx;
u08 posy;
};

static void pong_shotstart(struct pong_shotstruct *ball, u08 playpos,
                           u08 playerno) {
if (ball->movey == 0) {
  ball->posx = playpos;
  if (playerno == 1) {
    ball->posy = 14;
    ball->movey = -1;
  }
  if (playerno == 2) {
    ball->posy = 1;
    ball->movey = 1;
  }
  //Zeichenen des Schuss
  pixel_set_safe(ball->posx,ball->posy,pixel_get(ball->posx,ball->posy)|0x03);
}
}

static u08 pong_shotdown(struct pong_shotstruct *ball, u08 playpos,
                         u08 playerno) {
s08 tmpdiff;
if (ball->movey) { //Wenn sich der Ball noch bewegt
  if (((ball->posy == 0) && (playerno == 2)) || //Und an der richtigen Stelle
     ((ball->posy == 15) && (playerno == 1))) {
    tmpdiff = ball->posx - playpos; //Links ist negativ, rechts positiv
    if ((tmpdiff >= -1) && (tmpdiff <= 1)) { //Wenn getroffen
      return playerno;
    }
  }
}
return 0;
}

static void pong_moveshot(struct pong_shotstruct *ball,
                          struct pong_shotstruct *ball2){
//Entfernen des roten Punktes an bisheriger Position
if (ball->movey != 0) {
  pixel_set_safe(ball->posx,ball->posy,pixel_get(ball->posx,ball->posy)&0x30);
}
//Bewegen des Balls
if ((ball->posy == 0) || (ball->posy == 15)) { //Wenn am Ende, Schluss stoppen
  ball->movey = 0;
}
if (ball->movey != 0) { //Wenn er sich noch bewegt
  ball->posy += ball->movey; //Neue Position
  //An neuer Position roten Punkt zeichnen
  pixel_set_safe(ball->posx,ball->posy,pixel_get(ball->posx,ball->posy)|0x03);
}
//Gegenseitiges Vernichten beider Schüsse, wenn auf gleicher Position und noch
//eher in der Mitte des Spielfeldes (noch nicht direkt vor dem Spieler)
if ((ball->posx == ball2->posx) && (ball->posy == ball2->posy) &&
    (ball->movey != 0) && (ball2->movey != 0) &&
    (ball->posx > 1) && (ball->posx < 14)) {
  ball->movey = 0;
  ball2->movey = 0;
  pixel_set_safe(ball->posx,ball->posy,pixel_get(ball->posx,ball->posy)&0x30);
}
}

static u08 pong_ai_precalcball(struct pong_ballstruct *ball) {
//Gibt die Position aus, an der der Ball den nächsten Spieler trifft
struct pong_ballstruct tb; //Temp ball
tb.posx = ball->posx;
tb.posy = ball->posy;
tb.speedx = ball->speedx;
tb.speedy = ball->speedy;
if ((tb.speedy == 1) || (tb.speedy == -1)) { //Verhindern einer Endlosschleife
  while (1) {
    if (((tb.posy == 14) && (tb.speedy == 1)) ||
        ((tb.posy == 1) && (tb.speedy == -1))) {
      break;
    }
    tb.posx += tb.speedx;
    tb.posy += tb.speedy;
    if (tb.posx == 0) {
      tb.speedx = 1;
    }
    if (tb.posx == 15) {
      tb.speedx = -1;
    }
  }
}
return tb.posx;
};

static s08 pong_ai_analyze(struct pong_ballstruct *ball,
             struct pong_shotstruct *shot1, struct pong_shotstruct *shot2,
             u08 analyzepos, u08 player2pos) {
s08 bewertung = 0;
s08 tdiff, tdiff2;
/*Kommt Ball an dieser Stelle auf: +20
  Kommt Ball daneben auf: +15
  Kommt Ball daneben gerade auf: +7
  Wird Ball abgelenkt auf gerade: +10
  Wird Ball abgelengt auf schiefe Bahn: + 25
  Kommt Schuss an dieser Stelle auf: -8
  Kommt Schuss auf eine Kante auf: -5
  Ist Schuss an dieser Stelle näher als 3: -50
  Ist Schuss daneben näher als 2: -30
  Stelle ist nächer in der Mitte als vorher: +1
  Stelle ist weiter weg von der Mitte als Vorher: -1
  Wenn zu analysierende Position außerhalb der Gültigkeit: -100
*/
if (ball->speedy == -1) { //Ball nähert sich
  tdiff = pong_ai_precalcball(ball) - analyzepos;
  if (tdiff == 0) { //Ball an dieser Stelle
    bewertung += 20;
  }
  if ((tdiff == -1) || (tdiff == 1)) { //Ball an Stelle daneben
    bewertung += 15;
    if (ball->speedx == 0) { //Ball an Stelle daneben gerade
      bewertung += 7;
    }
  }
  if (((ball->speedx == -1) && (tdiff == 2)) || //Ball auf Gerade
      ((ball->speedx == 1) && (tdiff == -2))) {
    bewertung += 10;
  }
  if (((tdiff == -2) || (tdiff == 2)) && (ball->speedx == 0)) { //Ball schief
    bewertung += 25;
  }
}
if (shot1->movey != 0) { //Wenn sich feindlicher Schuss bewegt
  tdiff = shot1->posx - analyzepos;
  if (tdiff == 0) {
    bewertung -= 8;
  }
  if ((tdiff == -1) || (tdiff == 1)) {
    bewertung -= 5;
  }
  if ((tdiff == 0) && (shot1->posy < 3)) {
    bewertung -= 30;
  }
  if ((shot1->posy < 2) && ((tdiff == 1) || (tdiff == -1))) {
    bewertung -= 20;
  }
}
tdiff = abs(8-player2pos); //Je näher an 0, desto besser
tdiff2 = abs(8-analyzepos);
if (tdiff > tdiff2) {
  bewertung++;
}
if (tdiff < tdiff2) {
  bewertung--;
}
if ((analyzepos < 1) || (analyzepos > 14)) {  //Out of range
  bewertung = -100;
}
return bewertung;
}

static u08 pong_ai(struct pong_ballstruct *ball, struct pong_shotstruct *shot1,
          struct pong_shotstruct *shot2, u08 play1pos, u08 play2pos, u08 mode) {
u08 poswish = 8, ballhitposx;
s08 votes[4], bestvote;

if (mode < 3) {
  if (ball->speedx != 0) {
    poswish = ball->posx;
  } else { //Ball fällt gerade, dass hat gefälligst aufzuhören
    if (ball->posx < 8) { //Ob nach links oder rechts davon stellen
      poswish = ball->posx+2;
    } else {
      poswish = ball->posx-2;
    }
  }
  if (mode == 2) { //Gelegendlich einen Schuss abgeben
    if ((shot2->movey == 0) && ((rand() & 0x07) == 0x07)) { // 1/8 wahscheinlich
      pong_shotstart(shot2, play2pos,2);
    }
  }
}
if (mode > 2) { //Bessere KI
/* Wie mode = 2, jedoch versuche Schüssen auszuweichen.
   Versuche zur vorberechneten Position zu bewegen. Vermeide aber Felder mit
   einer Bewertung < 10. Ansonsten das höchstbewertetet Feld.
*/
  votes[0] = pong_ai_analyze(ball, shot1, shot2, play2pos, play2pos);
  votes[1] = pong_ai_analyze(ball, shot1, shot2, play2pos+1, play2pos);
  votes[2] = pong_ai_analyze(ball, shot1, shot2, play2pos-1, play2pos);
  ballhitposx = max(min(pong_ai_precalcball(ball),14),1);
  votes[3] = pong_ai_analyze(ball, shot1, shot2, ballhitposx, play2pos);
  //Ermitteln der besten Position
  poswish = play2pos;
  bestvote = votes[0];
  if (votes[1] > votes[0]) {
    poswish = play2pos+1;
    bestvote = votes[1];
  }
  if (votes[2] > bestvote) {
    poswish = play2pos-1;
    bestvote = votes[2];
  }
  if (ball->speedy < 0) { //Ball nähert sich
    if (votes[3] > bestvote) {
      //Ermittle ob position links oder rechts von aktueller Position
      if (((ballhitposx < play2pos) && (votes[2] > -11)) || //Links davon
          ((ballhitposx > play2pos) && (votes[1] > -11)) || //Rechts davon
          (mode == 3)) {
        poswish = ballhitposx;
      }
    } //Ende Ball hit pos besser
  } //Ende Ball nähert sich
  if (shot2->movey == 0) { //Wenn Schuss nicht bereits unterwegs
    if (ball->speedy > 0) { //Wenn sich Ball entfernt
      if ((rand() & 0x07) == 0x07) { // 1/8 wahscheinl.
        pong_shotstart(shot2, play2pos, 2);
      }
    }//Ende Ball entfernt sich
    //Auf gleicher Höhe mit feindl. Schuss
    if ((play2pos == shot1->posx) && (mode == 4)) {
      pong_shotstart(shot2, play2pos, 2); //Feindl. Schuss abschießen
    }
  } //Ende Schuss möglich
} //Ende mode > 2
//Bewegen
if (play2pos < poswish) {
  play2pos++;
}
if (play2pos > poswish) {
  play2pos--;
}
//Überläufe verhindern
play2pos = max(play2pos,1);
play2pos = min(play2pos,14);
return play2pos;
}

static void pong_moveball(struct pong_ballstruct *ball, u08 play1pos,
                          u08 play2pos) {
s08 tmpdiff;
pixel_set_safe(ball->posx,ball->posy,0x00); //Alten Ball löschen
ball->posx += ball->speedx;
if (ball->posx == 0) { //Linke Wand
  ball->speedx = 1;
}
if (ball->posx == 15) { //Rechte Wand
  ball->speedx = -1;
}
ball->posy += ball->speedy;
if (ball->posy == 1) { //Obere Wand
  tmpdiff = ball->posx - play2pos; //Links ist negativ, rechts positiv
  if ((tmpdiff >= -1) && (tmpdiff <= 1)) {
    ball->speedy = 1;
  }
  if (tmpdiff == -2) {
    if (ball->speedx == 1) { //Stoppt horizontale Bewegung
      ball->speedy = 1;
      ball->speedx = 0;
    } else
    if (ball->speedx == 0) { //Startet horizontale Bewegung
      ball->speedy = 1;
      ball->speedx = -1;
    }
  }
  if (tmpdiff == 2) {
    if (ball->speedx == -1) { //Stoppt horizontale Bewegung
      ball->speedy = 1;
      ball->speedx = 0;
    } else
    if (ball->speedx == 0) { //Startet horizontale Bewegung
      ball->speedy = 1;
      ball->speedx = 1;
    }
  }
}
if (ball->posy == 14) { //Untere Wand
  tmpdiff = ball->posx - play1pos; //Links ist negaktiv, rechts positiv
  if ((tmpdiff >= -1) && (tmpdiff <= 1)) {
    ball->speedy = -1;
  }
  if (tmpdiff == -2) {
    if (ball->speedx == 1) { //Stoppt horizontale Bewegung
      ball->speedy = -1;
      ball->speedx = 0;
    } else
    if (ball->speedx == 0) { //Startet horizontale Bewegung
      ball->speedy = -1;
      ball->speedx = -1;
    }
  }
  if (tmpdiff == 2) {
    if (ball->speedx == -1) { //Stoppt horizontale Bewegung
      ball->speedy = -1;
      ball->speedx = 0;
    } else
    if (ball->speedx == 0) { //Startet horizontale Bewegung
      ball->speedy = -1;
      ball->speedx = 1;
    }
  }
}
pixel_set_safe(ball->posx,ball->posy,0x30); //Neuen Ball zeichnen
}

const char pong_won[] PROGMEM = "Win ";
const char pong_lost[] PROGMEM = "Lost";
const char pong_both[] PROGMEM = "Tie ";

static void pong_endabs(u08 gameend) {
u08 color = 0x32;
//Spiel wird mit Ausgaben gewonnen/verloren/unentschieden beendet
load_text(pong_both);
if (gameend == 1) {
  load_text(pong_lost);
  color = 0x03;
}
if (gameend == 2) {
  load_text(pong_won);
  color = 0x30;
}
scrolltext(4,color,0,100);
}

static void pong_drawplayers(u08 play1pos, u08 play1pos_o,
                      u08 play2pos, u08 play2pos_o) {
//Zeichen der Spielerposition
if (play1pos != play1pos_o) { //Player1 ist unten, menschlicher Spieler
  draw_line(play1pos_o-1,15,3,0,0,0);  //Löschen an alter Position
  draw_line(play1pos-1,15,3,0,0x32,0); //Zeichnen an neuer Position
}
if (play2pos != play2pos_o) { //Player2 ist oben, AI Spieler
  draw_line(play2pos_o-1,0,3,0,0,0);  //Löschen an alter Position
  draw_line(play2pos-1,0,3,0,0x32,0); //Zeichnen an neuer Position
}
}

static void pong_drawselectmenu(u08 mode) {
clear_screen();
if (mode == 1) {
  draw_box(0,0,8,8,0x30,0x00);
}
if (mode == 2) {
  draw_box(8,0,8,8,0x30,0x00);
}
if (mode == 3) {
  draw_box(0,8,8,8,0x30,0x00);
}
if (mode == 4) {
  draw_box(8,8,8,8,0x30,0x00);
}
draw_char('1', 1, 1, 0x33,1,0);
draw_char('2', 9, 1, 0x23,1,0);
draw_char('3', 1, 9, 0x13,1,0);
draw_char('4', 9, 9, 0x03,1,0);
}

static u08 pong_selectmode(void) {
u08 mode = 1;
/*1. Modi: Einfaches hin und her spielen
  2. Modi: Möglichkeit zu schießen
  3. Modi: wie 2.Modi jedoch besserer Gegner
  4. Modi: wie 2.Modi jedoch optimaler Gegner
*/
pong_drawselectmenu(mode);
while (userin_press() == 0)  { //Warte auf Tastendruck
  if (userin_left() && ((mode & 1) == 0)) {
     mode--;
     pong_drawselectmenu(mode);
   }
  if (userin_right() && ((mode & 1) == 1)) {
     mode++;
     pong_drawselectmenu(mode);
   }
   if (userin_up() && (mode > 2)) {
     mode -= 2;
     pong_drawselectmenu(mode);
   }
   if (userin_down() && (mode < 3)) {
     mode += 2;
     pong_drawselectmenu(mode);
   }
}
while (userin_press()); //Warte auf loslassen der Taste
waitms(200);
clear_screen();
userin_flush();
return mode;
}

#define pong_virtual_timers 5

void pong_start(void) {
u08 mode, gameend = 0;
u08 play1pos = 8, play1pos_o = 0;
u08 play2pos = 8, play2pos_o = 0;
u08 timings[pong_virtual_timers] = {0,0,0,0,0};
u08 nun;
u16 points = 0;
struct pong_ballstruct ball = {1,1,0,7};
struct pong_shotstruct shot1 = {0,0,0}, shot2 = {0,0,0};
//Menu zur Auswahl des Spielmodis
mode = pong_selectmode();
//Zufallsgenerator initialisieren
init_random();
//Timer1 wird für das Timing verwendet, 31,25KHZ Takt
TCNT1 = 0; //Reset Timer
TCCR1B = (1<<CS12); //Prescaler: 256
while (gameend == 0) {
  //Setzen der Spieler 1 Position
  play1pos = 7 + userin.x/18;
  play1pos = min(play1pos,14);
  play1pos = max(play1pos,1);
  //Timings erzeugen
  if (TCNT1 > (F_CPU/51282)) { //Wenn 5ms vergangen sind; (8MHZ/51282)=156
    TCNT1 = 0;
    for (nun = 0; nun < pong_virtual_timers; nun++) {
      if (timings[nun] != 0xff) {
        timings[nun]++;
      }
    }
  }
  //Ball Position berechnen
  if (timings[0] >= 30) { //Also nach 150ms
    timings[0] = 0;
    pong_moveball(&ball, play1pos, play2pos);
  }
  //AI ausführen
  if (timings[1] >= (35 - mode*5)) { //Je höher der mode desto schneller
    timings[1] = 0;
    play2pos = pong_ai(&ball, &shot1, &shot2, play1pos, play2pos, mode);
  }
  //Erster Schuss
  if (timings[2] >= 60) { //Also nach 300ms
    timings[2] = 0;
    pong_moveshot(&shot1,&shot2);
  }
  //Zweiter Schuss
  if (timings[3] >= 60) { //Also nach 300ms
    timings[3] = 0;
    pong_moveshot(&shot2,&shot1);
  }
  //Punkte zählen
  if (timings[4] >= 200) {//Jede Sekunde ein Punkt
    timings[4] = 0;
    points++;
  }
  //Schuss starten
  if ((mode > 1) && userin_press()) {
    pong_shotstart(&shot1,play1pos,1);
    timings[2] = 0;
  }
  //Ball herausgesprungen überprüfen
  if (ball.posy == 0) { gameend = 2; }  //Player 2 hat verloren
  if (ball.posy == 15) { gameend = 1; } //Player 1 hat verloren
  //Schuss abbekommen überprüfen
  gameend |= pong_shotdown(&shot2,play1pos,1);
  gameend |= pong_shotdown(&shot1,play2pos,2);
  //Game Abbruch
  pong_drawplayers(play1pos, play1pos_o, play2pos, play2pos_o);
  play1pos_o = play1pos;
  play2pos_o = play2pos;
}
TCCR1B = 0; //Stopp Timer1
waitms(1000);
draw_box(0,1,16,14,0x00,0x00);
userin_flush();
if (mode == 1) {
  draw_gamepoints(points, PONG_ID);
} else {
  pong_endabs(gameend);
  while(userin_press() == 0); //Warte auf Tastendruck
}
}

#endif
