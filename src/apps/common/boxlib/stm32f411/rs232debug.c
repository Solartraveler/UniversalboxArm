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

#include "boxlib/rs232debug.h"

#include "main.h"

#include "boxlib/peripheral.h"

//Minimum allowed value is 2
#define UARTBUFFERLEN 256

char g_uartBuffer[UARTBUFFERLEN];
volatile uint16_t g_uartBufferReadIdx;
volatile uint16_t g_uartBufferWriteIdx;

void Rs232Init(void) {

	GPIO_InitTypeDef GPIO_InitStruct = {0};

	__HAL_RCC_GPIOA_CLK_ENABLE();

	GPIO_InitStruct.Pin = Rs232Tx_Pin | Rs232Rx_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	PeripheralPowerOn();
	g_uartBufferReadIdx = 0;
	g_uartBufferWriteIdx = 0;

	__HAL_RCC_USART2_CLK_ENABLE();
	__HAL_RCC_USART2_FORCE_RESET();
	__HAL_RCC_USART2_RELEASE_RESET();

	/* The lib von ST creates following init values for a USART:
	   CR1: 0xD
	   CR2: 0x0
	   CR3: 0x0
	   BRR: 0x1A1 -> 417 -> 19200 * 417 =>8MHz clock source
	*/

	uint32_t pclk =  HAL_RCC_GetPCLK1Freq();
	const uint32_t baudrate = 19200;

	USART2->BRR = pclk / baudrate;
	USART2->CR1 = USART_CR1_TE | USART_CR1_RE;
	USART2->CR1 |= USART_CR1_UE;
	HAL_NVIC_SetPriority(USART2_IRQn, 8, 0);
	NVIC_EnableIRQ(USART2_IRQn);
}

void Rs232Stop(void) {
	NVIC_DisableIRQ(USART2_IRQn);
	USART2->CR1 &= ~USART_CR1_UE;
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
	uint8_t writeThis = g_uartBufferWriteIdx;
	uint8_t writeNext = (writeThis + 1) % UARTBUFFERLEN;
	if (writeNext != g_uartBufferReadIdx) {
		g_uartBuffer[writeThis] = out;
		g_uartBufferWriteIdx = writeNext;
		succeed = true;
	}
	__disable_irq();
	if ((USART2->CR1 & USART_CR1_TXEIE) == 0) {
		USART2->CR1 |= USART_CR1_TXEIE;
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

void USART2_IRQHandler(void) {
	if (USART2->SR & USART_SR_TXE) {
		char c = Rs232WriteGetChar();
		if (c) {
			USART2->DR = c;
		} else {
			USART2->CR1 &= ~USART_CR1_TXEIE;
		}
	}
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

int printfDirect(const char * format, ...) {
	va_list args;
	va_start(args, format);
	char buffer[256];
	int params = vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);
	USART2->DR = '\0'; //dummy transfer
	for (size_t i = 0; i < sizeof(buffer); i++) {
		while (((USART2->SR) & USART_SR_TXE) == 0);
		char c = buffer[i];
		if (c) {
			USART2->DR = c;
		} else {
			break;
		}
	}
	return params;
}

char Rs232GetChar(void) {
	char val = 0;
	//the sequence to read SR and then DR also clears all error bits
	if ((USART2->SR) & USART_SR_RXNE) {
		val = (char)(USART2->DR);
	}
	return val;
}
