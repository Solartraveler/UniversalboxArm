/* SD card playground
(c) 2025 - 2026 by Malte Marwedel

License: BSD-3-Clause

Determines properies of SD cards not to be found with an USB card reader and
usually not found in any datasheet.
Unlike other cards or operating systems, this code only reads blocks which should be read (no speculative read in advanced).
Some cards have bad blocks and some card readers stop responding on the first bad block.
So if speculative reading hits a bad block the card appears non functional while another
reading method might succed in reading and rescuing most of the card content.
This program can help identifying if this is the case. (Rescue must be done by another tool).

-Determines the maximum SD card speed (in a power of 2 frequency up to 24MHz).
-Determines the minimum time needed power cycle a SD card.
-Determines the maximum initialization frequency (cards only need to support 400kHz).
-Checks if there is a delay needed between power on and initialization of the card.
-Checks if the card is working with less dummy clock cycles for init (specification require at least 72).

-Checks the behaviour of the card with any CMD and a parameter. ACMD is not supported.
-Can seek for a parameter where the card answers to be successful or to be failing.
  Example: Send a read single block to every block and stops if one fails - usually at the end of the designed card capacity.
-Estimates the time in clock cycles needed until a card answers.
  Example: Some old cards vary widely until data are delivered. More than 50ms have been observed.
  A second read appears to be a lot faster. Some devices might have trouble with cards needing so long for an answer.
-For long seeks, the current parameter is saved in an RTC backup register, so seeking can continue at the last position another time.
-Log files of found data or failed data are written to the internal filesystem.
-If needed a power cycle can be done after each tried command. This however limits the speed to a few tries per second.
-Optional reading a block can be done after each power cycle to check if the initialization is really successful.
-Answer data can be dumped.

Limits:
  -Commands needing a stop command, like read multiple or write multiple will not work properly.
  -Always 514 bytes of answer data are searched (512 bytes block size and 2 bytes CRC).
  -Write commands will only support writing 0xFF.
  -The logic requires the whole answer to be stored in the RAM, even if a lot of SPI transfers are.
   needed until data arrived. As result the maximum time for waiting for an answer is limited
   by the RAM and the selected maximum clock frequency. For maximum speed a large as possible
   maximum transfer length with a minimum SPI divider should be selected.
   So for a divider of 4 and a max transfer length of 51200, this results in a time limit of
   48MHz / 4 / 8 / 51200 = (1/29)s = 34ms. Within this time, the card needs to complete the transfer of
   9 bytes for accepting the command 1 start byte, 512 data bytes and 2 CRC bytes.

The readme.md provides a sample setup to read all blocks.

*/
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <ctype.h>

#include "boxlib/clock.h"
#include "boxlib/coproc.h"
#include "boxlib/flash.h"
#include "boxlib/keys.h"
#include "boxlib/leds.h"
#include "boxlib/mcu.h"
#include "boxlib/peripheral.h"
#include "boxlib/readLine.h"
#include "boxlib/rs232debug.h"
#include "boxlib/spiExternal.h"
#include "boxlib/spiExternalDma.h"
#include "boxlib/timer32Bit.h"
#include "ff.h"
#include "filesystem.h"
#include "json.h"
#include "main.h"
#include "powerControl.h"
#include "sdmmcAccess.h"
#include "utility.h"

#define CONFIG_FILENAME "/etc/sdcardplayground.json"

//Backup register index to store the current tried parameter. Should be unique across all apps.
#define BKREG_IDX 31

#define SD_CHIPSELECT 1

//48MHz allows 24MHz as maximum SPI frequency - close to the allowed maximum of 25MHz
#define F_CPU 48000000ULL

//Default command to test (read single block)
#define CMD_TEST 17

//Should be a block within the size of the SD card (and valid value for both SD and SDHC cards)
#define READ_BLOCK_CHECK 0

//1 command byte, 4 parameter bytes, 1 CRC byte
#define SDMMC_COMMAND_LEN 6

/*By breaking up the transfer to multiple smaller DMA transfers, we can increase the speed
  by searching for data while the next block is already be transferred
*/
#define TRANSFER_BLOCK_SIZE 32

/*1 byte CMD index, 4 bytes CMD parameter, 1 byte CRC, up to 8 wait byte, 512 response bytes,
   but this is not enough as the cards sometimes take time until the data is available.
   ~300 wait bytes until data is there has been observed. So maybe 1024 is enough.
   Must be multiple of CMD_BLOCK_TRANSFER_SIZE.
*/
#define TRANSFER_LEN_MIN 640
#define TRANSFER_LEN_MAX 51200

//printing is really slow...
#define PRINT_LIMIT 6400

static uint32_t g_resetTime; //measured on power up
static uint32_t g_waitTime; //measured on power up
static uint32_t g_prescalerInit; //measured on power up, limited by g_minDivder
static uint32_t g_initClock8; //measured on power up

static uint32_t g_cmdTest = CMD_TEST; //read from configuration
static uint32_t g_testArg = 1; //read from configuration or RTC backup register
static uint32_t g_increment = 2; //read from configuration
static uint32_t g_maxTransferLen = TRANSFER_LEN_MIN; //read from configuration
static uint32_t g_minDivider = 2; //read from configuration
static bool g_needPowercycle = true; //read from configuration
static bool g_runTest; //read from configuration
static bool g_checkReadWorks = true; //read from configuration
static bool g_stopOnFound = true; //read from configuration
static bool g_seekError = false; //read from configuration
static bool g_dumpAllData = false; //read from configuration
static bool g_requireData = true; //read from configuration

