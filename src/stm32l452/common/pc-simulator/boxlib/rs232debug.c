/* Boxlib emulation
(c) 2022 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause
*/

#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <pthread.h>
#include <poll.h>

#include "rs232debug.h"

#include "peripheral.h"

pthread_t g_keyboardThread;
volatile bool g_keyboardThreadTerminate;

char g_inputChar;

void * ConsoleInput(void * parameter) {
	struct pollfd fds;
	fds.fd = 0;
	fds.events = POLLIN;
	fds.revents = 0;

	/* "stty raw" would allow reading every char but also includes -isig.
	   This would block the user to terminate the program by control+c
	   Just -icanon allows reading every char, and not waiting for a newline.
	   The other two parameters set the minium input lenght to 1 char and the
	   timeout to 0.
	   -echo disables echoing the input characters, we already do this in the
	   application.
	*/
	system("stty -icanon min 1 time 0 -echo");
	while(g_keyboardThreadTerminate == false) {
		if (poll(&fds, 1, 10) > 0) {
			char val = getchar();
			if (val != 0) {
				//printf("%u\n", val);
			}
			if (isascii(val)) {
				__atomic_store_n(&g_inputChar, val, __ATOMIC_SEQ_CST);
			}
		}
	}
	pthread_exit(0);
	return NULL;
}

void Rs232Init(void) {
	PeripheralPowerOn();
	g_keyboardThreadTerminate = false;
	pthread_create(&g_keyboardThread, NULL, &ConsoleInput, NULL);
}

void Rs232Stop(void) {
	g_keyboardThreadTerminate = true;
	if (g_keyboardThread) {
		pthread_join(g_keyboardThread, NULL);
		g_keyboardThread = 0;
	}
	system("stty cooked echo");
}

void Rs232Flush(void) {
	fflush(stdout);
}

void Rs232WriteString(const char * str) {
	printf("%s", str);
}

void Rs232WriteStringNoWait(const char * str) {
	printf("%s", str);
}

int printfNowait(const char * format, ...) {
	va_list args;
	va_start(args, format);
	char buffer[256];
	int params = vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);
	Rs232WriteStringNoWait(buffer);
	return params;
}

char Rs232GetChar(void) {
	char input = __atomic_exchange_n(&g_inputChar, 0, __ATOMIC_SEQ_CST);
	return input;
}
