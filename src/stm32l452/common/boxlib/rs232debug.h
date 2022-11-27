#pragma once

#include <stdint.h>
#include <stdarg.h>

void Rs232Init(void);

void Rs232Stop(void);

//normal print. Don't use from within interrupts
void Rs232WriteString(const char * str);

//throws away data if the FIFO is full. Can be used within interrupt routines
void Rs232WriteStringNoWait(const char * str);

char Rs232GetChar(void);

int putchar(int c);

int puts(const char * string);

//normal print. Don't use from within interrupts
int printf(const char * format, ...);
/*Throws away data if the FIFO is full. Can be used within interrupt routines,
  Interrupts need to be enabled later, otherwise the message will not be printed
*/
int printfNowait(const char * format, ...);

/*Busy waits for the serial port to become free, then writes the data.
  The fifo is ignored.
  Use when interrupts are disabled, and writing to the fifo is unsuitable.
  Intended usage: Exception handlers, where a system reset or endless loop follows.
*/
int printfDirect(const char * format, ...);


/*Returns as soon as the FIFO is empty. Call before disabling the uart or
  changing its clock source / baud rate
*/
void Rs232Flush(void);