static bool g_sdcardOn; //current state of SD card power supply
static bool g_dumpNextAnswer; //print the hex values of the next test cycle
static uint32_t g_timeLastPowerOff; //last time when power was turned off

#if (TRANSFER_BLOCK_SIZE > 32)
#define TRANSFER_OUT_LEN_MAX TRANSFER_BLOCK_SIZE
#else
#define TRANSFER_OUT_LEN_MAX 32
#endif

static uint8_t g_dataOut[TRANSFER_OUT_LEN_MAX];
static uint8_t g_dataIn[TRANSFER_LEN_MAX];

/*Having one 4 byte entry for every possible dataOut would limit TRANSFER_LEN_MAX too much
  best would be a value 2^n for faster computation, but other values would be possible too.
*/
#define STATS_SCALE 4


#define STATS_ENTRIES (TRANSFER_LEN_MAX / STATS_SCALE)

static uint32_t g_waitingStats[STATS_ENTRIES];

/*CRC calculation takes some time. So while waiting for the first data to arrive,
  we prepare the data (command, arg, CRC) for the next argument in advanced.
*/
static uint32_t g_commandOutCached[2];
static uint32_t g_commandOutCachedArg; //if 0, its invalid and we need to calculate it for the first time

static_assert(sizeof(g_dataOut) >= sizeof(g_commandOutCached), "Increase g_dataOut size");

void Help(void) {
	printf("\r\nSelect operation:\r\n");
	printf("0: Stop testing\r\n");
	printf("1: Set command\r\n");
	printf("2: Set start arg\r\n");
	printf("3: Start testing\r\n");
	printf("4: Re-run calibration sequence\r\n");
	printf("5: Print current test settings\r\n");
	printf("6: Save test settings to file\r\n");
	printf("7: Toggle powercycle\r\n");
	printf("8: Dump next test answer\r\n");
	printf("9: Set increment\r\n");
	printf("a: Toggle check read\r\n");
	printf("b: Toggle stop on found\r\n");
	printf("c: Toggle found criteria\r\n");
	printf("d: Toggle dump all data\r\n");
	printf("e: Toggle require data\r\n");
	printf("f: Set max transfer length\r\n");
	printf("g: Print waiting statistics\r\n");
	printf("h: This text\r\n");
	printf("i: Set minimum SPI divider\r\n");
	printf("r: Reboot\r\n");
}

void SafeSdCardOn(void) {
	/*The order of the sequence is important, otherwise the input pins of the SD
	  card migh have a higher voltage than Vcc, which may violate absolute maximum
	  ratings.*/
	SdCardPowerOn();
	HAL_Delay(1);
	SpiExternalInit();
	SpiExternalPrescaler(g_prescalerInit);
	g_sdcardOn = true;
}

void SafeSdCardOff(void) {
	/*The order of the sequence is important, otherwise the input pins of the SD
	  card migh have a higher voltage than Vcc, which may violate absolute maximum
	  ratings.*/
	SpiExternalDeinit();
	HAL_Delay(1);
	g_timeLastPowerOff = HAL_GetTick();
	SdCardPowerOff();
	g_sdcardOn = false;
}

void HardReset(uint32_t duration) {
	SafeSdCardOff();
	HAL_Delay(duration);
	SafeSdCardOn();
}

static uint32_t CardInit(void) {
	return SdmmcInit(SpiExternalTransferDma, SD_CHIPSELECT);
}

/*This assumes the card is the same, so we do not need to setup the things
  the first CardInit already detected. Mostly a copy & paste from SdmmcInit.
*/
static uint32_t CardReinit(uint32_t startupDelay, uint32_t clocks8) {
	/*SD/MMC needs at least 74 clocks with DI and CS to be high to enter native operating mode,
	  but maybe the card works faster.
	*/
	uint8_t initArray[16]; //128 clocks :P
	memset(&initArray, 0xFF, sizeof(initArray));
	SpiExternalTransferDma(initArray, NULL, MIN(clocks8, sizeof(initArray)), 0, true);
	HAL_Delay(startupDelay);
	//CMD0 -> software reset to enter SPI mode
	if (SdmmcCheckCmd0() != 0) {
		printf("Error, card not in idle state\r\n");
		return 1;
	}
	/*At least the tested cards work without this init sequence, but this
	  might not be true for other cards. So enable if needed.
	*/
#if 0
	//CMD8 to determine working voltage range (and if it is a SDHC/SDXC card)
	if (SdmmcCheckCmd8() > 2) {
		return 1;
	}
	//CMD58 -> get status (the card is not ready now)
	if (SdmmcCheckCmd58(NULL) >= 2) {
		printf("Error, no proper voltage range or error, stopping init\r\n");
		return 3;
	}

	//CMD59 -> enable CRC checking
	if (SdmmcCheckCmd59() != 0) {
		printf("Error, could not enable CRC checksums\r\n");
		return 1;
	}
#endif
	//ACMD41 -> init the card SD/SDHC card and wait for the init to be done
	uint8_t result;
	for (uint32_t i = 0; i < 5000; i++) {
		result = SdmmcCheckAcmd41(true);
		if (result >= 2) {
			printf("Error, MMC cards are not supported\r\n");
			return 1; //MMC cards will stop here
		}
		if (result == 0) {
			break;
		}
		HAL_Delay(1);
	}
	if (result != 0) {
		printf("Error, card init did not complete\r\n");
		return 1;
	}
	//CMD58 -> get status (now the busy bit should be cleared and the report if it is a SD or SDHC card be valid)
	bool isSdhc = false;
	if (SdmmcCheckCmd58(&isSdhc) != 0) {
		printf("Error, card is still busy\r\n");
		return 1;
	}
	//set block length to be 512byte (for SD cards, SDHC and SDXC always use 512)
	if (isSdhc == false) {
		if (SdmmcCheckCmd16() != 0) {
			return 1;
		}
	}
	if (g_checkReadWorks) {
		uint8_t buffer[SDMMC_BLOCKSIZE];
		if (!SdmmcReadSingleBlock(buffer, READ_BLOCK_CHECK))
		{
			printf("Error, reading block %u failed\r\n", (unsigned int)READ_BLOCK_CHECK);
			return 1;
		}
	}
	return 0;
}

