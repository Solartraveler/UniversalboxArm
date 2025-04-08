#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "sequenceToPwm.h"

#include "boxlib/leds.h"

#define FIFO_DATA_GET_OK Led1Green
#define FIFO_DATA_GET_FAIL Led1Red
#include "locklessfifo.h"
#include "main.h"

static FifoState_t g_sequence;

/*One timer (TIM2) will get the data from the fifos and set
  the next PWM value. As this is a 32bit timer, no prescaler calculation is needed
  it simply counts to seqMax.
  The second timer (TIM3) will generate the PWM value on PB4 with a 8bit resolution (125KHz @ 32MHz base clock)
*/
void SeqStart(uint32_t pwmDivider, uint32_t seqMax, uint8_t * fifoBuffer, size_t fifoLen) {
	HAL_NVIC_DisableIRQ(TIM2_IRQn);

	FifoInit(&g_sequence, fifoBuffer, fifoLen);

	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_TIM2_CLK_ENABLE();
	__HAL_RCC_TIM3_CLK_ENABLE();

	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = GPIO_PIN_4;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	//code sequence mostly created by ChatGPT:
	//Timer for PWM
	TIM3->CR1 &= ~TIM_CR1_CEN;
	TIM3->PSC = pwmDivider;
	TIM3->CR1 = (0 << TIM_CR1_CKD_Pos) | TIM_CR1_ARPE;
	TIM3->CR2 = 0;
	TIM3->CCMR1 = (6 << TIM_CCMR1_OC1M_Pos); //PWM mode 1
	TIM3->CCMR1 |= TIM_CCMR1_OC1PE; //use preload
	TIM3->CCER |= TIM_CCER_CC1E | TIM_CCER_CC1P;
	TIM3->BDTR |= TIM_BDTR_MOE; // main output enable, ChatGPT forgot this line :P
	TIM3->ARR = 255; //maximum PWM value
	TIM3->CCR1 = 128; //actual compare value
	TIM3->EGR = TIM_EGR_UG; //update event generation
	TIM3->CR1 |= TIM_CR1_CEN;

	//Timer for ISR data feeder
	TIM2->CR1 = 0; //all stopped
	TIM2->CR2 = 0;
	TIM2->CNT = 0;
	TIM2->PSC = 0;
	TIM2->SR = 0;
	TIM2->DIER = TIM_DIER_UIE;
	TIM2->ARR = seqMax;
	HAL_NVIC_SetPriority(TIM2_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(TIM2_IRQn);
	TIM2->CR1 |= TIM_CR1_CEN;
}

//sending fifo put
//returns true if the char could be put into the queue
bool SequencePutData(uint8_t out) {
	return FifoDataPut(&g_sequence, out);
}

void TIM2_IRQHandler(void) {
	TIM2->SR = 0;
	NVIC_ClearPendingIRQ(TIM2_IRQn);
	TIM3->CCR1 = FifoDataGet(&g_sequence);
	//TIM3->CCR1 = 255 - TIM3->CCR1;
}

void SeqStop(void) {
	TIM2->CR1 &= ~TIM_CR1_CEN;
	TIM3->CR1 &= ~TIM_CR1_CEN;
	Led1Off();
}

size_t SeqFifoFree(void) {
	size_t unused = FifoDataFree(&g_sequence);
	//printf("Unused: %u\r\n", (unsigned int)unused);
	return unused;
}

void SeqFifoPut(const uint8_t * data, size_t dataLen) {
	bool success = FifoBufferPut(&g_sequence, data, dataLen);
	if (!success) {
		printf("Error, FIFO overflow, wanting to put %u\r\n", (unsigned int)dataLen);
	}
}
