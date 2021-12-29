#pragma once

#include <stdint.h>
#include <stdarg.h>

void rs232Init(void);

void rs232WriteString(const char * str);

char rs232GetChar(void);

int putchar(int c);

int puts(const char * string);

int printf(const char * format, ...);
