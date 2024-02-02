/* Coprocessor-uart-forward
(c) 2022 by Malte Marwedel

SPDX-License-Identifier: GPL-3.0-or-later
*/

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>

#include "forwarder.h"

#include "boxlib/keys.h"
#include "boxlib/leds.h"
#include "boxlib/rs232debug.h"
#include "boxlib/lcd.h"
#include "boxlib/flash.h"
#include "boxlib/peripheral.h"

#include "main.h"

#include "filesystem.h"

#include "utility.h"

#include "json.h"
#include "gui.h"
#include "uartCoproc.h"

#define UARTBUFFERLEN 1024

char g_uart4Buffer[UARTBUFFERLEN];
volatile uint16_t g_uart4BufferReadIdx;
volatile uint16_t g_uart4BufferWriteIdx;

void ForwarderInit(void) {
	LedsInit();
	Led1Yellow();
	PeripheralPowerOff();
	HAL_Delay(100);
	PeripheralPowerOn();
	Rs232Init();
	printf("\r\nCoprocessor UART forwarder %s\r\n", APPVERSION);
	printf("\r\nReset with 'r' or key up\r\n");
	KeysInit();
	PeripheralInit();
	FlashEnable(4); //4MHz
	FilesystemMount();
	GuiInit();
	UartCoprocInit();
	Led1Green();
}

//rx fifo get
char Uart4WriteGetChar(void) {
	char out = 0;
	if (g_uart4BufferReadIdx != g_uart4BufferWriteIdx) {
		uint8_t ri = g_uart4BufferReadIdx;
		out = g_uart4Buffer[ri];
		__sync_synchronize(); //the pointer increment may only be visible after the copy
		ri = (ri + 1) % UARTBUFFERLEN;
		g_uart4BufferReadIdx = ri;
	} else {
		Led1Off();
	}
	return out;
}

//returns true if the char could be put into the queue
bool Uart4WritePutChar(char out) {
	bool succeed = false;
	uint8_t writeThis = g_uart4BufferWriteIdx;
	uint8_t writeNext = (writeThis + 1) % UARTBUFFERLEN;
	if (writeNext != g_uart4BufferReadIdx) {
		g_uart4Buffer[writeThis] = out;
		g_uart4BufferWriteIdx = writeNext;
		succeed = true;
	}
	return succeed;
}

void ForwarderCycle(void) {
	static uint32_t ledCycle = 0;
	//led flash
	if (ledCycle < 250) {
		Led2Green();
	} else {
		Led2Off();
	}
	if (ledCycle >= 500) {
		ledCycle = 0;
	}
	ledCycle++;
	char input = Rs232GetChar();
	bool reset = false;
	if (input) {
		printf("%c", input);
		if (input == 'r') {
			reset = true;
		}
	}
	if (KeyUpPressed()) {
		reset = true;
	}
	if (reset) {
		printf("Reset selected\r\n");
		Rs232Flush();
		NVIC_SystemReset();
	}
	GuiCycle(input);
	char buffer[64] = {0};
	uint32_t chars = 0;
	do {
		input = Uart4WriteGetChar();
		if ((chars > 0) && (input == 0)) {
			/* This avoids to redraw the screen already after the first char and then
			   redrawing the screen after all other chars are present. Instead
			   redraw once after all data are collected. This speeds things up.
			*/
			HAL_Delay(11); //~8.3ms per char @1200baud
			input = Uart4WriteGetChar();
		}
		buffer[chars] = input;
		chars++;
	} while ((input != 0) && ((chars + 1) < sizeof(buffer)));
	if (chars > 1) {
		printf("%s", buffer);
		GuiAppendString(buffer);
	}
	HAL_Delay(1); //call this loop ~1000x per second
}
