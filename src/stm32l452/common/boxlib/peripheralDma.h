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
