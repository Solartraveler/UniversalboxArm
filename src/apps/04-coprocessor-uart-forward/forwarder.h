#pragma once

#include <stdbool.h>

void AppInit(void);

void AppCycle(void);

bool Uart4WritePutChar(char out);
