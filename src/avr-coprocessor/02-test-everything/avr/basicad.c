/*Basic A/D Libray
  Version 1.2
  (c) 2005, 2008, 2009, 2019 by Malte Marwedel
  www.marwedels.de/malte

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

#include "basicad.h"
#include <stdlib.h>


uint16_t getadc500(uint8_t channel) {
	loop_until_bit_is_clear(ADCSRA,ADSC); //wait for a possible conversion to end
	ADMUX = (1<<REFS0) | channel; //Select AD channel with Vcc as ref
	ADCSRA |= (1<<ADSC); //set bit to start conversion
	//waits until the conversion is complete
	loop_until_bit_is_clear(ADCSRA,ADSC);
	return ADCW;
}

/*On the mega328, the correct name would be getadc110()
  as the reference is lower */
uint16_t getadc256(uint8_t channel) {
	loop_until_bit_is_clear(ADCSRA,ADSC); //wait for a possible conversion to end
	ADMUX = (1<<REFS0) | (1<<REFS1) | channel; //Select AD channel, with 2.56V ref
	ADCSRA |= (1<<ADSC); //set bit to start conversion
	//waits until the conversion is complete
	loop_until_bit_is_clear(ADCSRA,ADSC);
	return ADCW;
}

#if 0

//function currently simply unused

//gets the average over n A/D converted measurements
uint16_t sample_ad(uint8_t channel, uint8_t samples) {
	uint8_t i;
	uint16_t pure_ad = 0;
	for (i = 0; i < samples; i++) {
		pure_ad += getadc(channel);
	}
	return (pure_ad / samples);
}



#define NUMSAMPLES 60

int comparatorFuncUint16(const void * pa, const void * pb) {
	uint16_t a = *(uint16_t*)pa;
	uint16_t b = *(uint16_t*)pb;
	if (a > b) return 1;
	if (a < b) return -1;
	return 0;
}

//Takes 60 samples and gives the average of the 50% quartil (30measures)
//uses 1 bit of oversampling -> 0... 2046 as maximum result
uint16_t samle_adQuartil(uint8_t channel) {
	uint16_t samples[NUMSAMPLES];
	uint8_t i;
	//1. measure
	for (i = 0; i < NUMSAMPLES; i++) {
		samples[i] = getadc(channel);
	}
	//2. sort
	qsort(samples, NUMSAMPLES, sizeof(uint16_t), &comparatorFuncUint16);
	//3. get average of the middle
	uint16_t sum = 0;
	for (i = 0; i < NUMSAMPLES/2; i++) {
		sum += samples[i + NUMSAMPLES/4];
	}
	return sum/(NUMSAMPLES/4);
}
#endif