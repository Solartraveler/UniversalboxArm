#pragma once

#include <stdint.h>
#include <pthread.h>

typedef void * SemaphoreHandle_t;

typedef struct {
	/* If type == 1 -> its a mutex */
	uint8_t type;
	pthread_mutex_t mutex;
} StaticSemaphore_t;

SemaphoreHandle_t xSemaphoreCreateMutexStatic(StaticSemaphore_t *pState);

bool xSemaphoreTake(SemaphoreHandle_t semaphore, uint32_t waitTicks);

bool xSemaphoreGive(SemaphoreHandle_t semaphore);
