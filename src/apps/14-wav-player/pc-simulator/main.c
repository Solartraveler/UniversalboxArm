#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "main.h"

#include "wavplayer.h"
#include "ff.h"

#include "simhelper.h"


/* Parameters:
   [LCD resolution] [filenameSource] [filenameDestination]
   repeating source and destination for multiple files.
*/
int main(int argc, char ** argv) {
	signal(SIGTERM, CatchSignal);
	signal(SIGHUP, CatchSignal);
	signal(SIGINT, CatchSignal);
	SimulatedInit();
	if (argc > 1) {
		CreateFilesystem(argv[1]);
		uint32_t copied = 0;
		for (int i = 0; i < ((argc - 2) / 2); i++) {
			bool success = CopyFileToFilesystem(argv[i * 2 + 2], argv[i * 2 + 3]);
			if (success) {
				copied++;
			}
		}
		printf("Copied %u files to the internal filesystem\n", copied);
	}
	AppInit();
	while(1) {
		AppCycle();
	}
}
