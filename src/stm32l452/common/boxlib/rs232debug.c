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

#include "peripheral.h"

//Minimum allowed value is 2
#define UARTBUFFERLEN 256

char g_uartBuffer[UARTBUFFERLEN];
volatile uint16_t g_uartBufferReadIdx;
volatile uint16_t g_uartBufferWriteIdx;

void Rs232Init(void) {
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	__HAL_RCC_GPIOB_CLK_ENABLE();

	GPIO_InitStruct.Pin = Rs232Tx_Pin | Rs232Rx_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	PeripheralPowerOn();
	g_uartBufferReadIdx = 0;
	g_uartBufferWriteIdx = 0;

	__HAL_RCC_USART1_CLK_ENABLE();
	__HAL_RCC_USART1_FORCE_RESET();
	__HAL_RCC_USART1_RELEASE_RESET();

	/* The lib von ST creates following init values for a USART:
	   CR1: 0xD
	   CR2: 0x0
	   CR3: 0x0
	   BRR: 0x1A1 -> 417 -> 19200 * 417 =>8MHz clock source
	*/

	uint32_t pclk =  HAL_RCC_GetPCLK2Freq();
	const uint32_t baudrate = 19200;

	USART1->BRR = pclk / baudrate;
	USART1->CR1 = USART_CR1_TE | USART_CR1_RE;
	USART1->CR1 |= USART_CR1_UE;

	NVIC_EnableIRQ(USART1_IRQn);
}

void Rs232Stop(void) {
	NVIC_DisableIRQ(USART1_IRQn);
	USART1->CR1 &= ~USART_CR1_UE;
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
	if ((USART1->CR1 & USART_CR1_TXEIE) == 0) {
		USART1->CR1 |= USART_CR1_TXEIE;
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
	if (USART1->ISR & USART_ISR_TXE) {
		char c = Rs232WriteGetChar();
		if (c) {
			USART1->TDR = c;
		} else {
			USART1->CR1 &= ~USART_CR1_TXEIE;
		}
	}
	//just clear all flags
	USART1->ICR |= USART_ICR_PECF | USART_ICR_FECF | USART_ICR_NECF | USART_ICR_ORECF | USART_ICR_IDLECF | USART_ICR_TCCF | USART_ICR_TCBGTCF | USART_ICR_LBDCF | USART_ICR_CTSCF | USART_ICR_RTOCF | USART_ICR_EOBCF | USART_ICR_CMCF | USART_ICR_WUCF;
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
	USART1->TDR = '\0'; //dummy transfer
	for (size_t i = 0; i < sizeof(buffer); i++) {
		while (((USART1->ISR) & USART_ISR_TXE) == 0);
		char c = buffer[i];
		if (c) {
			USART1->TDR = c;
		} else {
			break;
		}
	}
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
