#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "main.h"

#include "forwarder.h"
#include "ff.h"
#include "boxlib/flash.h"

#define CONFIGFILENAME "/etc/display.json"

void ConfigWriteLcd(const char * lcdType) {
	char buffer[256];
	f_mkdir("/etc");
	snprintf(buffer, sizeof(buffer), "{\n  \"lcd\": \"%s\"\n}\n", lcdType);
	FIL f;
	if (FR_OK == f_open(&f, CONFIGFILENAME, FA_WRITE | FA_CREATE_ALWAYS)) {
		UINT written = 0;
		FRESULT res = f_write(&f, buffer, strlen(buffer), &written);
		if (res == FR_OK) {
			printf("Display set\n");
		}
		f_close(&f);
	} else {
		printf("Error, could not create file\n");
	}
}

void GuiLcdSet(const char * type) {
	if (strcmp(type, "128x128") == 0) {
		ConfigWriteLcd("ST7735_128x128");
	} else if (strcmp(type, "160x128") == 0) {
		ConfigWriteLcd("ST7735_160x128");
	} else if (strcmp(type, "320x240") == 0) {
		ConfigWriteLcd("ILI9341_320x240");
	} else {
		ConfigWriteLcd("NONE");
	}
}

//create a file with a 320x240 LCD setting
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
			GuiLcdSet(display);
			f_unmount("0");
		} else {
			printf("Error, could not mount drive\n");
		}
	}
}

void putString(const char * s) {
	size_t l = strlen(s);
	for (size_t i = 0; i < l; i++) {
		Uart4WritePutChar(s[i]);
	}
}

int main(int argc, char ** argv) {
	SimulatedInit();
	if (argc > 1) {
		CreateFilesystem(argv[1]);
	}
	ForwarderInit();
	//could not figure out how to add \n as parameter when called from a makefile
	const char * mystring = "Hello Word!\nThis is a second line.\nSome utf-8: 42°C.\n23µF\n666Ω\nKey up -> terminate.\nThe LED flashes slowly.\nThat's it.";
	//const char * mystring = "This is a second line.\nSome utf-8: 42°C.\n23µF\n666Ω\nKey up -> terminate.\nThe LED flashes slowly.\nThat's it.";
	putString(mystring);
	uint32_t i = 0;
	uint32_t j = 2;
	while(1) {
		ForwarderCycle();
		i++;
		if (i == 20) {
			if (j < argc) {
				char buffer[512];
				snprintf(buffer, sizeof(buffer), "%s\n", argv[j]);
				j++;
				putString(buffer);
			}
			i = 0;
		}
	}
}

void UartCoprocInit(void) {
	//nothing to be done here
}