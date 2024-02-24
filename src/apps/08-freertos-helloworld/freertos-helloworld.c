/* FreeRTOS hello world
(c) 2022 by Malte Marwedel

SPDX-License-Identifier: GPL-3.0-or-later

This program lets three tasks run. One waits for the serial input,
another allows blinking the LEDs and one polls the buttons at the same time.
*/

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>

#include "freertos-helloworld.h"

#include "boxlib/keys.h"
#include "boxlib/leds.h"
#include "boxlib/rs232debug.h"
#include "boxlib/coproc.h"
#include "boxlib/mcu.h"
#include "boxlib/readLine.h"
#include "boxlib/peripheral.h"
#include "boxlib/systickWithFreertos.h"
#ifdef PC_SIM
#include "boxlib/lcd.h"
#endif

#include "main.h"

#include "utility.h"
#include "femtoVsnprintf.h"

#include "FreeRTOS.h"
#include "task.h"


StaticTask_t g_taskIdleTcb;
StackType_t g_taskIdleStack[64];

StaticTask_t g_taskBlinkyTcb;
StackType_t g_taskBlinkyStack[64];

StaticTask_t g_taskButtonTcb;
StackType_t g_taskButtonStack[64];

StaticTask_t g_taskWatchdogTcb;
StackType_t g_taskWatchdogStack[64];

StaticTask_t g_taskSerialTcb;
StackType_t g_taskSerialStack[256];

void ControlHelp(void) {
	printf("Available commands\r\n");
	printf("h: Print help\r\n");
	printf("r: Reset\r\n");
}

//callback required for FreeRTOS on systems without malloc/free support
void vApplicationGetIdleTaskMemory(StaticTask_t ** ppxIdleTaskTCBBuffer, StackType_t ** ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize) {
	*ppxIdleTaskTCBBuffer = &g_taskIdleTcb;
	*ppxIdleTaskStackBuffer = g_taskIdleStack;
	*pulIdleTaskStackSize = sizeof(g_taskIdleStack) / sizeof(StackType_t);
}

void ExecReset(void) {
	printf("Reset selected\r\n");
	Rs232Flush();
	NVIC_SystemReset();
}

void TaskBlinky(void * param) {
	(void)param;
	while (1) {
		Led1Green();
		vTaskDelay(500);
		Led1Off();
		vTaskDelay(500);
		Led1Red();
		vTaskDelay(500);
		Led1Off();
		vTaskDelay(500);
		taskYIELD(); //be nice
	}
}

void TaskButton(void * param) {
	(void)param;
	while (1) {
		if (KeyRightPressed()) {
			Led2Red();
		} else if (KeyUpPressed()) {
			Led2Green();
		} else if (KeyDownPressed()) {
			Led2Yellow();
		} else {
			Led2Off();
		}
		taskYIELD(); //be nice
	}
}

void TaskWatchdog(void * param) {
	(void)param;
	while (1) {
		CoprocWatchdogReset();
		vTaskDelay(10000);
	}
}

void TaskSerial(void * param) {
	(void)param;
	while (1) {
		char buffer[128];
		printf("Enter command and press enter\r\n");
		ReadSerialLine(buffer, sizeof(buffer));
		if (strcmp(buffer, "h") == 0) {
			ControlHelp();
		} else if (strcmp(buffer, "r") == 0) {
			ExecReset();
		} else if (strcmp(buffer, "42") == 0) {
			printf("You found the answer!\r\n");
		}
	}
}

void AppInit(void) {
	LedsInit();
	Led1Green();
	PeripheralPowerOff();
	HAL_Delay(100);
	PeripheralPowerOn();
	Rs232Init();
	printf("\r\nFreeRTOS hello world %s\r\n", APPVERSION);
	ControlHelp();
	KeysInit();
	CoprocInit();
#ifdef PC_SIM
	/*Since the real program has no LCD/GUI, but the gui is required in the
	  simulation for buttons and LEDs, we enable the GUI here.
	*/
	LcdEnable(1);
	LcdBacklightOn();
	LcdInit(ST7735_128);
#endif
	TaskHandle_t tBlinky = xTaskCreateStatic(&TaskBlinky, "blinky", sizeof(g_taskBlinkyStack) / sizeof (StackType_t), NULL, 1, g_taskBlinkyStack, &g_taskBlinkyTcb);
	TaskHandle_t tButton = xTaskCreateStatic(&TaskButton, "button", sizeof(g_taskButtonStack) / sizeof (StackType_t), NULL, 1, g_taskButtonStack, &g_taskButtonTcb);
	TaskHandle_t tWatchdog = xTaskCreateStatic(&TaskWatchdog, "watchdog", sizeof(g_taskWatchdogStack) / sizeof (StackType_t), NULL, 1, g_taskWatchdogStack, &g_taskWatchdogTcb);
	TaskHandle_t tSerial = xTaskCreateStatic(&TaskSerial, "serial", sizeof(g_taskSerialStack) / sizeof (StackType_t), NULL, 1, g_taskSerialStack, &g_taskSerialTcb);
	printf("Task handles: %x, %x, %x, %x\r\nStarting scheduler\r\n", (unsigned int)(uintptr_t)tBlinky, (unsigned int)(uintptr_t)tButton, (unsigned int)(uintptr_t)tWatchdog, (unsigned int)(uintptr_t)tSerial);
	Rs232Flush();
	SystickDisable();
	SystickForFreertosEnable();
	Led1Off();
	vTaskStartScheduler();
	printf("Error, starting scheduler failed\n");
	Rs232Flush();
	ExecReset();
}
