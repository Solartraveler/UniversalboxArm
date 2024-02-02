#pragma once

#include <stdio.h>
#include "boxlib/leds.h"

#define configMINIMAL_STACK_SIZE 64
#define configMAX_PRIORITIES 2
#define configUSE_PREEMPTION 1
#define configUSE_IDLE_HOOK 0
#define configUSE_TICK_HOOK 0
#define configUSE_16_BIT_TICKS 0
#define configKERNEL_INTERRUPT_PRIORITY 255
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 191
#define configLIBRARY_KERNEL_INTERRUPT_PRIORITY 15
#define configCPU_CLOCK_HZ 16000000UL
#define configTICK_RATE_HZ 1000
#define configSUPPORT_STATIC_ALLOCATION 1
#define configSUPPORT_DYNAMIC_ALLOCATION 0
#define INCLUDE_vTaskDelay 1
#define configASSERT(X) if (!(X)) {Led1Red(); Led2Red(); printf("Error, assert in %s:%u\r\n", __FILE__, __LINE__);};

#define vPortSVCHandler SVC_Handler
#define xPortPendSVHandler PendSV_Handler
