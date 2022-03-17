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

//Minimum allowed value is 2
#define UARTBUFFERLEN 256

char g_uartBuffer[UARTBUFFERLEN];
volatile uint16_t g_uartBufferReadIdx;
volatile uint16_t g_uartBufferWriteIdx;

void Rs232Init(void) {
	PeripheralPowerOn();
	g_uartBufferReadIdx = 0;
	g_uartBufferWriteIdx = 0;
	MX_USART1_UART_Init();
	NVIC_EnableIRQ(USART1_IRQn);
}

void Rs232Stop(void) {
	NVIC_DisableIRQ(USART1_IRQn);
	HAL_UART_MspDeInit(&huart1);
}

//sending fifo get
char Rs232WriteGetChar(void) {
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
bool Rs232WritePutChar(char out) {
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

void Rs232Flush(void) {
	/* The countdown is needed, should an interrupt or other thread fill the FIFO
	   always to the top. Without the countdown, the Flush could wait endless in
	   this case.
	*/
	uint16_t countdown = UARTBUFFERLEN;
	uint16_t lastIndex = g_uartBufferReadIdx;
	uint16_t nextIndex = nextIndex;
	while ((nextIndex != g_uartBufferWriteIdx) && (countdown)) {
		nextIndex = g_uartBufferReadIdx;
		if (lastIndex != nextIndex) {
			lastIndex = nextIndex;
			countdown--;
		}
	}
	//There can be one byte in the shift register, needing ~0.5ms to be sent
	HAL_Delay(1);
}

void USART1_IRQHandler(void) {
	UART_HandleTypeDef * phuart = &huart1;
	if (__HAL_UART_GET_FLAG(phuart, UART_FLAG_TXE) == SET) {
		char c = Rs232WriteGetChar();
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
	while (Rs232WritePutChar(c) == false);
	return c;
}

//returns as soon as all data are in the FIFO. Use for normal prints
void Rs232WriteString(const char * str) {
	while (*str) {
		putchar(*str);
		str++;
	}
}

//tries to put the chars to the FIFO. If the fifo is full, they are discarded
//Use for prints from within an interrupt routine
void Rs232WriteStringNoWait(const char * str) {
	while (*str) {
		if (Rs232WritePutChar(*str) == false) {
			break;
		}
		str++;
	}
}

int puts(const char * string) {
	Rs232WriteString(string);
	Rs232WriteString("\n");
	return 0;
}

int printf(const char * format, ...) {
	va_list args;
	va_start(args, format);
	char buffer[256];
	int params = vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);
	Rs232WriteString(buffer);
	return params;
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
