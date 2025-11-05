/* dac-tester
(c) 2025 by Malte Marwedel

SPDX-License-Identifier: GPL-3.0-or-later
*/
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>

#include "boxlib/coproc.h"
#include "boxlib/keys.h"
#include "boxlib/leds.h"
#include "boxlib/mcu.h"
#include "boxlib/peripheral.h"
#include "boxlib/readLine.h"
#include "boxlib/rs232debug.h"
#include "femtoVsnprintf.h"
#include "main.h"
#include "dacIo.h"
#include "utility.h"

#define F_CPU 32000000ULL

#define DAC_MAX 4095
#define DAC_VALUES (DAC_MAX + 1)

uint32_t g_cycleTick;
uint16_t g_dacValue;

void MatrixHelp(void) {
	printf("h: Print help\r\n");
	printf("r: Reset\r\n");
	printf("x: Enter value to set to the DAC\r\n");
	printf("t: Produces 1Hz sawtooth\r\n");
	printf("s: Produces 1kHz sine\r\n");
	printf("l: Produces 1MHz rectangle - but the DAC is not fast enough so its a triangle\r\n");
	printf("+: Increase DAC value by 10\r\n");
	printf("-: Decrease DAC value by 10\r\n");
}

void AppInit(void) {
	LedsInit();
	Led1Yellow();
	McuClockToHsiPll(F_CPU, RCC_HCLK_DIV1);
	PeripheralPowerOn();
	Rs232Init();
	printf("\r\nDAC tester %s\r\n", APPVERSION);
	printf("h: Print help\r\n");
	Rs232Flush();
	KeysInit();
	CoprocInit();
	PeripheralInit();
	DacInit();
	Led1Off();
	g_cycleTick = HAL_GetTick();
}

void ExecReset(void) {
	printf("Reset selected\r\n");
	Rs232Flush();
	NVIC_SystemReset();
}

void DacUpdate(void) {
	if (g_dacValue <= DAC_MAX) {
		DacSet(g_dacValue);
	} else {
		printf("Error, DAC value out of range\r\n");
	}
}

void DacInc(void) {
	g_dacValue += 10;
	if (g_dacValue > DAC_MAX) {
		g_dacValue = DAC_MAX;
	}
	printf("New value: %u\r\n", (unsigned int)g_dacValue);
	DacUpdate();
}

void DacDec(void) {
	if (g_dacValue >= 10) {
		g_dacValue -= 10;
	} else {
		g_dacValue = 0;
	}
	printf("New value: %u\r\n", (unsigned int)g_dacValue);
	DacUpdate();
}

void DacUser(void) {
	//The watchdog collides with the implementation of ReadSerialLine
	uint16_t watchdogState = CoprocReadWatchdogCtrl();
	CoprocWatchdogCtrl(0);
	char buffer[10];
	printf("Enter DAC value [0...4095]:\r\n");
	ReadSerialLine(buffer, sizeof(buffer));
	g_dacValue = atoi(buffer);
	CoprocWatchdogCtrl(watchdogState); //restore original value
	DacUpdate();
}

void DacSawtooth(void) {
	//Connect an LED and see it going bright within 1s, then dark again
	static uint16_t sawtooth[DAC_VALUES];
	for (uint16_t i = 0; i < DAC_VALUES; i++) {
		sawtooth[i] = i;
	}
	DacWaveform(sawtooth, DAC_VALUES, F_CPU / DAC_VALUES);
}

/*Should be divideable so F_CPU / (SINE_VALUE * 1000) gives an integer value (no remainder)
  Also it looks like there is a minimum time until the memory values are written, because
  4000 values @ 32MHz -> one value every 8 clock cycles simply does not work and gives just
  ~833Hz. But 32000 -> One value every 10 clock cycles works fine.
*/
#define SINE_VALUES 3200

void DacSine(void) {
	//Good to see on an oscilloscope
	static uint16_t sine[SINE_VALUES];
	float pi2 = (float)M_PI * (float)2.0;
	for (uint16_t i = 0; i < SINE_VALUES; i++) {
		sine[i] = ((float)DAC_MAX/2) + ((float)DAC_MAX/2) * sinf((float)i * pi2 / (float)SINE_VALUES);
	}
	DacWaveform(sine, SINE_VALUES, F_CPU / (SINE_VALUES * 1000));
}

#define LIMIT_VALUES 2

void DacLimit(void) {
	/*Good to see on an oscilloscope
	  The output is basically just a odd shaped triangle with a min voltage
	  of 2.02V and a max voltage of 3.16V. Even with no output load.
	*/
	static uint16_t limit[LIMIT_VALUES];
	limit[0] = 0;
	limit[1] = DAC_MAX;
	DacWaveform(limit, LIMIT_VALUES, F_CPU / (LIMIT_VALUES * 1000000));
}

void AppCycle(void) {
	static uint32_t ledCycle = 0;
	//led flash
	if (ledCycle < 500) {
		Led2Green();
	} else {
		Led2Off();
	}
	if (ledCycle >= 1000) {
		CoprocWatchdogReset();
		ledCycle = 0;
	}
	ledCycle++;
	char input = Rs232GetChar();
	if (input) {
		printf("%c", input);
		if (input == 'h') {
			MatrixHelp();
		} else if (input == 'r') {
			ExecReset();
		} else if (input == 'x') {
			DacUser();
		} else if (input == '+') {
			DacInc();
		} else if (input == '-') {
			DacDec();
		} else if (input == 't') {
			DacSawtooth();
		} else if (input == 's') {
			DacSine();
		} else if (input == 'l') {
			DacLimit();
		}

	}
	uint32_t cycleTickLast = g_cycleTick;
	g_cycleTick++; //next call expected tick value
	uint32_t tick;
	do {
		tick = HAL_GetTick();
	} while ((tick < g_cycleTick) && (tick >= cycleTickLast));
}
