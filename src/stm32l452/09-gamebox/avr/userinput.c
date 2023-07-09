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

/* Wie die Formeln für die Berechnungen des Joysticks zustandekommen
  kann in widerstand-berechnung-joystick.txt nachgelesen werden.
*/


#include "main.h"


struct userinputstruct userin;
//Reine A/D Werte: 484, 640, 1022
//Die min Werte müssen mit (-1) mal-genommen werden (also nur positive Werte)
struct userinputcalibstruct calib_x = {71, 192, 61}; //Default Werte
struct userinputcalibstruct calib_y = {71, 192, 61}; //Default Werte

u08 volatile userinputtype; //0 = None, 1 Joystick, 2 = Joystick ohne Achsen
u08 volatile snap_x;
u08 volatile snap_y;
u08 volatile snap_key;

u16 volatile presample_x = 0, presample_y = 0;
u08 volatile precount_x = 0, precount_y = 0;

u08 input_calib_ignoretext = 0;

#if modul_calib_save

/*Da die AVRs leider beim Starten EEPRom Zellen überschreiben und dies besonders
  oft die erste Zelle betrifft, wird diese reservier aber nicht verwendet:
*/
u08 dummy eeprom_data;

struct userinputcalibstruct calib_x_eep[2] eeprom_data;
struct userinputcalibstruct calib_y_eep[2] eeprom_data;

//Wenn die Kalibrierungen im EEProm gespeichert werden sollen

const char input_calib_load1[] PROGMEM = "EEPROM error, calibrate";

static void calib_load(void) {
struct userinputcalibstruct calib_load[2];
u08 nun, errorfree = 1;
for (nun = 0; nun < 2; nun++) { //Werte der X Achse
    eeprom_read_block(&calib_load[nun],&calib_x_eep[nun],
                       sizeof(struct userinputcalibstruct));
}
if (calib_load[0].min == calib_load[1].min) {
  calib_x.min = calib_load[0].min;
} else {
  errorfree = 0;
}
if (calib_load[0].zero == calib_load[1].zero) {
  calib_x.zero = calib_load[0].zero;
} else {
  errorfree = 0;
}
if (calib_load[0].max == calib_load[1].max) {
  calib_x.max = calib_load[0].max;
} else {
  errorfree = 0;
}
for (nun = 0; nun < 2; nun++) { //Werte der Y Achse
    eeprom_read_block(&calib_load[nun],&calib_y_eep[nun],
                       sizeof(struct userinputcalibstruct));
}
if (calib_load[0].min == calib_load[1].min) {
  calib_y.min = calib_load[0].min;
} else {
  errorfree = 0;
}
if (calib_load[0].zero == calib_load[1].zero) {
  calib_y.zero = calib_load[0].zero;
} else {
  errorfree = 0;
}
if (calib_load[0].max == calib_load[1].max) {
  calib_y.max = calib_load[0].max;
} else {
  errorfree = 0;
}
//Wenn fehlerhaft, oder 0xffff auf erstmaligen Start hinweist:
if ((errorfree == 0) || (calib_load[0].zero == 0xffff))  {
  clear_screen();
  load_text(input_calib_load1);
  scrolltext(0,0x03,0,120);
  waitms(300);
  input_calib();
}
}

const char input_calib_save1[] PROGMEM = "Save joystick calib in EEPROM?";
const char input_calib_no[] PROGMEM = "No";
const char input_calib_yes[] PROGMEM = "Yes";
const char input_calib_save2[] PROGMEM = "Values saved";

