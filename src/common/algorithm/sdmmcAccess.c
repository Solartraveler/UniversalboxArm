/* sdmmcAccess.c
(c) 2024 by Malte Marwedel

SPDX-License-Identifier: BSD-3-Clause

Supports:
MMC, SD, SDHC, SDXC read and write, protected with CRC checksums.
Maximum supported size should be 2TiB.
SPI interface.
The SPI frequency must be set before calling init and may be increased after
init has returned.
Thread safety must be provided by the caller.

Tested cards:
64MB MMC from Dual (as expected, it reports not supported when sending ACMD41)
512MB SD from ExtremeMemory
1GB Micro SD from Sandisk (sends one more wait bytes until CMD10 delivers data)
16GB SDHC from Platinum
64GB Micro SDXC from Samsung

By default only single block read and write is enabled.
Multiblock read can be enabled. But it does not increase the speed a lot and
just needs more program memory.

Multiblock write has not been added.

Changelog:
2024-07-13: Version 1.0
*/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "sdmmcAccess.h"

//should provide HAL_Delay()
#include "main.h"
#include "utility.h"

typedef struct {
	SpiTransferFunc_t * pSpi;
	uint8_t chipSelect;
	bool isInitialized;
	bool isSd; //if true, its a SD or SDHC card, not MMC card
	bool isSdhc; //if true, its a SDHC or SDXC card, not SD and not MMC
	uint32_t capacity; //in units of SDMMC_BLOCKSIZE
} sdmmcState_t;

static sdmmcState_t g_sdmmcState;

//enable to get debug messages
//#define SDMMC_DEBUG printf
#define SDMMC_DEBUGERROR printf
//#define SDMMC_DEBUGHEX PrintHex

#ifndef SDMMC_DEBUG
#define SDMMC_DEBUG(...)
#endif

#ifndef SDMMC_DEBUG
#define SDMMC_DEBUGERROR(...)
#endif

#ifndef SDMMC_DEBUGHEX
#define SDMMC_DEBUGHEX(...)
#endif

#define SDMMC_R1_ILLEGALCMD 0x4

//Suggested value, but any others might? be possible too
#define SDMMC_CHECKPATTERN 0xAA

//time in [100ms] to wait for card init
#define SDMMC_INIT_TIMEOUT 100

//time in [ms] to wait for data
#define SDMMC_TIMEOUT 1000

//1 command byte, 4 parameter bytes, 1 CRC byte
#define SDMMC_COMMAND_LEN 6
/*Up to 8 dummy bytes can be sent, then there should be at least the R1 response in the 9th byte
  Tested cards only seem to make use of one dummy byte
*/
#define SDMMC_R1_RESPONSE_RANGE 9


/*Fills in the command in outBuff, also calculating the CRC. Everything after
  the crc is padded with 0xFF.
  inBuff and outBuff must be buffLen in size.
  bufLen must be at least 6.
*/
void SdmmcFillCommand(uint8_t * outBuff, uint8_t * inBuff, size_t buffLen, uint8_t cmd, uint32_t param) {
	memset(inBuff, 0, buffLen);
	memset(outBuff, 0xFF, buffLen);
	outBuff[0] = 0x40 | cmd;
	outBuff[1] = param >> 24;
	outBuff[2] = param >> 16;
	outBuff[3] = param >> 8;
	outBuff[4] = param;
	uint8_t crc = 0;
	for (uint8_t i = 0; i < 5; i++) {
		uint8_t d = outBuff[i];
		for (uint8_t j = 0; j < 8; j++) {
			crc <<= 1;
			if ((d ^ crc) & 0x80) {
				crc ^= 0x09;
			}
			d <<= 1;
		}
	}
	outBuff[5] = (crc << 1) | 1;
}

/*See https://www.nongnu.org/avr-libc/user-manual/group__util__crc.html
  _crc_xmodem_update()
*/
uint16_t SdmmcDataCrc(const uint8_t * datablock) {
	uint16_t crc = 0;
	for (uint32_t i = 0; i < SDMMC_BLOCKSIZE; i++) {
		uint8_t d = datablock[i];
		crc = crc ^ ((uint16_t)d << 8);
		for (uint32_t j = 0; j < 8; j++) {
			if (crc & 0x8000) {
				crc = (crc << 1) ^ 0x1021;
			} else {
				crc <<= 1;
			}
		}
	}
	return crc;
}

/* Returns len in the case of an error. Otherwise the index in data with the response.
*/
static size_t SdmmcSeekSDR1Response(const uint8_t * data, size_t len) {
	for (size_t i = 0; i < len; i++) {
		if ((data[i] & 0x80) == 0) {
			uint8_t r1 = data[i];
			if (r1 & 0x1) { SDMMC_DEBUG("  Idle state\r\n"); }
			if (r1 & 0x2) { SDMMC_DEBUGERROR("  Erase reset\r\n"); }
			if (r1 & SDMMC_R1_ILLEGALCMD) { SDMMC_DEBUG("  Illegal command\r\n"); }
			if (r1 & 0x8) { SDMMC_DEBUGERROR("  Command crc error\r\n"); }
			if (r1 & 0x10) { SDMMC_DEBUGERROR("  Erase sequence error\r\n"); }
			if (r1 & 0x20) { SDMMC_DEBUGERROR("  Address error\r\n"); }
			if (r1 & 0x40) { SDMMC_DEBUGERROR("  Parameter error\r\n"); }
			return i;
		}
	}
	return len; //not found
}

