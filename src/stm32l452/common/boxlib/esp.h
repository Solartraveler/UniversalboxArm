#pragma once

#include <stdint.h>
#include <stdbool.h>

//sets up the GPIOs, after init, the ESP is not enabled.
void EspInit(void);

//Enables the power supply of the ESP
void EspEnable(void);

//Disables the power supply of the ESP
void EspStop(void);

char EspGetChar(void);

void EspSendString(const char * text);

uint32_t EspCommand(const char * command, char * response, size_t maxResponse, uint32_t timeout);