static bool CardInitWithReadCheck(void) {
	uint8_t buffer[SDMMC_BLOCKSIZE];
	if (CardInit()) {
		if (SdmmcReadSingleBlock(buffer, READ_BLOCK_CHECK)) {
			return true;
		}
	}
	return false;
}

bool CalibrateSd(void) {
	//initializing SD/MMC cards require 100kHz - 400kHz
	g_prescalerInit = 256;  //so 187kHz @ 48MHz peripheral clock
	SafeSdCardOn();
	if (CardInitWithReadCheck()) {
		printf("Error, could not initialize SD card (1)\r\n");
		return false;
	}
	printf("Card present\r\n");
	//1. check if powering down the card works
	HardReset(1000);
	uint8_t cmd58 = SdmmcCheckCmd58(NULL);
	if (cmd58 == 0) {
		printf("Error, could not hard reset SD card - code %u\r\n", (unsigned int)cmd58);
		return false;
	}
	HardReset(1000); //some cards might not answer if SdmmcCheckCmd58 is called before the init
	if (CardInitWithReadCheck()) {
		printf("Error, could not initialize SD card (2)\r\n");
		return false;
	}
	//2. Check minimal reset time
	uint32_t tWorking = 1000;
	uint32_t tNonworking = 0;
	while ((tWorking - 1) > tNonworking) {
		uint32_t middle = (tWorking - tNonworking) / 2 + tNonworking;
		HardReset(middle);
		cmd58 = SdmmcCheckCmd58(NULL);
		if (cmd58 == 0) {
			tNonworking = middle;
			printf("Resetting for %ums was ignored\r\n", (unsigned int)middle);
		}
		if (cmd58 != 0) {
			printf("1. Resetting for %ums worked\r\n", (unsigned int)middle);
			HardReset(middle); //the check itself could otherwise fail the next init
			if (CardInitWithReadCheck()) {
				printf("2. Resetting for %ums did not work\r\n", (unsigned int)middle);
				tNonworking = middle;
			} else {
				printf("2. Resetting for %ums worked\r\n", (unsigned int)middle);
				tWorking = middle;
			}
		}
		CoprocWatchdogReset();
	}
	g_resetTime = tWorking * 3 / 2;
	printf("Using reset time %ums\r\n", (unsigned int)g_resetTime);
	/*3. Check maximum initialization frequency. The spec only requies a maximum of 400kHz,
	  but maybe the card supports a faster init.
	*/
	uint32_t prescaler = 256;
	while (prescaler >= 2) {
		HardReset(g_resetTime);
		SpiExternalPrescaler(prescaler);
		uint32_t frequencykHz = F_CPU / prescaler / 1000;
		uint32_t tStart = HAL_GetTick();
		if (!CardInitWithReadCheck()) {
			uint32_t tStop = HAL_GetTick();
			uint32_t tDelta = tStop - tStart;
			printf("Card init works with prescaler %u (%ukHz), time %ums\r\n", (unsigned int)prescaler, (unsigned int)frequencykHz, (unsigned int)tDelta);
		} else {
			printf("Failed to init card with prescaler %u (%ukHz)\r\n", (unsigned int)prescaler, (unsigned int)frequencykHz);
			break;
		}
		prescaler /= 2;
		CoprocWatchdogReset();
	}
	g_prescalerInit = MAX(prescaler * 2, g_minDivider);
	HardReset(g_resetTime);
	SpiExternalPrescaler(g_prescalerInit);
	if (CardInitWithReadCheck()) {
		printf("Error, could not initialize SD card (3)\r\n");
		return false;
	}
	//4. Check minimum card init delay
	tWorking = 100;
	tNonworking = 0;
	while ((tWorking - 1) > tNonworking) {
		uint32_t middle = (tWorking - tNonworking) / 2 + tNonworking;
		HardReset(g_resetTime);
		uint32_t tStart = HAL_GetTick();
		if (CardReinit(middle, 16)) {
			printf("Reseting with a delay of %ums did not work\r\n", (unsigned int)middle);
			tNonworking = middle;
		} else {
			uint32_t tStop = HAL_GetTick();
			printf("Reseting with a delay of %ums worked (%ums)\r\n", (unsigned int)middle, (unsigned int)(tStop - tStart));
			tWorking = middle;
		}
		CoprocWatchdogReset();
	}
	g_waitTime = tWorking * 3 / 2;
	printf("Using startup wait time of %ums\r\n", (unsigned int)g_waitTime);
	HardReset(g_resetTime);
	if (CardReinit(g_waitTime, 16)) {
		printf("Error, could not initialize SD card (3)\r\n");
		return false;
	}
	//5. Check minimum dummy init cycles
	int32_t initClocks = 16; //actually bytes, so *8 to get the clock cycles
	while (initClocks >= 0) {
		HardReset(g_resetTime);
		uint32_t tStart = HAL_GetTick();
		if (CardReinit(g_waitTime, initClocks)) {
			printf("Reseting with %u detection clocks did not work\r\n", (unsigned int)initClocks * 8);
			break;
		} else {
			uint32_t tStop = HAL_GetTick();
			printf("Reseting with %u detection clocks did work (%ums)\r\n", (unsigned int)initClocks * 8, (unsigned int)(tStop - tStart));
		}
		CoprocWatchdogReset();
		initClocks--;
	}
	initClocks++;
	g_initClock8 = initClocks;
	return true;
}

