/*
Copyright 2021 Malte Marwedel

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Changelog:
2021-02-14: Version 1.0.0
2021-11-24: Version 1.1.0
2022-11-13: Version 1.2.1

*/

#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#ifdef TEST_PC
#include <stdio.h>
#endif

#include "femtoVsnprintf.h"

/*
	Supported formats:
	%c
	%s
	%u
	%i
	%X
	%Wu where W is in the range 1...9
	%WX where W is in the range 1...9
	%W0u where W is in the range 1...9
	%W0X where W is in the range 1...9
The output is always \0 terminated
*/

#if defined(FEMTO_SUPPORT_DECIMAL) || defined(FEMTO_SUPPORT_HEX) || defined(FEMTO_SUPPORT_DECIMAL_NEGATIVE)

//converted, but chars in reverse order. Returns the string length (not counting the \0 termination)
static size_t utoaReversed(unsigned int value, char * out, size_t outLen, unsigned int radix)
{
	size_t i;
	for (i = 0; i < outLen - 1; i++)
	{
		if (value == 0)
		{
			break;
		}
		unsigned int digit = value % radix;
		value /= radix;
#ifdef FEMTO_SUPPORT_HEX
		if (digit > 9)
		{
			digit += 'A' - 10;
		}
		else
#endif
		{
			digit += '0';
		}

		out[i] = digit;
	}
	if (i == 0)
	{
		out[i] = '0';
		i++;
	}
	out[i] = '\0';
	return i;
}
#endif

#if defined(FEMTO_SUPPORT_DECIMAL_NEGATIVE)

//converted, but chars in reverse order
static size_t itoaReversed(int value, char * out, size_t outLen, unsigned int radix)
{
	int negative = 0;
	if (value < 0)
	{
		value = -value;
		negative = 1;
	}
	size_t len = utoaReversed(value, out, outLen, radix);
	if (((len + 1) < outLen) && (negative))
	{
		out[len] = '-';
		out[len + 1] = '\0';
		len++;
	}
	return len;
}
#endif

void femtoVsnprintf(char * output, size_t outLen, const char * format, va_list args)
{
	while (outLen > 1)
	{
		char input = *format;
		format++;
		if (input == '\0')
		{
			break;
		}
		if (input == '%')
		{
			input = *format;
			if (input == '\0')
			{
				break;
			}
			format++;
#ifdef FEMTO_SUPPORT_C
			if (input == 'c')
			{
				unsigned int x = va_arg(args, int);
				*output = x;
				output++;
				outLen--;
			}
			else
#endif
#ifdef FEMTO_SUPPORT_S
			if (input == 's')
			{
				const char * strIn = va_arg(args, char *);
				while ((strIn) && (*strIn) && (outLen > 1))
				{
					*output = *strIn;
					output++;
					strIn++;
					outLen--;
				}
			}
			else
#endif
			{
				unsigned int x = va_arg(args, int);
				uint8_t minWidht = 0;
#ifdef FEMTO_SUPPORT_LEADINGZEROS
				bool leadingZeros = false;
#endif
				if (isdigit(input))
				{
					minWidht = input - '0'; //single digit conversion
					input = *format;
					if (input == '\0')
					{
						break;
					}
					format++;
#ifdef FEMTO_SUPPORT_LEADINGZEROS
					if (minWidht == 0) //leading zeros
					{
						minWidht = input - '0'; //single digit conversion
						input = *format;
						if (input == '\0')
						{
							break;
						}
						format++;
						leadingZeros = true;
					}
#endif
				}
				char buffer[12];
				size_t len = 0;
#ifdef FEMTO_SUPPORT_DECIMAL
				if (input == 'u')
				{
					len = utoaReversed(x, buffer, sizeof(buffer), 10);
				}
				else
#endif
#ifdef FEMTO_SUPPORT_DECIMAL_NEGATIVE
				if (input == 'i')
				{
					len = itoaReversed(x, buffer, sizeof(buffer), 10);
				}
				else
#endif
				{
#ifdef FEMTO_SUPPORT_HEX
					len = utoaReversed(x, buffer, sizeof(buffer), 16);
#endif
				}
				size_t resultLen = len;
				while ((resultLen < minWidht) && (outLen > 1)) //fill leading spaces/zeros
				{
#ifdef FEMTO_SUPPORT_LEADINGZEROS
					if (leadingZeros) {
						*output = '0';
					}
					else
#endif
					{
						*output = ' ';
					}
					output++;
					resultLen++;
					outLen--;
				}
				for (size_t i = 0; i < len; i++)
				{
					if (outLen <= 1)
					{
						break;
					}
					*output = buffer[len - i - 1];
					output++;
					outLen--;
				}
			}
		} else
		{
			*output = input;
			output++;
			outLen--;
		}
	}
	if (outLen)
	{
		*output = '\0';
	}
	va_end(args);
}

#ifdef FEMTO_SUPPORT_SNPRINTF
void femtoSnprintf(char * output, size_t outLen, const char * format, ...) {
	va_list args;
	va_start(args, format);
	femtoVsnprintf(output, outLen, format, args);
	va_end(args);
}
#endif

#ifdef TEST_PC

int testMe(const char * format, ...) {
	va_list args;
	char bufferA[128];
	char bufferB[128];
	va_start(args, format);
	femtoVsnprintf(bufferA, sizeof(bufferA), format, args);
	va_end(args);
	va_start(args, format);
	vsnprintf(bufferB, sizeof(bufferB), format, args);
	va_end(args);
	if (strcmp(bufferA, bufferB) == 0)
	{
		printf("OK: %s", bufferA);
		return 0;
	}
	else
	{
		printf("Does not match:\n");
		printf("should: %s", bufferB);
		printf("is:     %s", bufferA);
		return 1;
	}
}

int main(void) {
	int result = 0;
	result |= testMe("Hello World\n");
	result |= testMe("A char: >%c<\n", 'X');
	const char * teststring = "More Hello";
	result |= testMe("A string: >%s<\n", teststring);
	result |= testMe("A decimal number: >%u<\n", 1234567890);
	result |= testMe("A decimal zero: >%u<\n", 0);
	result |= testMe("A hex number: >%X<\n", 0xFEDCBA98);
	result |= testMe("A decimal number 4 chars wide: >%4u<\n", 42);
	result |= testMe("A hex number 8 chars wide: >%8X<\n", 0xABC);
	result |= testMe("Decimal leading zeros: >%06u<\n", 665);
	result |= testMe("A char: >%c< and a number >%u< and another number >%u<\n", 'X', 23, 123);
	result |= testMe("A positive and negative number: >%i< and >%i<\n", 123, -54321);
	return result;
}

#endif


