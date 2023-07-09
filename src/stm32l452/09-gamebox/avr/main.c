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
  Zweck:    Spiele auf einem Grafikdisplay
  Software: AVR-GCC
  Hardware: LED Panel, ATMEGA32 oder ähnlichen mit 8MHZ
            Einzelne Module müssten auch auf einem ATMEGA16 laufen.
            "modul_pxxo" und "modul_psnake" könnten jedoch mangels RAM
            möglicherweise garnicht oder nur fehlerhaft laufen.
  Wer Fragen oder Anregungen zu dem Programm hat, kann an
             m.marwedel <AT> onlinehome DOT de mailen.
            Mehr über Elektronik und AVRs gibt es auf meiner Homepage:
             http://www.marwedels.de/malte/
  Code Größe:ATMEGA32: 27194 Byte (getestet, alle Module)
  Compiler Optionen: -Os -ffast-math -fweb -Winline
  Verwendete Software zum compilieren:
            avr-gcc  Version: 3.4.3
            avr-libc Version: 1.4.4

Pinbelegung des LED Panels:
Die Pinbelegung kann einfach in der Datei graphicint.h angepasst werden.
Die Standardbelegung (und nur mit der wurde das Programm auch getestet) ist:
PC0: Red
PC1: Green
PC2: Clock
PC3: Reset
PC4: Select
PC5: Bright

Belegung des Joysticks. Die Pinbelegung kann in userinput.h geändert werden.
Der Port muss jedoch der mit dem A/D Wandler sein!
Beim ATMEGA16 und ATMEGA32 also PORTA.

PA0: J1 Taste 2
PA1: J1 Y-Achse
PA2: J2 X-Achse beziehungsweise J1 3. Achse (z.B. Schubregelung)
PA3: J1 X-Achse
PA4: J1 Taste 1
*/

#include "main.h"


static void init_io_pins(void) {
//Initialisiert die Ausgänge
//Darf nicht nach init_led_display() ausgeführt werden
#ifdef DDRA
//PORTA wäre beim ATMEGA8 überhaupt nicht vorhanden
DDRA = 0x00; //Pullup für Taster + unbelegte Ports (bei Standard Belegung)
PORTA = 0xFF;
#endif
DDRB = 0x00; //unbenutzte Pins + ISP Interface
PORTB = 0xFF;
DDRC = 0x00; //Wird später fürs Display neu definiert (bei Standard Belegung)
PORTD = 0xFF;
DDRD = 0x00; //Unbenutzte Pins
PORTD = 0xFF;
}

void init_random(void) {
if (init_random_done == 0) { //Nur einmal initialisieren
  srand(TCNT0); //TCNT0 müsste irgendwo zwischen 130 und 254 sein
  init_random_done = 1;
}
}

int main(void) {
//Register Initialisieren
OSCCAL = osccaleradout;
__malloc_margin = 250; //250 Byte Reserve für den Stack, beim malloc() Aufruf!
#if modul_sram
maxheap = (u16)__malloc_heap_start;
#endif
init_io_pins();
init_led_display();
input_init();
sei();             //Interrupts aktiviert
resync_led_display();
input_select();
menu_start();
}
