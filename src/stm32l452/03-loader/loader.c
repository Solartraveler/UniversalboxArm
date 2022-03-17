/* Loader
(c) 2022 by Malte Marwedel

SPDX-License-Identifier: GPL-3.0-or-later
*/

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <ctype.h>
#include <stdlib.h>

#include "loader.h"

#include "boxlib/keys.h"
#include "boxlib/leds.h"
#include "boxlib/rs232debug.h"
#include "boxlib/lcd.h"
#include "boxlib/flash.h"
#include "boxlib/coproc.h"
#include "boxlib/peripheral.h"
#include "boxlib/simpleadc.h"
#include "boxlib/boxusb.h"
#include "boxlib/mcu.h"

#include "md5.h"

#include "main.h"

#include "usbd_core.h"
#include "usb_std.h"

#include "ff.h"

#include "utility.h"
#include "tarextract.h"
#include "jsmn.h"

#define USB_STRING_MANUF 1
#define USB_STRING_PRODUCT 2
#define USB_STRING_SERIAL 3
#define USB_STRING_TARGET 4
#define USB_STRING_EXTTARGET 5

/* The PID used here is reserved for general test purpose.
See: https://pid.codes/1209/
*/
uint8_t g_deviceDescriptor[] = {
	0x12,       //length of this struct
	0x01,       //always 1
	0x00,0x01,  //usb version
	0x0,        //device class
	0x0,        //subclass
	0x0,        //device protocol
	USB_MAX_PACKET_SIZE, //maximum packet size
	0x09,0x12,  //vid
	0x03,0x00,  //pid
	0x00,0x01,  //revision
	USB_STRING_MANUF,   //manufacturer index
	USB_STRING_PRODUCT, //product name index
	USB_STRING_SERIAL,  //serial number index
	0x01        //number of configurations
};

uint8_t g_DeviceConfiguration[] = {
	9,     //length of this entry
	0x2,   //device configuration
	45, 0, //total length of this struct
	0x1,   //number of interfaces
	0x1,   //this config
	0x0,   //descriptor of this config index, not used
	0x80, //bus powered
	25,   //50mA
	//DFU interface descriptor for copy to RAM
	9,    //length
	0x04, //interface descriptor
	0x00, //interface number
	0x00, //alternate setting
	0x00, //no other endpoints
	0xFE, //application specific class
	0x01, //device firmware upgrade code
	0x02, //DFU mode protocol
	USB_STRING_TARGET,
	//DFU functional
	9,    //length
	0x21, //dfu functional descriptor
	0x07, //no detach, communicate after mainfestation, support download + upload
	0xFF, 0x00, //255ms timeout

/* Number of bytes per write before requesting the state. Using more byts speeds things up.
   But the number must always be smaller than USB_BUFFERSIZE_BYTES - 8.
*/
	0x00, 0x08, //2048 byte
	0x01, 0x01, //DFU version 1.1
	//DFU interface descriptor descriptor for copy to RAM and then to external flash
	9,    //length
	0x04, //interface descriptor
	0x00, //interface number
	0x01, //alternate setting
	0x00, //no other endpoints
	0xFE, //application specific class
	0x01, //device firmware upgrade code
	0x02, //DFU mode protocol
	USB_STRING_EXTTARGET,
	//DFU functional descriptor
	9,    //length
	0x21, //dfu functional descriptor
	0x07, //no detach, communicate after mainfestation, support download + upload
	0xFF, 0x00, //255ms timeout
	0x00, 0x08, //2048 byte
	0x01, 0x01  //DFU version 1.1
};

/* The dfu-util can not load to address 0. So we give an offset as workaround
   the same offset must be encoded in the g_target_desc, g_exttarget_desc and
   dfu-upload-ram.sh scripts
*/

#define DFU_FAKEOFFSET 0x1000

static struct usb_string_descriptor g_lang_desc     = USB_ARRAY_DESC(USB_LANGID_ENG_US);
static struct usb_string_descriptor g_manuf_desc_en = USB_STRING_DESC("marwedels.de");
static struct usb_string_descriptor g_prod_desc_en  = USB_STRING_DESC("UniversalboxARM");
static struct usb_string_descriptor g_serial_desc   = USB_STRING_DESC("Loader");

