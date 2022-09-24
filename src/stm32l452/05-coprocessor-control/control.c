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

#include "control.h"

#include "boxlib/keys.h"
#include "boxlib/leds.h"
#include "boxlib/rs232debug.h"
#include "boxlib/lcd.h"
#include "boxlib/flash.h"
#include "boxlib/peripheral.h"
#include "boxlib/coproc.h"

#include "main.h"

#include "utility.h"
#include "femtoVsnprintf.h"

#include "gui.h"
#include "filesystem.h"

uint32_t g_cycleTick;

void ControlHelp(void) {
	printf("h: Print help\r\n");
	printf("r: Reset\r\n");
	printf("p: Read out all data from the coprocessor\r\n");
	printf("n: New battery\r\n");
	printf("x: Reset stats\r\n");
	printf("f: Force full charge\r\n");
	printf("c: Set maximum current\r\n");
	printf("o: Power down - off\r\n");
	printf("x: Set alarm\r\n");
	printf("m: Set mode on disconnect\r\n");
	printf("w-a-s-d: Send key code to GUI\r\n");
}

void ControlInit(void) {
	Led1Yellow();
	PeripheralPowerOff();
	HAL_Delay(500);
	PeripheralPowerOn();
	Rs232Init();
	printf("\r\nCoprocessor control %s\r\n", APPVERSION);
	printf("h: Print help\r\n");

	PeripheralInit();
	FlashEnable(64); //250kHz
	FilesystemMount();
	GuiInit();
	Led1Off();
	g_cycleTick = HAL_GetTick();
}

void ExecReset(void) {
	printf("Reset selected\r\n");
	Rs232Flush();
	NVIC_SystemReset();
}

void TemperatureToString(char * output, size_t len, int16_t temperature) {
	char sign[2] = {0};
	if (temperature < 0) {
		sign[0] = '-';
		temperature *= -1;
	}
	unsigned int degree = temperature / 10;
	unsigned int degree10th = temperature % 10;
	femtoSnprintf(output, len, "%s%u.%u°C", sign, degree, degree10th);
}

const char * g_chargerState[STATES_MAX] = {
"Battery full, not charging",
"Battery charging",
"Battery temperature out of bounds",
"No battery / battery defective",
"Battery voltage overflow. Battery defective",
"Input voltage too low for charge",
"Input voltage too high for charge",
"Battey precharging because of very low voltage",
"Failed to fully charge",
"Software failure",
"Charger defective"
};


const char * g_chargerError[ERRORS_MAX] = {
"No error",
"Battery voltage too high - defective",
"Battery voltage too low - defective, or started without a battery inserted",
"Failed to fully charge - charger or battery defective",
"Failed to measure current - charger or battery defective"
};

void ExecPrintStats(void) {
	char text[16];

	uint16_t version = CoprocReadVersion();
	printf("Coproc firmware: %u, version %u\r\n", version >> 8, version & 0xFF);

	uint16_t vcc = CoprocReadVcc();
	printf("Vcc = %umV\r\n", vcc);

	int16_t cpuTemperature = CoprocReadCpuTemperature();
	TemperatureToString(text, sizeof(text), cpuTemperature);
	printf("CPU temperature: %s (0x%x)\r\n", text, cpuTemperature);

	uint16_t uptime = CoprocReadUptime();
	printf("Uptime: %uh\r\n", uptime);

	uint16_t optime = CoprocReadOptime();
	printf("Optime: %udays\r\n", optime);

	uint16_t batTemperature = CoprocReadBatteryTemperature();
	TemperatureToString(text, sizeof(text), batTemperature);
	printf("Battery temperature: %s (0x%x)\r\n", text, batTemperature);

	uint16_t batVcc = CoprocReadBatteryVoltage();
	printf("Battery: %umV\r\n", batVcc);

	uint16_t batI = CoprocReadBatteryCurrent();
	printf("Battery charging: %umA\r\n", batI);

	uint8_t state = CoprocReadChargerState();
	if (state < STATES_MAX) {
		printf("Charger state: %u - %s\r\n", state, g_chargerState[state]);
	} else {
		printf("Charger state: %u - unknown\r\n", state);
	}

	uint8_t error = CoprocReadChargerError();
	if (error < ERRORS_MAX) {
		printf("Charger error: %u - %s\r\n", error, g_chargerError[error]);
	} else {
		printf("Charger error: %u - unknown\r\n", error);
	}

	uint16_t batCap = CoprocReadChargerAmount();
	printf("Battery charged: %umAh\r\n", batCap);

	uint16_t batTotal = CoprocReadChargedTotal();
	printf("Battery total charged: %uAh\r\n", batTotal);

	uint16_t batChargecycles = CoprocReadChargedCycles();
	printf("Battery charge cycles: %u\r\n", batChargecycles);

	uint16_t batPrechargecycles = CoprocReadPrechargedCycles();
	printf("Battery precharge cycles: %u\r\n", batPrechargecycles);

	uint16_t batPwm = CoprocReadChargerPwm();
	printf("Battery PWM: %u\r\n", batPwm);

	uint16_t batTime = CoprocReadBatteryChargeTime();
	uint16_t min = batTime / 60;
	uint16_t sec = batTime % 60;
	printf("Battery started charge %u:%02u", min, sec);
}

