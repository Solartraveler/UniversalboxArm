/* Coprocessor-control
(c) 2022 by Malte Marwedel

SPDX-License-Identifier: GPL-3.0-or-later
*/

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>

#include "infrared.h"

#include "boxlib/keys.h"
#include "boxlib/leds.h"
#include "boxlib/rs232debug.h"
#include "boxlib/lcd.h"
#include "boxlib/flash.h"
#include "boxlib/peripheral.h"
#include "boxlib/coproc.h"
#include "boxlib/mcu.h"
#include "boxlib/readLine.h"
#include "boxlib/ir.h"

#include "main.h"

#include "utility.h"
#include "femtoVsnprintf.h"
#include "filesystem.h"
#include "gui.h"

#include "irmp.h"

uint32_t g_cycleTick;
uint32_t g_maxTimeIsr;

extern const char * const irmp_protocol_names[IRMP_N_PROTOCOLS + 1];

void AppHelp(void) {
	printf("h: Print help\r\n");
	printf("r: Reset\r\n");
}

void TimerInit(void) {
	__HAL_RCC_TIM6_CLK_ENABLE();
	uint16_t div = 1;
	TIM6->CR1 = 0; //all stopped
	TIM6->CR2 = 0;
	TIM6->DIER = TIM_DIER_UIE;
	TIM6->PSC = (div - 1);
	TIM6->CNT = 0;
	TIM6->ARR = SystemCoreClock / F_INTERRUPTS;
	TIM6->CR1 |= TIM_CR1_CEN;
	HAL_NVIC_SetPriority(TIM6_DAC_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);
}

void TIM6_DAC_IRQHandler(void) {
	if (IrPinSignal()) {
		Led1Red();
	} else {
		Led1Off();
	}

	uint32_t tick = TIM6->CNT;
	irmp_ISR();
	uint32_t tock = TIM6->CNT;
	uint32_t delta = tock - tick;
	g_maxTimeIsr = MAX(g_maxTimeIsr, delta);

	TIM6->SR = 0;
	HAL_NVIC_ClearPendingIRQ(TIM6_DAC_IRQn);
}

void AppInit(void) {
	LedsInit();
	Led2Yellow();
	PeripheralPowerOff();
	McuClockToHsiPll(32000000, RCC_HCLK_DIV1);
	HAL_Delay(100);
	PeripheralPowerOn();
	Rs232Init();
	printf("\r\nInfrared decoder %s\r\n", APPVERSION);
	printf("h: Print help\r\n");
	Rs232Flush();
	KeysInit();
	CoprocInit();
	PeripheralInit();
	FlashEnable(8); //4MHz
	FilesystemMount();
	GuiInit();
	IrInit();
	IrOn();
	irmp_init();
	TimerInit();
	Led2Off();
	g_cycleTick = HAL_GetTick();
}

void ExecReset(void) {
	printf("Reset selected\r\n");
	Rs232Flush();
	NVIC_SystemReset();
}

void AppCycle(void) {
	static uint32_t ledCycle = 0;
	static uint8_t guiUpdate = 1;
	//led flash
	if (ledCycle < 500) {
		Led2Green();
	} else {
		Led2Off();
	}
	if (ledCycle >= 1000) {
		ledCycle = 0;
		CoprocWatchdogReset();
	}
	ledCycle++;
	char input = Rs232GetChar();
	if (input) {
		printf("%c", input);
		if (input == 'h') {
			AppHelp();
		}
		if (input == 'r') {
			ExecReset();
		}
	}
	IRMP_DATA irmpData;
	if (irmp_get_data(&irmpData)) {
		char buffer[60];
		char buffer2[64];
		snprintf(buffer, sizeof(buffer), "Prot %02u, addr 0x%04x, cmd 0x%04x, flags 0x%x (%s)",
		       (unsigned int)irmpData.protocol, (unsigned int)irmpData.address,
		       (unsigned int)irmpData.command, (unsigned int)irmpData.flags,
		       irmp_protocol_names[irmpData.protocol]);
		uint32_t ticks = AtomicExchange32(&g_maxTimeIsr, 0);
		float cpu = ((float)ticks * (float)100.0) / ((float)SystemCoreClock / (float)F_INTERRUPTS);
		printf("%s, max %u ticks - CPU %u%c\r\n", buffer, (unsigned int)ticks, (unsigned int)cpu, '%');
		snprintf(buffer2, sizeof(buffer2), "%s\n", buffer);
		GuiAppendString(buffer2); //really slow, because it redraws the GUI
		CoprocWatchdogReset(); //multiple strings appended really result in not much loops
	}
	if (guiUpdate) {
		GuiCycle(input);
	}
	/* Call this function 1000x per second, if one cycle took more than 1ms,
	   we skip the wait to catch up with calling.
	   cycleTick last is needed to prevent endless wait in the case of a 32bit
	   overflow.
	*/
	uint32_t cycleTickLast = g_cycleTick;
	g_cycleTick++; //next call expected tick value
	uint32_t tick;
	do {
		tick = HAL_GetTick();
		if (tick < g_cycleTick) {
			HAL_Delay(1);
		}
	} while ((tick < g_cycleTick) && (tick >= cycleTickLast));
}
