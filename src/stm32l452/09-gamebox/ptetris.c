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

/*
Spielfeld 12x16 Felder
Levelanstieg alle 10 Zeilen, 10 Level
Farben für Steine zufällig außer 0x00
Links Statusanzeige, rote Begrenzungslinien
Objektspeicher 4x4 Felder, wird bei jeder Bewegung mit Kollision im
Bildspeicher überprüft.
Bei Drehen wird ebenfalls auf Kollisionen mit dem Bildspeicher überprüft
und gegebenenfalls abgebrochen.
Block Typen:
0:    1:    2:    3:    4:    5:    6:
.#..  .#..  ..#.  .##.  .#..  ....  .#..
.#..  .##.  .##.  .#..  .#..  .##.  .##.
.#..  ..#.  .#..  .#..  .##.  .##.  .#..
.#..  ....  ....  ....  ....  ....  ....

*/

#include "main.h"

#if modul_ptetris

struct tetris_blockstruct{
u08 block[4][4];
u08 posx;
u08 posy;
u08 color;
u08 nexttype;
};

const u16 tetris_compact_block_data[] PROGMEM = {
0x4444, 0x4620, 0x2640, 0x6440, 0x4460, 0x0660, 0x4640 };

static void tetris_newblock(struct tetris_blockstruct *theblock) {
u08 nunx, nuny;
u16 compactblock;
u08 color;
//Nächsten Stein aus Flash lesen
compactblock = pgm_read_word(tetris_compact_block_data+theblock->nexttype);
//Altes Array leeren und neue Werte hereinsetzen
for (nuny = 0; nuny < 4; nuny++) {
  for (nunx = 0; nunx < 4; nunx++) {
    theblock->block[nunx][nuny] = 0;
    if (compactblock & 0x8000) { //Bit setzen
      theblock->block[nunx][nuny] = 1;
    }
    compactblock = compactblock << 1;
  }
}
//Neue Koordinaten setzen
theblock->posx = 8;
theblock->posy = 0;
//Neue Farbe ermitteln
theblock->color = ((rand() % 4)) + (((rand() % 3) +1)<<4);
//Nächsten Block ermitteln
theblock->nexttype = rand() % 7;
//Vorschau Block laden
compactblock = pgm_read_word(tetris_compact_block_data+theblock->nexttype);
//Vorschau Block zeichnen
for (nuny = 0; nuny < 4; nuny++) {
  compactblock = compactblock << 1;
  for (nunx = 1; nunx < 3; nunx++) {
    if (compactblock & 0x8000) { //Bit setzen
      color = 0x30;
    } else {
      color = 0;
    }
    pixel_set_safe(nunx-1,nuny,color);
    compactblock = compactblock << 1;
  }
  compactblock = compactblock << 1;
}
}

static u08 tetris_checkcollide(struct tetris_blockstruct *oldblock,
                 struct tetris_blockstruct *newblock) {
u08 nunx,nuny;
u08 collide = 0;
//Testen ob es an der neuen Position zu einer Kollision kommen würde
for (nuny = newblock->posy; nuny < newblock->posy+4; nuny++) {
  for (nunx = newblock->posx; nunx < newblock->posx+4; nunx++) {
    //Wenn an der Stelle ein Punkt im neuem Block
    if (newblock->block[nunx-newblock->posx][nuny-newblock->posy]) {
      if ((nunx < 3) || (nunx > 14) || (nuny > 15)) { //Spielfeld Grenze
        collide = 1;
      }
      //Bildspeicher und neuer Block kollidieren
      if (pixel_get(nunx,nuny) != 0) {
        //Testen ob es überhaubt einen altern Block gibt
        if (oldblock != NULL) {
          //Position ist jedoch innerhalb des alten Blockes
          if ((nunx >= oldblock->posx) && (nunx < oldblock->posx+4) &&
              (nuny >= oldblock->posy) && (nuny < oldblock->posy+4)) {
            //Testen ob der Bildpunkt einfach nur von dem alten Block herrührt
         if ((oldblock->block[nunx-oldblock->posx][nuny-oldblock->posy]) == 0) {
            //Ne- ist nicht der Fall, also doch Kollision
              collide = 1;
            }
          } else { //Nicht innerhalb des alten Blocks
            collide = 1;
          }
        } else {
          collide = 1;
        }//Ende oldblock nicht existent
      } //Ende kollision mit Bildspeicher
    } //Ende Bildspeicher und neuer Block kollidieren
  } //Ende nunx Durchlauf
} //Ende nuny Durchlauf
return collide;
}

