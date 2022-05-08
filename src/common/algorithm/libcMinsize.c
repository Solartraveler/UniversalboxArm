#include <stdint.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>

#include "libcMinsize.h"

#include "femtoVsnprintf.h"

size_t strlen(const char * text) {
	size_t len = 0;
	while(*text) {
		text++;
		len++;
	}
	return len;
}

int snprintf(char *str, size_t size, const char *format, ...) {
	va_list args;
	va_start(args, format);
	femtoVsnprintf(str, size, format, args);
	va_end(args);
	return strlen(str);
}

int vsnprintf(char *str, size_t size, const char *format, va_list ap) {
	femtoVsnprintf(str, size, format, ap);
	return strlen(str);
}