size_t SdmmcSDR1ResponseIndex(const uint8_t * data, size_t len) {
	if (len < (SDMMC_COMMAND_LEN + SDMMC_R1_RESPONSE_RANGE)) {
		return 0;
	}
	size_t index = SdmmcSeekSDR1Response(data + SDMMC_COMMAND_LEN, SDMMC_R1_RESPONSE_RANGE);
	if (index == SDMMC_R1_RESPONSE_RANGE) {
		SDMMC_DEBUG("  No valid R1 response found\r\n");
		return 0; //error, not found
	}
	return index + SDMMC_COMMAND_LEN;
}

/*See http://problemkaputt.de/gbatek-dsi-sd-mmc-protocol-ocr-register-32bit-operation-conditions-register.htm
  pIsSdHc is set to true if it is detected. Init it to false
  returns 0: ok, 1: busy (retry), 2: voltage range not fitting, 3: error
*/
static uint32_t SdmmcSeekSDR3Response(const uint8_t * data, size_t len, bool *pIsSdHc) {
	uint8_t result = 3;
	size_t i = SdmmcSeekSDR1Response(data, len);
	if (i + 4 < len) {
		uint32_t ocr = (data[i + 1] << 24) | (data[i + 2] << 16) | (data[i + 3] << 8) | (data[i + 4]);
		if (ocr & 0x01000000) {
			SDMMC_DEBUG("  1.8V ok\r\n");
		}
		if (ocr & 0x00800000) {
			SDMMC_DEBUG("  3.5 - 3.6V ok\r\n");
		}
		if (ocr & 0x00400000) {
			SDMMC_DEBUG("  3.4 - 3.5V ok\r\n");
		}
		if (ocr & 0x00200000) {
			SDMMC_DEBUG("  3.3 - 3.4V ok\r\n");
		}
		if (ocr & 0x00100000) {
			SDMMC_DEBUG("  3.2 - 3.3V ok\r\n");
		}
		if (ocr & 0x00080000) {
			SDMMC_DEBUG("  3.1 - 3.2V ok\r\n");
		}
		if (ocr & 0x00040000) {
			SDMMC_DEBUG("  3.0 - 3.1V ok\r\n");
		}
		if (ocr & 0x00020000) {
			SDMMC_DEBUG("  2.9 - 3.0V ok\r\n");
		}
		if (ocr & 0x00010000) {
			SDMMC_DEBUG("  2.8 - 2.9V ok\r\n");
		}
		if (ocr & 0x00008000) {
			SDMMC_DEBUG("  2.7 - 2.8V ok\r\n");
		}
		if ((ocr & 0x003E0000) != 0x003E0000) { //2.9...3.4 V supported
			result = 2;
		}
		//this is only set when ACMD41 was sent before
		if (ocr & 0x80000000) {
			SDMMC_DEBUG("  ready\r\n");
			if (ocr & 0x40000000) {
				if (pIsSdHc) {
					*pIsSdHc = true;
				}
				SDMMC_DEBUG("    SDHC card\r\n");
			} else {
				SDMMC_DEBUG("    SD/MMC card\r\n");
			}
			if (result != 2) {
				result = 0;
			}
		} else {
			SDMMC_DEBUG("  not ready\r\n");
			if (result != 2) {
				result = 1;
			}
		}
	}
	return result;
}

//returns 0: ok, otherwise an error
static uint8_t SdmmcSeekSDR7Response(const uint8_t * data, size_t len) {
	if (len < (SDMMC_R1_RESPONSE_RANGE + 4)) {
		return 1;
	}
	size_t i = SdmmcSeekSDR1Response(data, SDMMC_R1_RESPONSE_RANGE);
	if (i >= SDMMC_R1_RESPONSE_RANGE) {
		return 1;
	}
	if (((data[i] & SDMMC_R1_ILLEGALCMD) == 0) && ((i + 4) < len)) {
		SDMMC_DEBUG("  -> SD version 2.0 or later\r\n");
		SDMMC_DEBUG("  Command set version %u\r\n", (unsigned int)(data[i + 1] >> 4));
		if (data[i + 4] == SDMMC_CHECKPATTERN) {
			SDMMC_DEBUG("  Check pattern ok\r\n");
		} else {
			SDMMC_DEBUG("  Check pattern error\r\n");
		}
		uint8_t voltageRange = data[i + 3];
		if (voltageRange == 0x1) {
			SDMMC_DEBUG("  2.7 - 3.3V accepted\r\n");
		} else {
			SDMMC_DEBUG("  unsuppored voltage!\r\n");
		}
	} else {
		SDMMC_DEBUG("  -> SD ver 1.0 or MMC card\r\n");
	}
	return 0;
}

/* Returns len in the case of an error. Otherwise the index in data with the data start pattern.
*/
size_t SdmmcSeekDataStart(const uint8_t * data, size_t len) {
	for (size_t i = 0; i < len; i++) {
		if (data[i] == 0xFE) {
			return i;
		}
	}
	return len;
}

//returns 0: ok, otherwise an error
static uint8_t SdmmcCheckCmd0(void) {
	//CMD0 to reset the card
	uint8_t dataOutCmd0[15]; //1 byte CMD index, 4 bytes CMD parameter, 1 byte CRC, up to 8 wait bytes, 1 response byte
	uint8_t dataInCmd0[15];
	SdmmcFillCommand(dataOutCmd0, dataInCmd0, sizeof(dataOutCmd0), 0, 0);
	g_sdmmcState.pSpi(dataOutCmd0, dataInCmd0, sizeof(dataOutCmd0), g_sdmmcState.chipSelect, true);
	SDMMC_DEBUG("Response from CMD0 (reset):\r\n");
	SDMMC_DEBUGHEX(dataInCmd0, sizeof(dataInCmd0));
	uint32_t idx = SdmmcSDR1ResponseIndex(dataInCmd0, sizeof(dataInCmd0));
	if ((idx == 0) || ((dataInCmd0[idx] & 0x81) != 0x01)) { //lowest bit must be set, otherwise the card is not idle
		return 1;
	}
	return 0;
}

