#pragma once

#include <stdint.h>
#include <stddef.h>

void PeripheralPowerOn(void);

void PeripheralPowerOff(void);

void PeripheralTransfer(const uint8_t * dataOut, uint8_t * dataIn, size_t len);
