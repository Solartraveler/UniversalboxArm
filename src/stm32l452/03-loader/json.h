#pragma once

#include <stdint.h>
#include <stdbool.h>

bool JsonValueGet(uint8_t * jsonStart, size_t jsonLen, const char * key, char * valueOut, size_t valueMax);
