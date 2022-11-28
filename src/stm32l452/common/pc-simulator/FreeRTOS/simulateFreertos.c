/* Simulation helper
(c) 2022 by Malte Marwedel

SPDX-License-Identifier: BSD-3-Clause

Some very basic FreeRTOS simulation with posix threads.

*/

#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include "FreeRTOS.h"

#include "simulated.h"

#include "task.h"



void vTaskDelay(uint32_t ticks) {
	HAL_Delay(ticks);
}

void taskYIELD(void) {
	HAL_Delay(0);
}

void * ThreadStarter(void * param) {
	StaticTask_t * pData = (StaticTask_t *)param;
	pData->pStart(pData->pParam); //usually does not return
	return NULL;
}

TaskHandle_t xTaskCreateStatic(TaskFunction_t pTask, const char * name, size_t stackElements,
  void * param, uint32_t priority, StackType_t * pStack, StaticTask_t * pState) {
	(void)stackElements;
	(void)pStack;
	(void)priority;
	if (pState) {
		pState->pStart = pTask;
		pState->pParam = param;
		strncpy(pState->name, name, TASK_NAME_MAX);
		pState->name[TASK_NAME_MAX - 1] = '\0';
		if (pthread_create(&(pState->pThread), NULL, &ThreadStarter, pState) == 0) {
			return pState;
		}
	}
	return NULL;
}

uint32_t vTaskStartScheduler(void) {
	__enable_irq();
	pthread_exit(NULL);
}