//returns 0: ok, 1: idle, 2: unsupported, 3: error
static uint8_t SdmmcCheckCmd1(void) {
	//CMD1 to get the card to be initalized
	uint8_t dataOutCmd1[15]; //1 byte CMD index, 4 bytes CMD parameter, 1 byte CRC, up to 8 wait bytes, 1 response byte
	uint8_t dataInCmd1[15];
	SdmmcFillCommand(dataOutCmd1, dataInCmd1, sizeof(dataOutCmd1), 1, 0x0);
	g_sdmmcState.pSpi(dataOutCmd1, dataInCmd1, sizeof(dataOutCmd1), g_sdmmcState.chipSelect, true);
	SDMMC_DEBUG("Response from CMD1 (start init):\r\n");
	SDMMC_DEBUGHEX(dataInCmd1, sizeof(dataInCmd1));
	size_t idxR1 = SdmmcSDR1ResponseIndex(dataInCmd1, sizeof(dataInCmd1));
	if (idxR1 == 0) {
		return 3;
	}
	if (dataInCmd1[idxR1] & SDMMC_R1_ILLEGALCMD) {
		return 2; //illegal command
	}
	if (dataInCmd1[idxR1] & 0xFE) {
		return 3; //some other error
	}
	if (dataInCmd1[idxR1] & 1) {
		return 1; //idle
	}
	return 0; //ready
}

//returns 0: ok, otherwise an error
static uint8_t SdmmcCheckCmd8(void) {
	//CMD8 to determine working voltage range (and if it is a SDHC/SDXC card)
	uint8_t dataOutCmd8[19]; //1 byte CMD index, 4 bytes CMD parameter, 1 byte CRC, up to 8 wait bytes, 5 response bytes
	uint8_t dataInCmd8[19];
	SdmmcFillCommand(dataOutCmd8, dataInCmd8, sizeof(dataOutCmd8), 8, 0x100 | SDMMC_CHECKPATTERN); //0x100 -> 2.7 - 3.6 V range supported?, 0xAA -> check pattern
	g_sdmmcState.pSpi(dataOutCmd8, dataInCmd8, sizeof(dataOutCmd8), g_sdmmcState.chipSelect, true);
	SDMMC_DEBUG("Response from CMD8 (if cmd):\r\n");
	SDMMC_DEBUGHEX(dataInCmd8, sizeof(dataInCmd8));
	return SdmmcSeekSDR7Response(dataInCmd8 + SDMMC_COMMAND_LEN, sizeof(dataInCmd8) - SDMMC_COMMAND_LEN);
}

//returns 0: ok, otherwise an error
static uint8_t SdmmcCheckCmd9(uint32_t * capacity) {
	//CMD9: get card CSD for the size of the card
	uint8_t dataOutCmd9[31]; //1 byte CMD index, 4 bytes CMD parameter, 1 byte CRC, up to 8 wait bytes, 17 response bytes
	uint8_t dataInCmd9[31];
	SdmmcFillCommand(dataOutCmd9, dataInCmd9, sizeof(dataOutCmd9), 9, 0x0);
	g_sdmmcState.pSpi(dataOutCmd9, dataInCmd9, sizeof(dataInCmd9), g_sdmmcState.chipSelect, true);
	SDMMC_DEBUG("Response from CMD9 (card specific data):\r\n");
	SDMMC_DEBUGHEX(dataInCmd9, sizeof(dataInCmd9));
	size_t idxR1 = SdmmcSDR1ResponseIndex(dataInCmd9, sizeof(dataInCmd9));
	if (idxR1 == 0) {
		SDMMC_DEBUG("  No response found\r\n");
		return 1;
	}
	size_t index = idxR1 + 1;
	size_t bytesLeft = sizeof(dataInCmd9) - index;
	size_t dataStart = SdmmcSeekDataStart(dataInCmd9 + index, bytesLeft);
	if (dataStart >= bytesLeft) {
		SDMMC_DEBUG("  No data start found\r\n");
		return 1;
	}
	dataStart++;
	if ((bytesLeft - dataStart) < 15) {
		/*In theory there could be hundreds of 0xFF until data start, but only zero or one 0xFF has been
		  observed with the tested cards.
		*/
		SDMMC_DEBUG("  Data started too late... should increase number of bytes to read...\r\n");
		return 1;
	}
	index += dataStart;
	uint8_t csdVersion = 1; //MMC have the relevant bits set like version 1
	if (g_sdmmcState.isSd == true) {
		csdVersion = ((dataInCmd9[index + 0] & 0xC0) >> 6) + 1;
		SDMMC_DEBUG("  CSD version %u\r\n", csdVersion);
	}
	uint32_t csize = 0;
	uint32_t cmult = 0;
	uint32_t bllen = 0;
	if (csdVersion == 1) {
		//SD/MMC
		bllen = 1 << (dataInCmd9[index + 5] & 0x0F); //allowed range 512 ... 2048
		csize = ((dataInCmd9[index + 6] & 0x3) << 10) | (dataInCmd9[index + 7] << 2) | ((dataInCmd9[index + 8] & 0xC0) >> 6);
		cmult = ((dataInCmd9[index + 9] & 0x3) << 1) | ((dataInCmd9[index + 10] & 0x80) >> 7);
		cmult = 4 << cmult;
	} else if (csdVersion == 2) {
		//SDHC cards
		bllen = 512; //fixed :P
		cmult = 1024; //fixed :P (specs just says multiply (csize + 1) with 512KiB)
		csize = ((dataInCmd9[index + 7] & 0x3F) << 16) | (dataInCmd9[index + 8] << 8) | (dataInCmd9[index + 9]);
	}
	uint32_t blocks = (csize + 1) * cmult;
	uint64_t bytes = (uint64_t)blocks * (uint64_t)bllen;
	SDMMC_DEBUG("  csize %u, mult %u, bl_len %u -> %u blocks, %uMiB\r\n", (unsigned int)csize, (unsigned int)cmult, (unsigned int)bllen,
	       (unsigned int)blocks, (unsigned int)(bytes / (1024LLU * 1024LLU)));
	if (capacity) {
		*capacity = bytes / SDMMC_BLOCKSIZE;
	}
	return 0;
}

