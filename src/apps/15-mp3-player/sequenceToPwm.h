#pragma once

#include <stddef.h>
#include <stdint.h>

/* Initializes two timers. One 8 bit timer with pwmDivider, generating a PWM signal.
And a second timer which will change the PWM value each seqDivider interrupt.
fifoLen must be number of fifoBuffer in bytes.
pwmDivider will be interpreted as 2^pwmDivider
seqMax maximum value for the timer doing the timing control.
*/
void SeqStart(uint32_t pwmDivider, uint32_t seqMax, uint8_t * fifoBuffer, size_t fifoLen);

/* Stops both timers.
*/
void SeqStop(void);

/* Returns the number of bytes free in the FIFO.*/
size_t SeqFifoFree(void);

/* Puts dataLen bytes to the fifo, if enought space is free. Check with SeqFifoFree before putting the data*/
void SeqFifoPut(const uint8_t * data, size_t dataLen);

