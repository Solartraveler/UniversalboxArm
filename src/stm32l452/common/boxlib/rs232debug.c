/* Boxlib
(c) 2021 by Malte Marwedel

SPDX-License-Identifier: BSD-3-Clause
*/

#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "rs232debug.h"

#include "main.h"
#include "usart.h"

#include "peripheral.h"

void rs232Init(void) {
	PeripheralPowerOn();
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

void rs232WriteString(const char * str) {
	size_t len = strlen(str);
	HAL_UART_Transmit(&huart1, (uint8_t*)str, len, 1000);
}

int putchar(int c) {
	uint8_t x = c;
	HAL_UART_Transmit(&huart1, &x, 1, 1000);
	return c;
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