//returns 0: ok, otherwise an error
static uint8_t SdmmcCheckCmd16(void) {
	//set block length to be 512byte (for SD cards, SDHC and SDXC always use 512)
	uint8_t dataOutCmd16[15]; //1 byte CMD index, 4 bytes CMD parameter, 1 byte CRC, up to 8 wait bytes, 1 response byte
	uint8_t dataInCmd16[15];
	SdmmcFillCommand(dataOutCmd16, dataInCmd16, sizeof(dataOutCmd16), 16, SDMMC_BLOCKSIZE); //512 byte block length
	g_sdmmcState.pSpi(dataOutCmd16, dataInCmd16, sizeof(dataOutCmd16), g_sdmmcState.chipSelect, true);
	SDMMC_DEBUG("Response from CMD16 (set block length):\r\n");
	SDMMC_DEBUGHEX(dataInCmd16, sizeof(dataInCmd16));
	if (SdmmcSDR1ResponseIndex(dataInCmd16, sizeof(dataInCmd16)) == 0) {
		return 1;
	}
	return 0;
}

//returns 0: ok, 1: busy (retry), 2: voltage range not fitting, 3: error
static uint8_t SdmmcCheckCmd58(bool * pIsSdHc) {
	uint8_t dataOutCmd58[19]; //1 byte CMD index, 4 bytes CMD parameter, 1 byte CRC, up to 8 wait bytes, 5 response bytes
	uint8_t dataInCmd58[19];
	SdmmcFillCommand(dataOutCmd58, dataInCmd58, sizeof(dataOutCmd58), 58, 0);
	g_sdmmcState.pSpi(dataOutCmd58, dataInCmd58, sizeof(dataOutCmd58), g_sdmmcState.chipSelect, true);
	SDMMC_DEBUG("Response from CMD58 (get ocr):\r\n");
	SDMMC_DEBUGHEX(dataInCmd58, sizeof(dataInCmd58));
	return SdmmcSeekSDR3Response(dataInCmd58 + SDMMC_COMMAND_LEN, sizeof(dataInCmd58) - SDMMC_COMMAND_LEN, pIsSdHc);
}

//returns 0: ok, otherwise an error
static uint8_t SdmmcCheckCmd59(void) {
	//set block length to be 512byte (for SD cards, SDHC and SDXC always use 512)
	uint8_t dataOutCmd59[15]; //1 byte CMD index, 4 bytes CMD parameter, 1 byte CRC, up to 8 wait bytes, 1 response byte
	uint8_t dataInCmd59[15];
	SdmmcFillCommand(dataOutCmd59, dataInCmd59, sizeof(dataOutCmd59), 59, 1); //1 = CRC enabled
	g_sdmmcState.pSpi(dataOutCmd59, dataInCmd59, sizeof(dataOutCmd59), g_sdmmcState.chipSelect, true);
	SDMMC_DEBUG("Response from CMD59 (enable CRC):\r\n");
	SDMMC_DEBUGHEX(dataInCmd59, sizeof(dataInCmd59));
	if (SdmmcSDR1ResponseIndex(dataInCmd59, sizeof(dataInCmd59)) == 0) {
		return 1;
	}
	return 0;
}

//returns 0: ok, 1: idle, 2: unsupported, 3: error
static uint8_t SdmmcCheckAcmd41(void) {
	//CMD55 - application specific command follows
	uint8_t dataOutCmd55[15]; //1 byte CMD index, 4 bytes CMD parameter, 1 byte CRC, up to 8 wait bytes, 1 response byte
	uint8_t dataInCmd55[15];
	SdmmcFillCommand(dataOutCmd55, dataInCmd55, sizeof(dataOutCmd55), 55, 0);
	g_sdmmcState.pSpi(dataOutCmd55, dataInCmd55, sizeof(dataOutCmd55), g_sdmmcState.chipSelect, true);
	SDMMC_DEBUG("Response from CMD55 (app cmd):\r\n");
	SDMMC_DEBUGHEX(dataInCmd55, sizeof(dataInCmd55));
	size_t idxR1 = SdmmcSDR1ResponseIndex(dataInCmd55, sizeof(dataInCmd55));
	if (idxR1 == 0) {
		return 3;
	}
	if (dataInCmd55[idxR1] & SDMMC_R1_ILLEGALCMD) {
		return 2; //illegal command
	}
	//ACMD41 to get the card to be initalized
	uint8_t dataOutCmd41[15]; //1 byte CMD index, 4 bytes CMD parameter, 1 byte CRC, up to 8 wait bytes, 1 response byte
	uint8_t dataInCmd41[15];
	SdmmcFillCommand(dataOutCmd41, dataInCmd41, sizeof(dataOutCmd41), 41, 0x40000000); //param -> tell, we support SDHC
	g_sdmmcState.pSpi(dataOutCmd41, dataInCmd41, sizeof(dataOutCmd41), g_sdmmcState.chipSelect, true);
	SDMMC_DEBUG("Response from ACMD41 (start init):\r\n");
	SDMMC_DEBUGHEX(dataInCmd41, sizeof(dataInCmd41));
	idxR1 = SdmmcSDR1ResponseIndex(dataInCmd41, sizeof(dataInCmd41));
	if (idxR1 == 0) {
		return 3;
	}
	if (dataInCmd41[idxR1] & SDMMC_R1_ILLEGALCMD) {
		return 2; //illegal command
	}
	if (dataInCmd41[idxR1] & 0xFE) {
		return 3; //some other error
	}
	if (dataInCmd41[idxR1] & 1) {
		return 1; //idle
	}
	return 0; //ready
}

