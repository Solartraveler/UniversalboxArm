#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "boxlib/lcd.h"
#include "ff.h"

#define DISPLAYFILENAME "/etc/display.json"

extern FATFS g_fatfs;

//assumes the filesystem is not mounted. Returns true if successful
bool FilesystemMount(void);

//Requires a mounted filesystem
eDisplay_t FilesystemReadLcd(void);

bool FilesystemWriteFile(const char * filename, const void * data, size_t dataLen);

//Like FilesystemWriteFile, but creates the folder /etc before
bool FilesystemWriteEtcFile(const char * filename, const void * data, size_t dataLen);

void FilesystemWriteLcd(const char * lcdType);

//acceptes strings "128x128", "160x128" and "320x240", all else will set "no display"
void FilesystemLcdSet(const char * type);
