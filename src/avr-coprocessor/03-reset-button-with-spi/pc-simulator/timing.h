#pragma once

#include <stdint.h>
#include <unistd.h>

static inline void waitms(uint16_t ms) {
	while(ms) {
		usleep(1000);
		ms--;
	}
}
