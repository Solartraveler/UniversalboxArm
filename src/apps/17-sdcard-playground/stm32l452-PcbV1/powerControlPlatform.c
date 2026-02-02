#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "powerControl.h"

#include "main.h"

static void SdCardPowerPinInit() {
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	__HAL_RCC_GPIOA_CLK_ENABLE();

	GPIO_InitStruct.Pin = Extern14_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(Extern14_GPIO_Port, &GPIO_InitStruct);
}

void SdCardPowerOn(void) {
	SdCardPowerPinInit();
	HAL_GPIO_WritePin(Extern14_GPIO_Port, Extern14_Pin, GPIO_PIN_SET);
}

void SdCardPowerOff(void) {
	SdCardPowerPinInit();
	HAL_GPIO_WritePin(Extern14_GPIO_Port, Extern14_Pin, GPIO_PIN_RESET);
}

