#pragma once

#include <stdint.h>

void NVIC_SystemReset(void);

void HAL_Delay(uint32_t delay);

extern const uint8_t g_dummyLoader[];

#define ROM_BOOTLOADER_START_ADDRESS ((uintptr_t)(&g_dummyLoader))
