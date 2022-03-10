/* STM HAL and CMSIS simulation
(c) 2022 by Malte Marwedel

SPDX-License-Identifier: BSD-3-Clause
*/

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "simulated.h"

#define ROM_LOADER_SIZE (12 * 1024)


const uint8_t g_dummyLoader[ROM_LOADER_SIZE];


void NVIC_SystemReset(void) {
	printf("%s called\n", __func__);
	exit(0);
}

void HAL_Delay(uint32_t delay) {
	usleep(delay * 1000);
}
