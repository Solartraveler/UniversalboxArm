#pragma once

#include <stdint.h>
#include <stdbool.h>

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <avr/sleep.h>

#include "timing.h"
#include "basicad.h"

#include "configuration.h"

#define PWM_MAX 128

extern volatile uint8_t g_timer0Cnt;
extern uint8_t g_timer0LastPoll;

/* The .init3 section is done after the stack has been set up, but before
the bss section is copied. So we can not rely on any global variables,
but we are faster after startup with settings the pins correctly.
HardwareInit may set the same pins again with the same values.
*/
void HardwareInitEarly(void) __attribute__ ((naked)) __attribute__ ((section (".init3")));

static inline void HardwareInit(void) {
	DDRA = (1<<7) | (1<<1);
	DDRB = (1<<1) | (1<<3) | (1<<4);
	PORTA = (1<<7); //start in ARM reset state, but no bootloader
	PORTB = (1<<2) | (1<<6); //pullup for the two buttons
	//save some power
	PRR = (1<<PRUSI);
	DIDR0 = (1<<1) | (1<<3) | (1<<4) | (1<<5) | (1<<6) | (1<<7);
	DIDR1 = (1<<7) | (1<<5) | (1<<4);

#if !defined(LOWSPEEDOSC)

	//drop down the frequency to the intended one (assuming 8MHz oscillator)
	//This code is only reliable if interrupts are still disabled
	CLKPR = (1<<CLKPCE);
#if (F_CPU == 8000000)
	CLKPR = 0;
#elif (F_CPU == 4000000)
	CLKPR = (1<<CLKPS0);
#elif (F_CPU == 2000000)
	CLKPR = (1<<CLKPS1);
#elif (F_CPU == 1000000)
	CLKPR = (1<<CLKPS1) | (1<<CLKPS0);
#elif (F_CPU == 500000)
	CLKPR = (1<<CLKPS2);
#elif (F_CPU == 250000)
	CLKPR = (1<<CLKPS2) | (1<<CLKPS0);
#elif (F_CPU == 125000)
	CLKPR = (1<<CLKPS2) | (1<<CLKPS1);
#elif (F_CPU == 62500)
	CLKPR = (1<<CLKPS2) | (1<<CLKPS1) | (1<<CLKPS0);
#elif (F_CPU == 31250)
	CLKPR = (1<<CLKPS3);
#else
#error "Not supported for prescaler"
#endif

//the lowspeed oscillator can really change a lot...
#elif (F_CPU > 128000) || (F_CPU < 100000)

#error "Not supported for prescaler"

#endif

#if defined(USE_WATCHDOG)
	wdt_enable(WDTO_2S);
#endif
}

//only the pullups are left
static inline void PinsPowerdown(void) {
	/* We should be able to set DIDR0 = 0xFF here, but for some unknown reason
	   then we do not get a high level after running PinsPowerup on 3.3V battery
	   power.
	   On ~4.7V USB power it works.
	   TODO: Dig down deeper why this is the case.
	*/
	DIDR0 = 0xFE;
	DIDR1 = (1<<7) | (1<<5) | (1<<4);
}

static inline void PinsPowerup(void) {
	DIDR0 = (1<<1) | (1<<3) | (1<<4) | (1<<5) | (1<<6) | (1<<7);
	DIDR1 = (1<<7) | (1<<5) | (1<<4);
}

static inline void PinsWakeupByKeyPressOn(void) {
	//only a low level ISR can wake up the CPU from powerdown
	//not compatible with the software SPI
	MCUCR &= ~((1<<ISC00) | (1<<ISC01));
	GIMSK |= (1<<INT0); //allows waking up the CPU by the left key press
}

static inline void PinsWakeupByKeyPressOff(void) {
	GIMSK &= ~(1<<INT0);
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
#ifdef ARMPOWERORIGINALPIN
	//PB5 if no PCB fix is applied
	PORTB &= ~(1<<5);
	DDRB |= (1<<5);
#endif
#ifdef ARMPOWERBUGFIXPIN
	//PB0 if PCB fix is applied
	PORTB &= ~(1<<0);
	DDRB |= (1<<0);
#endif
}

