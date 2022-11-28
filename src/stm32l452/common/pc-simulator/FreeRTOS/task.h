#pragma once

#include <stdint.h>
#include <pthread.h>

typedef void * TaskHandle_t;

typedef uint32_t StackType_t;

typedef void (* TaskFunction_t)(void *);

#define TASK_NAME_MAX 16

typedef struct {
	TaskFunction_t pStart;
	void * pParam;
	char name[TASK_NAME_MAX];
	pthread_t pThread;
} StaticTask_t;



void vTaskDelay(uint32_t ticks);

void taskYIELD(void);

TaskHandle_t xTaskCreateStatic(TaskFunction_t pTask, const char * name, size_t stackElements,
  void * param, uint32_t priority, StackType_t * pStack, StaticTask_t * pState);