//the g encodes: read + write + eraseable
static struct usb_string_descriptor g_target_desc   = USB_STRING_DESC("@Internal RAM/0x1000/0064*0002Kg");
static struct usb_string_descriptor g_exttarget_desc   = USB_STRING_DESC("@External flash/0x1000/0064*0002Kg");

usbd_device g_usbDev;

bool g_usbEnabled;

#define DFU_STATUS_OK 0

//We can't write te memory - main thread is processing the data
#define DFU_STATUS_WRITE 3
//address out of range
#define DFU_STATUS_ADDRESS 8


#define DFU_STATE_DFUIDLE 2
#define DFU_STATE_DNBUSY 4
#define DFU_STATE_DOWNLOAD_IDLE 5
#define DFU_STATE_MANIFEST_SYNC 6
#define DFU_STATE_MANIFEST 7
#define DFU_STATE_ERROR 10

typedef struct {
	uintptr_t address;
	uintptr_t highestAddress;
	uint32_t blockId;
	uint32_t state;
	uint32_t status; //error status
	uint8_t target; //0 = only RAM, 1 = copy to external flash

	//accessed by the main thread for printing progress
	volatile uint32_t bytesDownloaded;

	//should the program be executed? Sets by the ISR, cleared by the main thread
	bool commStartProgram;

	/* Set and cleared by the main thread. If true, the ISR may not update the comm
	   variables and the transfer RAM. */
	bool commMainProcessing;
	uint32_t commFileSize;
	bool commToFlash; //should the file be stored in the flash?
	//set by the interrupt, cleared by the main thread
	bool commTransferDone;

} dfuState_t;

dfuState_t g_dfuState = {0, 0, 0, DFU_STATE_DFUIDLE, DFU_STATUS_OK,  0, false};

FATFS g_fatfs;



//================== code for USB DFU ==============

void UsbIrqOnEnter(void) {
	Led1Red();
}

void UsbIrqOnLeave(void) {
	Led1Off();
}

static usbd_respond usbGetDesc(usbd_ctlreq *req, void **address, uint16_t *length) {
	const uint8_t dtype = req->wValue >> 8;
	const uint8_t dnumber = req->wValue & 0xFF;
	void* desc = NULL;
	uint16_t len = 0;
	usbd_respond result = usbd_fail;
	//printfNowait("des: %x-%x-%x-%x\r\n", req->bmRequestType, req->bRequest, req->wValue, req->wIndex);
	switch (dtype) {
		case USB_DTYPE_DEVICE:
			desc = g_deviceDescriptor;
			len = sizeof(g_deviceDescriptor);
			result = usbd_ack;
			break;
		case USB_DTYPE_CONFIGURATION:
			desc = g_DeviceConfiguration;
			len = sizeof(g_DeviceConfiguration);
			result = usbd_ack;
			break;
		case USB_DTYPE_STRING:
			if (dnumber <= USB_STRING_EXTTARGET) {
				struct usb_string_descriptor * pStringDescr = NULL;
				if (dnumber == 0) {
					pStringDescr = &g_lang_desc;
				}
				if (dnumber == USB_STRING_MANUF) {
					pStringDescr = &g_manuf_desc_en;
				}
				if (dnumber == USB_STRING_PRODUCT) {
					pStringDescr = &g_prod_desc_en;
				}
				if (dnumber == USB_STRING_SERIAL) {
					pStringDescr = &g_serial_desc;
				}
				if (dnumber == USB_STRING_TARGET) {
					pStringDescr = &g_target_desc;
				}
				if (dnumber == USB_STRING_EXTTARGET) {
					pStringDescr = &g_exttarget_desc;
				}
				desc = pStringDescr;
				len = pStringDescr->bLength;
				result = usbd_ack;
			}
			break;
	}
	*address = desc;
	*length = len;
	return result;
}

static usbd_respond usbSetConf(usbd_device *dev, uint8_t cfg) {
	usbd_respond result = usbd_fail;
	switch (cfg) {
		case 0:
			//deconfig
			break;
		case 1:
			//set config
			result = usbd_ack;
			break;
	}
	return result;
}

