/*
   Gamebox
    Copyright (C) 2004-2006, 2023  by Malte Marwedel
    m.talk AT marwedels dot de

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

#ifndef TIMINGS_H
 #define TIMINGS_H

static inline void timer_start(uint8_t prescaler) {
	TCNT1 = 0; //Reset Timer
	TCCR1B = prescaler;
}

static inline void timer_set(uint16_t newValue) {
	TCNT1 = newValue;
}

static inline uint16_t timer_get(void) {
	return TCNT1;
}

static inline void timer_stop(void) {
	TCCR1B = 0;
}

void waitms(uint16_t zeitms);

#endif
