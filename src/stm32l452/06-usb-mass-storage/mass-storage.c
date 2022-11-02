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

Tested with:
Linux kernel 5.13: Disk needs 10s to appear when using 250KHz as flash clock, no double buffering
Linux kernel 5.13: Disk needs 4s to appear when using 4MHz as flash clock, no double buffering
Windows 10: Disk needs 14s to appear when using 250KHz as flash clock, no double buffering

Measurements are done with 1MiB test size:
dd if=/dev/sdX of=foo.bin count=2048
Without USB doublebuffering:
Reading speed (with 1MiB test) over USB, 16MHz CPU clock, divider for flash:
Divider 64 (250KHz): 21.8kB/s
Divider 32 (500KHz): 37.3kB/s
Divider 16 (1MHz): 60.7kB/s
Divider 8 (2MHz): 81.2kB/s
Divider 4 (4MHz): 97.6kB/s
Divider 4 (4MHz): 94.6kB/s with performance counters, delay in main removed
Dummy RAM disk: 104kB/s -> need to increase USB, not flash performance

With 64MHz CPU clock and the same as bus speed:
Divider 16 (4MHz): 104kB/s
-> USB itself seems to be the bottleneck. It saturates at 3000 ISRs/s, CPU load
for ISR processing goes down from 13% (16MHz) to 3% (64MHz), but data are just
waiting to be received from the host.

With USB doublebuffering and 16MHz:
Divider 4 (4MHz): 93.7kB/s -> nothing changed - looks like the queue is not filled fast enough
and 32MHz:
Divider 8 (4MHz): 160kB/s -> processing 4100 ISR/s (9% of the CPU time)
and 64MHz:
Divider 16 (4MHz): 240kB/s -> processing 5600 ISR/s (6% of the CPU time)
Divider 8 (8MHz): 319kB/s -> (might not work reliable) processing 7160 ISR/s (9% of the CPU time)

64MHz, using 4MiB test size:
dummy RAM disk: 1.1MB/s -> close to the theoretical maximum of 1.2MB/s,
                              processing 18900 ISR/s (31% of the CPU time)
Divider 8 (8MHz): 372kB/s -> Might not work reliable, but would be a satisfying speed :)

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

#define USB_BULK_BLOCKSIZE 64

/* Minimum is 2. 8 allows a full FLASH_BLOCK to be queued, and then the next
   can already be read while the previous one is still transmitted over USB.
*/
#define USB_BULK_QUEUE_LEN 8

//from host to device
#define USB_ENDPOINT_FROMHOST 0x02

//from device to host
#define USB_ENDPOINT_TOHOST 0x81


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
			uint8_t command = cbw->data[0];
			if ((command == 0x12) && (cbw->length >= 6)) { //inquiry command (1. done by the Linux host)
				if (cbw->data[4] >= 36) { //Linux requests 36, Windows requests 255
					scsiInquiry_t dataOut;
					dataOut.deviceType = 0x0;
					dataOut.rmb = 0x80; //removeable media
					dataOut.versions = 0;
					dataOut.responseFormat = 0x0;
					dataOut.additionalLength = 31; //always this value
					strcpy((char *)dataOut.vendorInformation, "Marw");
					strcpy((char *)dataOut.productInformation, "UniversalboxARM");
					memset(dataOut.productrevisionLevel, 0, 4);
					StorageQueueToHost(dev, &dataOut, sizeof(dataOut));
					StorageQueueCsw(dev, cbw->tag, 0);
				} else {
					g_storageState.senseKey = 0x5; //illegal request
					g_storageState.additionalSenseCode = 0x24; //invalid field in command packet
					StorageQueueCsw(dev, cbw->tag, 1);
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
			} else if ((command == 0x25) && (cbw->length >= 6)) { //read capacity (3. request by the host)
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
				uint8_t data[4];
				data[0] = 3; //three bytes follow
				data[1] = 0; //medium type
				if (g_storageState.writeProtected) {
					data[2] = 0x80;
				} else {
					data[2] = 0x0;
				}
				data[3] = 0; //block descriptor length
				StorageQueueToHost(dev, data, sizeof(data));
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
			} else { //unsupported command
				printfNowait("Cmd 0x%x unsuppored\r\n", command);
				g_storageState.senseKey = 0x5;
				g_storageState.additionalSenseCode = 0x20; //invalid command operational code
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

void EndpointEventTx(usbd_device *dev, uint8_t event, uint8_t ep) {
	if ((ep == USB_ENDPOINT_TOHOST) && (event == usbd_evt_eptx)) {
		g_storageState.toHostFree++;
		StorageDequeueToHost(dev);
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
			if (!usbd_ep_config(dev, USB_ENDPOINT_TOHOST, USB_EPTYPE_BULK | USB_EPTYPE_DBLBUF, USB_BULK_BLOCKSIZE)) {
				printfNowait("Error, configure ep to host\r\n");
			} else {
				g_storageState.toHostFree = 2;
			}
			if (!usbd_ep_config(dev, USB_ENDPOINT_FROMHOST, USB_EPTYPE_BULK | USB_EPTYPE_DBLBUF, USB_BULK_BLOCKSIZE)) {
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
	printf("r: Reboot with reset controller\r\n");
}

void AppInit(void) {
	LedsInit();
	Led1Green();
	McuClockToHsiPll(64000000, RCC_HCLK_DIV1);
	PeripheralPowerOff();
	HAL_Delay(100);
	PeripheralPowerOn();
	Rs232Init();
	printf("\r\nUSB Mass-storage %s\r\n", APPVERSION);
	KeysInit();
	CoprocInit();
	PeripheralInit();
	FlashEnable(16); //4MHz
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
