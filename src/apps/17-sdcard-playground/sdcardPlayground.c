/*
(c) 2025 by Malte Marwedel

License: BSD-3-Clause
*/
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <ctype.h>

#include "boxlib/keys.h"
#include "boxlib/leds.h"
#include "boxlib/rs232debug.h"
#include "boxlib/flash.h"
#include "boxlib/clock.h"
#include "boxlib/coproc.h"
#include "boxlib/peripheral.h"
#include "boxlib/spiExternal.h"
#include "boxlib/spiExternalDma.h"
#include "boxlib/mcu.h"
#include "boxlib/readLine.h"
#include "boxlib/timer32Bit.h"
#include "ff.h"
#include "filesystem.h"
#include "json.h"
#include "main.h"
#include "powerControl.h"
#include "sdmmcAccess.h"
#include "utility.h"

#define CONFIG_FILENAME "/etc/sdcardplayground.json"

//Should be unique across all apps
#define BKREG_IDX 31

#define SD_CHIPSELECT 1
//48MHz allows 24MHz as maximum SPI frequency - close to the allowed maximum of 25MHz
#define F_CPU 48000000ULL

static uint32_t g_resetTime; //measured on power up
static uint32_t g_waitTime; //measured on power up
static uint32_t g_prescalerInit; //measured on power up
static uint32_t g_initClock8; //measured on power up
static uint32_t g_testArg = 1; //read from configuration or RTC backup register
static bool g_needPowercycle = true; //read from configuration
static bool g_runTest; //read from configuration
static uint32_t g_increment = 2; //read from configuration
static bool g_sdcardOn; //current state of SD card power supply
static bool g_dumpNextAnswer; //print the hex values of the next test cycle
static uint32_t g_timeLastPowerOff; //last time when power was turned off


//1 command byte, 4 parameter bytes, 1 CRC byte
#define SDMMC_COMMAND_LEN 6

/*By breaking up the transfer to multiple smaller DMA transfers, we can increase the speed
  by searching for data while the next block is already be transferred
*/
#define TRANSFER_BLOCK_SIZE 32
/*1 byte CMD index, 4 bytes CMD parameter, 1 byte CRC, up to 8 wait byte, 512 response bytes,
   but this might not be enough and we increment the value to multiple of CMD_BLOCK_TRANSFER_SIZE.
*/
#define TRANSFER_LEN 640

_Static_assert(TRANSFER_LEN % TRANSFER_BLOCK_SIZE == 0, "Please fix defines");

static uint8_t g_dataOut[TRANSFER_LEN];
static uint8_t g_dataIn[TRANSFER_LEN];

static uint32_t g_commandOutCached[2];
static uint32_t g_commandOutCachedArg; //if 0, its invalid, and we never try even args


void Help(void) {
	printf("\r\nSelect operation:\r\n");
	printf("0: Stop testing\r\n");
	printf("1: Set start arg\r\n");
	printf("2: Start testing\r\n");
	printf("3: Re-run calibration sequence\r\n");
	printf("4: Print current test settings\r\n");
	printf("5: Update configuration file\r\n");
	printf("6: Toggle powercycle\r\n");
	printf("7: Dump next test answer\r\n");
	printf("8: Set increment\r\n");
	printf("h: This text\r\n");
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
	return 0;
}

