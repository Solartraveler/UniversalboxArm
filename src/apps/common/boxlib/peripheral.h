#pragma once

#include <stdint.h>
#include <stddef.h>

//Do not call directly, already done by PeripheralInit
void PeripheralBaseInit();

/*Required for GPIO init and if used, for DMA.
  PeripheralPowerOn and Off are independed from this function, so power can be
  enabled and disabled without initializing everything else of the peripherals.
  NOTE: When PeripheralInit is called, PeripheralPowerOn should have been called before.

  Recommended init sequence:
  1. PeripheralPowerOff()
  2. Wait some time for capacitors to discharge, 100ms should be very safe
  3. PeripheralPowerOn()
  Enable RS232 or prepare for SPI peripherals:
  4.1 Rs232Init()
  or 4.2 PeripheralInit()
  After PeripheralInit() flash or the LCD can be enabled
*/
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

/*Weak dummy functions, for real implementation and thread safety,
peripheralMt.c must be compiled, which provide proper functions.
Note: The functions above will stay not thread safe, but when the calls in lcd.c
and flash.c call the lock before every usage, this will be thread safe.
Adding this into functions like PeripheralTransfer makes little sense, because
they interact with PeripheralPrescaler and PeripheralTransferWaitDone. So making
them thread safe alone would not result in the intended behaviour.
*/
void PeripheralLockMt(void);

void PeripheralUnlockMt(void);
