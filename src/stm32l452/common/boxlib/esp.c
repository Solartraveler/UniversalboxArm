/* Boxlib
(c) 2022 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause
*/

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "esp.h"

#include "main.h"
#include "usart.h"

void EspEnable(void) {
	MX_USART3_UART_Init();
	HAL_GPIO_WritePin(EspPower_GPIO_Port, EspPower_Pin, GPIO_PIN_RESET);
}

void EspStop(void) {
	HAL_UART_DeInit(&huart3);
	HAL_GPIO_WritePin(EspPower_GPIO_Port, EspPower_Pin, GPIO_PIN_SET);
}

char EspGetChar(void) {
	char val = 0;
	if ((USART3->ISR) & USART_ISR_RXNE) {
		val = (char)(USART3->RDR);
	}
	/* If there are too many data, the ISR register switches from
	  0x6000d0 to 0x6010d8
		This is USART_ICR_ORECF and USART_ICR_EOBCF
		So we need to clear those flags
	*/
	USART3->ICR = USART_ICR_ORECF | USART_ICR_EOBCF;
	return val;
}

void EspSendString(const char * str) {
	size_t len = strlen(str);
	HAL_UART_Transmit(&huart3, (uint8_t*)str, len, 1000);
}

uint32_t EspCommand(const char * command, char * response, size_t maxResponse, uint32_t timeout) {
	const char * good = "OK\r\n";
	const char * bad = "ERROR\r\n";
	const size_t goodl = strlen(good);
	const size_t badl = strlen(bad);
	EspSendString(command);
	timeout += HAL_GetTick();
	size_t i = 0;
	while ((timeout > HAL_GetTick()) && (maxResponse > 1)) {
		char c = EspGetChar();
		if (c != 0) {
			response[i] = c;
			i++;
			maxResponse--;
		}
		if (((i >= goodl) && (memcmp(response + i - goodl, good, goodl) == 0)) ||
		    ((i >= badl) && (memcmp(response + i - badl, bad, badl) == 0))) {
			break;
		}
	}
	response[i] = '\0';
	return i;
}
