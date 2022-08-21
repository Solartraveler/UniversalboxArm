#pragma once

#define sei()

#define cli()

void isrFunc(void);

#define ISR(X) void isrFunc(void)

#define ISC00 0
#define ISC01 1
#define INT1 8
#define INTF1 8

extern uint8_t MCUCR;
extern uint8_t GIMSK;
extern uint8_t GIFR;

