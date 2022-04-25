#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include "main.h"

#include "loader.h"
#include "boxlib/boxusb.h"

uint8_t * g_DfuMem;
size_t g_DfuMemSize;


void UsbControlCall(uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex,
                    uint8_t * data, size_t len) {
	uint8_t buffer[USB_BUFFERSIZE_BYTES]; //bytes must be larger than the structure
	usbd_ctlreq * request = (usbd_ctlreq *)buffer;
	request->bmRequestType = bmRequestType;
	request->bRequest = bRequest;
	request->wValue = wValue; //alternate interface select
	request->wIndex = wIndex;
	request->wLength = len;
	if (len <= (USB_BUFFERSIZE_BYTES - 8)) {
		if ((data) && (len)) {
			memcpy(request->data, data, len);
		}
		__disable_irq();
		if (g_usbControlCallback(g_pUsbDev, request, NULL) == usbd_ack) {
			if ((data) && (len)) {
				memcpy(data, request->data, len);
			}
		} else {
			printf("Error, request %x-%x failed\n", bmRequestType, bRequest);
		}
		__enable_irq();
	}
}

void UsbSetInterface(uint16_t interface) {
	UsbControlCall(0x1, 0xB, interface, 0, NULL, 0);
	__enable_irq();
}

uint8_t UsbDfuGetStatus(void) {
	uint8_t data[6] = {0};
	memset(data, 0xEE, sizeof(data));
	UsbControlCall(0xA1, 0x03, 0, 0, data, sizeof(data));
	if (data[0] != 0) {
		printf("Error, command failed\n");
	}
	return data[4];
}

void UsbDfuErasePage(void) {
	uint8_t cmd[1] = {0x41}; //erase
	UsbControlCall(0x21, 0x1, 0, 0, cmd, sizeof(cmd));
}

void UsbDfuSetAddress(uint32_t addr) {
	addr += 0x1000; //dummy offset to satisfy dfu-util
	uint8_t data[5];
	data[0] = 0x21;
	data[1] = addr & 0xFF;
	data[2] = (addr >> 8) & 0xFF;
	data[3] = (addr >> 16) & 0xFF;
	data[4] = (addr >> 24) & 0xFF;
	UsbControlCall(0x21, 0x01, 0, 0, data, sizeof(data));
}

void UsbDfuSendData(uint32_t blockId, uint8_t * data, size_t len) {
	UsbControlCall(0x21, 0x01, blockId, 0, data, len);
}

void UsbDfuAbort(void) {
	UsbControlCall(0x21, 0x06, 0, 0, NULL, 0);
}

//The commands send represent those found by using dfu-util
void * UsbDfuUploader(void * parameter) {
	const char * filename = (const char*)parameter;
	FILE * f = fopen(filename, "rb");
	if (f) {
		printf("Filling with DFU packets\n");
		//1. set interface to alternate 1
		UsbSetInterface(1);
		//2. check if we are in dfu idle state
		uint8_t status = UsbDfuGetStatus();
		if (status != 2) {
			printf("Error, status not DFU idle. Returned %u\n", status);
			return NULL;
		}
		//copy file
		size_t blocksize = USB_BUFFERSIZE_BYTES - 8;
		uint8_t buffer[USB_BUFFERSIZE_BYTES - 8];
		size_t r;
		uint32_t offset = 0;
		uint32_t block = 1;
		do {
			r = fread(buffer, 1, blocksize, f);
			if (r) {
				UsbDfuErasePage();
				UsbDfuGetStatus();
				UsbDfuGetStatus();
				UsbDfuSetAddress(offset);
				UsbDfuGetStatus();
				UsbDfuGetStatus();
				UsbDfuSendData(block, buffer, r);
				UsbDfuGetStatus();
				block++;
				offset += r;
				usleep(50000);
			}
		} while (r == blocksize);
		fclose(f);
		UsbDfuAbort();
		UsbDfuGetStatus();
		//the following is only requested if a :leave is added to dfu-util commands
#if 0
		UsbDfuSetAddress(0);
		UsbDfuGetStatus();
		UsbDfuGetStatus();
		UsbDfuSendData(block, NULL, 0);
		UsbDfuGetStatus();
		UsbDfuGetStatus();
#endif
		//The following would terminate the file and start the new program:
		//UsbDfuSendData(block, NULL, 0);
		//UsbDfuGetStatus();
	} else {
		printf("Error, could not open %s\n", filename);
	}
	return NULL;
}

int main(int argc, char ** argv) {
	g_DfuMemSize = (132 * 1024);
	g_DfuMem = (uint8_t *)malloc(g_DfuMemSize);
	if (!g_DfuMem) {
		return 1;
	}
	memset(g_DfuMem, 0xEE, g_DfuMemSize);
	SimulatedInit();
	LoaderInit();
	pthread_t thread;
	if (argc == 2) {
		pthread_create(&thread, NULL, &UsbDfuUploader, argv[1]);
	}
	while(1) {
		LoaderCycle();
	}
}
