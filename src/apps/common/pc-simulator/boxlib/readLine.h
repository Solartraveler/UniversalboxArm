#pragma once

#include <unistd.h>

/*
Gets a line from the console. Returns as soon as \r or \n is received or
number of chars reach len - 1. \r and \n are not part of the input string.
Backspace is supported, and therefore not part of the input too.
There is no timeout. input will always be '\0' terminated
*/
void ReadSerialLine(char * input, size_t len);
