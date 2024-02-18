/*
   Gamebox
    Copyright (C) 2004-2006, 2023  by Malte Marwedel
    m.talk AT marwedels dot de

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdio.h>

#include "main.h"

#include "FreeRTOS.h"
#include "task.h"

#include "boxlib/keys.h"
#include "boxlib/leds.h"
#include "boxlib/rs232debug.h"
#include "boxlib/lcd.h"
#include "boxlib/flash.h"
#include "boxlib/peripheral.h"
#include "boxlib/coproc.h"
#include "boxlib/mcu.h"
#include "boxlib/readLine.h"
#include "boxlib/systickWithFreertos.h"

#include "framebufferLowres.h"
#include "framebufferConfig.h"
#include "filesystem.h"
#include "peripheralMt.h"
#include "utility.h"
#include "screenshot.h"

#define EEPROM_FILENAME "/etc/gamebox.eep"

#define COLORS (1 << (FB_RED_IN_BITS + FB_GREEN_IN_BITS + FB_BLUE_IN_BITS))

/*x-axis: green, output: highest 6 bit
  y-axis: red, output: 5 bit

  Originally, the red LEDs are brighter than the green ones, so RED_2 | GREEN_3
  should result in a yellow.
*/
#define RED_0 (0x0 << 11)
#define RED_1 (0xA << 11)
#define RED_2 (0x14 << 11)
#define RED_3 (0x1F << 11)
#define GREEN_0 (0x0 << 5)
#define GREEN_1 (0xD << 5)
#define GREEN_2 (0x1A << 5)
#define GREEN_3 (0x29 << 5)

const uint16_t g_palette[COLORS] = {
  RED_0 | GREEN_0, RED_0 | GREEN_1, RED_0 | GREEN_2, RED_0 | GREEN_3,
  RED_1 | GREEN_0, RED_1 | GREEN_1, RED_1 | GREEN_2, RED_1 | GREEN_3,
  RED_2 | GREEN_0, RED_2 | GREEN_1, RED_2 | GREEN_2, RED_2 | GREEN_3,
  RED_3 | GREEN_0, RED_3 | GREEN_1, RED_3 | GREEN_2, RED_3 | GREEN_3
};

//for the screenshots
#define RED24_0 (0x0 << 16)
#define RED24_1 (0x55 << 16)
#define RED24_2 (0xAA << 16)
#define RED24_3 (0xFF << 16)
#define GREEN24_0 (0x0 << 8)
#define GREEN24_1 (0x38 << 8)
#define GREEN24_2 (0x70 << 8)
#define GREEN24_3 (0xAA << 8)

const uint32_t g_palette24[COLORS] = {
  RED24_0 | GREEN24_0, RED24_0 | GREEN24_1, RED24_0 | GREEN24_2, RED24_0 | GREEN24_3,
  RED24_1 | GREEN24_0, RED24_1 | GREEN24_1, RED24_1 | GREEN24_2, RED24_1 | GREEN24_3,
  RED24_2 | GREEN24_0, RED24_2 | GREEN24_1, RED24_2 | GREEN24_2, RED24_2 | GREEN24_3,
  RED24_3 | GREEN24_0, RED24_3 | GREEN24_1, RED24_3 | GREEN24_2, RED24_3 | GREEN24_3
};

StaticTask_t g_IdleTcb;
StackType_t g_IdleStack[configMINIMAL_STACK_SIZE];

//512 is not enough for the GUI
#define TASK_STACK_ELEMENTS 1024

//for redrawing the display
StaticTask_t g_guiTask;
StackType_t g_guiStack[TASK_STACK_ELEMENTS];

//for the whole game logic
StaticTask_t g_mainTask;
StackType_t g_mainStack[TASK_STACK_ELEMENTS];

//for the joystick input
StaticTask_t g_inputTask;
StackType_t g_inputStack[TASK_STACK_ELEMENTS];


//for RS232 inputs and watchdog trigger so an easy reset can be done
StaticTask_t g_debugTask;
StackType_t g_debugStack[TASK_STACK_ELEMENTS];