bool CalibrateSd(void) {
	//initializing SD/MMC cards require 100kHz - 400kHz
	g_prescalerInit = 256;  //so 187kHz @ 48MHz peripheral clock
	SafeSdCardOn();
	if (CardInit()) {
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
	if (CardInit()) {
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
			printf("Reseting for %ums was ignored\r\n", (unsigned int)middle);
		}
		if (cmd58 != 0) {
			printf("1. Reseting for %ums worked\r\n", (unsigned int)middle);
			HardReset(middle); //the check itself could otherwise fail the next init
			if (CardInit()) {
				printf("2. Reseting for %ums did not work\r\n", (unsigned int)middle);
				tNonworking = middle;
			} else {
				printf("2. Reseting for %ums worked\r\n", (unsigned int)middle);
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
		if (!CardInit()) {
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
	g_prescalerInit = prescaler * 2;
	HardReset(g_resetTime);
	SpiExternalPrescaler(g_prescalerInit);
	if (CardInit()) {
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
			printf("Reseting with %u detection clocks did not work\r\n", (unsigned int)initClocks*8);
			break;
		} else {
			uint32_t tStop = HAL_GetTick();
			printf("Reseting with %u detection clocks did work (%ums)\r\n", (unsigned int)initClocks*8, (unsigned int)(tStop - tStart));
		}
		CoprocWatchdogReset();
		initClocks--;
	}
	initClocks++;
	g_initClock8 = initClocks;
	return true;
}

void StartArgSet(void) {
	CoprocWatchdogReset();
	printf("Enter start argument for search (in hex)\r\n");
	char buffer[16] = {0};
	ReadSerialLine(buffer, sizeof(buffer));
	unsigned int arg;
	sscanf(buffer, "%x", &arg);
	printf("\r\n new arg = 0x%x\r\n", arg);
	g_testArg = arg;
	ClockBackupRegSet(BKREG_IDX, g_testArg);
}

void IncrementSet(void) {
	CoprocWatchdogReset();
	printf("Enter increment (must be at least 2 and multiple of 2)\r\n");
	char buffer[16] = {0};
	ReadSerialLine(buffer, sizeof(buffer));
	unsigned int arg;
	sscanf(buffer, "%u", &arg);
	printf("\r\n new increment = %u\r\n", arg);
	g_increment = arg;
}

bool WriteSuccess(const uint8_t * data, size_t dataLen) {
	char filename[32];
	snprintf(filename, sizeof(filename), "/logs/sd-found-0x%x.bin", (unsigned int)g_testArg);
	if (FilesystemWriteFile(filename, data, dataLen) != true) {
		Led2Red();
		return false;
	}
	return true;
}

bool WriteError(void) {
	char filename[32];
	snprintf(filename, sizeof(filename), "/logs/sd-error-0x%x.log", (unsigned int)g_testArg);
	const char * fail = "Failed to init";
	if (FilesystemWriteFile(filename, fail, strlen(fail)) != true) {
		Led2Red();
		return false;
	}
	return true;
}

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

#define CMD_TEST 56

#define SDMMC_R1_RESPONSE_RANGE 9

/*returns 0 if data have been found and the next param should be tried
  1 if the next param should be tried
  2 if the current command should continue to seek
  3 if an error occurred and therefore the seek should stop
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
			if (data[i] == 0xFE) {
				idxData = dataOffset + i + 1;
				//printf("idxData: %u, [%u]\r\n", idxData, i);
				break;
			}
		}
	}
	if ((idxData) && (dataFound == false)) {
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
		if (dataFound) {
			return 0;
		}
		return 1;
	}
	return 2;
}

/*Interesing data are written to a file and the 2. LED will be green.
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
		if (g_commandOutCachedArg == g_testArg) {
			//this should be faster than calculating the crc for the command
			memcpy((uint32_t *)g_dataOut, g_commandOutCached, sizeof(g_commandOutCached));
		} else {
			SdmmcFillCommand(g_dataOut, NULL, 6, CMD_TEST, g_testArg);
		}
		SpiExternalChipSelect(SD_CHIPSELECT, true);
		uint8_t res = 0;
		size_t transferred = 0;
		for (uint32_t i = 0; i < TRANSFER_LEN; i += TRANSFER_BLOCK_SIZE) {
			SpiExternalTransferBackground(g_dataOut + i, g_dataIn + i, TRANSFER_BLOCK_SIZE);
			transferred += TRANSFER_BLOCK_SIZE;
			if (i == 0) {
				//use the time of the transfer to prepare the next command, since the 1. transfer is still running
				g_commandOutCachedArg = g_testArg + g_increment;
				SdmmcFillCommand((uint8_t *)g_commandOutCached, NULL, sizeof(g_commandOutCached), CMD_TEST, g_commandOutCachedArg);
			} else {
				res = TestCycleAnalyze(g_dataIn + i - TRANSFER_BLOCK_SIZE, TRANSFER_BLOCK_SIZE, i - TRANSFER_BLOCK_SIZE);
				if (res != 2) {
					break;
				}
			}
		}
		SpiExternalTransferWaitDone();
		SpiExternalChipSelect(SD_CHIPSELECT, false);
		if (g_needPowercycle == true) {
			SafeSdCardOff();
		}
		if (res == 2) {
			res = TestCycleAnalyze(g_dataIn + transferred - TRANSFER_BLOCK_SIZE, TRANSFER_BLOCK_SIZE, transferred - TRANSFER_BLOCK_SIZE);
		}
		/*if (g_dumpNextAnswer)*/ {
			printf("Arg = 0x%x, transferred %u\r\n", (unsigned int)g_testArg, (unsigned int)transferred);
			PrintHex(g_dataIn, transferred);
			g_dumpNextAnswer = 0;
		}
		if (res == 0) {
			Led2Green();
			printf("Success with arg 0x%x\r\n", (unsigned int)g_testArg);
			if (WriteSuccess(g_dataIn, transferred) != true) {
				printf("Stop testing (1)\r\n");
				g_runTest = false;
				SafeSdCardOff();
			}
		}
		if (res == 3) {
			SafeSdCardOff();
			Led2Red();
			printf("No card answer with arg 0x%x\r\n", (unsigned int)g_testArg);
			PrintHex(g_dataIn, transferred);
			g_runTest = false;
			WriteError();
		}
		g_testArg += g_increment;
		ClockBackupRegSet(BKREG_IDX, g_testArg);
		Led1Off();
	} else {
		SafeSdCardOff();
		Led2Red();
		g_runTest = false;
		printf("Stop testing (3)\r\n");
		WriteError();
	}
}

void ExecReset(void) {
	printf("Reset selected\r\n");
	Rs232Flush();
	NVIC_SystemReset();
}

void LoadSettings(void) {
	uint8_t json[128] = {0};
	char value[16];
	size_t readLen;
	if (FilesystemReadFile(CONFIG_FILENAME, json, sizeof(json), &readLen)) {
		if (JsonValueGet(json, readLen, "autostart", value, sizeof(value))) {
			if (strcmp(value, "true") == 0) {
				printf("Autostart set\r\n");
				g_runTest = true;
			}
		}
		if (JsonValueGet(json, readLen, "powercycle", value, sizeof(value))) {
			if (strcmp(value, "true") == 0) {
				printf("Need power cycle set\r\n");
				g_needPowercycle = true;
			} else {
				g_needPowercycle = false;
			}
		}
		if (JsonValueGet(json, readLen, "arg", value, sizeof(value))) {
			unsigned int arg = 0;
			if ((sscanf(value, "0x%x", &arg) == 1) && (arg & 1)) {
				g_testArg = arg;
			}
		}
		if (JsonValueGet(json, readLen, "increment", value, sizeof(value))) {
			unsigned int arg = 0;
			if ((sscanf(value, "%u", &arg) == 1) && ((arg & 1) == 0) && (arg > 0)) {
				g_increment = arg;
			}
		}
	} else {
		printf("No config file %s found\r\n", CONFIG_FILENAME);
	}
	uint32_t bkReg = ClockBackupRegGet(BKREG_IDX);
	printf("Backup register arg: 0x%x\r\n", (unsigned int)bkReg);
	if ((bkReg & 1) && (bkReg != 0xFFFFFFFF) && (bkReg > g_testArg)) {
		printf("...valid\r\n");
		g_testArg = bkReg;
	}
	printf("Using arg 0x%x\r\n", (unsigned int)g_testArg);
}

void WriteSettings(void) {
	char json[128] = {0};
	snprintf(json, sizeof(json), "{\n  \"autostart\": \"%s\",\n  \"arg\": \"0x%x\",\n  \"powercycle\": \"%s\"\n  \"increment\": \"%u\"\n}\n",
	     g_runTest ? "true" : "false",
	     (unsigned int)g_testArg,
	     g_needPowercycle ? "true" : "false",
	     (unsigned int)g_increment);
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

void PrintSettings(void) {
	printf("Arg: 0x%x\r\n", (unsigned int)g_testArg);
	printf("Powercycle: %s\r\n", g_needPowercycle ? "true" : "false");
	printf("Increment: %u\r\n", (unsigned int)g_increment);
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
	memset(g_dataOut, 0xFF, TRANSFER_LEN);
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
		case '1': StartArgSet(); break;
		case '2': g_runTest = true; break;
		case '3': CalibrateSd(); break;
		case '4': PrintSettings(); break;
		case '5': WriteSettings(); break;
		case '6': ToggleNeedPowercycle(); break;
		case '7': g_dumpNextAnswer = true; break;
		case '8': IncrementSet(); break;
		case 'r': ExecReset(); break;
		case 'h': Help(); break;
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
