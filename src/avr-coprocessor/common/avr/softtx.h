#ifndef _SOFTTX_H
 #define _SOFTTX_H

#include <avr/io.h>
#include <inttypes.h>
#include <string.h>

/* In your sourcecode, define:

SOFTTX_DDR
SOFTTX_PORT
SOFTTX_PIN

F_CPU
BAUDRATE

and then include the place of the values here

*/

#include "hardware.h"
#include "timing.h"

//not required to be called, but sets the initial voltage level
//after calling, wait the transmit of one char before starting to print
static inline void softtx_init(void) {
	SOFTTX_DDR |= (1<<SOFTTX_PIN);
	SOFTTX_PORT |= (1<<SOFTTX_PIN);
}

void softtx_char(char c);

void print_p(const char * s);

void print(const char * s);

#endif