static u08 tetris_moveblock(struct tetris_blockstruct *theblock, s08 movex,
                            s08 movey){
struct tetris_blockstruct testblock;
u08 nunx,nuny, color;
u08 collide;
//Kopiere block, color und nexttype werden nicht kopiert, da unnötig
testblock.posx = theblock->posx + movex;
testblock.posy = theblock->posy + movey;
for (nuny = 0; nuny < 4; nuny++) {
  for (nunx = 0; nunx < 4; nunx++) {
    testblock.block[nunx][nuny] = theblock->block[nunx][nuny];
  }
}
if ((movex) || (movey)) { //Bewegt sich
  collide = (tetris_checkcollide(theblock,&testblock));
} else {
  /* Es ist egal on theblock oder testblock als zweiter Parameter übergeben
     wird, da beide bei keiner Bewegung den gleichen Inhalt haben. */
  collide = (tetris_checkcollide(NULL,theblock));
}
if (collide == 0) { //Kollidieren nicht
  for (nuny = 0; nuny < 16; nuny++) {
    for (nunx = 3; nunx < 15; nunx++) {
      color = 0xff; //0xff bedeutet keine Änderung
      //Wenn innerhalb von theblock, dann löschen
      if ((nunx >= theblock->posx) && (nunx < theblock->posx+4) &&
          (nuny >= theblock->posy) && (nuny < theblock->posy+4)) {
        if (theblock->block[nunx-theblock->posx][nuny-theblock->posy]) {
          color = 0;
        }
      }
      //Wenn innerhalb von testblock, dann setzen
      if ((nunx >= testblock.posx) && (nunx < testblock.posx+4) &&
          (nuny >= testblock.posy) && (nuny < testblock.posy+4)) {
        if (testblock.block[nunx-testblock.posx][nuny-testblock.posy]) {
          color = theblock->color;
        }
      }
      if (color != 0xff) {
        pixel_set_safe(nunx,nuny,color);
      }
    }
  }
  //theblock bekommt die gleiche Position wie der testblock
  theblock->posx += movex;
  theblock->posy += movey;
  return 1;
}
//Kollidieren
return 0; //Bewegen nicht möglich
}

static void tetris_removeline(u08 liney) {
u08 nunx,nuny;
for (nunx = 3; nunx < 15; nunx++) {
  for (nuny = liney; nuny > 0; nuny--) {
    pixel_set_safe(nunx,nuny,pixel_get(nunx,nuny-1));
  }
}
draw_line(3,0,12,0,0x00,0);
}

static u08 tetris_checkline(void) {
u08 fulllines = 0;
u08 linefull = 0;
u08 nunx,nuny;

nuny = 0;
while (nuny < 16) {
  linefull = 1;
  for (nunx = 3; nunx < 15; nunx++) {
    //Überprüfe eine Zeile
    if (pixel_get(nunx,nuny) == 0) {
      linefull = 0;
    }
  }
  if (linefull) {
    fulllines++;
    tetris_removeline(nuny);
  } else {
    nuny++;
  }
}
return fulllines;
}

static void tetris_rotateblock(struct tetris_blockstruct *theblock) {
struct tetris_blockstruct testblock;
u08 nunx,nuny;
u08 color;
//Drehe in Testblock -gegen den Uhrzeigersinn
for (nuny = 0; nuny < 4; nuny++) {
  for (nunx = 0; nunx < 4; nunx++) {
    testblock.block[nuny][3-nunx] = theblock->block[nunx][nuny];
  }
}
//Übertrage Position
testblock.posx = theblock->posx;
testblock.posy = theblock->posy;
//Auf Kollision testen
if (tetris_checkcollide(theblock,&testblock) == 0) { //Kollidiert nicht
  //Zeichnen des neuen Blocks
  for (nuny = 0; nuny < 4; nuny++) {
    for (nunx = 0; nunx < 4; nunx++) {
      color = 0xff; //0xff bedeutet keine Änderung
      //Wenn innerhalb von theblock, dann löschen
      if (theblock->block[nunx][nuny]) {
        color = 0;
      }
      //Wenn innerhalb von testblock, dann setzen
      if (testblock.block[nunx][nuny]) {
        color = theblock->color;
      }
      if (color != 0xff) { //wenn Änderung
        pixel_set_safe(theblock->posx+nunx,theblock->posy+nuny,color);
      }
    }
  }
  //Kopiere gedrehten Block
  for (nuny = 0; nuny < 4; nuny++) {
    for (nunx = 0; nunx < 4; nunx++) {
      theblock->block[nunx][nuny] = testblock.block[nunx][nuny];
    }
  }

}
}

