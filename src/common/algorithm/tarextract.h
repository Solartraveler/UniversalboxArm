#pragma once

#include <stdint.h>
#include <stdbool.h>

bool TarFileStartGet(const char * filename, uint8_t * tarData, size_t tarLen,  uint8_t ** startAddress, size_t * fileLen, uint32_t * timestamp);
