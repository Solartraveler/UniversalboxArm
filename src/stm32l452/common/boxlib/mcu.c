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
	g_mcuFrequceny = frequency;
	g_mcuApbDivider = apbDivider;
	return true;
}

/* supported frequencies: 16, 24, 32 48, 64 and 80MHz.
returns:
  0: ok
  1: frequency unsupported
  2: setting latency failed
  3: failed to start PLL
  4: failed to set new divider and latency

*/
uint8_t McuClockToHsiPll(uint32_t frequency, uint32_t apbDivider) {
	uint32_t latency; //div 2
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
	RCC_OscInitStruct.PLL.PLLM = 1;
	RCC_OscInitStruct.PLL.PLLN = 8;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;

	if (frequency == 16000000) {
		latency = FLASH_LATENCY_0;
		RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV8;
		RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV8;
	} else if (frequency == 24000000) {
		latency = FLASH_LATENCY_1;
		RCC_OscInitStruct.PLL.PLLN = 12;
		RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV8;
		RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV8;
	} else if (frequency == 32000000) {
		latency = FLASH_LATENCY_1;
		RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV4;
		RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV4;
	} else if (frequency == 48000000) {
		latency = FLASH_LATENCY_2;
		RCC_OscInitStruct.PLL.PLLN = 12;
		RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV4;
		RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV4;
	} else if (frequency == 64000000) {
		latency = FLASH_LATENCY_3;
		RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
		RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
	} else if (frequency == 80000000) {
		latency = FLASH_LATENCY_4;
		RCC_OscInitStruct.PLL.PLLN = 10;
		RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
		RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
	} else {
		return 1;
	}
	//first set slowest latency, suitable for all frequencies
	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK) {
		return 2;
	}
	HAL_StatusTypeDef result = HAL_RCC_OscConfig(&RCC_OscInitStruct);
	if (result != HAL_OK) {
		return 3;
	}

	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
	                              RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = apbDivider;
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

uint64_t McuTimestampUs(void) {
	uint32_t stamp1, stamp2;
	uint32_t substamp;
	//get the data
	do {
		stamp1 = HAL_GetTick();
		substamp = SysTick->VAL;
		stamp2 = HAL_GetTick();
	} while (stamp1 != stamp2); //don't read VAL while an overflow happened
	uint32_t load = SysTick->LOAD;
	//NVIC_GetPendingIRQ(SysTick_IRQn) does not work!
	if (SCB->ICSR & SCB_ICSR_PENDSTSET_Msk) {
		/* Looks like the systick interrupts is locked, and 2x HAL_GetTick
		   just got the same timestamp because the increment could not be done.
		   So if the substamp is > load/2, it has underflown before the readout and
		   the stamp value needs to be increased.
		   Note: The pending alredy gets set when the value switches from 1 -> 0,
		   but we only want to add 1ms when a 0 -> load underflow happened.
		   The 1 -> 0 never is a problem with ISRs enabled, because this case is
		   catched by the stamp1 != stamp2 comparison.
		   Important: This logic assumes SCB_ICSR_PENDSTSET_Msk could never be set
		   as pending, when the ISR is not blocked (and this function is not called
		   from within the systick interrupt itself).
		*/
		if (substamp > (load / 2)) {
			stamp1++;
		}
	}
	//now calculate
	substamp = load - substamp; //this counts down, so invert it
	substamp = substamp * 1000 / load;
	uint64_t stamp = stamp1;
	stamp *= 1000;
	stamp += substamp;
	//printf("Stamp: %x-%x\r\n", (uint32_t)(stamp >> 32LLU), (uint32_t)stamp);
	return stamp;
}

void McuDelayUs(uint32_t us) {
	uint64_t tEnd = McuTimestampUs() + us;
	while (tEnd > McuTimestampUs());
}

uint32_t McuApbFrequencyGet(void) {
	return g_mcuFrequceny / g_mcuApbDivider;
}

uint32_t McuCpuFrequencyGet(void) {
	return g_mcuFrequceny;
}