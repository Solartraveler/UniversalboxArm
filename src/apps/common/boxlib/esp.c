/* Boxlib
(c) 2022 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause
*/

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "esp.h"

#include "main.h"

#include "utility.h"

/* At 115200baud, 128 byte are enough for buffering 11ms. So with a preemptive
scheduler and 10 tasks, cycling every 1ms, this should be enough.
*/
#define ESPBUFFERLEN 128

char g_espRxBuffer[ESPBUFFERLEN];
volatile uint16_t g_espRxBufferReadIdx;
volatile uint16_t g_espRxBufferWriteIdx;

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

	RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

	PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART3;
	PeriphClkInit.Usart3ClockSelection = RCC_USART3CLKSOURCE_SYSCLK;
	PeriphClkInit.AdcClockSelection = RCC_ADCCLKSOURCE_SYSCLK;
	HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit);

	uint32_t sysclk = HAL_RCC_GetSysClockFreq();
	const uint32_t baudrate = 115200; //bootup garbage can be decoded at 74880baud

	USART3->BRR = sysclk / baudrate;
	USART3->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE;
}

//rx fifo get
bool EspRxFifoGetChar(char * out) {
	*out = '\0';
	if (g_espRxBufferReadIdx != g_espRxBufferWriteIdx) {
		uint8_t ri = g_espRxBufferReadIdx;
		*out = g_espRxBuffer[ri];
		__sync_synchronize(); //the pointer increment may only be visible after the copy
		ri = (ri + 1) % ESPBUFFERLEN;
		g_espRxBufferReadIdx = ri;
		return true;
	}
	return false;
}

//returns true if the char could be put into the queue
bool EspRxFifoPutChar(char out) {
	bool succeed = false;
	uint8_t writeThis = g_espRxBufferWriteIdx;
	uint8_t writeNext = (writeThis + 1) % ESPBUFFERLEN;
	if (writeNext != g_espRxBufferReadIdx) {
		g_espRxBuffer[writeThis] = out;
		g_espRxBufferWriteIdx = writeNext;
		succeed = true;
	}
	return succeed;
}

void USART3_IRQHandler(void) {
	if ((USART3->ISR) & USART_ISR_RXNE) {
		char c = USART3->RDR;
		EspRxFifoPutChar(c);
	}
	//just clear all flags
	USART3->ICR |= USART_ICR_PECF | USART_ICR_FECF | USART_ICR_NECF | USART_ICR_ORECF | USART_ICR_IDLECF | USART_ICR_TCCF | USART_ICR_TCBGTCF | USART_ICR_LBDCF | USART_ICR_CTSCF | USART_ICR_RTOCF | USART_ICR_EOBCF | USART_ICR_CMCF | USART_ICR_WUCF;
}

void EspEnable(void) {
	__HAL_RCC_USART3_CLK_ENABLE();
	USART3->CR1 |= USART_CR1_UE;
	HAL_GPIO_WritePin(EspPower_GPIO_Port, EspPower_Pin, GPIO_PIN_RESET);
	NVIC_EnableIRQ(USART3_IRQn);
}

void EspStop(void) {
	NVIC_DisableIRQ(USART3_IRQn);
	USART3->CR1 &= ~USART_CR1_UE;
	__HAL_RCC_USART3_CLK_DISABLE();
	HAL_GPIO_WritePin(EspPower_GPIO_Port, EspPower_Pin, GPIO_PIN_SET);
}

char EspGetChar(void) {
	char val = 0;
	EspRxFifoGetChar(&val);
	return val;
}

void EspSendData(const uint8_t * data, size_t len) {
	while (len) {
		USART3->TDR = *data;
		data++;
		len--;
		while (((USART3->ISR) & USART_ISR_TXE) == 0);
	}
}

uint32_t EspGetData(uint8_t * response, size_t maxResponse, uint32_t timeout) {
	uint32_t timeStart = HAL_GetTick();
	size_t i = 0;
	while (((HAL_GetTick() - timeStart) < timeout) && (maxResponse > 1)) {
		uint8_t c = 0;
		if (EspRxFifoGetChar((char *)&c)) {
			response[i] = c;
			i++;
			maxResponse--;
		}
	}
	return i;
}