uint32_t SdmmcInit(SpiTransferFunc_t * pSpiTransfer, uint8_t chipSelect) {
	memset(&g_sdmmcState, 0, sizeof(sdmmcState_t));
	if (pSpiTransfer == NULL) {
		return 4;
	}
	g_sdmmcState.pSpi = pSpiTransfer;
	g_sdmmcState.chipSelect = chipSelect;
	//SD/MMC needs at least 74 clocks with DI and CS to be high to enter native operating mode
	uint8_t initArray[16]; //128 clocks :P
	memset(&initArray, 0xFF, sizeof(initArray));
	pSpiTransfer(initArray, NULL, sizeof(initArray), 0, true);
	HAL_Delay(100);
	//CMD0 -> software reset to enter SPI mode
	if (SdmmcCheckCmd0() != 0) {
		SDMMC_DEBUGERROR("Error, card not in idle state\r\n");
		return 1;
	}
	//CMD8 to determine working voltage range (and if it is a SDHC/SDXC card)
	if (SdmmcCheckCmd8() != 0) {
		return 1;
	}
	//CMD58 -> get status (the card is not ready now)
	if (SdmmcCheckCmd58(NULL) >= 2) {
		SDMMC_DEBUGERROR("Error, no proper voltage range or error, stopping init\r\n");
		return 3;
	}
	//CMD59 -> enable CRC checking
	if (SdmmcCheckCmd59() != 0) {
		return 1;
	}
	//ACMD41 -> init the card SD/SDHC card and wait for the init to be done
	uint8_t result;
	for (uint32_t i = 0; i < SDMMC_INIT_TIMEOUT; i++) {
		result = SdmmcCheckAcmd41();
		if (result >= 2) {
			break; //MMC cards will stop here
		}
		if (result == 0) {
			g_sdmmcState.isSd = true;
			break;
		}
		HAL_Delay(100);
	}
	if (result == 2) { //try init for MMC
		for (uint32_t i = 0; i < SDMMC_INIT_TIMEOUT; i++) {
			result = SdmmcCheckCmd1();
			if (result >= 2) {
				SDMMC_DEBUGERROR("Error, card init did not complete\r\n");
				return 1;
			}
			if (result == 0) {
				break;
			}
			HAL_Delay(100);
		}
		if (result == 1) {
			SDMMC_DEBUGERROR("Error, card init did not complete\r\n");
		}
	} else if (g_sdmmcState.isSd != true) {
		SDMMC_DEBUGERROR("Error, card init did not complete\r\n");
		return 1;
	}
	//CMD58 -> get status (now the busy bit should be cleared and the report if it is a SD or SDHC card be valid)
	if (SdmmcCheckCmd58(&g_sdmmcState.isSdhc) != 0) {
		SDMMC_DEBUGERROR("Error, card is still busy\r\n");
		return 1;
	}
	//set block length to be 512byte (for SD cards, SDHC and SDXC always use 512)
	if (SdmmcCheckCmd16() != 0) {
		return 1;
	}
	//CMD9: get card CSD for the size of the card
	if (SdmmcCheckCmd9(&g_sdmmcState.capacity) != 0) {
		return 1;
	}
	if (g_sdmmcState.capacity > 0) {
		g_sdmmcState.isInitialized = true;
		return 0;
	}
	return 1;
}


static void SdmmcDisableCs(void) {
	g_sdmmcState.pSpi(NULL, NULL, 0,  g_sdmmcState.chipSelect, true);
}