void calib_save(void) {
u08 accepted = 0;
u08 nun;
clear_screen();
load_text(input_calib_save1);
scrolltext(0,0x03,0,120);
load_text(input_calib_no);
draw_string(1,8,0x31,0,1);
while (userin_press() == 0) { //Warte auf Tastendruck
  if ((userin_right()) && (accepted == 0)) { //Ja
    accepted = 1;
    load_text(input_calib_yes);
    draw_box(0,8,16,8,0x00,0x00);
  }
  if (userin_left() || userin_up() || userin_down()) { //Nein
    accepted = 0;
    load_text(input_calib_no);
    draw_box(0,8,16,8,0x00,0x00);
  }
  draw_string(1,8,0x31,0,1);
}
if (accepted == 1) { //in EEProm speichern
  for (nun = 0; nun < 2; nun++) {
    eeprom_write_block(&calib_x,&calib_x_eep[nun],
                       sizeof(struct userinputcalibstruct));
    eeprom_write_block(&calib_y,&calib_y_eep[nun],
                       sizeof(struct userinputcalibstruct));
  }
  load_text(input_calib_save2);
  scrolltext(3,0x13,0,120);
  waitms(500);
  userin_flush();
}
}

#endif

const char input_calib_text1[] PROGMEM = "All sides move, press key in center";
const char input_calib_text2[] PROGMEM = "Cali:";

void input_calib(void) {
u16 min_x = 1024,min_y = 1024;
u16 max_x = 1, max_y = 1;
u16 medium_x,medium_y;
u16 temp;

if (userinputtype == 1) {
  if (input_calib_ignoretext == 0) {
    input_calib_ignoretext = 1;
    load_text(input_calib_text1);
    scrolltext(0,0x03,0,100);
    waitms(250);
  }
  load_text(input_calib_text2);
  draw_box(0,0,16,8,0x00,0x00); //Löschen des Textes
  draw_string(0,0,0x03,0,1);
  //Deakiviere Timer0 Interrupt
  TIMSK &= ~(1<<TOV0);
  //Taster sind Low-Aktiv; solange keine Taste gedrückt wird:
  while ((AD_PIN & (JOY_KEY1_PIN_MASK | JOY_KEY2_PIN_MASK)) ==
         (JOY_KEY1_PIN_MASK | JOY_KEY2_PIN_MASK)) {
    //Kalib von X
    ADMUX = JOY_XAXIS_PIN;   //Kanal wählen
    ADCSRA|= (1<<ADSC); //Starte Konvertierung
    while ((ADCSRA & (1<<ADSC)) != 0); //Warte auf Ende
    temp = ADC;
    showbin(9, temp, 0x30);
    if (temp < min_x) {
      min_x = temp;
    }
    if (temp > max_x) {
      max_x = temp;
    }
    //Kalib von Y
    ADMUX = JOY_YAXIS_PIN;   //Kanal wählen
    ADCSRA|= (1<<ADSC); //Starte Konvertierung
    while ((ADCSRA & (1<<ADSC)) != 0); //Warte auf Ende
    temp = ADC;
    showbin(13, temp, 0x30);
    if (temp < min_y) {
      min_y = temp;
    }
    if (temp > max_y) {
      max_y = temp;
    }
    //Anzeigen der bisherigen Werte in Binärzahlen
    showbin(8 , min_x, 0x03);
    showbin(10, max_x, 0x03);
    showbin(12, min_y, 0x03);
    showbin(14, max_y, 0x03);
  }
  //Mittelstellung Messen
  ADMUX = JOY_XAXIS_PIN;   //Kanal wählen (X)
  ADCSRA|= (1<<ADSC); //Starte Konvertierung
  while ((ADCSRA & (1<<ADSC)) != 0); //Warte auf Ende
  medium_x = ADC;
  ADMUX = JOY_YAXIS_PIN;   //Kanal wählen (Y)
  ADCSRA|= (1<<ADSC); //Starte Konvertierung
  while ((ADCSRA & (1<<ADSC)) != 0); //Warte auf Ende
  medium_y = ADC;
  //Da dividiert wird, darf kein Wert = 0 sein!
  //max Werte können nicht Null sein, brauchen nicht überprüft werden
  if (min_x == 0) {
    min_x = 1;
  }
  if (min_y == 0) {
    min_y = 1;
  }
  if (medium_x == 0) {
    medium_x = 1;
  }
  if (medium_y == 0) {
    medium_y = 1;
  }
  //Jetzt Kalibrierungswerte berechnen
  calib_x.zero = 122880/medium_x;
  calib_y.zero = 122880/medium_y;
  calib_x.max = (122880/max_x - calib_x.zero) *(-1); // *(-1) für positive Werte
  calib_y.max = (122880/max_y - calib_y.zero) *(-1); // *(-1) für positive Werte
  calib_x.min = 122880/min_x - calib_x.zero;
  calib_y.min = 122880/min_y - calib_y.zero;
  //Reaktiviere Timer0 Interrupt
  TIMSK |= (1<<TOV0);
}
}

