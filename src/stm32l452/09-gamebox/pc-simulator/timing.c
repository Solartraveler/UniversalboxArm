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

u08 volatile TCCR1B;
u16 volatile TCNT1;


unsigned long long get_time10k(void) {
struct timeval data1;
unsigned long long time10k;
//10000=10k ist eine Sekunde
gettimeofday (&data1, NULL);
time10k = data1.tv_sec*10000+data1.tv_usec/100;
return time10k;
}

unsigned long long get_time1M(void) {
struct timeval data1;
unsigned long long time1000k;
//1000000=1M ist eine Sekunde
gettimeofday (&data1, NULL);
time1000k = data1.tv_sec*1000000+data1.tv_usec;
return time1000k;
}


//void timer1_sim(int foo) {
void * timer1_sim(void * arg) {
unsigned long long currenttime, delta;
long long volatile timeprev = -1;
while (1) {
  currenttime = get_time1M();
  if (TCCR1B) { //wenn timer aktiv
    delta = abs(currenttime - timeprev);
    //printf("Timer Update: %lld, delta: %lld\n",currenttime,delta);
    if (TCCR1B == 0x04) {
      //TCNT1 muss 32768 mal pro Sec hoch-zählen
      TCNT1 += (u16)((float)delta*0.032768);
    }
  }
  timeprev = currenttime;
  usleep(30);
}
}

void waitms(uint16_t zeitms) {
long long starttime, endtime;
static long long missingtime = 0;
if (no_delays != 1) {
  starttime = get_time1M();
  if (zeitms > (missingtime/1000)) {
    usleep(zeitms*1000 - missingtime);
  }
  endtime = get_time1M();
  if (endtime > starttime) {
    missingtime += endtime - starttime - zeitms*1000;
  }
}
}
