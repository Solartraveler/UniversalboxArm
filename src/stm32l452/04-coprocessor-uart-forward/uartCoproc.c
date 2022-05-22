#include <stdint.h>


#include "uartCoproc.h"

#include "main.h"
#include "usart.h"
#include "forwarder.h"
#include "boxlib/leds.h"

void UartCoprocInit(void) {
	MX_UART4_Init();
	NVIC_EnableIRQ(UART4_IRQn);
	__HAL_UART_ENABLE_IT(&huart4, UART_IT_RXNE);
}

void UART4_IRQHandler(void) {
	Led1Red();
	UART_HandleTypeDef * phuart = &huart4;
	if (__HAL_UART_GET_FLAG(phuart, UART_FLAG_RXNE) == SET) {
		char c = phuart->Instance->RDR;
		Uart4WritePutChar(c);
	}
	//just clear all flags
	__HAL_UART_CLEAR_FLAG(phuart, UART_CLEAR_PEF | UART_CLEAR_FEF | UART_CLEAR_NEF | UART_CLEAR_OREF | UART_CLEAR_IDLEF | UART_CLEAR_TCF | UART_CLEAR_LBDF | UART_CLEAR_CTSF | UART_CLEAR_CMF | UART_CLEAR_WUF | UART_CLEAR_RTOF);
}
