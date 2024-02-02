#pragma once

#include <stdbool.h>

void ForwarderInit(void);

void ForwarderCycle(void);

bool Uart4WritePutChar(char out);