//out will be always 6 bytes in length
void DfuGetStatus(uint8_t * out) {
	out[0] = g_dfuState.status; //dfu error code
	out[1] = 1; //can poll every 10ms (LSBs)
	out[2] = 0; //poll timeout MSBs
	out[3] = 0; //poll timeout HSBs
	out[4] = g_dfuState.state; //state after this command
	out[5] = 0; //status descriptor index
	//more or less the call of GetStatus should execute the current state, so
	//the call des the transition to the next state.
	if (g_dfuState.state == DFU_STATE_DNBUSY) {
		g_dfuState.state = DFU_STATE_DFUIDLE;
	}
	if (g_dfuState.state == DFU_STATE_MANIFEST_SYNC) {
		g_dfuState.state = DFU_STATE_MANIFEST;
		g_dfuState.commStartProgram = true;
	}
}

void DfuGetState(uint8_t * out) {
	out[0] = g_dfuState.state;
}

void DfuClearState(void) {
	g_dfuState.address = 0;
	g_dfuState.highestAddress = 0;
	g_dfuState.bytesDownloaded = 0;
	g_dfuState.blockId = 0;
	g_dfuState.state = DFU_STATE_DFUIDLE;
	g_dfuState.status = DFU_STATUS_OK;
}

void DfuTransferEnd(void) {
	g_dfuState.bytesDownloaded = 0; //only for progress shown.
	if ((g_dfuState.highestAddress) && (g_dfuState.commMainProcessing == false)) {
		g_dfuState.commFileSize = g_dfuState.highestAddress;
		if (g_dfuState.target == 1) {
			g_dfuState.commToFlash = true;
		} else {
			g_dfuState.commToFlash = false;
		}
		__sync_synchronize();
		g_dfuState.commTransferDone = true;
		__sync_synchronize();
	}
}

void DfuDownloadBlock(const uint8_t * data, size_t dataLen, uint32_t wValue) {
	size_t ramSize = g_DfuMemSize;
	bool accept = false;

	if (wValue > 0) {
		if (g_dfuState.state == DFU_STATE_DFUIDLE) {
			g_dfuState.blockId = wValue;
			g_dfuState.state = DFU_STATE_DOWNLOAD_IDLE;
			accept = true;
		}
		if (g_dfuState.state == DFU_STATE_DOWNLOAD_IDLE) {
			if ((g_dfuState.blockId + 1) == wValue) {
				g_dfuState.blockId = wValue;
				accept = true;
			}
		}
		if (accept) {
			if (dataLen == 0) {//end of file
				//printfNowait("Eof\r\n");
				g_dfuState.state = DFU_STATE_MANIFEST_SYNC;
			} else {
				if ((g_dfuState.address + dataLen) <= ramSize) {
					//printfNowait("Data\r\n");
					if (g_dfuState.commMainProcessing == false) {
						memcpy(g_DfuMem + g_dfuState.address, data, dataLen);
						g_dfuState.address += dataLen;
						g_dfuState.bytesDownloaded += dataLen;
						g_dfuState.highestAddress = MAX(g_dfuState.address, g_dfuState.highestAddress);
					} else {
						g_dfuState.state = DFU_STATE_ERROR;
						g_dfuState.status = DFU_STATUS_ADDRESS;
					}
				} else {
					g_dfuState.state = DFU_STATE_ERROR;
					g_dfuState.status = DFU_STATUS_WRITE;
				}
			}
		}
	} else if (dataLen > 0) {
		if (data[0] == 0x41) { //erase page
			//printfNowait("erase\r\n");
			g_dfuState.state = DFU_STATE_DNBUSY; //we are not interested in the state, we just need to support it for dfu-util
		}
		if ((data[0] == 0x21) && (dataLen == 5)) { //set address
			uint32_t address = (data[4] << 24) | (data[3] << 16) | (data[2] << 8) | data[1];
			address -= DFU_FAKEOFFSET;
			//printfNowait("Set address %x\r\n", address);
			if (address < ramSize) {
				g_dfuState.address = address;
				g_dfuState.state = DFU_STATE_DNBUSY;
			} else {
				g_dfuState.state = DFU_STATE_ERROR;
				g_dfuState.status = DFU_STATUS_ADDRESS;
			}
		}
		if (data[0] == 0x91) { //read unprotected. Not called by dfu-upload
			printfNowait("read unprotected\r\n");
		}
	}
}