/*For simulation of the eeprom */
extern char __eep_start__;
extern char __eep_end__;


//callback required for FreeRTOS on systems without malloc/free support
void vApplicationGetIdleTaskMemory(StaticTask_t ** ppxIdleTaskTCBBuffer, StackType_t ** ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize) {
	*ppxIdleTaskTCBBuffer = &g_IdleTcb;
	*ppxIdleTaskStackBuffer = g_IdleStack;
	*pulIdleTaskStackSize = sizeof(g_IdleStack) / sizeof(StackType_t);
}

void sei(void) {

}

void cli(void) {

}

u08 pgm_read_byte(const u08 *data) {
	return *data;
}

u16 pgm_read_word(const u16 *data) {
	return *data;
}

u08 eeprom_read_byte(const u08 *addr) {
	return *addr;
}

u16 eeprom_read_word(const u16 *addr) {
	return *addr;
}

void EepromSaveToFile(void) {
	uintptr_t eepromStart = (uintptr_t)&__eep_start__;
	uintptr_t eepromEnd = (uintptr_t)&__eep_end__;
	size_t len = eepromEnd - eepromStart;
	if (FilesystemWriteFile(EEPROM_FILENAME, (void*)eepromStart, len)) {
		printf("%u bytes of eeprom content saved\r\n", (unsigned int)len);
	}
}

void eeprom_write_byte(u08 *addr, u08 value) {
	*addr = value;
	EepromSaveToFile();
}

void eeprom_write_word(u16 *addr, u16 value) {
	*addr = value;
	EepromSaveToFile();
}

void EepromLoadFromFile(void) {
	uintptr_t eepromStart = (uintptr_t)&__eep_start__;
	uintptr_t eepromEnd = (uintptr_t)&__eep_end__;
	size_t len = eepromEnd - eepromStart;
	memset((void*)eepromStart, 0, len);
	if (FilesystemReadFile(EEPROM_FILENAME, (void*)eepromStart, len, NULL)) {
		printf("Eeprom content loaded\r\n");
	}
}

void init_random(void) {
	if (init_random_done == 0) {
		srand(McuTimestampUs());
		init_random_done = 1;
	}
}

/* This might generate false positives, should files be written at the same time
   which only happens when writing the eeprom emulation
*/
void RunFlashTest(void) {
	printf("Flash test started, press any key to terminate\r\n");
	uint32_t round = 0;
	while(Rs232GetChar() == 0) {
		round++;
		if (!FlashTest()) {
			printf("Error in round %u\r\n", (unsigned int)round);
		}
	}
	printf("Test stopped after %u rounds\r\n", (unsigned int)round);
}

static void DebugThread(void * arg) {
	printf("press r for reboot\r\n");
	printf("press x for screenshot\r\n");
	printf("press t testing the flash access\r\n");
	printf("press w-a-s-d and space for game control\r\n");
	uint32_t watchdogCnt = 0;
	uint16_t watchdogState = CoprocReadWatchdogCtrl();
	if (watchdogState) {
		printf("Watchdog enabled\r\n");
	}
	while (1) {
		//handle debug input
		char input = Rs232GetChar();
		if (input) {
			printf("%c", input);
		}
		switch (input) {
			case 'r': NVIC_SystemReset(); break;
			case 'x': ScreenshotPalette24(8, g_palette24, COLORS); break;
			case 't': RunFlashTest(); break;
			case ' ':
			case 'w':
			case 'a':
			case 's':
			case 'd': InputDebug(input); break;
			default: break;
		}
		if (watchdogState) {
			/*As the coprocessor communication is slow, we only do a reset if it is
			 enabled. Therefore the game performance should be higher with a disabled
			 watchdog.
			*/
			if (watchdogCnt == 0) {
				CoprocWatchdogReset();
				watchdogCnt = 10;
			}
			watchdogCnt--;
		}
		vTaskDelay(500);
	}
}

