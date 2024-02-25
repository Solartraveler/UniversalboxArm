#include <stdint.h>


#include "uartCoproc.h"

#include "main.h"
#include "forwarder.h"
#include "boxlib/leds.h"

#define AvrTx_Pin AvrSpiSck_Pin
#define AvrTx_GPIO_Port AvrSpiSck_GPIO_Port
#define AvrRx_Pin AvrSpiMiso_Pin
#define AvrRx_GPIO_Port AvrSpiMiso_GPIO_Port


void UartCoprocInit(void) {
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	__HAL_RCC_UART4_CLK_ENABLE();

	__HAL_RCC_GPIOC_CLK_ENABLE();

	GPIO_InitStruct.Pin = AvrTx_Pin | AvrRx_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF8_UART4;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	uint32_t pclk =  HAL_RCC_GetPCLK1Freq();
	const uint32_t baudrate = 1200;

	UART4->BRR = pclk / baudrate;
	UART4->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE;
	UART4->CR1 |= USART_CR1_UE;

	NVIC_EnableIRQ(UART4_IRQn);
}

void UART4_IRQHandler(void) {
	Led1Red();
	if ((UART4->ISR) & USART_ISR_RXNE) {
		char c = UART4->RDR;
		Uart4WritePutChar(c);
	}
	//just clear all flags
	UART4->ICR |= USART_ICR_PECF | USART_ICR_FECF | USART_ICR_NECF | USART_ICR_ORECF | USART_ICR_IDLECF | USART_ICR_TCCF | USART_ICR_TCBGTCF | USART_ICR_LBDCF | USART_ICR_CTSCF | USART_ICR_RTOCF | USART_ICR_EOBCF | USART_ICR_CMCF | USART_ICR_WUCF;
}
