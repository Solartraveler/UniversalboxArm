#pragma once

#include <stdint.h>
#include <stddef.h>

//call before PeripheralTransferDma can be used
void PeripheralInit(void);

/*The dataOut and dataIn buffers needs to stay valid until PeripheralTransferWaitDone returns
  No other Peripheral function may be called until this point
*/
void PeripheralTransferBackground(const uint8_t * dataOut, uint8_t * dataIn, size_t len);

void PeripheralTransferWaitDone(void);

/*Combination of PeripheralTransferBackground with a call to PeripheralTransferWaitDone.
*/
void PeripheralTransferDma(const uint8_t * dataOut, uint8_t * dataIn, size_t len);


/*Same as PeripheralTransferDma
*/
void PeripheralTransfer(const uint8_t * dataOut, uint8_t * dataIn, size_t len);
