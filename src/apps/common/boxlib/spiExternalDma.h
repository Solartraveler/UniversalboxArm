#pragma once

#include <stdint.h>
#include <stddef.h>

/*Required for GPIO init.
*/
void SpiExternalInit(void);

void SpiExternalTransferWaitDone(void);

/*dataOut or dataIn may be a NULL pointer.
  chipSelect drives low the associated CS pin during the transfer. If chipSelect is zero, no pin is driven.
  This can be used when the CS pin is driven by another function or no CS is needed.
  SD/MMC cards need no CS for entering SPI mode.
  If resetChipSelect is true, the CS pin is disabled at the end of the transfer. Has no meaning if chipSelect is zero.
  Call with dataOut and DataIn as NULL and resetChipSelect to be true to just disable the chip select again.
  In this case, if the chip select was not set before, a short set pulse is given.
*/
void SpiExternalTransferDma(const uint8_t * dataOut, uint8_t * dataIn, size_t len, uint8_t chipSelect, bool resetChipSelect);

/*Same as SpiExternalTransferDma.
*/
void SpiExternalTransfer(const uint8_t * dataOut, uint8_t * dataIn, size_t len, uint8_t chipSelect, bool resetChipSelect);
