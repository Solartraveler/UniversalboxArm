#pragma once

#define EEMEM

static inline uint8_t eeprom_read_byte(const uint8_t * addr) {
	return *addr;
}
