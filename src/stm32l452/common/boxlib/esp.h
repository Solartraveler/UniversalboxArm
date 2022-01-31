#pragma once

#include <stdint.h>
#include <stdbool.h>

void EspEnable(void);

void EspDisable(void);

char EspGetChar(void);

void EspSendString(const char * text);

uint32_t EspCommand(const char * command, char * response, size_t maxResponse, uint32_t timeout);