bool SdmmcReadSingleBlock(uint8_t * buffer, uint32_t block) {
	if ((g_sdmmcState.isInitialized == false) || (buffer == NULL) || (block > g_sdmmcState.capacity)) {
		SDMMC_DEBUGERROR("Error, invalid read request 0x%x, max 0x%x\r\n", (unsigned int)block, (unsigned int)g_sdmmcState.capacity);
		return false;
	}
	uint8_t * bufferStart = buffer;
	if (g_sdmmcState.isSdhc == false) {
		block *= SDMMC_BLOCKSIZE;
	}
	SDMMC_DEBUG("Read 0x%x\r\n", (unsigned int)block);
	uint8_t dataOutCmd17[16]; //1 byte CMD index, 4 bytes CMD parameter, 1 byte CRC, up to 8 wait byte, 1 response bytes, 1 extra byte for have a good buffer size
	uint8_t dataInCmd17[16];
	SdmmcFillCommand(dataOutCmd17, dataInCmd17, sizeof(dataOutCmd17), 17, block);
	size_t thisRound = sizeof(dataOutCmd17);
	g_sdmmcState.pSpi(dataOutCmd17, dataInCmd17, thisRound, g_sdmmcState.chipSelect, false);
	SDMMC_DEBUG("Response from CMD17 (read single block):\r\n");
	SDMMC_DEBUGHEX(dataInCmd17, sizeof(dataInCmd17));
	size_t idxR1 = SdmmcSDR1ResponseIndex(dataInCmd17, sizeof(dataInCmd17));
	if (idxR1 == 0) {
		SDMMC_DEBUGERROR("Error, no R1 response\r\n");
		SdmmcDisableCs();
		return false;
	}
	if (dataInCmd17[idxR1] != 0) {
		SdmmcDisableCs();
		return false;
	}
	size_t idx = idxR1 + 1;
	size_t bytesLeft = SDMMC_BLOCKSIZE;
	//1. do we already have some data we can search and copy?
	bool gotDataStart = false;
	size_t dataSearchRange = sizeof(dataInCmd17) - idx;
	size_t dStart = SdmmcSeekDataStart(dataInCmd17 + idx, dataSearchRange) + idx;
	if (dStart < sizeof(dataInCmd17)) {
		gotDataStart = true;
		size_t dData = dStart + 1;
		SDMMC_DEBUG("Start already present\r\n");
		if (dData < sizeof(dataInCmd17)) { //there are already data we need to copy
			size_t dLen = sizeof(dataInCmd17) - dData;
			SDMMC_DEBUG("Preserved bytes: %u\r\n", (unsigned int)dLen);
			memcpy(buffer, dataInCmd17 + dData, dLen);
			bytesLeft -= dLen;
			buffer += dLen;
		}
	}
	//2. no start? lets wait for the start
	if (gotDataStart == false) {
		for (uint32_t i = 0; i < SDMMC_TIMEOUT; i++) {
			uint8_t dataOut = 0xFF;
			uint8_t dataIn = 0;
			g_sdmmcState.pSpi(&dataOut, &dataIn, sizeof(dataIn), g_sdmmcState.chipSelect, false);
			SDMMC_DEBUGHEX(&dataIn, sizeof(dataIn));
			if (SdmmcSeekDataStart(&dataIn, sizeof(dataIn)) == 0) {
				SDMMC_DEBUG("Data start found after %u reads\r\n", (unsigned int)(i + 1));
				gotDataStart = true;
				break;
			}
			HAL_Delay(1);
		}
	}
	if (!gotDataStart) {
		SDMMC_DEBUGERROR("Error, no data start found\r\n");
		SdmmcDisableCs();
		return false;
	}
	//Copy rest of the data
	uint8_t outBuffer[64]; //we could use one huge transfer, but this would need 512 byte on the stack
	memset(outBuffer, 0xFF, sizeof(outBuffer));
	while (bytesLeft) {
		thisRound = MIN(bytesLeft, sizeof(outBuffer));
		g_sdmmcState.pSpi(outBuffer, buffer, thisRound, g_sdmmcState.chipSelect, false);
		SDMMC_DEBUGHEX(buffer, thisRound);
		bytesLeft -= thisRound;
		buffer += thisRound;
	}
	//Read CRC and disable chip select
	uint8_t crc[2];
	g_sdmmcState.pSpi(outBuffer, crc, sizeof(crc), g_sdmmcState.chipSelect, true);
	SDMMC_DEBUG("CRC:\r\n");
	SDMMC_DEBUGHEX(crc, sizeof(crc));
	uint16_t crcIs = SdmmcDataCrc(bufferStart);
	uint16_t crcShould = (crc[0] << 8) | crc[1];
	if (crcIs != crcShould) {
		SDMMC_DEBUGERROR("Error, CRC mismatch, should %x, is %x at block %x\r\n", (unsigned int)crcShould, (unsigned int)crcIs, (unsigned int)block);
		SDMMC_DEBUGHEX(bufferStart, SDMMC_BLOCKSIZE);
		return false;
	}
	return true;
}

//Enable to use multi block read command
#if 0

static bool SdmmcContainsTerminate(const uint8_t * buffer, size_t len) {
	for (size_t i = 0; i < len; i++) {
		if (buffer[i] == 0xFF) {
			return true;
		}
	}
	return false;
}

static bool SdmmcTerminateTransfer(void) {
	SDMMC_DEBUG("Terminating...\r\n");
	bool success = false;
	uint8_t dataOutCmd12[15]; //1 byte CMD index, 4 bytes CMD parameter, 1 byte CRC, 1-8 wait bytes, 1 response bytes
	uint8_t dataInCmd12[15];
	SdmmcFillCommand(dataOutCmd12, dataInCmd12, sizeof(dataOutCmd12), 12, 0x0);
	g_sdmmcState.pSpi(dataOutCmd12, dataInCmd12, sizeof(dataOutCmd12), g_sdmmcState.chipSelect, false);
	SDMMC_DEBUGHEX(dataInCmd12, sizeof(dataInCmd12));

	size_t index = SdmmcSeekSDR1Response(dataInCmd12 + SDMMC_COMMAND_LEN + 1, SDMMC_R1_RESPONSE_RANGE - 1);

	if (index < SDMMC_R1_RESPONSE_RANGE) {
		size_t searchStart = SDMMC_COMMAND_LEN + index + 1;
		if ((searchStart == sizeof(dataInCmd12)) || (SdmmcContainsTerminate(dataInCmd12 + searchStart, sizeof(dataInCmd12) - searchStart) == false)) {
			for (uint32_t i = 0; i < 500; i++) {
				uint8_t dataOut[8];
				memset(dataOut, 0xFF, sizeof(dataOut));
				uint8_t dataIn[8] = {0};
				g_sdmmcState.pSpi(dataOut, dataIn, sizeof(dataIn), g_sdmmcState.chipSelect, false);
				SDMMC_DEBUGHEX(dataIn, sizeof(dataIn));
				if (SdmmcContainsTerminate(dataIn, sizeof(dataIn))) {
					success = true;
					break;
				}
				HAL_Delay(1);
			}
		} else {
			success = true;
		}
		if (!success) {
			SDMMC_DEBUGERROR("Error, card stays busy\r\n");
		}
	} else {
		SDMMC_DEBUGERROR("Error, no response found\r\n");
	}
	if (!success) {
		SDMMC_DEBUGHEX(dataInCmd12, sizeof(dataInCmd12));
	}
	g_sdmmcState.pSpi(NULL, NULL, 0, g_sdmmcState.chipSelect, true);
	return success;
}

