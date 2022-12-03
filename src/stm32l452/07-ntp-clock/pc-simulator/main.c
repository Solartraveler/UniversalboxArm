#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "main.h"

#include "ntp-clock.h"
#include "ff.h"

#include "simhelper.h"

int main(int argc, char ** argv) {
	signal(SIGTERM, CatchSignal);
	signal(SIGHUP, CatchSignal);
	signal(SIGINT, CatchSignal);
	SimulatedInit();
	if (argc > 1) {
		CreateFilesystem(argv[1]);
	}
	AppInit(); //does not return
	return -1;
}