static usbd_respond usbControl(usbd_device *dev, usbd_ctlreq *req, usbd_rqc_callback *callback) {
	//Printing can be done here as long it is buffered. Otherwise it might be too slow
	if (req->wIndex == 0) { //the interface
		if (req->bmRequestType == 0xA1) { //DFU device -> host
			if (req->bRequest == 2) { //upload
				//Not implemented
				//return usbd_ack;
			}
			if ((req->bRequest == 3) && (req->wLength == 6)) { //get status
				//printfNowait("status\r\n");
				DfuGetStatus(req->data);
				return usbd_ack;
			}
			if ((req->bRequest == 5) && (req->wLength == 1)) { //get state
				//printfNowait("state\r\n");
				DfuGetState(req->data);
				return usbd_ack;
			}
		}
		if (req->bmRequestType == 0x21) { //DFU host -> device
			if ((req->bRequest == 0) && (req->wLength == 0)) { //detach -> start the application
				//printfNowait("detach\r\n");
			}
			if (req->bRequest == 1) { //download
				DfuDownloadBlock(req->data, req->wLength, req->wValue);
				return usbd_ack;
			}
			if ((req->bRequest == 4) && (req->wLength == 0)) { //clear status
				//by default, this is never called by dfu-util
				//printfNowait("clrState\r\n");
				DfuClearState();
				return usbd_ack;
			}
			if ((req->bRequest == 6) && (req->wLength == 0)) { //abort - this also marks the end of the transfer
				//printfNowait("abort\r\n");
				DfuTransferEnd();
				DfuClearState();
				return usbd_ack;
			}
		}
	}
	if ((req->bmRequestType == 0x01) && (req->bRequest == 0x0B) && (req->wLength == 0)) { //set interface
		if (req->wValue < 2) {
			g_dfuState.target = req->wValue;
			return usbd_ack;
		}
	}
	if ((req->bmRequestType == 0x81) && (req->bRequest == 0x0A) && (req->wValue == 0) && (req->wLength == 1)) { //get interface
		req->data[0] = g_dfuState.target;
		return usbd_ack;
	}

	if ((req->bmRequestType != 0x80) && (req->bRequest != 0x6)) { //filter usbGetDesc resquests
		//printfNowait("req: %x-%x-%x-%x-%x\r\n", req->bmRequestType, req->bRequest, req->wValue, req->wIndex, req->wLength);
	}

	return usbd_fail;
}

void mainMenu(void) {
	printf("\r\nSelect operation:\r\n");
	printf("l: List flash content\r\n");
	printf("h: This screen\r\n");
	printf("r: Reboot with reset controller\r\n");
	printf("s: Jump to DFU bootloader\r\n");
	printf("u: Toggle USB device\r\n");
}

void printHex(const uint8_t * data, size_t len) {
	for (uint32_t i = 0; i < len; i++) {
		printf("%02X ", data[i]);
		if (((i % 8) == 7) || (i == len -1)) {
			printf("\r\n");
		}
	}
}

void loaderInit(void) {
	Led1Red();
	PeripheralPowerOff();
	HAL_Delay(500);
	PeripheralPowerOn();
	Rs232Init();
	printf("Loader 0.2\r\n");
	FlashEnable(64); //250kHz
	FRESULT fres = f_mount(&g_fatfs, "0", 1);
	if (fres == FR_NO_FILESYSTEM) {
		printf("No filesystem, formatting...\r\n");
		uint32_t buffer[512];
		fres = f_mkfs ("0", NULL, &buffer, sizeof(buffer));
		if (fres == FR_OK) {
			printf("Formatting done\r\n");
			fres = f_mount(&g_fatfs, "0", 1); //now mount
		} else {
			printf("Error, formatting returned %u\r\n", (unsigned int)fres);
		}
	}
	if (fres == FR_OK) {
		DWORD freeclusters;
		FATFS * pff = NULL;
		if (f_getfree("0", &freeclusters, &pff) == FR_OK) {
			uint32_t clustersize = pff->csize;
			uint32_t freesectors = freeclusters * clustersize;
			uint32_t totalsectors = (pff->n_fatent - 2) * clustersize;
			uint32_t freekib = freesectors * FF_MIN_SS / 1024;
			uint32_t totalkib = totalsectors * FF_MIN_SS / 1024;
			printf("Sectors: %u(%uKiB). Free: %u(%uKiB). Cluster size: %u.\r\n",
			       (unsigned int)totalsectors, (unsigned int)totalkib,
			       (unsigned int)freesectors,  (unsigned int)freekib, (unsigned int)clustersize);
		}
	} else {
		printf("Error, mounting returned %u\r\n", (unsigned int)fres);
	}
	FILINFO fi;
	if (f_stat("/bin", &fi) == FR_NO_FILE) {
		f_mkdir("/bin");
	}
	//LcdEnable();
	//LcdBacklightOn();
	//LcdInit(ST7735);
	//LcdInit(ILI9341);
	printf("\r\nStarting USB\r\n");
	int32_t result = UsbStart(&g_usbDev, &usbSetConf, &usbControl, &usbGetDesc);
	if (result == -1) {
		printf("Error, failed to start 48MHz clock. Error: %u\r\n", (unsigned int)result);
	}
	if (result == -2) {
		printf("Error, failed to set USB clock source\r\n");
	}
	if (result >= 0) {
		g_usbEnabled = true;
	}
}