bool WriteSuccess(const uint8_t * data, size_t dataLen) {
	char filename[64];
	snprintf(filename, sizeof(filename), "/logs/sd-found-CMD%u-0x%x.bin", (unsigned int)g_cmdTest, (unsigned int)g_testArg);
	if (FilesystemWriteFile(filename, data, dataLen) != true) {
		Led2Red();
		return false;
	}
	return true;
}

bool WriteError(const char * result) {
	char filename[64];
	snprintf(filename, sizeof(filename), "/logs/sd-error-CMD%u-0x%x.log", (unsigned int)g_cmdTest, (unsigned int)g_testArg);
	if (FilesystemWriteFile(filename, result, strlen(result)) != true) {
		Led2Red();
		return false;
	}
	return true;
}

/*Returns true if the data is not just 0x0 or 0xFF.
*/
static bool ContainsData(const uint8_t * data, size_t len) {
	bool contains0 = false;
	bool contains1 = false;
	while (len >= 8) {
		//ARM allows unaligned access to up to 4 byte. Not 8byte.
		uint32_t * p = (uint32_t *)data;
		if ((p[0] != 0xFFFFFFFF) && (p[0] != 0)) {
			return true;
		}
		if (p[0] == 0) {
			contains0 = true;
		}
		if (p[0] == 0xFFFFFFFF) {
			contains1 = true;
		}
		if ((p[1] != 0xFFFFFFFF) && (p[1] != 0)) {
			return true;
		}
		if (p[1] == 0) {
			contains0 = true;
		}
		if (p[1] == 0xFFFFFFFF) {
			contains1 = true;
		}
		len -= 8;
		data += 8;
	}
	for (size_t i = 0; i < len; i++) {
		if ((data[i] != 0xFF) && (data[i] != 0)) {
			return true;
		}
		if (data[i] == 0x0) {
			contains0 = true;
		}
		if (data[i] == 0xFF) {
			contains1 = true;
		}
	}
	if ((contains0 == true) && (contains1 == true)) {
		return true; //strange data, but its at least data...
	}
	return false;
}

#define SDMMC_R1_RESPONSE_RANGE 9

