/* Boxlib
(c) 2021 by Malte Marwedel

SPDX-License-Identifier: BSD-3-Clause
*/

#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "rs232debug.h"

#include "main.h"
#include "usart.h"

#include "peripheral.h"

#define UARTBUFFERLEN 256

char g_uartBuffer[UARTBUFFERLEN];
volatile uint16_t g_uartBufferReadIdx;
volatile uint16_t g_uartBufferWriteIdx;

void rs232Init(void) {
	PeripheralPowerOn();
	g_uartBufferReadIdx = 0;
	g_uartBufferWriteIdx = 0;
	NVIC_EnableIRQ(USART1_IRQn);
}

//sending fifo get
char rs232WriteGetChar(void) {
	char out = 0;
	if (g_uartBufferReadIdx != g_uartBufferWriteIdx) {
		uint8_t ri = g_uartBufferReadIdx;
		out = g_uartBuffer[ri];
		__sync_synchronize(); //the pointer increment may only be visible after the copy
		ri = (ri + 1) % UARTBUFFERLEN;
		g_uartBufferReadIdx = ri;
	}
	return out;
}

//sending fifo put
//returns true if the char could be put into the queue
bool rs232WritePutChar(char out) {
	bool succeed = false;
	UART_HandleTypeDef * phuart = &huart1;
	uint8_t writeThis = g_uartBufferWriteIdx;
	uint8_t writeNext = (writeThis + 1) % UARTBUFFERLEN;
	if (writeNext != g_uartBufferReadIdx) {
		g_uartBuffer[writeThis] = out;
		g_uartBufferWriteIdx = writeNext;
		succeed = true;
	}
	__disable_irq();
	if (__HAL_UART_GET_IT_SOURCE(phuart, UART_IT_TXE) == RESET) {
		__HAL_UART_ENABLE_IT(phuart, UART_IT_TXE);
	}
	__enable_irq();
	return succeed;
}

void rs232Flush(void) {
	/* The countdown is needed, should an interrupt or other thread fill the FIFO
	   always to the top. Without the countdown, the Flush could wait endless in
	   this case.
	*/
	uint16_t countdown = UARTBUFFERLEN;
	uint16_t lastIndex = g_uartBufferReadIdx;
	while ((g_uartBufferReadIdx != g_uartBufferWriteIdx) && (countdown))
	{
		if (lastIndex != g_uartBufferReadIdx)
		{
			lastIndex = g_uartBufferReadIdx;
			countdown--;
		}
	}
}

void USART1_IRQHandler(void) {
	UART_HandleTypeDef * phuart = &huart1;
	if (__HAL_UART_GET_FLAG(phuart, UART_FLAG_TXE) == SET) {
		char c = rs232WriteGetChar();
		if (c) {
			phuart->Instance->TDR = c;
		} else {
			__HAL_UART_DISABLE_IT(phuart, UART_IT_TXE);
		}
	}
	//just clear all flags
	__HAL_UART_CLEAR_FLAG(phuart, UART_CLEAR_PEF | UART_CLEAR_FEF | UART_CLEAR_NEF | UART_CLEAR_OREF | UART_CLEAR_IDLEF | UART_CLEAR_TCF | UART_CLEAR_LBDF | UART_CLEAR_CTSF | UART_CLEAR_CMF | UART_CLEAR_WUF | UART_CLEAR_RTOF);
}


int putchar(int c) {
	while (rs232WritePutChar(c) == false);
	return c;
}

//returns as soon as all data are in the FIFO. Use for normal prints
void rs232WriteString(const char * str) {
	while (*str) {
		putchar(*str);
		str++;
	}
}

//tries to put the chars to the FIFO. If the fifo is full, they are discarded
//Use for prints from within an interrupt routine
void rs232WriteStringNoWait(const char * str) {
	while (*str) {
		if (rs232WritePutChar(*str) == false) {
			break;
		}
		str++;
	}
}

int puts(const char * string) {
	rs232WriteString(string);
	rs232WriteString("\n");
	return 0;
}

int printf(const char * format, ...) {
	va_list args;
	va_start(args, format);
	char buffer[256];
	int params = vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);
	rs232WriteString(buffer);
	return params;
}

int printfNowait(const char * format, ...) {
	va_list args;
	va_start(args, format);
	char buffer[256];
	int params = vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);
	rs232WriteStringNoWait(buffer);
	return params;
}

char rs232GetChar(void) {
	char val = 0;
	if ((USART1->ISR) & USART_ISR_RXNE) {
		val = (char)(USART1->RDR);
	}
	/* If there are too many data, the ISR register switches from
	  0x6000d0 to 0x6010d8
		This is USART_ICR_ORECF and USART_ICR_EOBCF
		So we need to clear those flags
	*/
	USART1->ICR = USART_ICR_ORECF | USART_ICR_EOBCF;
	return val;
}
