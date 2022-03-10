#pragma once

#include <stdint.h>
#include <stddef.h>

void PeripheralPowerOn(void);

//This includes disabling of signals to LCD and flash too
void PeripheralPowerOff(void);

void PeripheralTransfer(const uint8_t * dataOut, uint8_t * dataIn, size_t len);

void PeripheralPrescaler(uint32_t prescaler);
