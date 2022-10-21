/* Boxlib
(c) 2022 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause
*/

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "esp.h"

#include "main.h"

void EspInit(void) {
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	__HAL_RCC_USART3_CLK_ENABLE();

	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();

	HAL_GPIO_WritePin(EspPower_GPIO_Port, EspPower_Pin, GPIO_PIN_SET);

	GPIO_InitStruct.Pin = EspPower_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(EspPower_GPIO_Port, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = EspRxArmTx_Pin | EspTxArmRx_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	uint32_t sysclk = HAL_RCC_GetSysClockFreq();
	const uint32_t baudrate = 115200;

	USART3->BRR = sysclk / baudrate;
	USART3->CR1 = USART_CR1_TE | USART_CR1_RE;
}

void EspEnable(void) {
	__HAL_RCC_USART3_CLK_ENABLE();
	USART3->CR1 |= USART_CR1_UE;
	HAL_GPIO_WritePin(EspPower_GPIO_Port, EspPower_Pin, GPIO_PIN_RESET);
}

void EspStop(void) {
	USART3->CR1 &= ~USART_CR1_UE;
	__HAL_RCC_USART3_CLK_DISABLE();
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
	while (*str) {
		USART3->TDR = *str;
		str++;
		while (((USART3->ISR) & USART_ISR_TXE) == 0);
	}
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
