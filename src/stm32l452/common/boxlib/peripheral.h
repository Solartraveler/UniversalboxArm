#pragma once

#include <stdint.h>
#include <stddef.h>

//Does nothing. But reqired for DMA compatibility
void PeripheralInit(void);

void PeripheralPowerOn(void);

//This includes disabling of signals to LCD and flash too
void PeripheralPowerOff(void);

//dataOut or dataIn may be a NULL pointer
void PeripheralTransfer(const uint8_t * dataOut, uint8_t * dataIn, size_t len);

void PeripheralTransferBackground(const uint8_t * dataOut, uint8_t * dataIn, size_t len);

void PeripheralPrescaler(uint32_t prescaler);

//for non DMA, this is a dummy function
void PeripheralTransferWaitDone(void);
