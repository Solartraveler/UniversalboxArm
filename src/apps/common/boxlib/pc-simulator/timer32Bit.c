/* Boxlib
(c) 2025 by Malte Marwedel

SPDX-License-Identifier: BSD-3-Clause
*/

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "boxlib/timer32Bit.h"

#include "main.h"

static uint16_t g_timer32Bit_divider;

static bool g_timerRunning;

static struct timespec g_timerStart;
static struct timespec g_timerStop;

void Timer32BitInit(uint16_t prescaler) {
	g_timer32Bit_divider = prescaler + 1;
}

void Timer32BitDeinit(void) {
	Timer32BitStop();
}

void Timer32BitReset(void) {
	clock_gettime(CLOCK_MONOTONIC, &g_timerStart);
	memcpy(&g_timerStop, &g_timerStart, sizeof(struct timespec));
}

void Timer32BitStart(void) {
	Timer32BitReset();
	g_timerRunning = true;
}

void Timer32BitStop(void) {
	clock_gettime(CLOCK_MONOTONIC, &g_timerStop);
	g_timerRunning = false;
}

#define NSEC_IN_SEC 1000000000ULL

/*The output should be exact as long as 1000000000 is a exact multiple of
(F_CPU / g_timer32Bit_divider).
*/
uint32_t Timer32BitGet(void) {
	struct timespec timeX;
	if (g_timerRunning) {
		clock_gettime(CLOCK_MONOTONIC, &timeX);
	} else {
		memcpy(&timeX, &g_timerStop, sizeof(struct timespec));
	}
	uint64_t stampNsecA = g_timerStart.tv_sec * NSEC_IN_SEC + g_timerStart.tv_nsec;
	uint64_t stampNsecB = timeX.tv_sec * NSEC_IN_SEC + timeX.tv_nsec;
	uint64_t deltaNsec = stampNsecB - stampNsecA;
	uint64_t divisor = g_timer32Bit_divider * F_CPU; //1M times larger than needed
	uint64_t stamp = deltaNsec * 1000000 / divisor;
	return stamp;
}
