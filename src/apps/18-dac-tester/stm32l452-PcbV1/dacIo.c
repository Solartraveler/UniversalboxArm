#include <stdint.h>
#include <stdio.h>

#include "dacIo.h"

#include "main.h"

void DacInit(void) {
	__HAL_RCC_GPIOA_CLK_ENABLE();
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = GPIO_PIN_4;
	GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	__HAL_RCC_DAC1_CLK_ENABLE();
	DAC1->DHR12R1 = 0;
	DAC1->CR = DAC_CR_EN1;
}

void DacWaveformStop(void) {
	TIM6->CR1 = 0;
	DMA1_Channel3->CCR = 0;
}

void DacSet(uint16_t value) {
	DacWaveformStop();
	DAC1->DHR12R1 = value;
}

void DacWaveform(uint16_t * data, uint32_t values, uint32_t cpuCyclesPerValue) {
	DacWaveformStop();
	/*ChatGPT is really nice here, but you have to control every bit, as otherwise
	  some values would be wrong as it mixes its output with the STM32F4.
	  And in the end https://m0agx.eu/generating-signals-with-stm32l4-timer-dma-and-dac.html
	  helped to get it working.
	*/
	DAC1->CR = DAC_CR_EN1;
	__HAL_RCC_DMA1_CLK_ENABLE();
	DMA1_Channel3->CCR = 0;
	// Memory increment, Circular mode, Read from memory, Memory size = 16 bits, Peripheral size = 16 bits, Priority medium-high
	DMA1_Channel3->CCR = DMA_CCR_MINC | DMA_CCR_CIRC |  DMA_CCR_DIR | DMA_CCR_MSIZE_0 | DMA_CCR_PSIZE_0 | DMA_CCR_PL_1;
	DMA1_CSELR->CSELR = (DMA1_CSELR->CSELR & ~DMA_CSELR_C3S) | (6 << DMA_CSELR_C3S_Pos);
	DMA1_Channel3->CPAR = (uint32_t)&DAC1->DHR12R1;
	DMA1_Channel3->CMAR = (uint32_t)data;
	DMA1_Channel3->CNDTR = values;
	DMA1_Channel3->CCR |= DMA_CCR_EN;

	__HAL_RCC_TIM6_CLK_ENABLE();
	uint32_t prescaler = 1;
	while (cpuCyclesPerValue > 65535) {
		cpuCyclesPerValue /= 2;
		prescaler = prescaler << 1;
	}
	TIM6->PSC = prescaler - 1;
	TIM6->ARR = cpuCyclesPerValue - 1;
	TIM6->CNT = 0;
	TIM6->CR2 = 0;
	TIM6->DIER = TIM_DIER_UDE;
	TIM6->CR1 = TIM_CR1_CEN;
}
