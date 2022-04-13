/*
(c) 2022 by Malte Marwedel

License: BSD-3-Clause
*/

#include <stdbool.h>
#include <stdint.h>

#include "mcu.h"

#include "leds.h"
#include "main.h"

typedef void (ptrFunction_t)(void);

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
	McuClockToMsi(4000000, RCC_HCLK_DIV1);
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
	__HAL_RCC_USART1_FORCE_RESET();
	__HAL_RCC_USART2_FORCE_RESET();
	__HAL_RCC_USART3_FORCE_RESET();
	__HAL_RCC_USB_FORCE_RESET();
	__HAL_RCC_SPI1_RELEASE_RESET();
	__HAL_RCC_SPI2_RELEASE_RESET();
	__HAL_RCC_SPI3_RELEASE_RESET();
	__HAL_RCC_USART1_RELEASE_RESET();
	__HAL_RCC_USART2_RELEASE_RESET();
	__HAL_RCC_USART3_RELEASE_RESET();
	__HAL_RCC_USB_RELEASE_RESET();
	//Is there a generic maximum interrupt number defined somewhere?
	for (uint32_t i = 0; i <= I2C4_ER_IRQn; i++) {
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

//debug prints may not work after changing. As the prescalers are not recalculated
bool McuClockToMsi(uint32_t frequency, uint32_t apbDivider) {
	uint32_t clockRange = 0;
	uint32_t flashLatency = FLASH_LATENCY_0;
	switch (frequency) {
		case 100000: clockRange = RCC_MSIRANGE_0; break;
		case 200000: clockRange = RCC_MSIRANGE_1; break;
		case 400000: clockRange = RCC_MSIRANGE_2; break;
		case 800000: clockRange = RCC_MSIRANGE_3; break;
		case 1000000: clockRange = RCC_MSIRANGE_4; break;
		case 2000000: clockRange = RCC_MSIRANGE_5; break;
		case 4000000: clockRange = RCC_MSIRANGE_6; break;
		case 8000000: clockRange = RCC_MSIRANGE_7; break;
		case 16000000: clockRange = RCC_MSIRANGE_8; break;
		case 24000000: clockRange = RCC_MSIRANGE_9; flashLatency = FLASH_LATENCY_1; break;
		case 32000000: clockRange = RCC_MSIRANGE_10; flashLatency = FLASH_LATENCY_1; break;
		case 48000000: clockRange = RCC_MSIRANGE_11; flashLatency = FLASH_LATENCY_2; break;
		default: return false;
	}
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
	RCC_OscInitStruct.MSIState = RCC_MSI_ON;
	RCC_OscInitStruct.MSICalibrationValue = 0;
	RCC_OscInitStruct.MSIClockRange = clockRange;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		return false;
	}
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
	                             | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = apbDivider;
	RCC_ClkInitStruct.APB2CLKDivider = apbDivider;
	//The call to set the flash latency properly does increase and decrease
	//depending on the current settings
	HAL_StatusTypeDef result = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, flashLatency);
	if (result != HAL_OK) {
		return false;
	}
	SystemCoreClockUpdate();
	return true;
}

void McuLockCriticalPins(void) {
	/*The bootloader seems not to reset the GPIO ports, so we can lock the pin for
	  SPI2 MISO and prevent it becoming a high level output
	  but to use our peripherals again, we might need a system reset or at least
	  a GPIO port reset
	*/
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = PerSpiMiso_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLDOWN;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	HAL_GPIO_Init(PerSpiMiso_GPIO_Port, &GPIO_InitStruct);
	HAL_GPIO_LockPin(PerSpiMiso_GPIO_Port, PerSpiMiso_Pin);
}

