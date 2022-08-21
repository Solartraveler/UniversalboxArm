#pragma once

#include <avr/io.h>
#include <inttypes.h>

/* ADEN: Activate converter
   ADSC: Start first A/D conversion
   prescal->ADPS0..ADPS2:
   prescal: divider:
   0        2
   1        2
   2        4
   3        8
   4        16
   5        32
   6        64
   7        128
   It is reccommend to use a frequency between 50 und 200khz for a 10 Bit
resolution.
   */

//the setting REFS[2:0] = 0x7 should not be used

static inline void AdStartExtRef(uint8_t prescal, uint8_t channel) {
	PRR &= ~(1<<PRADC);
	ADMUX = (1<<REFS0) | (channel & 0x1F); //Select AD channel, use external Aref pin
	ADCSRB = (channel >> 2) & 0x08;
	ADCSRA = (1 << ADEN) | (1 << ADSC) | (prescal & 0x07); //start conversion
}

static inline void AdStartExtRefGain(uint8_t prescal, uint8_t channel) {
	PRR &= ~(1<<PRADC);
	ADMUX = (1<<REFS0) | (channel & 0x1F); //Select AD channel, use external Aref pin
	ADCSRB = ((channel >> 2) & 0x08) | (1<<GSEL);
	ADCSRA = (1 << ADEN) | (1 << ADSC) | (prescal & 0x07); //start conversion
}

static inline void AdStart11Ref(uint8_t prescal, uint8_t channel) {
	PRR &= ~(1<<PRADC);
	ADMUX = (1<<REFS1) | (channel & 0x1F); //Select AD channel, use internal 1.1V ref
	ADCSRB = (channel >> 2) & 0x08;
	ADCSRA = (1 << ADEN) | (1 << ADSC) | (prescal & 0x07); //start conversion
}

static inline void AdStartAvccRef(uint8_t prescal, uint8_t channel) {
	PRR &= ~(1<<PRADC);
	ADMUX = (channel & 0x1F); //Select AD channel, use Avcc ref
	ADCSRB = (channel >> 2) & 0x08;
	ADCSRA = (1 << ADEN) | (1 << ADSC) | (prescal & 0x07); //start conversion
}

static inline uint16_t AdGet(void) {
	while (ADCSRA & (1<<ADSC));
	return ADC;
}

static inline void AdStop(void) {
	ADCSRA &= ~(1 << ADEN);

}

static inline void AdPowerdown(void) {
	AdStop();
	PRR |= (1<<PRADC);
}

uint16_t getadc500(uint8_t channel);

uint16_t getadc256(uint8_t channel);