void listFiles(const char * path) {
	DIR d;
	FILINFO fi;
	printf("Files of %s\r\n", path);
	if (f_opendir(&d, path) == FR_OK) {
		while (f_readdir(&d, &fi) == FR_OK) {
			if (fi.fname[0]) {
				printf("%c %8u %s\r\n", (fi.fattrib & AM_DIR) ? 'd' : ' ', (unsigned int)fi.fsize, fi.fname);
			} else {
				break;
			}
		}
		f_closedir(&d);
	}
}

void listStorage(void) {
	listFiles("/");
	listFiles("/bin");
}

void readSerialLine(char * input, size_t len) {
	memset(input, 0, len);
	size_t i = 0;
	while (i < (len - 1)) {
		char c = Rs232GetChar();
		if (c != 0) {
			input[i] = c;
			i++;
			printf("%c", c);
		}
		if ((c == '\r') || (c == '\n'))
		{
			break;
		}
	}
}

void PrepareOtherProgam(void) {
	Led2Off();
	//first all peripheral clocks should be disabled
	UsbStop(); //stops USB clock
	AdcStop(); //stops ADC clock
	Rs232Flush();
	PeripheralPowerOff(); //stops SPI2 clock, also RS232 level converter will stop
	Rs232Stop(); //stops UART1 clock
}

void JumpDfu(void) {
	uintptr_t dfuStart = ROM_BOOTLOADER_START_ADDRESS;
	Led1Green();
	printf("\r\nDirectly jump to the DFU bootloader\r\n");
	volatile uintptr_t * pProgramStart = (uintptr_t *)(dfuStart + 4);
	printf("Program start will be at 0x%x\r\n", (unsigned int)(*pProgramStart));
	McuLockCriticalPins();
	PrepareOtherProgam();
	McuStartOtherProgram((void *)dfuStart, true); //usually does not return
}

bool JsonValueGet(uint8_t * jsonStart, size_t jsonLen, const char * key, char * valueOut, size_t valueMax) {
	jsmn_parser p;
	jsmntok_t t[32];
	jsmn_init(&p);
	int elems = jsmn_parse(&p, (char *)jsonStart, jsonLen, t, sizeof(t) / sizeof(t[0]));
	if (elems < 0) {
		printf("Error, parsing json failed. Code: %i\r\n", elems);
		return false;
	}
	size_t keyLen = strlen(key);
	for (int i = 1; i < elems; i += 2) {
		size_t elemLen = t[i].end - t[i].start;
		if ((t[i].type == JSMN_STRING) && (elemLen == keyLen)) {
			if (memcmp(jsonStart + t[i].start, key, keyLen) == 0) {
				size_t valueLen = t[i + 1].end - t[i + 1].start;
				if (valueLen < valueMax) {
					memcpy(valueOut, jsonStart + t[i + 1].start, valueLen);
					valueOut[valueLen] = '\0';
					return true;
				}
			}
		}
	}
	return false;
}

bool MetaNameGet(uint8_t * jsonStart, size_t jsonLen, char * nameOut, size_t nameMax) {
	return JsonValueGet(jsonStart, jsonLen, "name", nameOut, nameMax);
}

bool MetaMcuGet(uint8_t * jsonStart, size_t jsonLen, char * mcuOut, size_t mcuMax) {
	return JsonValueGet(jsonStart, jsonLen, "mcu", mcuOut, mcuMax);
}