uint32_t EspGetResponseData(uint8_t * response, size_t maxResponse, size_t * responseLen, uint32_t timeout) {
	char metadata[32] = {0};
	uint32_t error = 0;
	size_t i = 0;
	uint32_t timeStart = HAL_GetTick();
	uint8_t mode = 0;
	size_t dataExpected = 0;
	while ((HAL_GetTick() - timeStart) < timeout) {
		char c = 0;
		if (EspRxFifoGetChar(&c)) {
			if (mode < 2) {
				metadata[i] = c;
				i++;
				if (i >= sizeof(metadata)) {
					break; //error, invalid input, no response
				}
				if (mode == 0) {
					if (EndsWith(metadata, "+IPD,")) {
						mode = 1;
					}
				} else if (mode == 1) {
					if (c == ':') {
						mode = 2;
						i = 0; //now count for actual data
						if (dataExpected > maxResponse) {
							*responseLen = dataExpected; //in error case, report expected data
							error = 2;
							break; //all data must fit into the given buffer
						}
					} else if ((c >= '0') && (c <= '9')) {
						dataExpected *= 10;
						dataExpected += c - '0';
					} else {
						break; //error, invalid response
					}
				}
			} else {
				if (i < maxResponse) {
					response[i] = c;
					i++;
				}
				if (i == maxResponse) {
					break; //good case, error stays 0.
				}
			}
		}
	}
	if (mode < 2) {
		error = 1;
	}
	if (mode == 2) {
		*responseLen = dataExpected; //in error case, report expected buffer size
	}
	return error;
}

uint32_t EspCommand(const char * command, char * response, size_t maxResponse, uint32_t timeout) {
	const char * good1 = "OK\r\n";
	const char * bad1 = "ERROR\r\n";
	const char * bad2 = "WIFI DISCONNECT\r\n";
	const size_t goodl1 = strlen(good1);
	const size_t badl1 = strlen(bad1);
	const size_t badl2 = strlen(bad2);
	EspSendData((const uint8_t*)command, strlen(command));
	uint32_t timeStart = HAL_GetTick();
	size_t i = 0;
	uint32_t error = 2;
	while (((HAL_GetTick() - timeStart) < timeout) && (maxResponse > 1)) {
		char c = EspGetChar();
		if (c != 0) {
			response[i] = c;
			i++;
			maxResponse--;
		}
		if ((i >= goodl1) && (memcmp(response + i - goodl1, good1, goodl1) == 0)) {
			error = 0;
			break;
		}
		if (((i >= badl1) && (memcmp(response + i - badl1, bad1, badl1) == 0)) ||
		    ((i >= badl2) && (memcmp(response + i - badl2, bad2, badl2) == 0))) {
			error = 1;
			break;
		}
	}
	response[i] = '\0';
	return error;
}

uint32_t EspUdpRequestResponse(const char * domain, uint16_t port, uint8_t * requestIn, size_t requestInLen,
     uint8_t * requestOut, size_t requestOutMax, size_t * requestOutLen) {
	char outBuffer[128];
	char inBuffer[128] = {0};
	const uint32_t timeoutComm = 5000;
	const uint32_t timeoutClose = 1000;

	snprintf(outBuffer, sizeof(outBuffer), "AT+CIPSTART=\"UDP\",\"%s\",%u\r\n", domain, port);
	if (EspCommand(outBuffer, inBuffer, sizeof(inBuffer), timeoutComm)) {
		return 1;
	}
	uint32_t error = 0;
	snprintf(outBuffer, sizeof(outBuffer), "AT+CIPSEND=%u\r\n", (unsigned int)requestInLen);
	if (EspCommand(outBuffer, inBuffer, sizeof(inBuffer), timeoutComm) == 0) {
		EspSendData(requestIn, requestInLen);
		if (EspCommand("", inBuffer, sizeof(inBuffer), timeoutComm) == 0) {
			uint32_t result = EspGetResponseData(requestOut, requestOutMax, requestOutLen, timeoutComm);
			if (result == 1) {
				error = 4;
			} else if (result == 2) {
				error = 5;
			}
		} else {
			error = 3;
		}
	} else {
		error = 2;
	}
	EspCommand("AT+CIPCLOSE\r\n", inBuffer, sizeof(inBuffer), timeoutClose);
	return error;
}

bool EspConnect(const char * ap, const char * password) {
	char outBuffer[192];
	snprintf(outBuffer, sizeof(outBuffer), "AT+CWJAP_CUR=\"%s\",\"%s\"\r\n", ap, password);
	char inBuffer[256] = {0};
	size_t maxBuffer = sizeof(inBuffer);
	if (EspCommand(outBuffer, inBuffer, maxBuffer, 15000) == 0) {
		return true;
	}
	return false;
}

bool EspDisconnect(void) {
	char inBuffer[128];
	if (EspCommand("AT+CWQAP", inBuffer, sizeof(inBuffer), 1000) == 0) {
		return true;
	}
	return false;
}

bool EspSetClient(void) {
	char inBuffer[256] = {0};
	if (EspCommand("AT+CWMODE_CUR=1\r\n", inBuffer, sizeof(inBuffer), 250) == 0) {
		return true;
	}
	return false;
}

bool EspWaitPowerupReady(void) {
	char inBuffer[1024] = {0}; //512 is not enough
	EspCommand("", inBuffer, sizeof(inBuffer), 1000); //will report nothing found
	if ((strstr(inBuffer, "ready")) || (strstr(inBuffer, "Ai-Thinker"))) {
		return true;
	}
	return false;
}
