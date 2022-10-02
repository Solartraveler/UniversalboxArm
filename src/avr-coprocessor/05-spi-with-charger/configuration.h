#pragma once

/* Even if the oscillator is described as 128kHz, it actually never does
128kHz. At 25°C and 4.5V it does 108kHz according to the datasheet.
Note the differences in the datasheet between the ATtiny261A, ATtiny461A and ATiny861A!
*/

#define F_CPU (108000)
//#define F_CPU (250000)


//Define if the clock source is the low speed oscillator (F_CPU = ~108000)
#define LOWSPEEDOSC

#define ARMPOWERBUGFIXPIN

//#define ARMPOWERORIGINALPIN

#define USE_WATCHDOG

#define CMD_READ_MAX 0x22
