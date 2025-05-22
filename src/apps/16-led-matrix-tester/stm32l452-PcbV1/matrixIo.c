#include <stdint.h>
#include <stdio.h>

#include "matrixIo.h"

#include "boxlib/leds.h"
#include "main.h"

//Defined in ledMatrixTester.c
extern uint32_t g_matrixLines;
extern uint32_t g_colorMax;
extern uint32_t g_colorData[];

void MatrixColorSet(uint8_t value) {
	if (value & 1) {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_SET);
	} else {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);
	}
	if (value & 2) {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_SET);
	} else {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);
	}
	if (value & 4) {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);
	} else {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);
	}
	uint32_t led1 = (value >> 3) & 3;
	if (led1 == 1) {
		Led1Red();
	} else if (led1 == 2) {
		Led1Green();
	} else if (led1 == 3) {
		Led1Yellow();
	} else {
		Led1Off();
	}
}

void MatrixGpioInit(void) {
	__HAL_RCC_GPIOB_CLK_ENABLE(); //SPI1 pins
	MatrixColorSet(0);
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void TIM1_UP_TIM16_IRQHandler(void) {
	TIM16->SR = 0;
	NVIC_ClearPendingIRQ(TIM1_UP_TIM16_IRQn);

	static uint32_t line = 0;
	static uint32_t time = 0;
	uint32_t color = 0;
	if (line == 0) {
		color = g_colorData[time];
	}
	time++;
	if (time >= g_colorMax) {
		time = 0;
		line++;
	}
	if (line >= g_matrixLines) {
		line = 0;
	}
	MatrixColorSet(color);
}

void MatrixTimerInit(uint32_t cycles) {
	__HAL_RCC_TIM16_CLK_ENABLE();
	TIM16->CR1 = 0; //all stopped
	TIM16->CR2 = 0;
	TIM16->CNT = 0;
	TIM16->PSC = 0;
	TIM16->SR = 0;
	TIM16->DIER = TIM_DIER_UIE;
	TIM16->ARR = cycles;
	HAL_NVIC_SetPriority(TIM1_UP_TIM16_IRQn, 4, 0);
	HAL_NVIC_EnableIRQ(TIM1_UP_TIM16_IRQn);
	TIM16->CR1 |= TIM_CR1_CEN;
}

void MatrixTimerStop(void) {
	TIM16->CR1 = 0;
}