static bool SdmmcReadBlockComplete(const uint8_t * startBytes, size_t startLen, uint8_t * outBlock) {
	size_t bytesLeft = SDMMC_BLOCKSIZE;
	uint8_t * bufferStart = outBlock;
	//1. do we already have some data we can search and copy?
	bool gotDataStart = false;
	printf("Seek in\r\n");
	SDMMC_DEBUGHEX(startBytes, startLen);
	size_t dStart = SdmmcSeekDataStart(startBytes, startLen);
	if (dStart < startLen) {
		gotDataStart = true;
		size_t dData = dStart + 1;
		SDMMC_DEBUG("Start already present\r\n");
		if (dData < startLen) { //there are already data we need to copy
			size_t dLen = startLen - dData;
			SDMMC_DEBUG("Preserved bytes: %u\r\n", (unsigned int)dLen);
			memcpy(outBlock, startBytes + dData, dLen);
			bytesLeft -= dLen;
			outBlock += dLen;
		}
	}
	//2. no start? lets wait for the start
	if (gotDataStart == false) {
		for (uint32_t i = 0; i < SDMMC_TIMEOUT; i++) {
			uint8_t dataOut = 0xFF;
			uint8_t dataIn = 0;
			g_sdmmcState.pSpi(&dataOut, &dataIn, sizeof(dataIn), g_sdmmcState.chipSelect, false);
			SDMMC_DEBUGHEX(&dataIn, sizeof(dataIn));
			if (SdmmcSeekDataStart(&dataIn, sizeof(dataIn)) == 0) {
				SDMMC_DEBUG("Data start found after %u reads\r\n", (unsigned int)(i + 1));
				gotDataStart = true;
				break;
			}
			HAL_Delay(1);
		}
	}
	if (!gotDataStart) {
		SDMMC_DEBUGERROR("Error, no data start found\r\n");
		return false;
	}
	//Copy rest of the data
	uint8_t outBuffer[64]; //we could use one huge transfer, but this would need 512 byte on the stack
	memset(outBuffer, 0xFF, sizeof(outBuffer));
	while (bytesLeft) {
		size_t thisRound = MIN(bytesLeft, sizeof(outBuffer));
		g_sdmmcState.pSpi(outBuffer, outBlock, thisRound, g_sdmmcState.chipSelect, false);
		SDMMC_DEBUGHEX(outBlock, thisRound);
		bytesLeft -= thisRound;
		outBlock += thisRound;
	}
	//Read CRC and compare
	uint8_t crc[2];
	g_sdmmcState.pSpi(outBuffer, crc, sizeof(crc), g_sdmmcState.chipSelect, false);
	SDMMC_DEBUG("CRC:\r\n");
	SDMMC_DEBUGHEX(crc, sizeof(crc));
	uint16_t crcIs = SdmmcDataCrc(bufferStart);
	uint16_t crcShould = (crc[0] << 8) | crc[1];
	if (crcIs != crcShould) {
		SDMMC_DEBUGERROR("Error, CRC mismatch, should %x, is %x\r\n", (unsigned int)crcShould, (unsigned int)crcIs);
		return false;
	}
	return true;
}


bool SdmmcRead(uint8_t * buffer, uint32_t block, uint32_t blockNum) {
	if ((g_sdmmcState.isInitialized == false) || (buffer == NULL) || (blockNum == 0) ||
	    (block > g_sdmmcState.capacity) || ((block + blockNum) > g_sdmmcState.capacity)) {
		SDMMC_DEBUGERROR("Error, invalid read request 0x%x, blocks %u, max 0x%x\r\n", (unsigned int)block, (unsigned int)blockNum, (unsigned int)g_sdmmcState.capacity);
		return false;
	}
	if (g_sdmmcState.isSdhc == false) {
		block *= SDMMC_BLOCKSIZE;
	}
	SDMMC_DEBUG("Read 0x%x, blocks %u\r\n", (unsigned int)block, (unsigned int)blockNum);
	bool success = false;
	uint8_t dataOutCmd18[16]; //1 byte CMD index, 4 bytes CMD parameter, 1 byte CRC, up to 8 wait bytes, 1 response bytes, 1 extra byte for have a good buffer size
	uint8_t dataInCmd18[16];
	SdmmcFillCommand(dataOutCmd18, dataInCmd18, sizeof(dataOutCmd18), 18, block);
	g_sdmmcState.pSpi(dataOutCmd18, dataInCmd18, sizeof(dataOutCmd18), g_sdmmcState.chipSelect, false);
	SDMMC_DEBUG("Response from CMD18 (read multiple blocks):\r\n");
	SDMMC_DEBUGHEX(dataInCmd18, sizeof(dataInCmd18));
	size_t idxR1 = SdmmcSDR1ResponseIndex(dataInCmd18, sizeof(dataInCmd18));
	if (idxR1 > 0) {
		size_t idx = idxR1 + 1;
		size_t preservedBytes = sizeof(dataInCmd18) - idx;
		SDMMC_DEBUG("Preserved bytes: %u\r\n", preservedBytes);
		for (uint32_t i = 0; i < blockNum; i++) {
			if (!SdmmcReadBlockComplete(dataInCmd18 + idx, preservedBytes, buffer + i * SDMMC_BLOCKSIZE)) {
				break;
			}
			SDMMC_DEBUG("Block %u\r\n", (unsigned int)i);
			//SDMMC_DEBUGHEX(buffer + i * SDMMC_BLOCKSIZE, SDMMC_BLOCKSIZE);
			if (i == blockNum - 1) {
				success = true;
			}
			preservedBytes = 0;
		}
	}
	//terminate the transfer
	success &= SdmmcTerminateTransfer();
	return success;
}