static void GuiThread(void * arg) {
	(void)arg;
	eDisplay_t type = FilesystemReadLcd();
	uint16_t pixelX = 0, pixelY = 0;
	if (type != NONE) {
		if (type == ST7735_128) {
			pixelX = 128;
			pixelY = 128;
		} else if (type == ST7735_160) {
			pixelX = 160;
			pixelY = 128;
		} else if (type == ILI9341) {
			pixelX = 320;
			pixelY = 240;
		}
		uint16_t scale = MIN(pixelX, pixelY) / FB_SIZE_X;
		uint16_t offsetX = (pixelX - (scale * FB_SIZE_X)) / 2;
		uint16_t offsetY = (pixelY - (scale * FB_SIZE_Y)) / 2;
		menu_screen_scale(offsetX, offsetY, scale, scale);
		//menu_screen_scale(offsetX + FB_SIZE_X / 2, offsetY + FB_SIZE_Y / 2, scale - 1, scale - 1);
		for (uint32_t i = 0; i < COLORS; i++) {
			menu_screen_palette_set(i, g_palette[i]);
		}
		LcdBacklightOn();
		LcdEnable(2); //32MHz
		LcdInit(type);
		menu_screen_size(pixelX, pixelY);
		menu_screen_clear();
		//black makes trouble with snake, because the end of the game field can not be seen
		uint16_t color = (4<<11) | (8<<5) | (2<<0); //looks dark silver (5:6:5 color bits)
		menu_screen_colorize_border(color, pixelX, pixelY);
		while (1) {
			if (!GraphicUpdate()) {
				vTaskDelay(5);
			}
		}
	} else {
		printf("Error: No display selected\r\n");
		while(1) {
			vTaskDelay(100);
		}
	}
}

static void MainThread(void * arg) {
	(void)arg;
	KeysInit();
	PeripheralInitMt();
	FlashEnable(16); //4MHz
	FilesystemMount();
	CoprocInit();
	JoystickInit();
	EepromLoadFromFile();
	xTaskCreateStatic(&GuiThread, "gui", TASK_STACK_ELEMENTS, NULL, 1, g_guiStack, &g_guiTask);
	xTaskCreateStatic(&InputThread, "input", TASK_STACK_ELEMENTS, NULL, 1, g_inputStack, &g_inputTask);
	printf("Starting game menu...\r\n");
	input_select();
	menu_start();
	printf("Error: main_thread stopped\r\n");
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
	printf("Stack overflow in task 0x%x (%s)!\r\n", (unsigned int)xTask, pcTaskName);
	Rs232Flush();
}

void AppInit(void) {
	LedsInit();
	Led1Green();
	PeripheralPowerOff();
	McuClockToHsiPll(configCPU_CLOCK_HZ, RCC_HCLK_DIV1);
	HAL_Delay(100);
	PeripheralPowerOn();
	Rs232Init();
	printf("Gamebox Version 'Final " APPVERSION " (c) 2004-2013, 2023, 2024 by Malte Marwedel\r\n\r\n");
	printf("This program is free software; you can redistribute it and/or modify\r\n");
	printf("it under the terms of the GNU General Public License as published by\r\n");
	printf("the Free Software Foundation; either version 3 of the License, or\r\n");
	printf("(at your option) any later version.\r\n\r\n");
	printf("This program is distributed in the hope that it will be useful,\r\n");
	printf("but WITHOUT ANY WARRANTY; without even the implied warranty of\r\n");
	printf("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\r\n");
	printf("GNU General Public License for more details.\r\n\r\n");
	printf("You should have received a copy of the GNU General Public License\r\n");
	printf("along with this program; if not, write to the Free Software\r\n");
	printf("Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA\r\n\r\n");
	printf("The program was originally written for an 8 Bit microcontroller (AVR) \r\n");
	printf("but was ported to a PC and ARM with a minimum of source modifications.\r\n");
	Rs232Flush();
	xTaskCreateStatic(&MainThread, "main", TASK_STACK_ELEMENTS, NULL, 1, g_mainStack, &g_mainTask);
	xTaskCreateStatic(&DebugThread, "debug", TASK_STACK_ELEMENTS, NULL, 1, g_debugStack, &g_debugTask);
	Rs232Flush();
	SystickDisable();
	SystickForFreertosEnable();
	Led1Off();
	vTaskStartScheduler();
}