void ReadSerialLine(char * input, size_t len) {
	memset(input, 0, len);
	size_t i = 0;
	while (i < (len - 1)) {
		char c = Rs232GetChar();
		if (c != 0) {
			input[i] = c;
			i++;
			printf("%c", c);
		}
		if ((c == '\r') || (c == '\n'))
		{
			break;
		}
	}
}

void ExecNewBattery(void) {
	printf("Enter y to confirm\r\n");
	char buffer[8];
	ReadSerialLine(buffer, sizeof(buffer));
	if (buffer[0] == 'y') {
		printf("Reset battery error state...\r\n");
		CoprocBatteryNew();
	} else {
		printf("Aborted\r\n");
	}
}


void ExecResetStats(void) {
	printf("Enter y to confirm\r\n");
	char buffer[8];
	ReadSerialLine(buffer, sizeof(buffer));
	if (buffer[0] == 'y') {
		printf("Reset battery stats...\r\n");
		CoprocBatteryStatReset();
	} else {
		printf("Aborted\r\n");
	}
}

void ExecMaxCurrent(void) {
	printf("Enter maximum current (0...200)[mA]\r\n");
	char buffer[8];
	ReadSerialLine(buffer, sizeof(buffer));
	unsigned int current = 0;
	sscanf(buffer, "%u", &current);
	if (current <= 200) {
		printf("Set maximum current to %umA\r\n", current);
		CoprocBatteryCurrentMax(current);
	} else {
		printf("Error, out of range\r\n");
	}
}

void ExecAlarm(void) {
	printf("Enter alarm time (0...65565)[s]\r\n");
	char buffer[8];
	ReadSerialLine(buffer, sizeof(buffer));
	unsigned int alarm = 0;
	sscanf(buffer, "%u", &alarm);
	if (alarm <= 65565) {
		printf("Set wakeup alarm to %us\r\n", alarm);
		CoprocWriteAlarm(alarm);
	} else {
		printf("Error, out of range\r\n");
	}
}

void ExecMode(void) {
	printf("Enter mode (0...1)\r\n");
	char buffer[8];
	ReadSerialLine(buffer, sizeof(buffer));
	unsigned int mode = 0;
	sscanf(buffer, "%u", &mode);
	if (mode <= 1) {
		printf("Set mode to %u\r\n", mode);
		CoprocWritePowermode(mode);
	} else {
		printf("Error, out of range\r\n");
	}
}

void ControlCycle(void) {
	static uint32_t ledCycle = 0;
	//led flash
	if (ledCycle < 500) {
		Led2Green();
	} else {
		Led2Off();
	}
	if (ledCycle >= 1000) {
		ledCycle = 0;
	}
	ledCycle++;
	char input = Rs232GetChar();
	if (input) {
		printf("%c", input);
		if (input == 'h') {
			ControlHelp();
		}
		if (input == 'r') {
			ExecReset();
		}
		if (input == 'p') {
			ExecPrintStats();
		}
		if (input == 'n') {
			ExecNewBattery();
		}
		if (input == 'x') {
			ExecResetStats();
		}
		if (input == 'f') {
			printf("Force max charge\r\n");
			CoprocBatteryForceCharge();
		}
		if (input == 'c') {
			ExecMaxCurrent();
		}
		if (input == 'o') {
			CoprocWritePowerdown();
		}
		if (input == 'x') {
			ExecAlarm();
		}
		if (input == 'm') {
			ExecMode();
		}
	}
	GuiCycle(input);
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
