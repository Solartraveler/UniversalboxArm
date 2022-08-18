#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "boxlib/lcd.h"
#include "ff.h"

#define CONFIGFILENAME "/etc/display.json"

extern FATFS g_fatfs;

//assumes the filesystem is not mounted. Returns true if successful
bool FilesystemMount(void);

//Requires a mounted filesystem
eDisplay_t FilesystemReadLcd(void);
