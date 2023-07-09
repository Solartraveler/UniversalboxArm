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

/* Datei Beschreibung
  Titel:    Gamebox mit SAMSUNG SLM1608 Display
  Autor:    (c) 2004-2006 by Malte Marwedel
  Datum:    2006-10-19
  Version:  1.00 (Final 1)
  Zweck:    Spiele auf einem Grafikdisplay - Demonstration auf dem PC.
  Software: GCC
  Hardware: PC mit mind 500MHZ und 800x600 @ 256 Farben 3D beschleunigte Grafik
            Für die Mikrocontroller Version siehe ../avr
  Wer Fragen oder Anregungen zu dem Programm hat, kann an
             m.marwedel <AT> onlinehome DOT de mailen.
            Mehr über Elektronik und AVRs gibt es auf meiner Homepage:
             http://www.marwedels.de/malte/
  Code Größe:ATMEGA32: 24524 Byte (getestet, alle Module)
  Compiler Optionen: -Os -ffast-math -fweb -Winline
  Verwendete Software zum compilieren:
            gcc  Version: 4.0.2 / 3.4.5 (epfohlen) / 3.3.6
            GLUT

Steuerung des Spiels:
Leertaste oder linke Maustaste: Auswählen
Cursor oder Maus: Nach links, rechts, oben, unten bewegen
Maus: Analoge Eingabe wie mit einem Joystic möglich

*/

#include "main.h"
#include <stdio.h>

pthread_t avr_thread_id;
pthread_t avr_timer_id;


void sei(void) {

}

void cli(void) {

}

u08 pgm_read_byte(const u08 *data) {
return *data;
}

u16 pgm_read_word(const u16 *data) {
return *data;
}

u08 eeprom_read_byte(const u08 *addr) {
return *addr;
}

u16 eeprom_read_word(const u16 *addr) {
return *addr;
}

void eeprom_write_byte(u08 *addr, u08 value) {
*addr = value;
}

void eeprom_write_word(u16 *addr, u16 value) {
*addr = value;
}

void init_random(void) {
if (init_random_done == 0) { //Nur einmal initialisieren
  srand(get_time10k()); //Auf dem AVR wird TCNT0 als Seed verwendet
  init_random_done = 1;
}
}

static void *avr_thread(void * arg) {
printf("Info: avr_thread is running\n");
menu_start();
printf("Warning: avr_thread stopped\n");
return(NULL);
}

int main(int argc, char **argv) {
//Ein paar Meldungen
printf("Gamebox Version 'Final 1.02' (c) 2004-2013 by Malte Marwedel\n\n");
printf("This program is free software; you can redistribute it and/or modify\n");
printf("it under the terms of the GNU General Public License as published by\n");
printf("the Free Software Foundation; either version 2 of the License, or\n");
printf("(at your option) any later version.\n\n");
printf("This program is distributed in the hope that it will be useful,\n");
printf("but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
printf("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n");
printf("GNU General Public License for more details.\n\n");
printf("You should have received a copy of the GNU General Public License\n");
printf("along with this program; if not, write to the Free Software\n");
printf("Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA\n\n");
printf("The program was originally written for an 8 Bit microcontroller (AVR) \n");
printf("but was ported to a PC later with a minimum of source modifications.\n");
printf("A PC is much faster but threads do not switch often so timing was \n");
printf("and is a big problem.\n");
//GLUT Init
glutInit(&argc, argv);
init_window();
glutPassiveMotionFunc(input_mouse_move); //Mausbewegung
glutMouseFunc(input_mouse_key);          //Mausklick
glutSpecialFunc(input_key_cursor);       //Pfeil Tasten
glutKeyboardFunc(input_key_key);         //Space Taste
//glutTimerFunc(5,timer1_sim, 0);          //Simuliert Timer1
if (pthread_create(&avr_timer_id, NULL, timer1_sim, (void *)(0))) {
  printf("Error: Creating avr_timer_thread failed\n");
}
//Die primäre Endlosschleife des AVRs läuft auf dem PC als extra Thread
if (pthread_create(&avr_thread_id, NULL, avr_thread, (void *)(0))) {
  printf("Error: Creating avr_thread failed\n");
}
glutMainLoop();
printf("Exiting\n");
}
