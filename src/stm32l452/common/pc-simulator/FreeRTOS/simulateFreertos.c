/* Simulation helper
(c) 2022 by Malte Marwedel

SPDX-License-Identifier: BSD-3-Clause

Some very basic FreeRTOS simulation with posix threads.

*/

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>

#include "FreeRTOS.h"

#include "simulated.h"

#include "task.h"
#include "queue.h"
#include "semphr.h"

bool g_schedulerStarted;

void vTaskDelay(uint32_t ticks) {
	HAL_Delay(ticks);
}

void taskYIELD(void) {
	HAL_Delay(0);
}

void * ThreadStarter(void * param) {
	StaticTask_t * pData = (StaticTask_t *)param;
	/*The waiting here allows setting up the tasks before start task is called.
	  This is important if the thread is accessing a variable which is set up
	  between the task setup and the scheduler start
	*/
	while(g_schedulerStarted == false) {
		usleep(1000);
		__sync_synchronize();
	}
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
	g_schedulerStarted = true;
	pthread_exit(NULL);
}

QueueHandle_t xQueueCreateStatic(size_t queueElements, size_t itemSize, uint8_t * buffer, StaticQueue_t *pQueueState) {
	pQueueState->rptr = 0;
	pQueueState->wptr = 0;
	pQueueState->dataArray = buffer;
	pQueueState->elements = queueElements;
	pQueueState->elementSize = itemSize;
	if (pthread_mutex_init(&(pQueueState->mutex), NULL)) {
		return NULL; //error
	}
	return pQueueState;
}

bool xQueueSendToBack(QueueHandle_t queue, const void * dataIn, uint32_t waitTicks) {
	StaticQueue_t *pQueueState = (StaticQueue_t *)queue;
	do {
		bool found = false;
		if (pthread_mutex_lock(&(pQueueState->mutex))) {
			return false; //some error
		}
		size_t writeNext = (pQueueState->wptr + 1) % pQueueState->elements;
		if (writeNext != pQueueState->rptr) {
			memcpy(pQueueState->dataArray + pQueueState->wptr * pQueueState->elementSize, dataIn, pQueueState->elementSize);
			found = true;
			pQueueState->wptr = (pQueueState->wptr + 1) % pQueueState->elements;
		}
		pthread_mutex_unlock(&(pQueueState->mutex));
		if (found) {
			return true;
		}
		if (waitTicks) {
			usleep(1000);
		}
	} while(waitTicks--);
	return false;
}

bool xQueueReceive(QueueHandle_t queue, void * dataOut,  uint32_t waitTicks) {
	StaticQueue_t *pQueueState = (StaticQueue_t *)queue;
	do {
		bool found = false;
		if (pthread_mutex_lock(&(pQueueState->mutex))) {
			return false; //some error
		}
		if (pQueueState->rptr != pQueueState->wptr) {
			memcpy(dataOut, pQueueState->dataArray + pQueueState->rptr * pQueueState->elementSize, pQueueState->elementSize);
			found = true;
			pQueueState->rptr = (pQueueState->rptr + 1) % pQueueState->elements;
		}
		pthread_mutex_unlock(&(pQueueState->mutex));
		if (found) {
			return true;
		}
		if (waitTicks) {
			usleep(1000);
		}
	} while(waitTicks--);
	return false;
}

SemaphoreHandle_t xSemaphoreCreateMutexStatic(StaticSemaphore_t *pState) {
	if (pthread_mutex_init(&(pState->mutex), NULL)) {
		return NULL;
	}
	pState->type = 1;
	return pState;
}

void vSemaphoreDelete(SemaphoreHandle_t semaphore) {
	StaticSemaphore_t * pSemaphoreState = (StaticSemaphore_t *)semaphore;
	pthread_mutex_destroy(&(pSemaphoreState->mutex));
	memset(pSemaphoreState, 0, sizeof(StaticSemaphore_t));
}

bool xSemaphoreTake(SemaphoreHandle_t semaphore, uint32_t waitTicks) {
	StaticSemaphore_t * pSemaphoreState = (StaticSemaphore_t *)semaphore;
	if (pSemaphoreState->type == 1) {
		int result;
		do {
			result = pthread_mutex_trylock(&(pSemaphoreState->mutex));
			if (result == 0) {
				return true;
			}
			if (result != EBUSY) {
				return false; //error condition
			}
			if (waitTicks) {
				usleep(1000);
			}
		} while(waitTicks--);
	}
	return false;
}

bool xSemaphoreGive(SemaphoreHandle_t semaphore) {
	StaticSemaphore_t * pSemaphoreState = (StaticSemaphore_t *)semaphore;
	if (pSemaphoreState->type == 1) {
		if (pthread_mutex_unlock(&(pSemaphoreState->mutex)) == 0) {
			return true;
		}
	}
	return false;
}