const char input_select_text1[] PROGMEM = "Select input device ";
const char input_select_text1b[] PROGMEM = "Dev";
const char input_select_text2[] PROGMEM = "Joy";
const char input_select_text3[] PROGMEM = "Grav";

void input_select(void) {
/* Wenn Taste am Joystick gedrückt: joystick = 1;
   Wenn Beschleunigungssensoren.z > 64: joystick = 0;
   Sobald userin_right() -> gehe zurück zu main, welches ins Menü wechselt.
*/
#if modul_xmas
u16 xmas;
#endif
userinputtype = 0;
userin_flush();
load_text(input_select_text1);
scrolltext(0,0x03,0,110);
load_text(input_select_text1b);
draw_box(0,0,16,8,0x00,0x00); //Löschen des Textes
draw_string(0,0,0x03,0,1);
while (userin_right() == 0) {
  //Bei der Taster des Joysticks
  if ((AD_PIN & (JOY_KEY1_PIN_MASK | JOY_KEY2_PIN_MASK)) <
      (JOY_KEY1_PIN_MASK | JOY_KEY2_PIN_MASK)) {
    userinputtype = 1;
    load_text(input_select_text2);
    draw_box(0,8,16,7,0x00,0x00);
    draw_string(0,8,0x13,0,1);
    waitms(100);
  }
#if modul_xmas
  waitms(1);
  xmas++;
  if (xmas == 20000) {
    userinputtype = 2;
    xmas_start();
    xmas = 0;
  }
#endif
}
#if modul_calib_save
if (userinputtype == 1) {
  calib_load();
}
#endif
}

void input_init(void) {
/*Initialisieren der Pins.
  Möglicherweise schon teilweise durch init_io_pins() erfolgt
  Alle Port Pins werden auf Eingang gestellt. Für alle Pins, mit Außnahme der
  der Joystick Achsen, werden die Pull-up Widerstände aktiviert.*/
AD_DDR = 0x00; //Eingang
AD_PORT = ~((1<<JOY_XAXIS_PIN)|(1<<JOY_YAXIS_PIN)|(1<<JOY_ZAXIS_PIN));
//Timer0 soll jede ms aufgerufen werden also alle 8000 Takte bei 8MHZ
TCNT0 = 0;    //Timer reset
TCCR0 = 0x03; //Prescaler:64
TIMSK |= (1<<TOV0);    //Timer0 Overflow Interrupt enabled
//Initialisieren des A/D Wandlers
ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1); //AD Enabled, Prescaler 64
}

