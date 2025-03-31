#pragma once

/*
Incrementing the buffer size to 3084 (and incrementing the USB descriptor
accordingly, only increases the download speed by ~5%. So the benefit is
negligible.
*/
#define USB_BUFFERSIZE_BYTES 2060

#define ROM_BOOTLOADER_START_ADDRESS 0x1FFF0000

extern uint8_t * g_DfuMem;
extern size_t g_DfuMemSize;
