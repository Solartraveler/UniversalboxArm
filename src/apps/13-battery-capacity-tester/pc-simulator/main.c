#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "main.h"

#include "capTester.h"

#include "simhelper.h"

int main(int argc, char ** argv) {
	signal(SIGTERM, CatchSignal);
	signal(SIGHUP, CatchSignal);
	signal(SIGINT, CatchSignal);
	SimulatedInit();
	CreateFilesystem("320x240");
	AppInit();
	while(1) {
		AppCycle();
	}
}
