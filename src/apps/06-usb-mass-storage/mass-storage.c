/* Mass storage
(c) 2022 by Malte Marwedel

SPDX-License-Identifier: GPL-3.0-or-later

This state machine implements all what is needed to connect the internal SPI
flash as mass storage to a PC.

Most communication is handled within the USB interrupt. Only the actual read
and write of a block is processed in the main loop. Then the USB state machine
needs to be triggered from the main loop to continue.

This results in some data to be exchanged between the USB ISR and the main loop.
All is saved in the g_storageState struct.

The state machine assume the host plays 'nice' with the device, otherwise it
will break and probably needs a reset for recovering. For a more robust
implementation more error checking could be added.

The assumptions are:
1. As the device reports only to have one LUN, it simply assumes all data are
   for this one LUN, no check for the right LUN of a command (CBW - command
   block wrapper) is implemented.
2. Several other fields of CBW might be simply ignored too, even if setting them
   to non standard values might require the command to fail.
3. The host may not send another CBW before the currently CBW has been answered
   with a status (CSW - command status wrapper). Otherwise the data of the
   two CBW might mix up and strange things happen.
4. If a CBW reports a failure, the host is expected to request the error with a
   request sense. Otherwise the error state will not be cleared and stays set
   until a request sense is sent.
5. No range check for the read and write blocks are done, so if the host
   requests outside of the supported flash, it will silently roll over.
   It would also get struck, should a very large amount of data to be requested
   for reading and writing. Worst case is 2^16 blocks to read, which would take
   more than a minute at the current speed.
6. Unknown commands are properly reported as unsupported.
7. Some commands do nothing and just report everything as fine.
8. The writeprotect just tells the device is writeprotect, however write
   commands would still work.
9. The state variables of g_storageState, shared between the main loop and
   the USB ISR, do not use volatile. Volatile is considered deprecated by some
   newer C++ standards. Instead this implementation assumes calling to other
   compilation units (like UsbLock, UsbUnlock in boxusb.c) will enforce the
   compiler to update and re-read global variables as the compiler can not know
   if this can be read or modified there. Some very aggressive optimizing future
   compilers might make this assumption void and would require the addition of
   __sync_synchronize(); calls as memory barriers.
10. Beside the timeouts within QueueBufferToHostWithTimeout and
   QueueCswToHostWithTimeout, this implementation should be free of any side
   effects timing and speed of execution has to the state machine.
11. Command 0x1B (start/stop unit) is issued by the OS on unmount, it is not
   supported. This is ok.

S.M.A.R.T. data can be get by
smartctl --all -d scsi /dev/sdX
returned values are however faked. Its just to prove how to implement it.
It will look like:
== START OF INFORMATION SECTION ===
Vendor:               Marwedel
Product:              UniversalboxARM
Revision:             0.13
Compliance:           SPC-4
User Capacity:        8.384.512 bytes [8,38 MB]
Logical block size:   512 bytes
scsiModePageOffset: response length too short, resp_len=4 offset=4 bd_len=0
Serial number:        31337
Device type:          disk
scsiModePageOffset: response length too short, resp_len=4 offset=4 bd_len=0
Local Time is:        Wed Jun 25 23:12:29 2025 CEST
SMART support is:     Available - device has SMART capability.
SMART support is:     Enabled
Temperature Warning:  Disabled or Not Supported
scsiModePageOffset: response length too short, resp_len=4 offset=4 bd_len=0
Read Cache is:        Unavailable
Writeback Cache is:   Unavailable

=== START OF READ SMART DATA SECTION ===
SMART Health Status: OK

Current Drive Temperature:     42 C
Drive Trip Temperature:        80 C

Elements in grown defect list: 1

Error Counter logging not supported

scsiModePageOffset: response length too short, resp_len=4 offset=4 bd_len=0
Device does not support Self Test logging
Device does not support Background scan results logging

sginfo -d /dev/sdX
shows a fake defect, like:

NQUIRY response (cmd: 0x12)
----------------------------
Device Type                        0
Vendor:                    Marwedel
Product:                   UniversalboxARM
Revision level:            0.13

Defect Lists
------------
0 entries (0 bytes) in primary (PLIST) table.
Format (4) is: bytes from index [Cyl:Head:Off]
Offset -1 marks whole track as bad.


1 entries (8 bytes) in grown (GLIST) table.
Format (4) is: bytes from index [Cyl:Head:Off]
Offset -1 marks whole track as bad.

     0:  0:    1337|

Tested with:
Linux kernel 5.13: Disk needs 10s to appear when using 250kHz as flash clock, no double buffering
Linux kernel 5.13: Disk needs 4s to appear when using 4MHz as flash clock, no double buffering
Windows 10: Disk needs 14s to appear when using 250kHz as flash clock, no double buffering

Performance, measured with dd if=/dev/sdX of=foo.bin count=2048 for 1MiB test size
and dd if=/dev/sdX of=foo.bin count=8192 for 4MiB test size
Code = Memory where the program code is executed from
Data = Memory where stacks and variables are stored
CPU = CPU clock
Opt = Compilation optimization level
SPI = SPI clock speed for the external flash. If Dummy no SPI transfer is done and just zeros are delivered.
Queue = Number of 64 byte packets which can be queued to communicate from the main loop to the USB ISR (USB_BULK_QUEUE_LEN)
Dbl buffer = Does the USB uses the hardware double buffering feature?
DMA = Does the SPI read uses polling (No) or DMA (Yes)?
Test sz = Read size with dd
Int spd = Speed while reading just from the flash (benchmark command), no USB transfer
USB spd = Speed value returned by dd command
Internal speed = Value returned by performance debug command
USB ISRs = Number of USB isr while reading
USB load = Computing time of the CPU consumed by the USB ISRs while reading

Code   Data   CPU    Opt  SPI     Queue  Dbl buff  DMA  Test sz  Int spd  USB spd   USB ISRs  USB load  Note
SRAM1  SRAM1  16MHz  s    250kHz  8      No        No   1MiB              21.8kB/s
SRAM1  SRAM1  16MHz  s    500kHz  8      No        No   1MiB              37.3kB/s
SRAM1  SRAM1  16MHz  s    1MHz    8      No        No   1MiB              60.7kB/s
SRAM1  SRAM1  16MHz  s    2MHz    8      No        No   1MiB              81.2kB/s
SRAM1  SRAM1  16MHz  s    4MHz    8      No        No   1MiB              97.7kB/s
SRAM1  SRAM1  16MHz  s    4MHz    8      No        No   1MiB              94.6kB/s                      Performance counters added
SRAM1  SRAM1  16MHz  s    Dummy   8      No        No   1MiB              104kB/s   3000/s    13%
SRAM1  SRAM1  64MHz  s    4MHz    8      No        No   1MiB              104kB/s   3000/s    3%
SRAM1  SRAM1  16MHz  s    4MHz    8      Yes       No   1MiB              93.7kB/s
SRAM1  SRAM1  32MHz  s    4MHz    8      Yes       No   1MiB              160kB/s   4100/s    9%
SRAM1  SRAM1  64MHz  s    4MHz    8      Yes       No   1MiB              240kB/s   5600/s    6%
SRAM1  SRAM1  64MHz  s    8MHz    8      Yes       No   1MiB              319kB/s   7160/s    9%        SPI not guaranteed to be reliable
SRAM1  SRAM1  64MHz  s    8MHz    8      Yes       No   4MiB              372kB/s   7160/s    9%        SPI not guaranteed to be reliable
SRAM1  SRAM1  64MHz  s    Dummy   8      Yes       No   4MiB              1100kB/s  18900/s   31%
SRAM1  SRAM1  80MHz  s    5MHz    8      Yes       No   1MiB    408kiB/s  299kB/s
SRAM1  SRAM1  80MHz  s    5MHz    8      Yes       Yes  1MiB    584kiB/s  450kB/s   9700/s    10%
SRAM1  SRAM1  48MHz  s    6MHz    8      Yes       Yes  1MiB    678kiB/s  482kB/s   10300/s   19%
SRAM1  SRAM1  48MHz  s    6MHz    8      Yes       Yes  4MiB              563kB/s
SRAM1  SRAM1  48MHz  s    6MHz    24     Yes       Yes  4MiB              564kB/s
SRAM1  SRAM1  24MHz  s    6MHz    24     Yes       Yes  4MiB    636kiB/s  479kB/s   8950/s    34%
SRAM1  SRAM2  24MHz  s    6MHz    24     Yes       Yes  4MiB    636kiB/s  479kB/s
SRAM1  SRAM1  24MHz  s    6MHz    24     Yes       Yes  4MiB    644kiB/s  480kB/s
SRAM1  SRAM1  48MHz  3    6MHz    24     Yes       Yes  1MiB    682kiB/s  513kB/s   10900/s   18%       Now in use
SRAM1  SRAM1  48MHz  3    6MHz    24     Yes       Yes  4MiB              598kB/s

TODO:
1. Get a final USB ID
*/

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

