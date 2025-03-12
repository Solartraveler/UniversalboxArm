#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "sequenceToPwm.h"

#include "boxlib/leds.h"
#include "main.h"

static size_t g_sequenceBuferLen;
static uint8_t * g_sequenceBuffer;
static volatile uint32_t g_sequenceBufferReadIdx;
static volatile uint32_t g_sequenceBufferWriteIdx;


/*One timer (TIM2) will get the data from the fifos and set
  the next PWM value. As this is a 32bit timer, no prescaler calculation is needed
  it simply counts to seqMax.
  The second timer (TIM3) will generate the PWM value on PB4 with a 8bit resolution (125KHz @ 32MHz base clock)
*/
void SeqStart(uint32_t pwmDivider, uint32_t seqMax, uint8_t * fifoBuffer, size_t fifoLen) {
	HAL_NVIC_DisableIRQ(TIM2_IRQn);

	g_sequenceBuffer = fifoBuffer;
	g_sequenceBuferLen = fifoLen;
	g_sequenceBufferReadIdx = 0;
	g_sequenceBufferWriteIdx = 0;

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

uint8_t SequenceGetData(void) {
	char out = 0;
	if ((g_sequenceBuffer) && (g_sequenceBufferReadIdx != g_sequenceBufferWriteIdx)) {
		size_t ri = g_sequenceBufferReadIdx;
		out = g_sequenceBuffer[ri];
		__sync_synchronize(); //the pointer increment may only be visible after the copy
		ri = (ri + 1) % g_sequenceBuferLen;
		g_sequenceBufferReadIdx = ri;
		Led1Green();
	} else {
		Led1Red();
	}
	return out;
}

//sending fifo put
//returns true if the char could be put into the queue
bool SequencePutData(uint8_t out) {
	bool succeed = false;
	size_t writeThis = g_sequenceBufferWriteIdx;
	size_t writeNext = (writeThis + 1) % g_sequenceBuferLen;
	if (writeNext != g_sequenceBufferReadIdx) {
		g_sequenceBuffer[writeThis] = out;
		g_sequenceBufferWriteIdx = writeNext;
		succeed = true;
	}
	return succeed;
}

void TIM2_IRQHandler(void) {
	TIM2->SR = 0;
	NVIC_ClearPendingIRQ(TIM2_IRQn);
	TIM3->CCR1 = SequenceGetData();
	//TIM3->CCR1 = 255 - TIM3->CCR1;
}


void SeqStop(void) {
	TIM2->CR1 &= ~TIM_CR1_CEN;
	TIM3->CR1 &= ~TIM_CR1_CEN;
	Led1Off();
}

size_t SeqFifoFree(void) {
	size_t rp = g_sequenceBufferReadIdx;
	size_t wp = g_sequenceBufferWriteIdx;
	size_t unused;
	if (wp >= rp) {
		unused = (g_sequenceBuferLen - wp) + rp;
	} else {
		unused = (rp - wp);
	}
	//printf("Unused: %u\r\n", (unsigned int)unused);
	return unused;
}

void SeqFifoPut(const uint8_t * data, size_t dataLen) {
	bool success = true;
	for (size_t i = 0; i < dataLen; i++) {
		success &= SequencePutData(data[i]);
		if (!success) {
			printf("Breaking at %u byte\r\n", (unsigned int)i);
			break;
		}
	}
	if (!success) {
		printf("Error, FIFO overflow, wanting to put %u\r\n", (unsigned int)dataLen);
	}
}
