#pragma once

#include <stdint.h>

/*
  This is nearly a 1:1 copy from https://www.nongnu.org/avr-libc/user-manual/group__util__crc.html
  So their copyright is reproduced here.
  Some formatting has been changed and the function declaration was adjusted
*/

/*
Portions of avr-libc are Copyright (c) 1999-2016
Werner Boellmann,
Dean Camera,
Pieter Conradie,
Brian Dean,
Keith Gudger,
Wouter van Gulik,
Bjoern Haase,
Steinar Haugen,
Peter Jansen,
Reinhard Jessich,
Magnus Johansson,
Harald Kipp,
Carlos Lamas,
Cliff Lawson,
Artur Lipowski,
Marek Michalkiewicz,
Todd C. Miller,
Rich Neswold,
Colin O'Flynn,
Bob Paddock,
Andrey Pashchenko,
Reiner Patommel,
Florin-Viorel Petrov,
Alexander Popov,
Michael Rickman,
Theodore A. Roth,
Juergen Schilling,
Philip Soeberg,
Anatoly Sokolov,
Nils Kristian Strom,
Michael Stumpf,
Stefan Swanepoel,
Helmut Wallner,
Eric B. Weddington,
Joerg Wunsch,
Dmitry Xmelkov,
Atmel Corporation,
egnite Software GmbH,
The Regents of the University of California.
All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

   * Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.

   * Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in
     the documentation and/or other materials provided with the
     distribution.

   * Neither the name of the copyright holders nor the names of
     contributors may be used to endorse or promote products derived
     from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
*/


static inline uint16_t _crc16_update(uint16_t crc, uint8_t a) {
	int i;
	crc ^= a;
	for (i = 0; i < 8; ++i) {
		if (crc & 1)
			crc = (crc >> 1) ^ 0xA001;
		else
			crc = (crc >> 1);
	}
	return crc;
}