#include "mass-storage.h"

#include "boxlib/keys.h"
#include "boxlib/leds.h"
#include "boxlib/rs232debug.h"
#include "boxlib/flash.h"
#include "boxlib/coproc.h"
#include "boxlib/peripheral.h"
#include "boxlib/boxusb.h"
#include "boxlib/mcu.h"

#include "main.h"

#include "usbd_core.h"
#include "usb_std.h"

#include "utility.h"

#include "diskio.h"

#define USB_STRING_MANUF 1
#define USB_STRING_PRODUCT 2
#define USB_STRING_SERIAL 3

#define USB_USE_DOUBLEBUFFERING

//Only 64 works
#define USB_BULK_BLOCKSIZE 64

/* Minimum is 2. 8 allows a full FLASH_BLOCK to be queued, and then the next
   can already be read while the previous one is still transmitted over USB.
   When there is no data, USB seems to poll again after 1ms, so to avoid
   the flash from doing nothing, the buffer can store data for 1ms to
   transfer, which would be 1.2KB -> next block size is 1.5KiB, which is 24
   packets. But compared to 8 blocks, the speed gain is hardly measureable.
*/
#define USB_BULK_QUEUE_LEN 24

//from host to device (out)
#define USB_ENDPOINT_FROMHOST 0x02

//from device to host (in)
#define USB_ENDPOINT_TOHOST 0x81

//fake data for S.M.A.R.T. Replace by some variable if real values are available.
#define SMART_TEMP_LIMIT 80
#define SMART_TEMP_MAX 50
#define SMART_TEMP_CURRENT 42
//if not 0, simulate reporting a defect sector
#define SMART_DEFECT 1337

typedef struct {
	uint32_t signature;
	uint32_t tag;
	uint32_t dataTransferLength;
	uint8_t flags;
	uint8_t lun;
	uint8_t length;
	uint8_t data[16];
} __attribute__((__packed__)) commandBlockWrapper_t;

_Static_assert(sizeof(commandBlockWrapper_t) == 31, "Please fix alignment!");

typedef struct {
	uint32_t signature;
	uint32_t tag;
	uint32_t dataResidue;
	uint8_t status;
} __attribute__((__packed__)) commandStatusWrapper_t;

_Static_assert(sizeof(commandStatusWrapper_t) == 13, "Please fix alignment!");

typedef struct {
	uint8_t deviceType;
	uint8_t rmb;
	uint8_t versions;
	uint8_t responseFormat;
	uint8_t additionalLength;
	uint8_t reserved1[3];
	uint8_t vendorInformation[8];
	uint8_t productInformation[16];
	uint8_t productrevisionLevel[4];
} __attribute__((__packed__)) scsiInquiry_t;

_Static_assert(sizeof(scsiInquiry_t) == 36, "Please fix alignment!");

typedef struct {
	uint8_t errorCode;
	uint8_t reserved1;
	uint8_t senseKey;
	uint8_t information[4];
	uint8_t additionalSenseLength;
	uint32_t reserved2;
	uint8_t senseCode;
	uint8_t senseCodeQualifier;
	uint32_t reserved3;
}  __attribute__((__packed__)) senseStandardData_t;

_Static_assert(sizeof(senseStandardData_t) == 18, "Please fix alignment!");




/* The PID used here is reserved for general test purpose.
See: https://pid.codes/1209/
*/
uint8_t g_deviceDescriptor[] = {
	0x12,       //length of this struct
	0x01,       //always 1
	0x00, 0x01, //usb version
	0x0,        //device class
	0x0,        //subclass
	0x0,        //device protocol
	USB_MAX_PACKET_SIZE, //maximum packet size
	0x09,0x12,  //vid
	0x04,0x00,  //pid
	0x00,0x01,  //revision
	USB_STRING_MANUF,   //manufacturer index
	USB_STRING_PRODUCT, //product name index
	USB_STRING_SERIAL,  //serial number index
	0x01        //number of configurations
};

uint8_t g_DeviceConfiguration[] = {
	9,     //length of this entry
	0x2,   //device configuration
	32, 0, //total length of this struct
	0x1,   //number of interfaces
	0x1,   //this config
	0x0,   //descriptor of this config index, not used
	0x80, //bus powered
	25,   //50mA
	//bulk only interface descriptor
	9,    //length
	0x04, //interface descriptor
	0x00, //interface number
	0x00, //alternate setting
	0x02, //no other endpoints
	0x08, //mass storage class
	0x06, //use scsi command set
	0x50, //bulk only transport
	0, //no string description given
	//bulk-in endpoint descriptor
	7,    //length
	0x05, //endpoint descriptor
	USB_ENDPOINT_TOHOST, //endpoint address - in, number 1
	0x02, //bulk endpoint
	USB_BULK_BLOCKSIZE, 0, //max 64byte per packet
	0x00, //interval, ignored
	//bulk-out endpoint descriptor
	7,    //length
	0x05, //endpoint descriptor
	USB_ENDPOINT_FROMHOST, //endpoint address - out, number 2
	0x02, //bulk endpoint
	USB_BULK_BLOCKSIZE, 0, //max 64byte per packet
	0x00, //interval, ignored
};

static struct usb_string_descriptor g_lang_desc     = USB_ARRAY_DESC(USB_LANGID_ENG_US);
static struct usb_string_descriptor g_manuf_desc_en = USB_STRING_DESC("marwedels.de");
static struct usb_string_descriptor g_prod_desc_en  = USB_STRING_DESC("UniversalboxARM-Storage");
//specs allow only 0...9 and A...F as serial number for mass storage
static struct usb_string_descriptor g_serial_desc   = USB_STRING_DESC("31337");

usbd_device g_usbDev;

typedef struct {
	uint32_t data[USB_BULK_BLOCKSIZE / sizeof(uint32_t)];
	size_t len;
} bulk_t;