#define tetris_virtual_timers 2

void tetris_start(void) {
u08 linesdone = 0;
u08 linesremoved;
u08 level = 1;
u16 points = 0;
u08 life = 1;
u08 timings[tetris_virtual_timers] = {0,0};
u08 tempabs;
struct tetris_blockstruct theblock;
clear_screen(); //Nur zur Vorsicht
//Zeichnen der Begrenzungslinien
draw_line(2,0,0,16,0x03,0);
draw_line(15,0,0,16,0x03,0);
//Initialisieren des Zufallsgenerators
init_random();
//Setzen des ersten Steines und der Vorschau
theblock.nexttype = rand() % 7;
tetris_newblock(&theblock);
tetris_moveblock(&theblock,0,0);
//Timer1 wird für das Timing verwendet, 31,25KHZ Takt
TCNT1 = 0; //Reset Timer
TCCR1B = (1<<CS12); //Prescaler: 256
userin_flush();
while (life) {
  if (userin_press())  {
    tetris_rotateblock(&theblock);
  }
  if (TCNT1 > (F_CPU/25641)) { //100 Durchläufe pro Sekunde; 8MHZ/25641 = 312
    TCNT1 = 0;
    //Links-Rechts schieben
    if (timings[1] != 0xff) { //Links-Rechts schieben Counter
      timings[1]++;
    }
    tempabs = abs(userin.x);
    if (timings[1] > (52-tempabs/3)) { //Links-Rechts schieben Eingabe
      if (userin.x < -40) {
        tetris_moveblock(&theblock,-1,0);
        timings[1] = 0;
      }
      if (userin.x > 40) {
        tetris_moveblock(&theblock,1,0);
        timings[1] = 0;
      }
    }
    //Block nach unten bewegen
    timings[0]++;
    tempabs = 100-(level-1)*10;
    if (userin.y > 40) { //Der Benutzer möchte den Block schneller platzieren
      tempabs = min(tempabs,(42-userin.y/4));
    }
    if (timings[0] >= tempabs) {
      timings[0] = 0;
      if (tetris_moveblock(&theblock,0,1) == 0) { //Wenn bewegen nicht möglich
        //Entferne volle Zeilen
        linesremoved = tetris_checkline();
        //Addiere Pumkte basierend auf der Anzahl der entfernten Zeilen
        if (linesremoved == 1) {
          points += 1;
        }
        if (linesremoved == 2) {
          points += 3;
        }
        if (linesremoved == 3) {
          points += 5;
        }
        if (linesremoved == 4) {
          points += 8;
        }
        //Bei einem wirklinch *gutem* Spieler Überlauf verhindern
        if (points > 64000) {
          points = 64000;
        }
        linesdone += linesremoved; //Addiere komplette Zeilen
        if ((level < 10) && (linesdone >= 10)) {
          level++;
          linesdone -= 10;
        }
        tetris_newblock(&theblock);
        if (tetris_moveblock(&theblock,0,0) == 0) {//Neuer Block nicht möglich
          life = 0;
        }
      }
    }//Ende Block nach unten
  }
  //Zeichnen der Level
  draw_line(0,15,0,-level,0x02,0); //Mittelroter Balken
  draw_line(0,6,0,10-level,0x00,0); //Dunkler Balken
  //Zeichnen der Zeilen
  draw_line(1,15,0,-(linesdone%10),0x31,0); //Gelber Balken
  draw_line(1,6,0,10-(linesdone%10),0x00,0); //Dunkler Balken
}
TCCR1B = 0; //Stopp Timer1
waitms(1000);
draw_gamepoints(points, TETRIS_ID);
}

#endif
