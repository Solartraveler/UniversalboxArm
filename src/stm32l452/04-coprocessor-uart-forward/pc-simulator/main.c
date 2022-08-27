#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "main.h"

#include "forwarder.h"

#include "simhelper.h"

void putString(const char * s) {
	size_t l = strlen(s);
	for (size_t i = 0; i < l; i++) {
		Uart4WritePutChar(s[i]);
	}
}

int main(int argc, char ** argv) {
	signal(SIGTERM, CatchSignal);
	signal(SIGHUP, CatchSignal);
	signal(SIGINT, CatchSignal);
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