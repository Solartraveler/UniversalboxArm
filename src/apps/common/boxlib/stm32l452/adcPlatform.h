#pragma once

#include <stdint.h>

#include "stm32l4xx_ll_adc.h"

//returns the calibration value, written by the vendor for every chip
static inline uint16_t AdcCalibGet(void) {
	uint16_t cal = *VREFINT_CAL_ADDR;
	return cal;
}