typedef struct {
	uint32_t flashBytes; //size of the external flash
	bool usbEnabled;
	bool writeProtected;
	uint8_t usbTraffic; //down counter for LED control
	uint32_t timeTrafficChecked;
	//bulk queue
	uint8_t toHostFree; //Number of free entries in the hardware buffer. Allowed: 0, 1 or with double buffering 2.
	bulk_t toHost[USB_BULK_QUEUE_LEN];
	uint32_t toHostR;
	uint32_t toHostW;
	//response data (needed in the main thread after reading or writing has been done)
	uint32_t tag;
	//sense data
	uint8_t senseKey; //0 = no error
	uint8_t additionalSenseCode; //0 = no error
	//read command
	bool needRead;
	uint32_t readBlock;
	uint32_t readBlockNum;
	/* Write command
	  logic: If needWriteRx is true, put data to writeBuffer
	  when writeBuffer is full, don't read more data and set needWrite
	  Then the main thread gets the buffer content,
	  increments writeBlock and decrementsWriteBlockNum
	  and writes, while the next buffer can already be received over USB.
	*/
	bool needWrite; //main thread should write buffer
	bool needWriteRx;
	bool writeFirstBlock; //only marker for printf notification
	uint32_t writeBlockIndex;
	uint32_t writeBlock;
	uint32_t writeBlockNum;
	uint8_t writeBuffer[DISK_BLOCKSIZE];
	uint32_t writeStatus; //accumulate errors of one write request
} storageState_t;

typedef struct {
	uint64_t ticksMain; //counts [µs], time processing, accessed from main
	uint64_t tickUsbIsrStart; //timestmap [µs], only accessed within the ISR
	uint32_t usbIsrCount; //accessed from main and ISR
	uint64_t ticksUsbIsr; //counts [µs], time processing, accessed from main and ISR
	uint32_t tickSampleStart; //stamp [ms] when the values above were resetted last, accessed from main
	uint64_t ticksMainLast; //relative time consumend within the last ~1s in [µs], accessed from main
	uint64_t ticksUsbIsrLast; //relative time consumend within the last ~1s in [µs], accessed from main
	uint32_t usbIsrCountLast; //number usb ISRs processed
	bool printPerformance;
} performanceState_t;

storageState_t g_storageState;

performanceState_t g_performanceState;


//================== code for USB ==============

void UsbIrqOnEnter(void) {
	Led1Red();
	g_performanceState.tickUsbIsrStart = McuTimestampUs();
}

