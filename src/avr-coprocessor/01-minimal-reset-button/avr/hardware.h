#pragma once

#include <stdint.h>
#include <stdbool.h>

#include <avr/io.h>

#include "timing.h"

static inline void PinsInit(void) {
	PORTA = 0;
	PORTB = (1<<2) | (1<<6) | (1<<5); //pullup for the two buttons, pullup for not power by battery
	DDRA = (1<<7) | (1<<1);
	DDRB = (1<<1) | (1<<3) | (1<<4) | (1<<5);
	//save some power
	PRR = (1<<PRTIM1) | (1<<PRUSI) | (1<<PRADC);
	DIDR0 = (1<<1) | (1<<3) | (1<<4) | (1<<5) | (1<<6) | (1<<7);
	DIDR1 = (1<<5) | (1<<4);
}

static inline void LedOn(void) {
	PORTB |= (1<<1);
}

static inline void LedOff(void) {
	PORTB &= ~(1<<1);
}

static inline void ArmReset(void) {
	PORTA |= (1<<7);
}

static inline void ArmRun(void) {
	PORTA &= ~(1<<7);
}

static inline void ArmBootload(void) {
	PORTA |= (1<<1);
}

static inline void ArmUserprog(void) {
	PORTA &= ~(1<<1);
}

static inline void ArmBatteryOn(void) {
	//on when pin is active low
	PORTB &= ~(1<<5);
	DDRB |= (1<<5);
}

static inline bool KeyPressedRight(void) {
	if (PINB & (1<<2)) {
		return false;
	}
	return true;
}

static inline bool KeyPressedLeft(void) {
	if (PINB & (1<<6)) {
		return false;
	}
	return true;
}

static inline void TimerInit(void) {
	TCCR0B = 0; //stop timer
	TIFR |= (1<<OCF0A); //clear compare flag
	TCCR0A = 1<<0; //clear timer on compare match (WGM00 in header, CTC0 in datasheet)
	TCNT0L = 0; //clear counter
	OCR0A = F_CPU/(64ULL* 100); //10ms timing
	TCCR0B = (1<<CS00) | (1<<CS01); //start timer with prescaler = 64
}

static inline bool TimerHasOverflown(void) {
	if (TIFR & (1<<OCF0A)) {
		TIFR |= (1<<OCF0A); //clear compare flag
		return true;
	}
	return false;
}

