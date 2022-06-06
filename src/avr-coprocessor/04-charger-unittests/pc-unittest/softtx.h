#pragma once

#include <stdio.h>
#include <stdint.h>

#define PSTR(X) X

#define PROGMEM

#define COLOR_GREEN "\e[0;32m"
#define COLOR_RESET "\e[m"

static inline void softtx_char(char c)
{
	printf("%c", c);
}

static inline void print_p(const char * s)
{
	printf("%s", s);
}

static inline void print(const char * s)
{
	printf("%s", s);
}
