#pragma once

#include <stdint.h>
#include <stdbool.h>

#include <avr/io.h>
#include <avr/pgmspace.h>

#include "timing.h"
#include "basicad.h"

#if 0

//using the LED as debug output
#define SOFTTX_DDR DDRB
#define SOFTTX_PORT PORTB
#define SOFTTX_PIN PIN1

#else

//using the AVR Do as debug output
#define SOFTTX_DDR DDRA
#define SOFTTX_PORT PORTA
#define SOFTTX_PIN PIN1

#endif

#define BAUDRATE 1200

#if ((F_CPU / BAUDRATE) < 100)
#error "Not supported for baudrate"
#endif

#define PWM_MAX 65

static inline void HardwareInit(void) {
	DDRA = (1<<7) | (1<<1);
	DDRB = (1<<1) | (1<<3) | (1<<4);
	PORTA = 0;
	PORTB = (1<<2) | (1<<6); //pullup for the two buttons
	//save some power
	PRR = (1<<PRTIM0) | (1<<PRUSI);
	DIDR0 = (1<<1) | (1<<3) | (1<<4) | (1<<5) | (1<<6) | (1<<7);
	DIDR1 = (1<<5) | (1<<4);
	//drop down the frequency to the intended one (assuming 8MHz oscillator)
	//This code is only reliable if interrupts are still disabled
	CLKPR = (1<<CLKPCE);
#if (F_CPU == 8000000)
#warning 1
	CLKPR = 0;
#elif (F_CPU == 4000000)
	CLKPR = (1<<CLKPS0);
#elif (F_CPU == 2000000)
	CLKPR = (1<<CLKPS1);
#elif (F_CPU == 1000000)
	CLKPR = (1<<CLKPS1) | (1<<CLKPS0);
#elif (F_CPU == 500000)
	CLKPR = (1<<CLKPS2) | (1<<CLKPS0);
#elif (F_CPU == 250000)
	CLKPR = (1<<CLKPS2) | (1<<CLKPS1);
#elif (F_CPU == 125000)
	CLKPR = (1<<CLKPS2) | (1<<CLKPS1) | (1<<CLKPS0);
#elif (F_CPU == 62500)
	CLKPR = (1<<CLKPS3);
#else
#error "Not supported for prescaler"
#endif
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
	PORTB &= ~(1<<0);
	DDRB |= (1<<0);
}

static inline void ArmBatteryOff(void) {
	//off when pin is high or disconnected
	PORTB |= (1<<0);
	DDRB &= ~(1<<0);
}

static inline void SensorsOn(void) {
	PORTB |= (1<<4);
}

static inline void SensorsOff(void) {
	PORTB &= ~(1<<4);
}

static inline bool SpiSckLevel(void) {
	if (PINA & (1<<2)) {
		return true;
	} else {
		return false;
	}
}

static inline bool SpiDiLevel(void) {
	if (PINA & (1<<0)) {
		return true;
	} else {
		return false;
	}
}

//in 0.1°C units
int16_t SensorsBatterytemperatureGet(void);

//in mV
uint16_t SensorsInputvoltageGet(void);

//directly the AD converter value
uint16_t VccRaw(void);

//directly the AD converter value
uint16_t DropRaw(void);

//in mV
uint16_t SensorsBatteryvoltageGet(void);

//in mA
uint16_t SensorsBatterycurrentGet(void);

//in 0.1°C units
int16_t SensorsChiptemperatureGet(void);

void PwmBatterySet(uint8_t val);

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

