/* Boxlib
(c) 2022 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause
*/

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "boxlib/esp.h"

#include "main.h"

#include "utility.h"

void EspInit(void) {
}

void EspEnable(void) {
}

void EspStop(void) {
}

char EspGetChar(void) {
	return 0;
}

void EspSendData(const uint8_t * data, size_t len) {
	(void)data;
	(void)len;
}

uint32_t EspGetData(uint8_t * response, size_t maxResponse, uint32_t timeout) {
	(void)response;
	(void)maxResponse;
	(void)timeout;
	return 0;
}

uint32_t EspGetResponseData(uint8_t * response, size_t maxResponse, size_t * responseLen, uint32_t timeout) {
	(void)response;
	(void)maxResponse;
	(void)responseLen;
	(void)timeout;
	return 1;
}

uint32_t EspCommand(const char * command, char * response, size_t maxResponse, uint32_t timeout) {
	(void)command;
	(void)response;
	(void)maxResponse;
	(void)timeout;
	return 2;
}

uint32_t EspUdpRequestResponse(const char * domain, uint16_t port, uint8_t * requestIn, size_t requestInLen,
     uint8_t * requestOut, size_t requestOutMax, size_t * requestOutLen) {
	(void)domain;
	(void)port;
	(void)requestIn;
	(void)requestInLen;
	(void)requestOut;
	(void)requestOut;
	(void)requestOutMax;
	(void)requestOutLen;
	return 1;
}

bool EspConnect(const char * ap, const char * password) {
	(void)ap;
	(void)password;
	return false;
}

bool EspDisconnect(void) {
	return false;
}

bool EspSetClient(void) {
	return false;
}

bool EspWaitPowerupReady(void) {
	return false;
}
