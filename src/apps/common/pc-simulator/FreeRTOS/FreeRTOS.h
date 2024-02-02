#pragma once

#include <stdint.h>
#include <stdbool.h>

uint32_t vTaskStartScheduler(void);

#define configMINIMAL_STACK_SIZE 64

#define pdFALSE false