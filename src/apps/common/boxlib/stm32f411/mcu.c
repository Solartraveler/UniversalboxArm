/*
(c) 2024 by Malte Marwedel

License: BSD-3-Clause
*/

#include <stdbool.h>
#include <stdint.h>

#include "boxlib/mcu.h"

#include "boxlib/leds.h"
#include "main.h"

uint32_t g_mcuFrequceny = 16000000;
uint32_t g_mcuApbDivider = 1;

//See https://stm32f4-discovery.net/2017/04/tutorial-jump-system-memory-software-stm32/
void McuStartOtherProgram(void * startAddress, bool ledSignalling) {
	volatile uint32_t * pStackTop = (uint32_t *)(startAddress);
	volatile uint32_t * pProgramStart = (uint32_t *)(startAddress + 0x4);
	if (ledSignalling) {
		Led2Green();
	}
	__HAL_FLASH_INSTRUCTION_CACHE_DISABLE();
	__HAL_FLASH_DATA_CACHE_DISABLE();
	__HAL_FLASH_PREFETCH_BUFFER_DISABLE();
	HAL_RCC_DeInit();
	SysTick->CTRL = 0;
	SysTick->LOAD = 0;
	SysTick->VAL = 0;
	__disable_irq();
	__DSB();
	__HAL_SYSCFG_REMAPMEMORY_SYSTEMFLASH();
	__DSB();
	__ISB();
	__HAL_RCC_SPI1_FORCE_RESET();
	__HAL_RCC_SPI2_FORCE_RESET();
	__HAL_RCC_SPI3_FORCE_RESET();
	__HAL_RCC_SPI4_FORCE_RESET();
	__HAL_RCC_SPI5_FORCE_RESET();
	__HAL_RCC_USART1_FORCE_RESET();
	__HAL_RCC_USART2_FORCE_RESET();
	__HAL_RCC_USART6_FORCE_RESET();
	__HAL_RCC_SPI1_RELEASE_RESET();
	__HAL_RCC_SPI2_RELEASE_RESET();
	__HAL_RCC_SPI3_RELEASE_RESET();
	__HAL_RCC_SPI4_RELEASE_RESET();
	__HAL_RCC_SPI5_RELEASE_RESET();
	__HAL_RCC_USART1_RELEASE_RESET();
	__HAL_RCC_USART2_RELEASE_RESET();
	__HAL_RCC_USART6_RELEASE_RESET();
	//Is there a generic maximum interrupt number defined somewhere?
	for (uint32_t i = 0; i <= SPI5_IRQn; i++) {
		NVIC_DisableIRQ(i);
		NVIC_ClearPendingIRQ(i);
	}
	__enable_irq(); //actually, the system seems to start with enabled interrupts
	if (ledSignalling) {
		Led1Off();
	}
	/* Writing the stack change as C code is a bad idea, because the compiler
	   can insert stack changeing code before the function call. And in fact, it
	   does with some optimization. So
	       __set_MSP(*pStackTop);
	       ptrFunction_t * pDfu = (ptrFunction_t *)(*pProgramStart);
	       pDfu();
	   would work with -Og optimization, but not with -Os optimization.
	   Instead we use two commands of assembly, where the compiler can't add code
	   inbetween.
*/
	asm("msr msp, %[newStack]\n bx %[newProg]"
	     : : [newStack]"r"(*pStackTop), [newProg]"r"(*pProgramStart));
}


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

void McuLockCriticalPins(void) {
}
