#pragma once

#include <stdint.h>
#include <stdbool.h>

void EspInit(void);

void EspEnable(void);

void EspStop(void);

//always returns 0
char EspGetChar(void);

//unsupported
void EspSendData(const uint8_t * data, size_t len);

//unsupported. Always returns 0
uint32_t EspGetData(uint8_t * response, size_t maxResponse, uint32_t timeout);

//unsupported
uint32_t EspGetResponseData(uint8_t * response, size_t maxResponse, size_t * responseLen, uint32_t timeout);

//unsupported, always returns 2 and sets response to a \0 terminated string
uint32_t EspCommand(const char * command, char * response, size_t maxResponse, uint32_t timeout);

//supported
uint32_t EspUdpRequestResponse(const char * domain, uint16_t port, uint8_t * requestIn, size_t requestInLen,
     uint8_t * requestOut, size_t requestOutMax, size_t * requestOutLen);

//always returns success
bool EspConnect(const char * ap, const char * password);

//always returns success
bool EspDisconnect(void);

//always returns success
bool EspSetClient(void);

//always returns success
bool EspWaitPowerupReady(void);
