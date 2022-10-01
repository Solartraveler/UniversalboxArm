#pragma once

/* Even if the oscillator is described as 128kHz, it actually never does
128kHz. At 25°C and 4.5V it does 108kHz according to the datasheet.
Note the differences in the datasheet between the ATtiny261A, ATtiny461A and ATiny861A!
*/
#define F_CPU (108000)

#define ARMPOWERBUGFIXPIN

#define LOWSPEEDOSC