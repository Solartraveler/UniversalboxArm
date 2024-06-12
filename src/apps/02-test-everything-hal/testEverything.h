#pragma once

#include <stdint.h>

#include "main.h"

typedef struct {
	GPIO_TypeDef * port;
	uint32_t pin;
	const char * name;
} pin_t;

void AppInit(void);

void AppCycle(void);

