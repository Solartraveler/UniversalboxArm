#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

//Use only for variables, as functions returning a value would be evaluated twice
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))


//like sscanf(string, "0x%x", &out) but uses less code
//also works with %x directly without a 0x prefix
uint32_t AsciiScanHex(const char * string);

/* Ignores whitspaces before the first digit found, otherwise breaks as soon
   as a non digit is found.
   Similar to sscanf(string, "%u", &out) but uses less code
*/
uint32_t AsciiScanDec(const char * string);

void PrintHex(const uint8_t * data, size_t len);

bool EndsWith(const char * string, const char * ending);

bool BeginsWith(const char * string, const char * starting);

uint32_t BytesFlip(uint32_t in);

size_t strlcpy(char * dest, const char * src, size_t maxDest);
