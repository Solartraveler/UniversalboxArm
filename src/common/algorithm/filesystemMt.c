/* Fatfs multithreading callback functions for FreeRTOS
(c) 2022 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause
*/


#include <stdint.h>

#include "FreeRTOS.h"
#include "semphr.h"

#include "ff.h"

SemaphoreHandle_t g_filesystemSemaphore;
StaticSemaphore_t g_filesystemSemaphoreState;

//all functions who return a value, return 1 on success and 0 on failure

int ff_cre_syncobj(BYTE vol, FF_SYNC_t* sobj) {
	if (vol == 0) {
		g_filesystemSemaphore = xSemaphoreCreateMutexStatic(&g_filesystemSemaphoreState);
		*sobj = g_filesystemSemaphore;
		if (g_filesystemSemaphore) {
			return 1; //success
		}
	}
	return 0;
}

int ff_del_syncobj(FF_SYNC_t sobj) {
	vSemaphoreDelete(sobj);
	return 1;
}

int ff_req_grant(FF_SYNC_t sobj) {
	if (xSemaphoreTake(sobj, FF_FS_TIMEOUT)) {
		return 1;
	}
	return 0;
}

void ff_rel_grant(FF_SYNC_t sobj) {
	xSemaphoreGive(sobj);
}
