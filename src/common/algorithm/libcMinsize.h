#pragma once

#include <stdint.h>
#include <ctype.h>
#include <stdarg.h>

size_t strlen(const char * text);
int snprintf(char *str, size_t size, const char *format, ...);
int vsnprintf(char *str, size_t size, const char *format, va_list ap);
