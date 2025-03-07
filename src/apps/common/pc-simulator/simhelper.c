/* Simulation helper
(c) 2022 by Malte Marwedel

SPDX-License-Identifier: BSD-3-Clause
*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "simhelper.h"

#include "ff.h"

#include "boxlib/flash.h"
#include "boxlib/rs232debug.h"
#include "boxlib/lcd.h"
#include <sys/stat.h>
#include "filesystem.h"

//create a filesystem and configuration with a LCD setting
void CreateFilesystem(const char * display) {
	FlashEnable(2);
	uint32_t buffer[512];
	FRESULT fres;
	fres = f_mkfs("0", NULL, &buffer, sizeof(buffer));
	FATFS fatfs = {0};
	if (fres == FR_OK) {
		printf("Formatting done\n");
		fres = f_mount(&fatfs, "0", 1); //now mount
		if (fres == FR_OK) {
			printf("Mounting done\n");
			FilesystemLcdSet(display);
			f_unmount("0");
		} else {
			printf("Error, could not mount drive\n");
		}
	}
}

void CatchSignal(int sig) {
	printf("Terminate by signal %u requested. Stopping threads...\n", sig);
	Rs232Stop();
	LcdDisable();
	printf("Stopped. Terminating now\n");
	exit(0);
}

bool CopyFileToFilesystem(const char * filenameSource, const char * filenameDest) {
	//Get the source file content
	struct stat statbuf;
	if (stat(filenameSource, &statbuf)) {
		printf("Error, could not stat %s\n", filenameSource);
		return false;
	}
	void * buffer = malloc(statbuf.st_size);
	if (!buffer) {
		printf("Error, %s too large?\n", filenameSource);
		return false;
	}
	FILE * f = fopen(filenameSource, "rb");
	if (!f) {
		printf("Error, could not open file %s\n", filenameSource);
		free(buffer);
		return false;
	}
	int result = fread(buffer, statbuf.st_size, 1, f);
	fclose(f);
	if (result != 1) {
		printf("Error, could not read file %s\n", filenameSource);
		free(buffer);
		return false;
	}

	//Write to the destination
	bool success = false;
	FATFS fatfs = {0};
	FRESULT fres = f_mount(&fatfs, "0", 1);
	if (fres == FR_OK) {
		success = FilesystemWriteFile(filenameDest, buffer, statbuf.st_size);
		f_unmount("0");
	} else {
		printf("Error, could not mount drive\n");
	}
	free(buffer);
	return success;
}
