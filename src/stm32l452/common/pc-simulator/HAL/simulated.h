#pragma once

#include <stdint.h>

//Call before using any other function:
void SimulatedInit(void);

//Call before exit, to clean things up
void SimulatedDeinit(void);

void NVIC_SystemReset(void);

void HAL_Delay(uint32_t delay);

extern const uint8_t g_dummyLoader[];

//Not really disabling IRSs, its just a common lock.
void __disable_irq(void);

void __enable_irq(void);

#define ROM_BOOTLOADER_START_ADDRESS ((uintptr_t)(&g_dummyLoader))
