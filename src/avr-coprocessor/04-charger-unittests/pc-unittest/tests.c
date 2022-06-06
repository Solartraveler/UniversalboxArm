#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>

#include "hardware.h"
#include "timing.h"


uint64_t g_timestamp;
uint8_t g_ledState;

//see: https://stackoverflow.com/questions/3756323/how-to-get-the-current-time-in-milliseconds-from-c-in-linux
uint64_t TimeGetMs(void) {
	struct timeval te;
	gettimeofday(&te, NULL);
	return te.tv_sec * 1000LL + te.tv_usec / 1000;
}

void HardwareInit(void) {
}

void LedOn(void) {
	if (g_ledState == 0)
	{
		printf("Led on\n");
		g_ledState = 1;
	}
}

void LedOff(void) {
	if (g_ledState)
	{
		printf("Led off\n");
		g_ledState = 0;
	}
}

void ArmRun(void) {
}

void ArmUserprog(void) {

}
