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

void softtx_char(char c);

void print_p(const char * s);

void print(const char * s);

#endif
