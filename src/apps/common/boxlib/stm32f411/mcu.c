/*
(c) 2024 by Malte Marwedel

License: BSD-3-Clause
*/

#include <stdbool.h>
#include <stdint.h>

#include "boxlib/mcu.h"

#include "main.h"

uint32_t g_mcuFrequceny = 16000000;
uint32_t g_mcuApbDivider = 1;

uint8_t McuClockToHsiPll(uint32_t frequency, uint32_t apbDivider) {
	uint32_t latency; //div 2
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
	RCC_OscInitStruct.PLL.PLLM = 16;

	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 2;

	if (frequency == 16000000) {
		latency = FLASH_LATENCY_0;
		RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV4;
		RCC_OscInitStruct.PLL.PLLN = 128;
	} else if (frequency == 24000000) {
		latency = FLASH_LATENCY_0;
		RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV2;
		RCC_OscInitStruct.PLL.PLLN = 192;
	} else if (frequency == 32000000) {
		latency = FLASH_LATENCY_1;
		RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV2;
		RCC_OscInitStruct.PLL.PLLN = 128;
	} else if (frequency == 48000000) {
		latency = FLASH_LATENCY_1;
		RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV2;
		RCC_OscInitStruct.PLL.PLLN = 192;
	} else if (frequency == 64000000) {
		latency = FLASH_LATENCY_1;
		RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
		RCC_OscInitStruct.PLL.PLLN = 128;
	} else if (frequency == 80000000) {
		latency = FLASH_LATENCY_2;
		RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
		RCC_OscInitStruct.PLL.PLLN = 160;
	} else if (frequency == 100000000) {
		latency = FLASH_LATENCY_3;
		RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
		RCC_OscInitStruct.PLL.PLLN = 200;
	} else {
		return 1;
	}
	//first set slowest latency, suitable for all frequencies
	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK) {
		return 2;
	}
	HAL_StatusTypeDef result = HAL_RCC_OscConfig(&RCC_OscInitStruct);
	if (result != HAL_OK) {
		return 3;
	}

	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
	                              RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	if ((apbDivider != RCC_HCLK_DIV1) || (frequency <= 50000000)) { //APB1 limit is 50MHz
		RCC_ClkInitStruct.APB1CLKDivider = apbDivider;
	} else {
		RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	}
	RCC_ClkInitStruct.APB2CLKDivider = apbDivider;
	//now set new dividers
	result = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, latency);
	if (result != HAL_OK) {
		return 4;
	}
	SystemCoreClockUpdate();
	g_mcuFrequceny = frequency;
	g_mcuApbDivider = apbDivider;
	return 0;
}