/*Call with dataOffset = 0 to restart the internal state machine.
  returns 0 if data have been found and the next param should be tried
  1 command not accepted, should try next command
  2 if the current command should continue to seek because data is not found or incomplete
  3 no command answer was found, and therefore the seek should stop
  4 if data start and end has been captured, but there is no useful data in the result, should try next command
  5 card reported out of range, try next command
  6 card reported ECC error, try next command
  7 card reported CC (card controller) error, try next command
  8 card reported some error, try next command
*/
static uint8_t TestCycleAnalyze(const uint8_t * data, size_t dataLen, size_t dataOffset) {
	static size_t idxR1 = 0;
	static size_t idxData = 0;
	static bool dataFound = false;
	if (dataOffset == 0) {
		//time to start a new seek
		idxR1 = 0;
		idxData = 0;
		dataFound = false;
	}
	if (idxR1 == 0) {
		if ((dataLen + dataOffset) > SDMMC_COMMAND_LEN) {
			size_t seekStart = 0;
			if (dataOffset < SDMMC_COMMAND_LEN) {
				seekStart += SDMMC_COMMAND_LEN - dataOffset;
			}
			for (size_t i = seekStart; i < dataLen; i++) {
				if ((dataOffset + i) > (SDMMC_COMMAND_LEN + SDMMC_R1_RESPONSE_RANGE)) {
					return 3; //no command answer found
				}
				if ((data[i] & 0x80) == 0) {
					idxR1 = dataOffset + i;
					//printf("idxR1: %u\r\n", idxR1);
					if (data[i] & 0x7F) {
						return 1; //illegal command or wrong state, try the next one
					}
					break;
				}
			}
		}
	}
	if ((idxR1) && (idxData == 0)) {
		size_t seekStart = 0;
		if (dataOffset < idxR1) {
			seekStart += idxR1 + 1 - dataOffset;
		}
		for (size_t i = seekStart; i < dataLen; i++) {
			/* 0xFE: Start block token
			   0xFC: Start of write token
			   0xFD: Stop transmission token
			   0x0X: Error token
			*/
			if ((data[i] < 0xFF) && (data[i] >= 0xFC)) {
				idxData = dataOffset + i + 1;
				uint32_t statsIdx = idxData / STATS_SCALE;
				if (statsIdx < STATS_ENTRIES) {
					g_waitingStats[statsIdx]++;
				}
				//printf("idxData: %u, [%u]\r\n", idxData, i);
				break;
			}
			if ((data[i] & 0xF0) == 0) {
				if (data[i] & 0x8) {
					return 5;
				}
				if (data[i] & 0x4) {
					return 6;
				}
				if (data[i] & 0x2) {
					return 7;
				}
				return 8;
			}
		}
	}
	//only look if the data contains more than 0x0 or 0xFF, does not change the control flow
	if ((g_requireData) && (idxData) && (dataFound == false)) {
		size_t seekStart = 0;
		if (dataOffset < idxData) {
			seekStart += idxData - dataOffset;
		}
		if (dataLen > seekStart) {
			size_t searched = dataOffset - idxData;
			if (searched < SDMMC_BLOCKSIZE) {
				size_t maxSearch = SDMMC_BLOCKSIZE - searched;
				size_t dataAvailable = dataLen - seekStart;
				size_t thisRound = MIN(maxSearch, dataAvailable);
				//printf("This round %u\r\n", thisRound);
				if (ContainsData(data + seekStart, thisRound)) {
					//printf("Data found\r\n");
					dataFound = true;
				}
			}
		}
	}
	if ((idxData) && ((dataLen + dataOffset) >= (idxData + SDMMC_BLOCKSIZE + 2))) { //+2 for the CRC
		//printf("transferred: %u, %u, %u = %u\r\n", idxData, dataLen, dataOffset, idxData + dataLen + dataOffset);
		if ((dataFound) || (g_requireData == false)) {
			return 0;
		}
		return 4;
	}
	return 2;
}

/*Interesting data are written to a file and the 2. LED will be green.
  If things will go wrong, the 2. LED will be red.
  During activity, the 1. LED will be yellow.
*/
void TestCycle(void) {
	bool sdWasOn = g_sdcardOn;
	if (sdWasOn == false) {
		uint32_t timePassed = HAL_GetTick() - g_timeLastPowerOff;
		if (timePassed < g_resetTime) {
			HAL_Delay(g_resetTime - timePassed);
		}
		SafeSdCardOn();
	}
	Led1Yellow();
	if ((sdWasOn == true) || (CardReinit(g_waitTime, g_initClock8) == 0)) {
		if ((g_commandOutCachedArg == g_testArg) && (g_commandOutCachedArg)) {
			//this should be faster than calculating the crc for the command
			memcpy((uint32_t *)g_dataOut, g_commandOutCached, sizeof(g_commandOutCached));
		} else {
			SdmmcFillCommand(g_dataOut, NULL, 6, g_cmdTest, g_testArg);
		}
		SpiExternalChipSelect(SD_CHIPSELECT, true);
		uint8_t res = 0;
		size_t transferred = 0;
		for (uint32_t i = 0; i < g_maxTransferLen; i += TRANSFER_BLOCK_SIZE) {
			if ((i + TRANSFER_BLOCK_SIZE) > sizeof(g_dataOut)) {
				SpiExternalTransferBackground(NULL, g_dataIn + i, TRANSFER_BLOCK_SIZE);
			} else {
				SpiExternalTransferBackground(g_dataOut + i, g_dataIn + i, TRANSFER_BLOCK_SIZE);
			}
			transferred += TRANSFER_BLOCK_SIZE;
			if (i == 0) {
				//use the time of the transfer to prepare the next command, since the 1. transfer is still running
				g_commandOutCachedArg = g_testArg + g_increment;
				SdmmcFillCommand((uint8_t *)g_commandOutCached, NULL, sizeof(g_commandOutCached), g_cmdTest, g_commandOutCachedArg);
			} else {
				res = TestCycleAnalyze(g_dataIn + i - TRANSFER_BLOCK_SIZE, TRANSFER_BLOCK_SIZE, i - TRANSFER_BLOCK_SIZE);
				if (res != 2) {
					break;
				}
			}
		}
		//Since we transfer the next block while analyzing the previous one, we always transfer one block more than needed
		SpiExternalTransferWaitDone();
		SpiExternalChipSelect(SD_CHIPSELECT, false);
		if (g_needPowercycle == true) {
			SafeSdCardOff();
		}
		if (res == 2) {
			res = TestCycleAnalyze(g_dataIn + transferred - TRANSFER_BLOCK_SIZE, TRANSFER_BLOCK_SIZE, transferred - TRANSFER_BLOCK_SIZE);
		}
		if ((g_dumpNextAnswer) || (g_dumpAllData)) {
			printf("Arg = 0x%x, transferred %u\r\n", (unsigned int)g_testArg, (unsigned int)transferred);
			PrintHex(g_dataIn, MIN(transferred, PRINT_LIMIT)); //otherwise the watchdog bites us while printing
			g_dumpNextAnswer = false;
		}
		if ((res == 0) && (g_seekError == false)) { //success
			Led2Green();
			printf("Success with arg 0x%x\r\n", (unsigned int)g_testArg);
			if ((WriteSuccess(g_dataIn, transferred) != true) || (g_stopOnFound)) {
				printf("Stop testing (1)\r\n");
				g_runTest = false;
				SafeSdCardOff();
			}
		}
		//1 = command failed, 3 = no answer
		if ((res != 0) && (g_seekError)) { //failed
			SafeSdCardOff();
			Led2Red();
			const char * result;;
			switch (res) {
				case 1: result = "Command rejected"; break;
				case 2:	result = "Incomplete data"; break;
				case 3: result = "No command answer"; break;
				case 4: result = "No useful data"; break;
				case 5: result = "Out of range"; break;
				case 6: result = "ECC error"; break;
				case 7: result = "CC error"; break;
				case 8: result = "Some error"; break;
				default: result = "?"; break;
			}
			printf("%s with arg 0x%x\r\n", result, (unsigned int)g_testArg);
			if (!g_dumpAllData) {
				PrintHex(g_dataIn, MIN(transferred, PRINT_LIMIT)); //otherwise the watchdog bites us while printing
			}
			WriteError(result);
			if (g_stopOnFound) {
				printf("Stop testing (2)\r\n");
				g_runTest = false;
				SafeSdCardOff();
			}
		}
		g_testArg += g_increment;
		ClockBackupRegSet(BKREG_IDX, g_testArg);
		Led1Off();
	} else {
		printf("Stop testing (3)\r\n");
		SafeSdCardOff();
		Led2Red();
		g_runTest = false;
		WriteError("No init");
	}
}