bool MetaWatchdogGet(uint8_t * jsonStart, size_t jsonLen, uint16_t * watchdogOut) {
	char buffer[16];
	bool success = JsonValueGet(jsonStart, jsonLen, "watchdog", buffer, sizeof(buffer));
	if (success) {
		*watchdogOut = atol(buffer);
		return true;
	}
	return false;
}

bool MetaProgramStartGet(uint8_t * jsonStart, size_t jsonLen, uintptr_t * programStart) {
	char addr[20] = {0};
	if (JsonValueGet(jsonStart, jsonLen, "appaddr", addr, sizeof(addr))) {
		unsigned int x = 0;
		sscanf(addr, "0x%x", &x);
		*programStart = x;
		return true;
	}
	return false;
}

bool MetaMd5Get(uint8_t * jsonStart, size_t jsonLen, uint8_t * checksumOut) {
	char checksum[33] = {0};
	if (JsonValueGet(jsonStart, jsonLen, "md5sum", checksum, sizeof(checksum))) {
		for (uint32_t i = 0; i < 16; i++) {
			char tmp[3] = {0};
			tmp[0] = checksum[i * 2];
			tmp[1] = checksum[i * 2 + 1];
			unsigned int out = 0;
			sscanf(tmp, "%x", &out);
			checksumOut[i] = out;
		}
		return true;
	}
	return false;
}

bool TarNameGet(uint8_t * tarStart, size_t tarLen, char * nameOut, size_t nameMax) {
	uint8_t * metaStart;
	size_t metaLen;
	if (TarFileStartGet("metadata.json", tarStart, tarLen, &metaStart, &metaLen, NULL)) {
		return JsonValueGet(metaStart, metaLen, "name", nameOut, nameMax);
	}
	return false;
}

void ProgTarStart(void * tarStart, size_t tarLen) {
	uint8_t * fileStart;
	size_t fileLen;
	uint8_t * metaStart;
	size_t metaLen;
	uint16_t watchdog = 0;
	uintptr_t programStart = 0;
	if (TarFileStartGet("metadata.json", tarStart, tarLen, &metaStart, &metaLen, NULL)) {
		if (!MetaWatchdogGet(metaStart, metaLen, &watchdog)) {
			printf("Warning, no watchdog value present\r\n");
		}
		if (!MetaProgramStartGet(metaStart, metaLen, &programStart)) {
			printf("Warning, no program start address present\r\n");
		}
	} else {
		printf("Warning, no metadata present\r\n");
	}
	if (TarFileStartGet("application.bin", tarStart, tarLen, &fileStart, &fileLen, NULL)) {
		if (fileLen <= g_DfuMemSize) {
			void * ramStart = g_DfuMem;
			if ((programStart >= (uintptr_t)g_DfuMem) && ((programStart + fileLen) <= ((uintptr_t)g_DfuMem+ g_DfuMemSize))) {
				ramStart = (void *)programStart;
			} else {
				printf("Warning, address 0x%x out of bounds\r\n", (unsigned int)programStart);
			}
			memmove(ramStart, fileStart, fileLen);
			uint8_t md5sum[16] = {0};
			md5(ramStart, fileLen, md5sum);
			printf("Starting program with size %u. Md5sum:\r\n", (unsigned int)fileLen);
			printHex(md5sum, sizeof(md5sum));
			Led1Green();
			if (watchdog) {
				CoprocWatchdogCtrl(watchdog);
				printf("Watchdog enabled. Timeout %ums\r\n", (unsigned int)watchdog);
			}
			volatile uint32_t * pProgramStart = (uint32_t *)((uintptr_t)ramStart + 0x4);
			printf("Program start will be at 0x%x\r\n", (unsigned int)(*pProgramStart));
			PrepareOtherProgam();
			McuStartOtherProgram(ramStart, true); //usually does not return
		}
	}
	printf("Error, program start failed\r\n");
}

