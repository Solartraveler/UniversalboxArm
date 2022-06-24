#pragma once

#define F_CPU (1000000)

//Define if the clock source is the low speed oscillator
//#define LOWSPEEDOSC

#define ARMPOWERBUGFIXPIN

//#define ARMPOWERORIGINALPIN

#define BAUDRATE 1200

#if ((F_CPU / BAUDRATE) < 100)
#error "Not supported for baudrate"
#endif

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

#define SOFTTX_PRINT_P

#endif