void ExecReset(void) {
	printf("Reset selected\r\n");
	CoprocWritePowermode(0);
	Rs232Flush();
	NVIC_SystemReset();
}

static bool LoadValueBool(const uint8_t* pJson, size_t jsonLen, const char * pKey, bool * pValueReturn) {
	char value[16];
	if (JsonValueGet(pJson, jsonLen, pKey, value, sizeof(value))) {
		if (strcmp(value, "true") == 0) {
			*pValueReturn = true;
			return true;
		} else if (strcmp(value, "false") == 0) {
			*pValueReturn = false;
			return true;
		} else {
			printf("Error, could not parse %s for %s\r\n", value, pKey);
		}
	} else {
		printf("Warning, no key %s\r\n", pKey);
	}
	return false;
}

static bool LoadValueUint32(const uint8_t* pJson, size_t jsonLen, const char * pKey, uint32_t * pValueReturn, uint32_t vMin, uint32_t vMax, uint32_t vModulo) {
	char value[16];
	if (JsonValueGet(pJson, jsonLen, pKey, value, sizeof(value))) {
		unsigned int arg = 0;
		uint32_t success = sscanf(value, "0x%x", &arg);
		if (success == 0) {
			success = sscanf(value, "%u", &arg);
		}
		if (success == 1) {
			if ((arg >= vMin) && (arg <= vMax) && ((arg % vModulo) == 0)) {
				*pValueReturn = arg;
				return true;
			} else {
				printf("Error, %u for %s out of range\r\n", arg, pKey);
			}
		} else {
			printf("Error, could not parse %s for %s\r\n", value, pKey);
		}
	} else {
		printf("Warning, no key %s\r\n", pKey);
	}
	return false;
}

void LoadSettings(void) {
	uint8_t json[512] = {0};
	size_t readLen;
	if (FilesystemReadFile(CONFIG_FILENAME, json, sizeof(json), &readLen)) {
		LoadValueBool(json, readLen, "autostart", &g_runTest);
		LoadValueUint32(json, readLen, "command", &g_cmdTest, 0, 255, 1);
		LoadValueUint32(json, readLen, "param", &g_testArg, 0, 0xFFFFFFFF, 1);
		LoadValueBool(json, readLen, "powercycle", &g_needPowercycle);
		LoadValueUint32(json, readLen, "increment", &g_increment, 0, 0xFFFFFFFF, 1);
		LoadValueBool(json, readLen, "checkread", &g_checkReadWorks);
		LoadValueBool(json, readLen, "stoponfound", &g_stopOnFound);
		LoadValueBool(json, readLen, "seekerror", &g_seekError);
		LoadValueBool(json, readLen, "dumpall", &g_dumpAllData);
		LoadValueBool(json, readLen, "requiredata", &g_requireData);
		LoadValueUint32(json, readLen, "maxtransfer", &g_maxTransferLen, TRANSFER_LEN_MIN, TRANSFER_LEN_MAX, TRANSFER_BLOCK_SIZE);
		LoadValueUint32(json, readLen, "mindivider", &g_minDivider, 2, 256, 2);
	} else {
		printf("No config file %s found\r\n", CONFIG_FILENAME);
	}
	uint32_t bkReg = ClockBackupRegGet(BKREG_IDX);
	printf("Backup register arg: 0x%x\r\n", (unsigned int)bkReg);
	if ((bkReg != 0xFFFFFFFF) && (bkReg > g_testArg)) {
		printf("...valid\r\n");
		g_testArg = bkReg;
	}
	printf("Using arg 0x%x\r\n", (unsigned int)g_testArg);
}

