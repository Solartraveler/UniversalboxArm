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

void waitms(uint16_t zeitms) {
//50% der Zeit gehen fürs Anzeigen drauf, also gehen wir von 4MHZ aus
if (no_delays != 1) {
  while (zeitms != 0) { //Nicht sonderlich genau
    zeitms--;
    _delay_loop_2(F_CPU_msdelay);//Jeder Duchlauf mit n=1 benötigt 4 Takte
  }
}
}