bool ProgTarCheck(void * tarStart, size_t tarLen) {
	uint8_t * fileStart;
	size_t fileLen = 0;
	uint8_t * metaStart;
	size_t metaLen;
	if (!TarFileStartGet("application.bin", tarStart, tarLen, &fileStart, &fileLen, NULL)) {
		printf("Error, no application found\r\n");
		return false;
	}
	uint8_t md5sum1[16];
	uint8_t md5sum2[16];
	md5(fileStart, fileLen, md5sum1);
	if (!TarFileStartGet("metadata.json", tarStart, tarLen, &metaStart, &metaLen, NULL)) {
		printf("Error, no metadata found\r\n");
		return false;
	}
	if (!MetaMd5Get(metaStart, metaLen, md5sum2)) {
		printf("Error, no checksum in metadata\r\n");
		return false;
	}
	if (memcmp(md5sum1, md5sum2, sizeof(md5sum1))) {
		printf("Error, checksum mismatch\r\n");
		printf("Should:\r\n");
		printHex(md5sum2, sizeof(md5sum2));
		printf("Is:\r\n");
		printHex(md5sum1, sizeof(md5sum1));
		return false;
	}
	char mcu[16];
	if (!MetaMcuGet(metaStart, metaLen, mcu, sizeof(mcu))) {
		printf("Error, no MCU in metadata\r\n");
		return false;
	}
	if (strcasecmp(mcu, "STM32L452")) {
		printf("Error, program is for wrong MCU %s\r\n", mcu);
		return false;
	}
	return true;
}

void toggleUsb(void) {
	if (g_usbEnabled == true) {
		printf("\r\nStopping USB\r\n");
		UsbStop();
		printf("USB disconnected\r\n");
		g_usbEnabled = false;
	} else {
		printf("\r\nRestarting USB\r\n");
		int32_t result = UsbStart(&g_usbDev, &usbSetConf, &usbControl, &usbGetDesc);
		printf("Result: %i\r\n", (int)result);
		g_usbEnabled = true;
	}
}

void loaderCycle(void) {
	static uint32_t downloadedLast = 0;
	static uint32_t size;

	Led2Green();
	HAL_Delay(250);
	Led2Off();
	HAL_Delay(250);
	char input = Rs232GetChar();
	if (input) {
		printf("%c", input);
	}
	switch (input) {
		case 'l': listStorage(); break;
		case 'h': mainMenu(); break;
		case 'r': NVIC_SystemReset(); break;
		case 's': JumpDfu(); break;
		case 'u': toggleUsb(); break;
		default: break;
	}
	uint32_t downloaded = g_dfuState.bytesDownloaded;
	if (downloaded != downloadedLast) {
		downloadedLast = downloaded;
		printf("\rDownloading... %u\r", (unsigned int)downloaded);
		Rs232Flush(); //needed for PC emulation
	}
	__disable_irq();
	bool transferDone = g_dfuState.commTransferDone;
	//startProgram can be set in an other cycle than transferDone becomes true
	bool startProgram = g_dfuState.commStartProgram;
	if (transferDone) {
		g_dfuState.commMainProcessing = true;
	}
	__enable_irq();

	if (transferDone) {
		size = g_dfuState.commFileSize;
		bool toFlash = g_dfuState.commToFlash;
		printf("Program with %ubytes transferred\r\n", (unsigned int)size);
		if (ProgTarCheck(g_DfuMem, size)) {
			if (toFlash) {
				char name[32];
				char filename[64];
				if (TarNameGet(g_DfuMem, size, name, sizeof(name))) {
					snprintf(filename, sizeof(filename), "/bin/%s.tar", name);
					FIL f;
					if (FR_OK == f_open(&f, filename, FA_WRITE | FA_CREATE_ALWAYS)) {
						UINT written = 0;
						FRESULT res = f_write(&f, g_DfuMem, size, &written);
						if (res == FR_OK) {
							printf("File %s written to disk\r\n", filename);
						} else {
							printf("Error, could not write %s. Error %u\r\n", filename, (unsigned int)res);
						}
						f_close(&f);
					} else {
						printf("Error, could not open %s\r\n", filename);
					}
				} else {
					printf("Error, could not get program name\r\n");
				}
			} else {
				printf("Valid program in RAM. Awaiting action\r\n");
			}
		}
		g_dfuState.commTransferDone = false;
		__sync_synchronize();
		g_dfuState.commMainProcessing = false; //now the ISR may access again
		__sync_synchronize();
	}
	if (startProgram) {
		__sync_synchronize();
		g_dfuState.commMainProcessing = true; //no access anylonger
		__sync_synchronize();
		if (ProgTarCheck(g_DfuMem, size)) {
			ProgTarStart(g_DfuMem, size); //usually does not return
		}
		__disable_irq();
		g_dfuState.commStartProgram = false;
		g_dfuState.commMainProcessing = false;
		__sync_synchronize();
		__enable_irq();
	}
}