void WriteSettings(void) {
	char json[512] = {0};
	snprintf(json, sizeof(json),
"{\n\
  \"autostart\": \"%s\",\n\
  \"command\": \"0x%x\",\n\
  \"param\": \"0x%x\",\n\
  \"powercycle\": \"%s\",\n\
  \"increment\": \"%u\",\n\
  \"checkread\": \"%s\",\n\
  \"stoponfound\": \"%s\",\n\
  \"seekerror\": \"%s\",\n\
  \"dumpall\": \"%s\",\n\
  \"requiredata\": \"%s\",\n\
  \"maxtransfer\": \"%u\",\n\
  \"mindivider\": \"%u\"\n\
}\n",
	g_runTest ? "true" : "false",
	(unsigned int)g_cmdTest,
	(unsigned int)g_testArg,
	g_needPowercycle ? "true" : "false",
	(unsigned int)g_increment,
	g_checkReadWorks ? "true" : "false",
	g_stopOnFound ? "true" : "false",
	g_seekError ? "true" : "false",
	g_dumpAllData ? "true" : "false",
	g_requireData ? "true" : "false",
	(unsigned int)g_maxTransferLen,
	(unsigned int)g_minDivider
);
	if (FilesystemWriteEtcFile(CONFIG_FILENAME, json, strlen(json))) {
		printf("Saved to %s\r\n", CONFIG_FILENAME);
	} else {
		printf("Error, could not create file %s\r\n", CONFIG_FILENAME);
	}
}

void ToggleNeedPowercycle(void) {
	g_needPowercycle = !g_needPowercycle;
	printf("Powercycle: %s\r\n", g_needPowercycle ? "true" : "false");
}

void ToggleCheckRead(void) {
	g_checkReadWorks = !g_checkReadWorks;
	printf("Check read works: %s\r\n", g_checkReadWorks ? "true" : "false");
}

void ToggleStopOnFound(void) {
	g_stopOnFound = !g_stopOnFound;
	printf("Stop on found: %s\r\n", g_stopOnFound ? "true" : "false");
}

void ToggleSeekFor(void) {
	g_seekError = !g_seekError;
	printf("Seek for: %s\r\n",  g_seekError ? "failure" : "success");
}

void ToggleDumpAll(void) {
	g_dumpAllData = !g_dumpAllData;
	printf("Dump all data: %s\r\n",  g_seekError ? "true" : "false");
}

void ToggleRequireData(void) {
	g_requireData = !g_requireData;
	printf("Require data: %s\r\n",  g_requireData ? "true" : "false");
}

void CommandSet(void) {
	CoprocWatchdogReset();
	printf("Enter command for the search (in hex)\r\n");
	char buffer[16] = {0};
	ReadSerialLine(buffer, sizeof(buffer));
	unsigned int arg = 0;
	sscanf(buffer, "%x", &arg);
	printf("\r\n new command = 0x%x\r\n", arg);
	g_cmdTest = arg;
	g_commandOutCachedArg = 0;
}

void StartArgSet(void) {
	CoprocWatchdogReset();
	printf("Enter start parameter for search (in hex)\r\n");
	char buffer[16] = {0};
	ReadSerialLine(buffer, sizeof(buffer));
	unsigned int arg = 0;
	sscanf(buffer, "%x", &arg);
	printf("\r\n new param = 0x%x\r\n", arg);
	g_testArg = arg;
	ClockBackupRegSet(BKREG_IDX, g_testArg);
}

void IncrementSet(void) {
	CoprocWatchdogReset();
	printf("Enter parameter increment (must be at least 1)\r\n");
	char buffer[16] = {0};
	ReadSerialLine(buffer, sizeof(buffer));
	unsigned int arg = 0;
	sscanf(buffer, "%u", &arg);
	printf("\r\n new increment = %u\r\n", arg);
	g_increment = arg;
}

void MaxTransferSet(void) {
	CoprocWatchdogReset();
	printf("Maximum transfer bytes (must be multiple of %u, min %u, max %u)\r\n", TRANSFER_BLOCK_SIZE, TRANSFER_LEN_MIN, TRANSFER_LEN_MAX);
	char buffer[16] = {0};
	ReadSerialLine(buffer, sizeof(buffer));
	unsigned int arg = 0;
	sscanf(buffer, "%u", &arg);
	if ((arg >= TRANSFER_LEN_MIN) && (arg <= TRANSFER_LEN_MAX) && ((arg % TRANSFER_BLOCK_SIZE) == 0)) {
		printf("\r\n new max transfer = %u\r\n", arg);
		g_maxTransferLen = arg;
	} else {
		printf("Invalid value\r\n");
	}
}

void MinDividerSet(void) {
	CoprocWatchdogReset();
	printf("Minimum SPI divider set. Must be a 2^n value, min 2, max 256.\r\n");
	char buffer[16] = {0};
	ReadSerialLine(buffer, sizeof(buffer));
	unsigned int arg = 0;
	sscanf(buffer, "%u", &arg);
	if ((arg >= 2) && (arg <= 256)) {
		/*Its overwrites the current minimum, regardless of the measurement.
		  But when saved, and recalibration is run it will limit the determined working divider.
		  SpiExternalPrescaler will do the round-up to the next 2^n value.
		*/
		printf("\r\n new SPI divider = %u\r\n", arg);
		g_minDivider = arg;
		g_prescalerInit = arg;
		SpiExternalPrescaler(g_prescalerInit);
	}
}

