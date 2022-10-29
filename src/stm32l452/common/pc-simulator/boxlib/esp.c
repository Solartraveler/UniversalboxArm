/* Boxlib emulation
(c) 2022 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause
*/

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include "esp.h"

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
	return 1; //no response or corrupt response
}

uint32_t EspCommand(const char * command, char * response, size_t maxResponse, uint32_t timeout) {
	(void)command;
	if ((maxResponse > 0) && (response)) {
		*response = '\0';
	}
	(void)timeout;
	return 2;
}

uint32_t EspUdpRequestResponse(const char * domain, uint16_t port, uint8_t * requestIn, size_t requestInLen,
     uint8_t * requestOut, size_t requestOutMax, size_t * requestOutLen) {
	uint32_t error = 0;
	struct hostent * ph = gethostbyname(domain);
	if (ph) {
		int s = socket (AF_INET, SOCK_DGRAM, 0);
		if (s < 0) {
			return 1;
		}
		struct sockaddr_in destAddr;
		destAddr.sin_port = htons(port);
		destAddr.sin_family = ph->h_addrtype;
		destAddr.sin_addr.s_addr = 0;
		if (ph->h_length <= (int)sizeof(unsigned long)) {
			memcpy(&(destAddr.sin_addr.s_addr), ph->h_addr_list[0], ph->h_length);
		} else {
			return 1;
		}

		struct sockaddr_in sourceAddr;
		sourceAddr.sin_family = AF_INET;
		sourceAddr.sin_port = 0;
		sourceAddr.sin_addr.s_addr = htonl(INADDR_ANY);

		if (bind(s, (struct sockaddr *)&sourceAddr, sizeof(sourceAddr)) == 0) {
			if (sendto(s, requestIn, requestInLen, 0, (struct sockaddr *)&destAddr, sizeof(destAddr)) == (ssize_t)requestInLen) {
				ssize_t got = recvfrom(s, requestOut, requestOutMax, 0, NULL, NULL);
				if (got >= 0) {
					*requestOutLen = got;
				} else {
					error = 4;
				}
			} else {
				error = 3;
			}
		} else {
			error = 2;
		}
		close(s);
	} else {
		error =  1; //could not start connection
	}
	return error;
}

bool EspConnect(const char * ap, const char * password) {
	(void)ap;
	(void)password;
	return true;
}

bool EspDisconnect(void) {
	return true;
}

bool EspSetClient(void) {
	return true;
}

bool EspWaitPowerupReady(void) {
	return true;
}