static inline void ArmBatteryOff(void) {
	//off when pin is high or disconnected
#ifdef ARMPOWERORIGINALPIN
	//PB5 if no PCB fix is applied
	PORTB |= (1<<5);
	DDRB &= ~(1<<5);
#endif
#ifdef ARMPOWERBUGFIXPIN
	PORTB |= (1<<0);
	DDRB &= ~(1<<0);
#endif
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

//If useIsr is false, the timer should be polled with TimerHasOverflown
//otherwise TimerHasOverflownIsr must be used
//sets the speed as TimerFast()
static inline void TimerInit(bool useIsr) {
	PRR &= ~(1<<PRTIM0); //enable power
	TCCR0B = 0; //stop timer
	TIFR |= (1<<OCF0A); //clear compare flag
	if (useIsr) {
		TIMSK |= (1<<OCIE0A);
	}
	TCCR0A = 1<<0; //clear timer on compare match (WGM00 in header, CTC0 in datasheet)
	TCNT0H = 0;
	TCNT0L = 0; //clear counter
	const uint16_t timerMax = F_CPU/(64UL* 100UL); //10ms timing
	OCR0B = (timerMax >> 8) & 0xFF; //high byte must be written before the low byte
	OCR0A = timerMax & 0xFF;
	TCCR0B = (1<<CS00) | (1<<CS01); //start timer with prescaler = 64
}

static inline void TimerSlow(void) {
	const uint16_t timerMax = F_CPU/(64UL* 10UL); //100ms timing
	TCNT0H = 0;
	TCNT0L = 0; //clear counter
	OCR0B = (timerMax >> 8) & 0xFF; //high byte must be written before the low byte
	OCR0A = timerMax & 0xFF;
}

static inline void TimerFast(void) {
	const uint16_t timerMax = F_CPU/(64UL* 100UL); //10ms timing
	TCNT0H = 0;
	TCNT0L = 0; //clear counter
	OCR0B = (timerMax >> 8) & 0xFF; //high byte must be written before the low byte
	OCR0A = timerMax & 0xFF;
}

static inline bool TimerHasOverflown(void) {
	uint8_t localVal = g_timer0Cnt; //may only be read once
	if ((TIFR & (1<<OCF0A)) || (localVal != g_timer0LastPoll)) {
		g_timer0LastPoll = localVal;
		TIFR |= (1<<OCF0A); //clear compare flag
		return true;
	}
	return false;
}

/* Unlike TimerHasOverflown, this functions catches up to 255 missed
   polls. So it allows catching up too slow processing.
*/
static inline bool TimerHasOverflownIsr(void) {
	if (g_timer0Cnt != g_timer0LastPoll) {
		g_timer0LastPoll++;
		return true;
	}
	return false;
}

static inline uint8_t TimerGetValue(void) {
	return g_timer0Cnt;
}

static inline uint16_t TimerGetTicksLeft(void) {
	const uint16_t timerMax = F_CPU/(64UL* 100UL); //10ms timing
	uint16_t counter = TCNT0L; //low must be read first
	counter |= (TCNT0H << 8);
	return timerMax - counter;
}

static inline uint32_t TimerGetTicksPerSecond(void) {
	return F_CPU/64UL;
}

static inline void TimerStop(void) {
	TCCR0B = 0;
	PRR |= (1<<PRTIM0); //disable power
}

static inline void WatchdogReset(void) {
	wdt_reset();
}

static inline void WatchdogDisable(void) {
	wdt_disable();
}

static inline void WaitForInterrupt(void) {
	MCUCR &= ~((1<<SM1) | (1<<SM0)); //idle
	sleep_enable();
	sleep_cpu();
	sleep_disable();
}

static inline void WaitForExternalInterrupt(void) {
	MCUCR |= (1<<SM1);
	MCUCR &= ~(1<<SM0); //power down mode
	sleep_enable();
	sleep_bod_disable();
	sleep_cpu();
	sleep_disable();
}

//provided by basicad.h:
static inline void AdPowerdown(void);
