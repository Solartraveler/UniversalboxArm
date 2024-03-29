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

bool FilesystemReadFile(const char * filename, void * data, size_t bufferLen, size_t * pReadLen);

bool FilesystemWriteFile(const char * filename, const void * data, size_t dataLen);

//Like FilesystemWriteFile, but creates the folder /etc before
bool FilesystemWriteEtcFile(const char * filename, const void * data, size_t dataLen);

void FilesystemWriteLcd(const char * lcdType);

//acceptes strings "128x128", "160x128" and "320x240", all else will set "no display"
void FilesystemLcdSet(const char * type);

/*Three functions to write buffered data to the file
1x call FilesystemBufferwriterStart
0x..Nx call FilesystemBufferwriterAppend
Finally call FilesystemBufferwriterClose
All return true in the case of a success
*/

#define FILEBUFFER_SIZE 512

typedef struct {
	FIL f;
	uint8_t buffer[FILEBUFFER_SIZE];
	size_t index;
} fileBuffer_t;

bool FilesystemBufferwriterStart(fileBuffer_t * pFileBuffer, const char * filename);
//pFB must be of type fileBuffer_t, but using void * allows easier use in interfaces
bool FilesystemBufferwriterAppend(const void * data, size_t len, void * pFileBuffer);
bool FilesystemBufferwriterClose(fileBuffer_t * pFileBuffer);

/* Scan a folder for an unused number, with a prefix of "img", and there are files
named like img002.png, img003.png, img004.png in the directory, then 5 will be
returned, as 004 is the highest found. So if none is found 1 is returned.

Returns the first index name to be used, 0 if something went wrong.
*/
uint32_t FilesystemGetUnusedFilename(const char * directory, const char * prefix);
