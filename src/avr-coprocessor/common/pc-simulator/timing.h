#pragma once

#include <stdint.h>
#include <unistd.h>

//should not be really be relevant on the pc
#define F_CPU (128000)

static inline void waitms(uint16_t ms) {
	while(ms) {
		usleep(1000);
		ms--;
	}
}