#else

//Simply use single block read instead

bool SdmmcRead(uint8_t * buffer, uint32_t block, uint32_t blockNum) {
	if (blockNum == 0) {
		SDMMC_DEBUGERROR("Error, invalid read request 0x%x, blocks %u, max 0x%x\r\n", (unsigned int)block, (unsigned int)blockNum, (unsigned int)g_sdmmcState.capacity);
		return false;
	}
	bool success = true;
	for (uint32_t i = 0; i < blockNum; i++) {
		success &= SdmmcReadSingleBlock(buffer + i * SDMMC_BLOCKSIZE, block + i);
		if (!success) {
			break;
		}
	}
	return success;
}

#endif

bool SdmmcWriteSingleBlock(const uint8_t * buffer, uint32_t block) {
	if ((g_sdmmcState.isInitialized == false) || (buffer == NULL) || (block > g_sdmmcState.capacity)) {
		SDMMC_DEBUGERROR("Error, invalid write request 0x%x, max 0x%x\r\n", (unsigned int)block, (unsigned int)g_sdmmcState.capacity);
		return false;
	}
	if (g_sdmmcState.isSdhc == false) {
		block *= SDMMC_BLOCKSIZE;
	}
	SDMMC_DEBUG("Write 0x%x\r\n", (unsigned int)block);
	uint8_t dataOutCmd24[15]; //1 byte CMD index, 4 bytes CMD parameter, 1 byte CRC, up to 8 wait byte, 1 response bytes,
	uint8_t dataInCmd24[15];
	SdmmcFillCommand(dataOutCmd24, dataInCmd24, sizeof(dataOutCmd24), 24, block);
	size_t thisRound = sizeof(dataOutCmd24);
	g_sdmmcState.pSpi(dataOutCmd24, dataInCmd24, thisRound, g_sdmmcState.chipSelect, false);

	SDMMC_DEBUGHEX(dataInCmd24, sizeof(dataInCmd24));
	size_t idxR1 = SdmmcSDR1ResponseIndex(dataInCmd24, sizeof(dataInCmd24));
	if (idxR1 == 0) {
		SDMMC_DEBUGERROR("Error, no R1 response\r\n");
		SdmmcDisableCs();
		return false;
	}
	if (dataInCmd24[idxR1] != 0x0) {
		SdmmcDisableCs();
		return false;
	}
	uint16_t crc = SdmmcDataCrc(buffer);
	uint8_t dataStart[2] = {0xFF, 0xFE};
	g_sdmmcState.pSpi(dataStart, NULL, sizeof(dataStart), g_sdmmcState.chipSelect, false);
	g_sdmmcState.pSpi(buffer, NULL, SDMMC_BLOCKSIZE,  g_sdmmcState.chipSelect, false);
	uint8_t terminate[3];
	terminate[0] = crc >> 8;
	terminate[1] = crc;
	terminate[2] = 0xFF;
	uint8_t response[3];
	g_sdmmcState.pSpi(terminate, response, sizeof(terminate),  g_sdmmcState.chipSelect, false);
	SDMMC_DEBUG("Data response: 0x%x\r\n", response[2]);
	if ((response[2] & 0x1F ) != 0x5) {
		SDMMC_DEBUGERROR("Error, data rejected\r\n");
		SdmmcDisableCs();
		return false;
	}
	//ok, now wait until the card indicates its not busy writing anymore
	bool success = false;
	for (uint32_t i = 0; i < SDMMC_TIMEOUT; i++) {
		uint8_t dataOut = 0xFF;
		uint8_t dataIn = 0;
		g_sdmmcState.pSpi(&dataOut, &dataIn, sizeof(dataIn), g_sdmmcState.chipSelect, false);
		if (dataIn == 0xFF) {
			SDMMC_DEBUG("Writing took %u cycles\r\n", (unsigned int)i);
			success = true;
			break;
		}
		HAL_Delay(1);
	}
	SdmmcDisableCs();
	return success;
}

bool SdmmcWrite(const uint8_t * buffer, uint32_t block, uint32_t blockNum) {
	if (blockNum == 0) {
		SDMMC_DEBUGERROR("Error, invalid write request 0x%x, blocks %u, max 0x%x\r\n", (unsigned int)block, (unsigned int)blockNum, (unsigned int)g_sdmmcState.capacity);
		return false;
	}
	bool success = true;
	for (uint32_t i = 0; i < blockNum; i++) {
		success &= SdmmcWriteSingleBlock(buffer + i * SDMMC_BLOCKSIZE, block + i);
		if (!success) {
			break;
		}
	}
	return success;
}

uint32_t SdmmcCapacity(void) {
	return g_sdmmcState.capacity;
}