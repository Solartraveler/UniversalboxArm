#pragma once

#include <stdbool.h>
#include <stdint.h>

//It is always this value for SDHC and SDXC cards. SD and MMC could support other values.
#define SDMMC_BLOCKSIZE 512LLU


/*
SPI transfer function.
dataOut or dataIn may be NULL. If dataOut and/or dataIn is given, they must be len bytes in size.
chipSelect will be the value given by SdmmcInit.
If resetChipSelect is false, the CS signal must stay active, because another spi transfer will follow.
The last call for SPI transfer will resetChipSelect have set true.
len may be zero, to just reset the chip select without any transfer.
*/

typedef void (SpiTransferFunc_t)(const uint8_t * dataOut, uint8_t * dataIn, size_t len, uint8_t chipSelect, bool resetChipSelect);

/*Initializes the SD or MMC card. Has an internal state for the later SdmmcRead and SdmmcWrite functions.
  The pSpiTransfer function must do the I/O transfer, and the SPI peripheral should be initialized
  before calling this function to a frequency between 100kHz and 400kHz.
  After this function returned successful, the SPI frequency may be increased to 25MHz (or 20MHz for MMC),
  or smaller depending of the wiring quality.
  The chipSelect is forwarded to the SPI transfer function.
  returns:
  0: Init was successful
  1: Card reported errors on init
  3: Card not compatible
  4: Wrong parameters
*/
uint32_t SdmmcInit(SpiTransferFunc_t * pSpiTransfer, uint8_t chipSelect);

/*Reads one block into the buffer.
  A block is 512 bytes in size.
  Returns true if reading was successful.
*/
bool SdmmcReadSingleBlock(uint8_t * buffer, uint32_t block);

/*Reads the blocks into the buffer.
  Each block is 512 bytes in size.
  blockNum number of blocks to read.
  Returns true if reading was successful.
*/
bool SdmmcRead(uint8_t * buffer, uint32_t block, uint32_t blockNum);

/*Writes one block onto the SD/MMC card.
  A block is 512 bytes in size.
  Returns true if writing was successful.
*/
bool SdmmcWriteSingleBlock(const uint8_t * buffer, uint32_t block);

/*Writes the blocks onto the SD/MMC card
  Each block is 512 bytes in size.
  blockNum number of blocks to write.
  Returns true if writing was successful.
*/
bool SdmmcWrite(const uint8_t * buffer, uint32_t block, uint32_t blockNum);

/*Returns the card capacity in blocks (512 bytes)
*/
uint32_t SdmmcCapacity(void);

//============ Commands intended for debugging ===================


/*Fills in the command in outBuff, also calculating the CRC. Everything after
  the crc is padded with 0xFF.
  inBuff and outBuff must be buffLen in size.
  bufLen must be at least 6.
*/
void SdmmcFillCommand(uint8_t * outBuff, uint8_t * inBuff, size_t buffLen, uint8_t cmd, uint32_t param);

/*Returns the index of R1 response in data. As this will be at least SDMMC_COMMAND_LEN from the data start,
  0 will indicate a failure.
*/
size_t SdmmcSDR1ResponseIndex(const uint8_t * data, size_t len);

/*Returns len in the case of an error.
  Otherwise the index in data with the data start pattern is returned.
*/
size_t SdmmcSeekDataStart(const uint8_t * data, size_t len);

/*Software reset.
  returns 0: ok, otherwise an error
*/
uint8_t SdmmcCheckCmd0(void);

/*If SDMMC_DEBUG is enabled, information about the supported voltage range are
  printed.
  returns 0: ok, otherwise an error
*/
uint8_t SdmmcCheckCmd8(void);

/*Sets the block length to 512 byte.
  returns 0: ok, otherwise an error
*/
uint8_t SdmmcCheckCmd16(void);

/*Inits the SD card (not MMC, these it will return unsupported).
  returns 0: ok, 1: idle, 2: unsupported, 3: error
*/
uint8_t SdmmcCheckAcmd41(void);

/*Reads the OCR register.
  If pIsSdHc is not NULL and the return value is 0, the variable is set to true.
  Therefore the parameter needs to be initialized to false before calling to be
  of any use.
  returns 0: ok, 1: busy (retry), 2: voltage range not fitting, 3: error
*/
uint8_t SdmmcCheckCmd58(bool * pIsSdHc);

/*Enables the CRC check of the card.
  returns 0: ok, otherwise an error
*/
uint8_t SdmmcCheckCmd59(void);
