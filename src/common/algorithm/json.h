#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
/*
Note: This function parses the json every time, so its slow. Moreover it only works
on a 'flat' json and may not contain more than 32 elements. Which is:
a opening bracket with
up to 15 key + value pairs
a closing bracket

jsonStart: pointer to the string of the json structure.
jsonLen: Length of jsonStart. This allows jsonStart to be a non \0 terminated string.
key: Key to search for
valueOut: The value of the key. Will be \0 terminated
valueMax: buffer length of valueOut
Returns: true if the key has been found and it fits within valueOut together with the \0 termination
*/
bool JsonValueGet(const uint8_t * jsonStart, size_t jsonLen, const char * key, char * valueOut, size_t valueMax);
