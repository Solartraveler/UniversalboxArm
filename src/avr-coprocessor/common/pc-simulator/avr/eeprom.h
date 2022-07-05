#pragma once

#include <string.h>

#define EEMEM

static inline uint8_t eeprom_read_byte(const uint8_t * addr) {
	return *addr;
}

static inline void eeprom_read_block(void * dst, const void * src, size_t len) {
	memcpy(dst, src, len);
}

static inline void eeprom_update_block(const void * src, void * dst, size_t len) {
	memcpy(dst, src, len);
}