ISR(TIMER0_OVF_vect) {  //knapp 1000 Aufrufe pro Sekunde
s16 adwert;
s08 tinyadwert;

sei(); //Wir müssen den Graphic Interrupt zulassen!
if (userinputtype != 0) { //Joystick oder Taster
  if ((AD_PIN & JOY_KEY1_PIN_MASK) == 0) { //Taste 1 gedrückt
    if (snap_key == 0) {
      userin.press = 1;
      snap_key = 1;
    }
  } else {
    snap_key = 0;
  }
}
if (userinputtype == 1) { //Joystick
  //Abfragen der A/D Wandler 1 und 3
  if ((ADCSRA & (1<< ADSC)) == 0) { //Wandlung komplett
    if (ADMUX == JOY_XAXIS_PIN) { //Wandler 3 wurde gewandelt (X)
      presample_x += ADC;
      precount_x++;
      ADMUX = JOY_YAXIS_PIN; //Anderer Kanal
      ADCSRA |= (1<< ADSC);  //Neue Wandlung
    } else { //Ansonsten behandle als Channel 1 (Y)
      presample_y += ADC;
      precount_y++;
      ADMUX = JOY_XAXIS_PIN; //Anderer Kanal
      ADCSRA |= (1<< ADSC);  //Neue Wandlung
    }
  }
  if (precount_x >= 25) {
    precount_x = 0;
    adwert = presample_x/25; //min 0, max 1023
    presample_x = 0;
    if (adwert > 10) { //Verhindern eines Überlaufes oder Division durch Null
      adwert = (s16)((u32)122880/adwert); //Linearisierung
      adwert -= calib_x.zero; //Offset abziehen, wenn man so möchte
      if (adwert < 0) {
        adwert = adwert*255/calib_x.min;
      } else
      if (adwert > 0) {
        adwert = adwert*255/calib_x.max;
      }
      if (adwert < -127) { //Überläufe nach unten verhindern
        adwert = -127;
      }
      if (adwert > 127) { //Überläufe nach oben verhindern
        adwert = 127;
      }
      tinyadwert = (u08)adwert;
      userin.x = tinyadwert;
      //Taster erkennen
      if ((tinyadwert > 96) && (snap_x == 0)) { //Nach rechts
        snap_x = 1; //Einschnappen
        userin.right = 1;
      }
      if ((tinyadwert < -96) && (snap_x == 0)) { //Nach links
        snap_x = 1; //Einschnappen
        userin.left = 1;
      }
      if ((tinyadwert < 32) && (tinyadwert > -32)){
        snap_x = 0; //Ausschnappen
      }
    }
  }
  if (precount_y >= 25) {
    precount_y = 0;
    adwert = presample_y/25;
    presample_y = 0;
    if (adwert > 10) { //Verhindern eines Überlaufes oder Division durch Null
      adwert = (s16)((u32)122880/adwert); //Linearisierung
      adwert -= calib_y.zero;
      if (adwert < 0) {
        adwert = adwert*255/calib_y.min;
      } else
      if (adwert > 0) {
        adwert = adwert*255/calib_y.max;
      }
      if (adwert < -127) { //überläufe nach unten verhindern
        adwert = -127;
      }
      if (adwert > 127) { //Überläufe nach oben verhindern
        adwert = 127;
      }
      tinyadwert = (u08)adwert;
      userin.y = tinyadwert;
      //Taster erkennen
      if ((tinyadwert > 96) && (snap_y == 0)) { //Nach unten
        snap_y = 1; //Einschnappen
        userin.down = 1;
      }
      if ((tinyadwert < -96) && (snap_y == 0)) { //Nach oben
        snap_y = 1; //Einschnappen
        userin.up = 1;
      }
      if ((tinyadwert < 32) && (tinyadwert > -32)){
        snap_y = 0; //Ausschnappen
      }
    }
  }
}
TCNT0 = 130; // (255-130)*64 = 8000 Takte
}


u08 userin_left(void) {
if (userin.left) {
  userin.left = 0;
  return 1;
}
return 0;
}

u08 userin_right(void) {
if (userin.right) {
  userin.right = 0;
  return 1;
}
return 0;
}

u08 userin_up(void) {
if (userin.up) {
  userin.up = 0;
  return 1;
}
return 0;
}

u08 userin_down(void) {
if (userin.down) {
  userin.down = 0;
  return 1;
}
return 0;
}

u08 userin_press(void) {
if (userin.press) {
  userin.press = 0;
  return 1;
}
return 0;
}

void userin_flush(void) {
userin.left = 0;
userin.right = 0;
userin.up = 0;
userin.down = 0;
userin.press = 0;
}
