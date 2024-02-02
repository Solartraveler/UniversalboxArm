#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "main.h"

#include "freertos-helloworld.h"
#include "boxlib/lcd.h"

int main(void) {
	SimulatedInit();
	AppInit(); //does not return
	return -1;
}
