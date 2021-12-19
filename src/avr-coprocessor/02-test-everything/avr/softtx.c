#include <avr/io.h>
#include <avr/pgmspace.h>

#include "softtx.h"

//needs F_CPU, coming from softtx.h:
#include <util/delay.h>

/*
Analysing the generated assembly shows that setting the pint to 0 is performed
one CPU cycle earlier than setting the pin to 1.
But because the setting to 0 takes a clock longer afterwards, the effect is not
summed over multiple bits. The total loop runs (without the delay functions)
in 9 clock cycles (per bit).

At 8MHz, 9600 baud and 10bits this will result in an error of:
1/8MHz*(9*10) = 12,25µs, while 1/9600*10 = 1ms is used for sending one char.
This means that the UART is ~1.18percent too slow. as result a small factor
is subtracted from the delay functions: -1.0/F_CPU*9.0)

At 1Mhz, 9600 baut and 10bits the error is:
1/1Mhz*(9*10) = 90µs, still using 1ms for one char. Resulting in up to 9%
too slow. But still working most of the times.

NOTE: This analysis depends highly on the compiler and the used optimisation
settings, which is -Os

By testing an atiny44, the UART got proper data with the osccal values
101...116 and 142...168. This would depend on a frequency range for both ranges
for 7..8MHz according to the datasheet. Assuming for our chip,
it is 7.5...8.5MHz, this results in an allowed frequency error for the UART in
the range 94% ... 106%. The internal oscillator is calibrated and guarantees an
accuracy within 3%. So the assumption is, that the serial port will always work
without problems with the internal oscillator.

Using a RS232->USB converter, the values were:
105... 118 and 147...171 -> 7.7..8.6MHz -> 96.3..206% -> fits into +-3% too.
*/

#pragma GCC push_options

#pragma GCC optimize ("-Os")

void softtx_char(char c) {
	uint8_t i = 0;
	uint8_t c2 = c;
	c2 = ~c2;
	SOFTTX_DDR |= (1<<SOFTTX_PIN);
	SOFTTX_PORT &= ~(1<<SOFTTX_PIN); //startbit
	_delay_ms((1.0/((double)BAUDRATE)*1000.0-1.0/F_CPU*9.0*1000.0));
	while (i < 9) {
		if (c2 & 0x01) {
			SOFTTX_PORT &= ~(1<<SOFTTX_PIN);
		} else {
			SOFTTX_PORT |= (1<<SOFTTX_PIN);
		}
		c2 >>= 1;
		i++;
		_delay_ms((1.0/((double)BAUDRATE))*1000.0-1.0/F_CPU*9.0*1000.0);
	}
}

#pragma GCC pop_options

void print_p(const char * s) {
	while (1) {
		uint8_t v = pgm_read_byte(s);
		if (v == '\0') {
			break;
		}
		softtx_char(v);
		s++;
	}
}

void print(const char * s) {
	while (1) {
		uint8_t v = *s;
		if (v == '\0') {
			break;
		}
		softtx_char(v);
		s++;
	}
}

