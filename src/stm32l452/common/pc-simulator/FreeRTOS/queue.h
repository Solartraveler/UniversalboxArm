#pragma once

#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>

typedef void * QueueHandle_t;

typedef struct {
	size_t rptr;
	size_t wptr;
	uint8_t * dataArray;
	size_t elements;
	size_t elementSize;
	pthread_mutex_t mutex;
} StaticQueue_t;


QueueHandle_t xQueueCreateStatic(size_t queueElements, size_t itemSize, uint8_t * buffer, StaticQueue_t *pQueueState);

//returns true if element could be queued
bool xQueueSendToBack(QueueHandle_t queue, const void * dataIn, uint32_t waitTicks);

//returns true if element was available and dataOut is written
bool xQueueReceive(QueueHandle_t queue, void * dataOut,  uint32_t waitTicks);

