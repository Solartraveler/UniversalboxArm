/* STM HAL and CMSIS simulation
(c) 2022 by Malte Marwedel

SPDX-License-Identifier: BSD-3-Clause
*/

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#include "simulated.h"

#include "boxlib/flash.h"
#include "boxlib/lcd.h"
#include "boxlib/rs232debug.h"

#define ROM_LOADER_SIZE (12 * 1024)


const uint8_t g_dummyLoader[ROM_LOADER_SIZE];

pthread_mutex_t g_IsrLocked;

void SimulatedInit(void) {
	pthread_mutex_init(&g_IsrLocked, NULL);
}

void SimulatedDeinit(void) {
	FlashDisable(); //writes changes back to the file
	LcdDisable(); //closes drawing thread
	Rs232Stop(); //resets the terminal
	pthread_mutex_destroy(&g_IsrLocked);
}

void NVIC_SystemReset(void) {
	printf("%s called\n", __func__);
	SimulatedDeinit();
	exit(0);
}

void HAL_Delay(uint32_t delay) {
	usleep(delay * 1000);
}

uint32_t HAL_GetTick(void) {
	struct timespec t = {0};
	clock_gettime(CLOCK_MONOTONIC, &t);
	return (t.tv_sec * 1000) + (t.tv_nsec / 1000000);
}

void __disable_irq(void) {
	pthread_mutex_lock(&g_IsrLocked);
}

void __enable_irq(void) {
	pthread_mutex_unlock(&g_IsrLocked);
}