void UsbIrqOnLeave(void) {
	Led1Off();
	g_storageState.usbTraffic = 2;
	uint64_t tNow = McuTimestampUs();
	g_performanceState.ticksUsbIsr += tNow - g_performanceState.tickUsbIsrStart;
	g_performanceState.usbIsrCount++;
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
			if (dnumber <= USB_STRING_SERIAL) {
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

//Must be called from the USB interrupt, or within the UsbLock from other threads
void StorageDequeueToHost(usbd_device * dev) {
	uint32_t thisIndex = g_storageState.toHostR;
	if ((g_storageState.toHostW != thisIndex) && (g_storageState.toHostFree > 0)) { //elements in queue
		size_t len = g_storageState.toHost[thisIndex].len;
		if (usbd_ep_write(dev, USB_ENDPOINT_TOHOST, g_storageState.toHost[thisIndex].data, len) == len) {
			uint32_t nextIndex = (thisIndex + 1) % USB_BULK_QUEUE_LEN;
			g_storageState.toHostFree--;
			g_storageState.toHostR = nextIndex;
		} else {
			printfNowait("Err, write\r\n");
		}
	}
}

//Must be called from the USB interrupt, or within the UsbLock from other threads
bool StorageQueueToHost(usbd_device * dev, const void * data, size_t len) {
	uint32_t thisIndex = g_storageState.toHostW;
	uint32_t nextIndex = (thisIndex + 1) % USB_BULK_QUEUE_LEN;
	bool queued = false;
	if (g_storageState.toHostR != nextIndex) { //space in queue
		memcpy(g_storageState.toHost[thisIndex].data, data, len);
		g_storageState.toHost[thisIndex].len = len;
		g_storageState.toHostW = nextIndex;
		queued = true;
	}
	StorageDequeueToHost(dev); //try to send if there is space in the hardware buffer
	return queued;
}

/* Must be called from the USB interrupt, or within the UsbLock from other threads
   Status: 0 = 0k, 1 = error, 2 = phase error
   returns: true if adding to queue was successful, otherwise call again
*/
bool StorageQueueCsw(usbd_device * dev, uint32_t tag, uint32_t status) {
	commandStatusWrapper_t csw;
	csw.signature = 0x53425355; //ASCII for USBS
	csw.tag = tag;
	csw.dataResidue = 0;
	csw.status = status;
	return StorageQueueToHost(dev, &csw, sizeof(csw));
}

//called from ISR or the main loop (within the USB lock)
void EndpointFillDatabuffer(usbd_device *dev, uint8_t ep) {
	uint32_t index = g_storageState.writeBlockIndex; //reset by main loop
	if (g_storageState.needWriteRx) {
		if (index < DISK_BLOCKSIZE) {
			uint32_t toRead = MIN(USB_BULK_BLOCKSIZE, DISK_BLOCKSIZE - index);
			int32_t res = usbd_ep_read(dev, ep, g_storageState.writeBuffer + index, toRead);
			if (res > 0) {
				index += res;
				g_storageState.writeBlockIndex = index;
				if (index == DISK_BLOCKSIZE) {
					g_storageState.needWrite = true;
				}
			}
		}
	}
}

void EndpointBulkOut(usbd_device *dev, uint8_t event, uint8_t ep) {
	//printfNowait("Bulk out %u %u\r\n", event, ep);
	if (g_storageState.needWriteRx) {
		EndpointFillDatabuffer(dev, ep);
		return;
	}
	//normal commands, no data
	uint32_t buffer[8];
	uint32_t res = usbd_ep_read(dev, ep, buffer, sizeof(buffer));
	if (res == 31) {
		commandBlockWrapper_t * cbw = (commandBlockWrapper_t *)buffer;
		if ((cbw->signature == 0x43425355) && (cbw->length > 0) && (cbw->length <= 16)) { //ASCII for USBC
#if 0
			printfNowait("%x\r\n", cbw->tag);
			printfNowait("len %u\r\n", cbw->dataTransferLength);
			printfNowait("flags %x\r\n", cbw->flags);
			printfNowait("lun %x\r\n", cbw->lun);
			printfNowait("len2 %x\r\n", cbw->length);
			for (uint32_t i = 0; i < cbw->length; i++) { //range check is already done
				printfNowait("%x ", cbw->data[i]);
			}
			printfNowait("\r\n");
#endif
			bool invalidCommand = false;
			bool invalidField = false;
			uint8_t command = cbw->data[0];
			if ((command == 0x12) && (cbw->length >= 6)) { //inquiry command (1. done by the Linux host)
				if (cbw->data[4] >= 36) { //Linux requests 36, Windows requests 255
					uint8_t evpd = cbw->data[1] & 1; //enable vital product data
					uint8_t pageCode = cbw->data[2];
					if (evpd == 0) {
						if (pageCode == 0) {
							scsiInquiry_t dataOut;
							dataOut.deviceType = 0x0;
							dataOut.rmb = 0x80; //removeable media
							dataOut.versions = 0x6; //SPC-4
							dataOut.responseFormat = 0x0;
							dataOut.additionalLength = 31; //always this value
							memcpy((char *)dataOut.vendorInformation, "Marwedel", sizeof(dataOut.vendorInformation));
							memcpy((char *)dataOut.productInformation, "UniversalboxARM", sizeof(dataOut.productInformation));
							for (uint32_t i = 0; i < 4; i++) {
								dataOut.productrevisionLevel[i] = APPVERSION[i];
							}
							StorageQueueToHost(dev, &dataOut, sizeof(dataOut));
							StorageQueueCsw(dev, cbw->tag, 0);
						} else {
							invalidField = true;
						}
					} else { //requested by smartctrl
						/*see
						  https://www.seagate.com/files/staticfiles/support/docs/manual/Interface%20manuals/100293068j.pdf
						  and
						  https://ftp.hydata.com/Downloads/scsitools/scsitools.DRT/Resources/LOD_FactoryMadeLOD-Files/t10/sat4r06.pdf
						*/
						if (pageCode == 0) { //returns a list of supported page codes
							uint8_t dataOut[7];
							dataOut[0] = 0; //same value as if evpd == 0
							dataOut[1] = pageCode;
							dataOut[2] = 0; //reserved
							dataOut[3] = sizeof(dataOut) - 4;
							dataOut[4] = 0; //we support this command
							dataOut[5] = 0x80; //we support reading the serial number
							dataOut[6] = 0x83; //we support Device Identification VPD page
							StorageQueueToHost(dev, &dataOut, sizeof(dataOut));
							StorageQueueCsw(dev, cbw->tag, 0);
						} else if (pageCode == 0x80) { //returns the serial number
							uint8_t dataOut[9];
							dataOut[0] = 0; //same value as if evpd == 0
							dataOut[1] = pageCode;
							dataOut[2] = 0; //reserved
							dataOut[3] = sizeof(dataOut) - 4;
							//serial number
							for (uint32_t i = 0; i < 5; i++) {
								dataOut[4 + i] = g_serial_desc.wString[i];
							}
							StorageQueueToHost(dev, &dataOut, sizeof(dataOut));
							StorageQueueCsw(dev, cbw->tag, 0);
						} else if (pageCode == 0x83) { //Device Identification VPD page
							const char * id = "Marwedels.de UniversalboxARM";
							uint8_t dataOut[64];
							size_t lenText = strlen(id);
							size_t len = lenText + 4;
							dataOut[0] = 0; //same value as if evpd == 0
							dataOut[1] = pageCode;
							dataOut[2] = 0; //size MSB
							dataOut[3] = len;
							dataOut[4] = 0xF2; //Protocol identifier = 0xF -> reserved (found used by a card reader), code set = 2 -> ascii
							dataOut[5] = 1; //PIV = 0, ASSOCIATION = 0, IDENTIFIER = 1 -> first 8 byte are the vendor id
							dataOut[6] = 0; //reserved
							dataOut[7] = lenText;
							memcpy(dataOut + 8, id, lenText);
							StorageQueueToHost(dev, &dataOut, len  + 4);
							StorageQueueCsw(dev, cbw->tag, 0);
						} else {
							invalidField = true;
						}
					}
				} else {
					invalidField = true;
				}
			} else if ((command == 0x0) && (cbw->length >= 6)) { //test unit ready (2. done by the host)
				StorageQueueCsw(dev, cbw->tag, 0);
			} else if ((command == 0x3) && (cbw->length >= 6)) { //request sense (3. done if unit is not reported to be ready)
				senseStandardData_t dataOut;
				dataOut.errorCode = 0xF0; //valid + error
				dataOut.senseKey = g_storageState.senseKey; //0 = no error
				g_storageState.senseKey = 0;
				dataOut.additionalSenseLength = 10;
				dataOut.senseCode = g_storageState.additionalSenseCode; //0 = no error
				g_storageState.additionalSenseCode = 0;
				dataOut.senseCodeQualifier = 0;
				dataOut.reserved1 = 0;
				dataOut.reserved2 = 0;
				dataOut.reserved3 = 0;
				StorageQueueToHost(dev, &dataOut, sizeof(dataOut));
				StorageQueueCsw(dev, cbw->tag, 0);
			} else if ((command == 0x25) && (cbw->length >= 6)) { //read capacity (10) (3. request by the host)
				uint32_t capacity[2];
				capacity[0] = BytesFlip(g_storageState.flashBytes / DISK_BLOCKSIZE - 1);
				capacity[1] = BytesFlip(DISK_BLOCKSIZE);
				StorageQueueToHost(dev, capacity, sizeof(capacity));
				StorageQueueCsw(dev, cbw->tag, 0);
			} else if ((command == 0x23) && (cbw->length >= 6)) { //read format capacity (Only Windows requests this, even works without, but requests a lot until it gives up)
				uint8_t data[12] = {0};
				data[3] = 8; //list length
				uint32_t blocks = BytesFlip(g_storageState.flashBytes / DISK_BLOCKSIZE - 1);
				memcpy(data + 4, &blocks, sizeof(uint32_t));
				data[8] = 2; //formatted media
				uint32_t blockSize = BytesFlip(DISK_BLOCKSIZE);
				data[9] = (blockSize >> 16) & 0xFF;
				data[10] = (blockSize >> 8) & 0xFF;
				data[11] = blockSize & 0xFF;
				StorageQueueToHost(dev, data, sizeof(data));
				StorageQueueCsw(dev, cbw->tag, 0);
			} else if ((command == 0x1A) && (cbw->length >= 6)) { //mode sense(6) (4. request by the host)
				uint8_t dbd = (cbw->data[1] >> 3) & 1; //disable block descriptor
				//uint8_t pc = (cbw->data[2] >> 6) & 3; //page control, 0 -> current values
				uint8_t pageCode = cbw->data[2]  & 0x3F; //smartctrl requests this with a value of 0x1C -> informational exceptions control
				//uint8_t subpageCode = cbw->data[3];

				size_t idx = 0;
				uint8_t data[64];
				//block descriptor header
				data[1] = 0; //medium type
				if (g_storageState.writeProtected) {
					data[2] = 0x80;
				} else {
					data[2] = 0x0;
				}
				if (dbd) {
					data[3] = 0; //block descriptor length, must be 0
					idx = 4;
				} else {
					data[3] = 0; //block descriptor length, can be 0
					idx = 4;
					//add block descriptors if needed
				}
				/*0x3F: Request all supported mode pages. Common by 4. request by the host
				  0x1C: Request informational exception control, requested by smartctrl
				*/
				if ((pageCode == 0x3F) || (pageCode == 0x1C)) {
					data[idx] = 0x1C; //support exception control
					const size_t pageLen = 0xA;
					data[idx + 1] = pageLen;
					/*1. byte: PERF = 0, EBF = 0, EWASC = 0, DEXCPT = 0, test = 0, EBACKERR = 0, LOGERR = 0
					  2. byte: MRIE = 0
					  3..6 byte: interval timer = 0
					  7..10 byte: report count = 0 -> no limit
					*/
					memset(data + idx + 2, 0, pageLen);
					idx += pageLen + 2;
				}
				data[0] = idx - 1; //this byte itself is excluded in the length field
				StorageQueueToHost(dev, data, idx);
				StorageQueueCsw(dev, cbw->tag, 0);
			} else if ((command == 0x5A) && (cbw->length >= 6)) { //mode sense(10) (4. looks like a MAC does this instead of mode sense(6))
				uint8_t data[8] = {0};
				data[0] = 0; //three bytes follow
				data[1] = 8; //size of this data array
				if (g_storageState.writeProtected) {
					data[3] = 0x80;
				} else {
					data[3] = 0x0;
				}
				StorageQueueToHost(dev, data, sizeof(data));
				StorageQueueCsw(dev, cbw->tag, 0);
			} else if ((command == 0x1E) && (cbw->length >= 6)) { //prevent medium removal (5. request by the host)
				uint32_t status = 0;
				if (cbw->data[4] & 1) { //prevents removal -> not supported
					status = 1; //command failed
					//sense key = illegal request, additional sense code = invalid field in command packet
					g_storageState.senseKey = 0x5;
					g_storageState.additionalSenseCode = 0x24;
				}
				StorageQueueCsw(dev, cbw->tag, status);
			} else if ((command == 0x28) && (cbw->length >= 10)) { //read(10) data
				Led2Green();
				uint32_t block;
				uint32_t num;
				memcpy(&block, &(cbw->data[2]), sizeof(uint32_t));
				block = BytesFlip(block);
				num = (cbw->data[7] << 8) | (cbw->data[8]);
				//printfNowait("R %u, len %u\r\n", block, num);
				g_storageState.needRead = true;
				g_storageState.readBlock = block;
				g_storageState.readBlockNum = num;
				g_storageState.tag = cbw->tag; //no response yet
			} else if ((command == 0x2A) && (cbw->length >= 10)) { //write(10) data
				Led2Red();
				uint32_t block;
				uint32_t num;
				memcpy(&block, &(cbw->data[2]), sizeof(uint32_t));
				block = BytesFlip(block);
				num = (cbw->data[7] << 8) | (cbw->data[8]);
				//printfNowait("W %u, len %u\r\n", block, num);
				g_storageState.needWriteRx = true;
				g_storageState.writeFirstBlock = true;
				g_storageState.writeBlockIndex = 0;
				g_storageState.writeBlock = block;
				g_storageState.writeBlockNum = num;
				g_storageState.tag = cbw->tag; //no response yet
			} else if ((command == 0x2F) && (cbw->length >= 10)) { //verify data
				//nothing to do. We simply tell everything is ok :P
				StorageQueueCsw(dev, cbw->tag, 0);
			} else if ((command == 0x9E) && (cbw->length >= 10)) { //service action (requested by smartctrl if called with --all -d scsi <device> parameter)
				uint8_t serviceAction = cbw->data[1];
				if (serviceAction == 0x10) { //read capacity (16)
					uint32_t capacity[8] = {0};
					capacity[1] = BytesFlip(g_storageState.flashBytes / DISK_BLOCKSIZE - 1); //capacity[0] is for data above 2TiB drives
					capacity[2] = BytesFlip(DISK_BLOCKSIZE);
					StorageQueueToHost(dev, capacity, sizeof(capacity));
					StorageQueueCsw(dev, cbw->tag, 0);
				} else {
					printfNowait("Service action 0x%x unsuppored\r\n", serviceAction);
					invalidCommand = true;
				}
			} else if ((command == 0x4D) && (cbw->length >= 10)) { //log sense command, requested by smartctrl if mode sense claims to support S.M.A.R.T.
				uint8_t sp = cbw->data[1] & 1;
				//uint8_t pc = (cbw->data[2] >> 6) & 0x3;
				uint8_t pageCode = cbw->data[2] & 0x3F;
				uint8_t subpageCode = cbw->data[3];
				//uint16_t paraPointer = (cbw->data[5] << 8) | cbw->data[6];
				uint16_t allocLen = (cbw->data[7] << 8) | cbw->data[8];
				//uint8_t control = cbw->data[9];
				/*smartctrl typically calls eache pageCode twiche. Once with a allocLen of 4 and then
				  with a larger allocLen. Should the first call return too much data, a reset is issued and
				  reading the data fails.
				*/
				//printfNowait("LSense sp 0x%x, pc 0x%x, pCode 0x%x, sp 0x%x, pP 0x%x, all 0x%x, ctrl 0x%x\r\n",
				//     (unsigned int)sp, (unsigned int)pc, (unsigned int)pageCode, (unsigned int)subpageCode, paraPointer, allocLen, control);
				if ((sp == 0) && (subpageCode == 0)) {
					if (pageCode == 0) { //list supported page codes
						uint8_t data[9] = {0}; //for some reasons, if the array is smaller (and actually fitting to the used length of 7 bytes), an USB reset is sent and wireshark reports a decoding error
						data[0] = 0x80 | pageCode; //ds = 1 -> disable save, spf = 0, so sub page code must be zero
						data[1] = 0; //sub page code
						data[2] = 0; //page length (high byte)
						data[3] = 3; //page length (low byte)
						data[4] = pageCode; //support this code
						data[5] = 0x0D; //support temperature page
						data[6] = 0x2F; //support information exception log page
						StorageQueueToHost(dev, data, MIN(sizeof(data), allocLen));
						StorageQueueCsw(dev, cbw->tag, 0);
					} else if (pageCode == 0x0D) { //temperature page
						uint8_t data[16];
						data[0] = 0x80 | pageCode; //ds = 1 -> disable save, spf = 0, so sub page code must be zero
						data[1] = 0; //sub page code
						data[2] = 0; //page length (high byte)
						data[3] = sizeof(data) - 4; //page length (low byte)
						size_t idx = 4;
						//one temperature entry is 12 byte in size
						data[idx + 0] = 0x00; //temperature report
						data[idx + 1] = 0x00; //temperature report
						data[idx + 2] = 3; //parameter control byte. format and linking = 3 -> binary list
						data[idx + 3] = 0x2; //parameter length
						data[idx + 4] = 0; //reserved
						data[idx + 5] = SMART_TEMP_CURRENT; //actual temperature
						data[idx + 6] = 0x00; //reference temperature report
						data[idx + 7] = 0x01; //reference temperature report
						data[idx + 8] = 3; //parameter control byte. format and linking = 3 -> binary list
						data[idx + 9] = 0x2; //parameter length
						data[idx + 10] = 0; //reserved
						data[idx + 11] = SMART_TEMP_LIMIT; //reference temperature
						StorageQueueToHost(dev, data, MIN(sizeof(data), allocLen));
						StorageQueueCsw(dev, cbw->tag, 0);
					} else if (pageCode == 0x2F) { //information exception log page
						uint8_t data[13];
						data[0] = 0x80 | pageCode; //ds = 1 -> disable save, spf = 0, so sub page code must be zero
						data[1] = 0; //sub page code
						data[2] = 0; //page length (high byte)
						data[3] = sizeof(data) - 4; //page length (low byte)
						size_t idx = 4;
						//one entry is at least 8 bytes in size
						data[idx + 0] = 0x00; //parameter code -> informational exceptions general parameter
						data[idx + 1] = 0x00; //parameter code -> informational exceptions general parameter
						data[idx + 2] = 3; //parameter control byte. format and linking = 3 -> binary list
						data[idx + 3] = 0x5; //parameter length
						data[idx + 4] = 0; //no information pending = 0
						data[idx + 5] = 0; //unspecified, because no information are pending
						data[idx + 6] = SMART_TEMP_CURRENT; //most recent temperature read
						data[idx + 7] = SMART_TEMP_LIMIT; //if this temperature is reached, a warning should be issued
						data[idx + 8] = SMART_TEMP_MAX; //maximum temperature ever measured
						StorageQueueToHost(dev, data, MIN(sizeof(data), allocLen));
						StorageQueueCsw(dev, cbw->tag, 0);
					} else {
						invalidField = true;
					}
				} else {
					invalidField = true; //we do not support saving and not subpages
				}
			} else if ((command == 0xB7) && (cbw->length >= 12)) { //read defect data (12), requested by smartctrl if mode sense claims to support S.M.A.R.T.
				uint8_t reqPList = (cbw->data[1] >> 4) & 1;
				uint8_t reqGList = (cbw->data[1] >> 3) & 1;
				uint8_t format = cbw->data[1] & 7;
				uint32_t descIndex = (cbw->data[2] << 24) | (cbw->data[3] << 16) | (cbw->data[4] << 8) | cbw->data[5];
				uint32_t allocLen = (cbw->data[6] << 24) | (cbw->data[7] << 16) | (cbw->data[8] << 8) | cbw->data[9];
				//printfNowait("0xB7 P %u, G: %u, format: 0x%x, idx %u len %u\r\n", reqPList, reqGList, format, (unsigned int)descIndex, (unsigned int)allocLen);
				size_t idx = 0;
				uint8_t data[16] = {0};
				idx = 8;
				data[1] = (reqPList << 4) | (reqGList << 3) | 4; //we only support format 4
				data[4] = 0; //defect list length high byte
				data[5] = 0; //defect list length
				data[6] = 0; //defect list length
				if ((SMART_DEFECT) && (reqGList) && (descIndex == 0)) {
					data[7] = 8; //defect list length low byte (8 byte per entry)
					data[8] = 0; //cylinder number
					data[9] = 0; //cylinder number
					data[10] = 0; //cylinder number
					data[11] = 0; //head number
					data[12] = (SMART_DEFECT >> 24) & 0xFF; //bytes from idx high
					data[13] = (SMART_DEFECT >> 16) & 0xFF; //bytes from idx
					data[14] = (SMART_DEFECT >> 8) & 0xFF; //bytes from idx
					data[15] = SMART_DEFECT & 0xFF; //bytes from idx low
					idx += 8;
				}
				StorageQueueToHost(dev, data, MIN(idx, allocLen));
				if (format != 4) { //bytes from index format descriptor
					g_storageState.senseKey = 0x1; //recovered error
					g_storageState.additionalSenseCode = 0x1C; //defect list not found
					StorageQueueCsw(dev, cbw->tag, 1);
				} else {
					StorageQueueCsw(dev, cbw->tag, 0);
				}
			} else if ((command == 0x37) && (cbw->length >= 10)) { //read defect data (10), requested by smartctrl if mode sense claims to support S.M.A.R.T. and read defect data (12) did not work
				uint8_t reqPList = (cbw->data[2] >> 4) & 1;
				uint8_t reqGList = (cbw->data[2] >> 3) & 1;
				uint8_t format = cbw->data[2] & 7;
				uint16_t allocLen = (cbw->data[7] << 8) | cbw->data[8];
				//printfNowait("0x37 p: %u, G: %u, format: 0x%x, len %u\r\n", reqPList, reqGList, format, allocLen);
				size_t idx = 0;
				uint8_t data[12] = {0};
				idx = 4;
				data[1] = (reqPList << 4) | (reqGList << 3) | 4; //we only support format 4
				data[2] = 0; //defect list length high byte
				if ((SMART_DEFECT) && (reqGList)) {
					data[3] = 8; //defect list length low byte (8 byte per entry)
					data[4] = 0; //cylinder number
					data[5] = 0; //cylinder number
					data[6] = 0; //cylinder number
					data[7] = 0; //head number
					data[8] = (SMART_DEFECT >> 24) & 0xFF; //bytes from idx high
					data[9] = (SMART_DEFECT >> 16) & 0xFF; //bytes from idx
					data[10] = (SMART_DEFECT >> 8) & 0xFF; //bytes from idx
					data[11] = SMART_DEFECT & 0xFF; //bytes from idx low
					idx += 8;
				}
				StorageQueueToHost(dev, data, MIN(idx, allocLen));
				if (format != 4) { //bytes from index format descriptor
					g_storageState.senseKey = 0x1; //recovered error
					g_storageState.additionalSenseCode = 0x1C; //defect list not found
					StorageQueueCsw(dev, cbw->tag, 1);
				} else {
					StorageQueueCsw(dev, cbw->tag, 0);
				}
			} else { //unsupported command
				/*HDSentinal requests:
				  0xE4: <no documentation found>
				  0x24: <no documentation found>
				  0xA1: ATA pass through (12)
				  0xDF: <no documentation found>
				  0xF8: <no documentation found>
				  0xD8: <no documentation found>
				  0xEE: <no documentation found>
				*/
				invalidCommand = true;
			}
			if (invalidCommand) {
				printfNowait("Cmd 0x%x unsuppored, length %u\r\n", command, (unsigned int)cbw->length);
				g_storageState.additionalSenseCode = 0x20; //invalid command operational code
			} else if (invalidField) {
				printfNowait("Field for cmd 0x%x unsuppored, length %u\r\n", command, (unsigned int)cbw->length);
				g_storageState.additionalSenseCode = 0x24; //invalid field in command packet
			}
			if ((invalidCommand) || (invalidField)) {
				g_storageState.senseKey = 0x5; //illegal request
				StorageQueueCsw(dev, cbw->tag, 1); //command failed
			}
		} else {
			StorageQueueCsw(dev, cbw->tag, 2); //phase error
			printfNowait("Phase error\r\n");
		}
	}
}

//call from USB ISR or within UsbLock
void StorageStateReset(void) {
	g_storageState.senseKey = 0;
	g_storageState.additionalSenseCode = 0;
	g_storageState.toHostFree = 0;
	g_storageState.toHostR = 0;
	g_storageState.toHostW = 0;
	g_storageState.needRead = false;
	g_storageState.needWrite = false;
	g_storageState.writeStatus = 0;
}


#ifdef USB_USE_DOUBLEBUFFERING
//function copied from usb stack:
inline static volatile uint16_t *EPR(uint8_t ep) {
    return (uint16_t*)((ep & 0x07) * 4 + USB_BASE);
}
#endif

void EndpointEventTx(usbd_device *dev, uint8_t event, uint8_t ep) {
	if ((ep == USB_ENDPOINT_TOHOST) && (event == usbd_evt_eptx)) {
		g_storageState.toHostFree++;
#ifdef USB_USE_DOUBLEBUFFERING
		g_storageState.toHostFree = MIN(g_storageState.toHostFree, 2);
		/*The problem, when there is a high CPU load, not every sent USB tx packet
		  gets a proper callback. So there is the need to fix toHostFree back to 2
		  if this happens. Without the fix, the device continues to work as if only
		  one buffer is used.
		  This is the case when DTOG and SW_BUF have the same value.
		  See RM0394 chapter 45.5.3
		*/
		if (g_storageState.toHostFree == 1) {
			uint16_t reg = *EPR(USB_ENDPOINT_TOHOST);
			uint16_t swbuf = reg & USB_EP_DTOG_RX; //used as swbuf bit with double buffering
			uint16_t dtog = reg & USB_EP_DTOG_TX;
			if (((swbuf) && (dtog)) || ((swbuf == 0) && (dtog == 0))) {
				g_storageState.toHostFree = 2;
			}
		}
		StorageDequeueToHost(dev); //extra call if a callback was forgotten
#endif
		StorageDequeueToHost(dev); //normal call
	}
}

static usbd_respond usbSetConf(usbd_device *dev, uint8_t cfg) {
	usbd_respond result = usbd_fail;
	switch (cfg) {
		case 0:
			//deconfig
			printfNowait("Deconfig\r\n");
			usbd_ep_deconfig(dev, USB_ENDPOINT_TOHOST);
			usbd_ep_deconfig(dev, USB_ENDPOINT_FROMHOST);
			break;
		case 1:
			//set config
			printfNowait("Set config\r\n");
			StorageStateReset();
			uint8_t epType = USB_EPTYPE_BULK;
#ifdef USB_USE_DOUBLEBUFFERING
			epType |= USB_EPTYPE_DBLBUF;
#endif
			if (!usbd_ep_config(dev, USB_ENDPOINT_TOHOST, epType, USB_BULK_BLOCKSIZE)) {
				printfNowait("Error, configure ep to host\r\n");
			} else {
#ifdef USB_USE_DOUBLEBUFFERING
				g_storageState.toHostFree = 2;
#else
				g_storageState.toHostFree = 1;
#endif
			}
			if (!usbd_ep_config(dev, USB_ENDPOINT_FROMHOST, epType, USB_BULK_BLOCKSIZE)) {
				printfNowait("Error, configure ep from host\r\n");
			}
			usbd_reg_endpoint(dev, USB_ENDPOINT_FROMHOST, &EndpointBulkOut);
			usbd_reg_event(dev, usbd_evt_eptx, EndpointEventTx);
			result = usbd_ack;
			break;
	}
	return result;
}

static usbd_respond usbControl(usbd_device *dev, usbd_ctlreq *req, usbd_rqc_callback *callback) {
	//Printing can be done here as long it is buffered. Otherwise it might be too slow
	//get MAX lun
	if ((req->bmRequestType == 0xA1) && (req->bRequest == 0xFE) && (req->wValue == 0) && (req->wLength == 1)) {
		req->data[0] = 0;
		return usbd_ack;
	}
	//bulk only reset
	if ((req->bmRequestType == 0x21) && (req->bRequest == 0xFF) && (req->wValue == 0) && (req->wLength == 1)) {
		printfNowait("Bulk reset\r\n");
		StorageStateReset();
		return usbd_ack;
	}
	//set interface
	if ((req->bmRequestType == 0x01) && (req->bRequest == 0x0B) && (req->wLength == 0)) {
		return usbd_ack; //we only use interface 0 anyway...
	}
	//get interface
	if ((req->bmRequestType == 0x81) && (req->bRequest == 0x0A) && (req->wValue == 0) && (req->wLength == 1)) {
		req->data[0] = 0;
		return usbd_ack;
	}
	/* Other valid commands:
	req: 0-5-6-0-0 -> set address
	req: 0-9-1-0-0 -> set configuration
	*/
	if ((req->bmRequestType != 0x80) && (req->bRequest != 0x6)) { //filter usbGetDesc resquests
		//printfNowait("req: %x-%x-%x-%x-%x\r\n", req->bmRequestType, req->bRequest, req->wValue, req->wIndex, req->wLength);
	}
	return usbd_fail;
}

void MainMenu(void) {
	printf("\r\nSelect operation:\r\n");
	printf("h: This screen\r\n");
	printf("u: Toggle USB connection\r\n");
	printf("w: Toggle write protection\r\n");
	printf("p: Toggle print performance stats\r\n");
	printf("b: Run read benchmark\r\n");
	printf("r: Reboot with reset controller\r\n");
}

void AppInit(void) {
	LedsInit();
	Led1Green();
	McuClockToHsiPll(48000000, RCC_HCLK_DIV1);
	PeripheralPowerOff();
	HAL_Delay(100);
	PeripheralPowerOn();
	Rs232Init();
	printf("\r\nUSB Mass-storage %s\r\n", APPVERSION);
	KeysInit();
	CoprocInit();
	PeripheralInit();
	FlashEnable(8); //6MHz
	g_storageState.flashBytes = FlashSizeGet();
	if (g_storageState.flashBytes >= DISK_RESERVEDOFFSET) {
		g_storageState.flashBytes -= DISK_RESERVEDOFFSET;
	}
	printf("Starting USB\r\n");
	int32_t result = UsbStart(&g_usbDev, &usbSetConf, &usbControl, &usbGetDesc);
	if (result == -1) {
		printf("Error, failed to start 48MHz clock. Error: %u\r\n", (unsigned int)result);
	}
	if (result == -2) {
		printf("Error, failed to set USB clock source\r\n");
	}
	if (result >= 0) {
		g_storageState.usbEnabled = true;
	}
	printf("Ready. Press h for available commands\r\n");
}

void StorageStop(void) {
	if (g_storageState.usbEnabled == true) {
		printf("\r\nStopping USB\r\n");
		UsbStop();
		printf("USB disconnected\r\n");
		g_storageState.usbEnabled = false;
	}
}

void StorageStart(void) {
	if (g_storageState.usbEnabled == false) {
		printf("\r\nRestarting USB\r\n");
		UsbLock();
		StorageStateReset();
		UsbUnlock();
		int32_t result = UsbStart(&g_usbDev, &usbSetConf, &usbControl, &usbGetDesc);
		printf("Result: %i\r\n", (int)result);
		g_storageState.usbEnabled = true;
	}
}

void ToggleUsb(void) {
	if (g_storageState.usbEnabled == true) {
		StorageStop();
	} else {
		StorageStart();
	}
}

void ToggleWriteprotect(void) {
	g_storageState.writeProtected = !g_storageState.writeProtected;
	printf("Writeprotect will be %s after next usb disconnect + reconnect\r\n", g_storageState.writeProtected ? "enabled" : "disabled");
}

bool QueueBufferToHostWithTimeout(usbd_device *dev, const void * buffer, size_t len) {
	uint32_t start = HAL_GetTick();
	const uint32_t timeout = 100;
	bool success;
	do {
		UsbLock();
		success = StorageQueueToHost(dev, buffer, len);
		UsbUnlock();
	} while ((!success) && ((HAL_GetTick() - start) < timeout));
	return success;
}

bool QueueCswToHostWithTimeout(usbd_device * dev, uint32_t tag, uint32_t status) {
	uint32_t start = HAL_GetTick();
	const uint32_t timeout = 100;
	bool success;
	do {
		UsbLock();
		success = StorageQueueCsw(dev, tag, status);
		UsbUnlock();
	} while ((!success) && ((HAL_GetTick() - start) < timeout));
	return success;
}


bool ProcessFlashAccess(void) {
	bool todo = false;
	if (g_storageState.needRead) {
		UsbLock();
		g_storageState.needRead = false;
		uint32_t blocks = g_storageState.readBlockNum;
		uint32_t block = g_storageState.readBlock;
		UsbUnlock();
		printf("Read %u, len %u\r\n", (unsigned int)block, (unsigned int)blocks);
		uint32_t address = DISK_RESERVEDOFFSET + block * DISK_BLOCKSIZE;
		uint32_t status = 0; //ok
		for (uint32_t i = 0; i < blocks; i++) {
			/* In order to simulate a RAM disk for speed measurements, set buffer to = {0}
			   and replace the if (Flash(...)) by if (1).
			   Don't forget to disable writing too.
			*/
			uint8_t buffer[DISK_BLOCKSIZE] = {0};
			uint32_t address2 = address + i * DISK_BLOCKSIZE;
			if (FlashRead(address2, buffer, DISK_BLOCKSIZE)) {
				for (uint32_t j = 0; j < DISK_BLOCKSIZE; j += USB_BULK_BLOCKSIZE) {
					bool success = QueueBufferToHostWithTimeout(&g_usbDev, buffer + j, USB_BULK_BLOCKSIZE);
					if (!success) {
						status = 1;
						printf("Error, could not queue block\r\n");
						break;
					}
				}
			} else {
				printf("Error, read failed\r\n");
				status = 1; //command failed
				g_storageState.senseKey = 0x3; //medium error
				g_storageState.additionalSenseCode = 0x11; //unrecovered read error
				break;
			}
		}
		Led2Off();
		if (!QueueCswToHostWithTimeout(&g_usbDev, g_storageState.tag, status)) {
			printf("Error, could not queue CSW\r\n");
		}
		todo = true;
	}
	if (g_storageState.needWrite) {
		UsbLock();
		g_storageState.needWrite = false;
		uint8_t buffer[DISK_BLOCKSIZE];
		memcpy(buffer, g_storageState.writeBuffer, DISK_BLOCKSIZE);
		uint32_t writeBlock = g_storageState.writeBlock;
		g_storageState.writeBlockIndex = 0; //this allows reception of the next block
		g_storageState.writeBlock++;
		bool firstBlock = g_storageState.writeFirstBlock;
		g_storageState.writeFirstBlock = false;
		bool lastBlock = false;
		uint32_t blocks = g_storageState.writeBlockNum;
		if (g_storageState.writeBlockNum) {
			g_storageState.writeBlockNum--;
		}
		if (g_storageState.writeBlockNum == 0) {
			lastBlock = true;
			g_storageState.needWriteRx = false;
		} else {
			/*If there are already data in the endpoint, and the ISR could not read it
			  because the buffer was already full, we need to re-check the endpoint
			  now, because there will not be another interrupt unless we have read the
			  data.
			*/
			EndpointFillDatabuffer(&g_usbDev, USB_ENDPOINT_FROMHOST);
		}
		UsbUnlock();
		uint32_t address = DISK_RESERVEDOFFSET + writeBlock * DISK_BLOCKSIZE;
#if 1
		if (firstBlock) {
			printf("Write block %u, len %u\r\n", (unsigned int)writeBlock, (unsigned int)blocks);
		}
		if (!FlashWrite(address, buffer, DISK_BLOCKSIZE)) {
			g_storageState.writeStatus = 1; //command failed
			g_storageState.senseKey = 0x3; //medium error
			g_storageState.additionalSenseCode = 0x3; //write fault
		}
#else
		if (firstBlock) {
			printf("Sim write block %u, len %u\r\n", (unsigned int)writeBlock, (unsigned int)blocks);
		}
#endif
		if (lastBlock) {
			Led2Off();
			if (!QueueCswToHostWithTimeout(&g_usbDev, g_storageState.tag, g_storageState.writeStatus)) {
				printf("Error, could not queue CSW\r\n");
			}
			g_storageState.writeStatus = 0;
		}
		todo = true;
	}
	return todo;
}

void PrintPerformance(void) {
	unsigned int mainPerc = g_performanceState.ticksMainLast / 10000; //µs to percent
	unsigned int isrPerc = g_performanceState.ticksUsbIsrLast / 10000; //µs to percent
	unsigned int numIsr = g_performanceState.usbIsrCountLast;
	printf("CPU load of flash rw: %2u%c, Usb ISR: %03u - %2u%c, \r\n", mainPerc, '%', numIsr, isrPerc, '%');
}

void TogglePrintPerformance(void) {
	g_performanceState.printPerformance = !g_performanceState.printPerformance;
}

#define BENCH_BLOCK 512
#define BENCH_SIZE (1024 * 512)

void BenchmarkRead() {
	printf("Reading %uKiB\r\n", BENCH_SIZE / 1024);
	uint8_t buffer[BENCH_BLOCK];
	uint64_t tStart = McuTimestampUs();
	for (uint32_t i = 0; i < BENCH_SIZE; i+= BENCH_BLOCK) {
		FlashRead(i, buffer, BENCH_BLOCK);
	}
	uint64_t tStop = McuTimestampUs();
	uint32_t deltaMs = (tStop - tStart) / 1000;
	uint32_t kbs = BENCH_SIZE * 1000 / deltaMs / 1024;
	printf("Took %ums -> %uKiB/s\r\n", (unsigned int)deltaMs, (unsigned int)kbs);
}

void AppCycle(void) {
	//call this loop as fast as possible to get the maxium flash read/write performance

	//handle debug input
	char input = Rs232GetChar();
	if (input) {
		printf("%c", input);
	}
	switch (input) {
		case 'h': MainMenu(); break;
		case 'r': NVIC_SystemReset(); break;
		case 'u': ToggleUsb(); break;
		case 'w': ToggleWriteprotect(); break;
		case 'p': TogglePrintPerformance(); break;
		case 'b': BenchmarkRead(); break;
		default: break;
	}

	if (KeyLeftPressed()) {
		NVIC_SystemReset();
	}
	if (KeyUpPressed()) {
		StorageStart();
	}
	if (KeyDownPressed()) {
		StorageStop();
	}
	//handle LED
	uint32_t stamp = HAL_GetTick();
	if ((stamp - g_storageState.timeTrafficChecked) > 0) {
		g_storageState.timeTrafficChecked = stamp;
		UsbLock();
		if (g_storageState.usbTraffic) {
			g_storageState.usbTraffic--;
		} else {
			Led1Green(); //no ISR for ~2 cycles
		}
		UsbUnlock();
	}
	//handle flash read-write
	uint64_t tStart = McuTimestampUs();
	UsbLock();
	uint64_t ticksIsrStart = g_performanceState.ticksUsbIsr;
	UsbUnlock();
	bool todo = ProcessFlashAccess();
	if (todo) { //otherwise we would measure only the polling and have a higher value the more we poll
		UsbLock();
		uint64_t ticksIsrStop = g_performanceState.ticksUsbIsr;
		UsbUnlock();
		uint32_t tStop = McuTimestampUs();
		g_performanceState.ticksMain += tStop - tStart;
		/* Because the USB ISR runs while the ProcessFlashAccess has been processed,
		   the time for the ISRs is parts of the ticksMain too, so they must be
		   substracted to avoid a result of > 100%.
		   Since the variable is reset in the main loop, it can not overflow
		   during the measurement here.
		*/
		g_performanceState.ticksMain -= (ticksIsrStop - ticksIsrStart);
	}
	//handle performance statistics
	if ((HAL_GetTick() - g_performanceState.tickSampleStart) >= 1000) { //1s passed
		uint32_t lastStart = g_performanceState.tickSampleStart;
		UsbLock(); //atomic update
		uint32_t thisStart = HAL_GetTick();
		uint64_t ticksUsb = g_performanceState.ticksUsbIsr;
		g_performanceState.ticksUsbIsr = 0;
		g_performanceState.tickSampleStart = thisStart;
		uint32_t isrCount = g_performanceState.usbIsrCount;
		g_performanceState.usbIsrCount = 0;
		UsbUnlock();
		uint64_t ticksMain = g_performanceState.ticksMain;
		g_performanceState.ticksMain = 0;
		//if this is run after more than 1000ms, the values needs to be adjusted
		uint32_t delta = thisStart - lastStart;
		if ((delta > 0) && (g_performanceState.printPerformance)) {
			g_performanceState.ticksMainLast = ticksMain * 1000 / delta;
			g_performanceState.ticksUsbIsrLast = ticksUsb * 1000 / delta;
			g_performanceState.usbIsrCountLast = isrCount * 1000 / delta;
			PrintPerformance();
		}
	}
}
