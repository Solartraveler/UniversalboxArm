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
//throws away data if the FIFO is full. Can be used within interrupt routines
int printfNowait(const char * format, ...);

/*Returns as soon as the FIFO is empty. Call before disabling the uart or
  changing its clock source / baud rate
*/
void Rs232Flush(void);