void PrintSettings(void) {
	printf("Command: %u (0x%x)\r\n", (unsigned int)g_cmdTest, (unsigned int)g_cmdTest);
	printf("Arg: 0x%x\r\n", (unsigned int)g_testArg);
	printf("Powercycle: %s\r\n", g_needPowercycle ? "true" : "false");
	printf("Increment: %u\r\n", (unsigned int)g_increment);
	printf("Check read works: %s\r\n", g_checkReadWorks ? "true" : "false");
	printf("Stop on found: %s\r\n", g_stopOnFound ? "true" : "false");
	printf("Seek for: %s\r\n", g_seekError ? "failure" : "success");
	printf("Dump all data: %s\r\n", g_dumpAllData ? "true" : "false");
	printf("Require data: %s\r\n", g_requireData ? "true" : "false");
	printf("Max transfer bytes: %u\r\n", (unsigned int)g_maxTransferLen);
	printf("Min SPI divider: %u\r\n", (unsigned int)g_minDivider);
	printf("Current SPI divider: %u\r\n", (unsigned int)g_prescalerInit);
}

void PrintWaitingStats(void) {
	//first do the fast file write, so we have the data, should the watchdog bites during the large serial print
	FIL f;
	bool error = true;
	char filename[64];
	snprintf(filename, sizeof(filename), "/logs/stats-CMD%u-0x%x.log", (unsigned int)g_cmdTest, (unsigned int)g_testArg);
	if (FR_OK == f_open(&f, filename, FA_WRITE | FA_CREATE_ALWAYS)) {
		error = false;
		for (uint32_t i = 0; i < STATS_ENTRIES; i++) {
			if (g_waitingStats[i]) {
				char buffer[64];
				snprintf(buffer, sizeof(buffer), "%04u: %u\r\n", (unsigned int)i * STATS_SCALE, (unsigned int)g_waitingStats[i]);
				UINT written = 0;
				size_t len = strlen(buffer);
				FRESULT res = f_write(&f, buffer, len, &written);
				if ((res != FR_OK) || (written != len)) {
					error = true;
					break;
				}
			}
		}
		f_close(&f);
	}
	if (error) {
		printf("Error, failed to write stats to %s\r\n", filename);
	}
	printf("Transfers until data started:\r\n");
	for (uint32_t i = 0; i < STATS_ENTRIES; i++) {
		if (g_waitingStats[i]) {
			printf("%04u: %u\r\n", (unsigned int)i * STATS_SCALE, (unsigned int)g_waitingStats[i]);
		}
	}
}

void AppInit(void) {
	LedsInit();
	Led1Red();
	PeripheralPowerOff();
	McuClockToHsiPll(F_CPU, RCC_HCLK_DIV1);
	HAL_Delay(50);
	Rs232Init(); //includes PeripheralPowerOn
	ClockInit();
	printf("SD card playground %s\r\n", APPVERSION);
	CoprocInit();
	PeripheralInit();
	FlashEnable(16); //3MHz
	FilesystemMount();
	f_mkdir("/logs");
	LoadSettings();
	memset(g_dataOut, 0xFF, TRANSFER_OUT_LEN_MAX);
	Led1Yellow();
	if (!CalibrateSd()) {
		g_runTest = false; //never start if calibration fails
		Led1Red();
	} else {
		Led1Green();
	}
	CoprocWritePowermode(1);
	Help();
}

void AppCycle(void) {
	static uint32_t tests = 0;
	static uint32_t tLast = 0;
	char input = Rs232GetChar();
	if (input) {
		printf("%c", input);
	}
	switch (input) {
		case '0': g_runTest = false; Led1Green(); break;
		case '1': CommandSet(); break;
		case '2': StartArgSet(); break;
		case '3': g_runTest = true; memset(g_waitingStats, 0, sizeof(g_waitingStats)); break;
		case '4': CalibrateSd(); break;
		case '5': PrintSettings(); break;
		case '6': WriteSettings(); break;
		case '7': ToggleNeedPowercycle(); break;
		case '8': g_dumpNextAnswer = true; break;
		case '9': IncrementSet(); break;
		case 'a': ToggleCheckRead(); break;
		case 'b': ToggleStopOnFound(); break;
		case 'c': ToggleSeekFor(); break;
		case 'd': ToggleDumpAll(); break;
		case 'e': ToggleRequireData(); break;
		case 'f': MaxTransferSet(); break;
		case 'g': PrintWaitingStats(); break;
		case 'h': Help(); break;
		case 'i': MinDividerSet(); break;
		case 'r': ExecReset(); break;

		default: break;
	}
	uint32_t t = HAL_GetTick();
	uint32_t delta = t- tLast;
	if (delta > 5000) {
		CoprocWatchdogReset();
		CoprocWritePowerdown();
		tLast = t;
		if (tests) {
			float perSec = (float)tests * 1000.0f / (float)delta;
			uint32_t a = perSec;
			uint32_t b = perSec * 100.0f;
			b %= 100;
			printf("Testing %u.%02u/s arg = 0x%x\r\n", (unsigned int)a, (unsigned int)b, (unsigned int)g_testArg);
			tests = 0;
		}
	}
	if (g_runTest) {
		TestCycle();
		tests++;
	}
}
