#pragma once

#include <stdio.h>
#include <stdint.h>

#define PSTR(X) X

#define PROGMEM

#define COLOR_GREEN "\e[0;32m"
#define COLOR_RESET "\e[m"

static inline void softtx_init(void) {
}

static inline void softtx_char(char c)
{
	printf(COLOR_GREEN "%c" COLOR_RESET, c);
}

static inline void print_p(const char * s)
{
	printf(COLOR_GREEN "%s" COLOR_RESET, s);
}

static inline void print(const char * s)
{
	printf(COLOR_GREEN "%s" COLOR_RESET, s);
}
