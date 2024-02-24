/* Boxlib emulation
(c) 2022 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause
*/

#include <stdbool.h>
#include <unistd.h>
#include <stdint.h>

#include "boxlib/coproc.h"

#include "simulated.h"

uint32_t g_coprocChargeStartTime;
bool g_coprocChargeStart;
uint16_t g_coprocMaxCharge = 70;
uint8_t g_coprocLedMode = 1;
uint8_t g_coprocPowerMode;
uint16_t g_coprocAlarm;
uint16_t g_coprocWatchdog;

void CoprocInit(void) {


}

bool CoprocInGet(void) {
	return false;
	return true;
}

uint16_t CoprocSendCommand(uint8_t command, uint16_t data) {
	(void)command;
	(void)data;
	return 0;
}

uint16_t CoprocReadTestpattern(void) {
	return 0xF055;
}

uint16_t CoprocReadVersion(void) {
	return 0x0001;
}

uint16_t CoprocReadVcc(void) {
	static uint16_t noise = 0;
	noise = (noise + 2) % 4;
	return 3210 + noise;
}

int16_t CoprocReadCpuTemperature(void) {
	static uint16_t noise = 0;
	noise = (noise + 1) % 3;
	return 405 + noise;
}

uint16_t CoprocReadUptime(void) {
	return 3;
}


uint16_t CoprocReadOptime(void) {
	return 4;
}

uint8_t CoprocReadLed(void) {
	return g_coprocLedMode;
}

uint16_t CoprocReadWatchdogCtrl(void) {
	return g_coprocWatchdog;
}

uint8_t CoprocReadPowermode(void) {
	return g_coprocPowerMode;
}

uint16_t CoprocReadAlarm(void) {
	return g_coprocAlarm;
}

int16_t CoprocReadBatteryTemperature(void) {
	static uint16_t noise = 0;
	noise = (noise + 1) % 2;
	return 351 + noise;
}

uint16_t CoprocReadBatteryVoltage(void) {
	static uint16_t noise = 0;
	noise = (noise + 1) % 10;
	if (g_coprocChargeStart) {
		return 3380 + noise;
	}
	return 3340 + noise;
}

uint16_t CoprocReadBatteryMinVoltage(void) {
	return 3321;
}

uint16_t CoprocReadBatteryCurrent(void) {
	static uint16_t noise = 0;
	if (g_coprocChargeStart) {
		noise = (noise + 1) % 3;
		return g_coprocMaxCharge + noise - 2;
	}
	return 0;
}

uint8_t CoprocReadChargerState(void) {
	if (g_coprocChargeStart) {
		return 1;
	}
	return 0;
}

uint8_t CoprocReadChargerError(void) {
	return 0;
}

uint16_t CoprocReadChargerAmount(void) {
	return 123;
}

uint16_t CoprocReadChargedTotal(void) {
	return 11;
}

uint16_t CoprocReadChargedCycles(void) {
	return 23;
}

uint16_t CoprocReadPrechargedCycles(void) {
	return 0;
}

uint16_t CoprocReadChargerPwm(void) {
	if (g_coprocChargeStart) {
		return 80;
	}
	return 0;
}

//read back the value set by CoprocBatteryCurrentMax in [mA]
uint16_t CoprocReadBatteryCurrentMax(void) {
	return g_coprocMaxCharge;
}

//read back the time since starting of current charge in [s]
uint16_t CoprocReadBatteryChargeTime(void) {
	if (g_coprocChargeStart) {
		uint32_t time = HAL_GetTick() - g_coprocChargeStartTime;
		return time / 1000;
	}
	return 0;
}

uint8_t CoprocReadCpuLoad(void) {
	return 10;
}

void CoprocWriteReboot(uint8_t mode) {
	(void)mode;
}

void CoprocWriteLed(uint8_t mode) {
	g_coprocLedMode = mode;
}

void CoprocWatchdogCtrl(uint16_t timeout) {
	g_coprocWatchdog = timeout;
}

void CoprocWatchdogReset(void) {
}


void CoprocWritePowermode(uint8_t powermode) {
	g_coprocPowerMode = powermode;
}

void CoprocWriteAlarm(uint16_t alarm) {
	g_coprocAlarm = alarm;
}

void CoprocWritePowerdown(void) {
}

void CoprocWriteKeyPressTime(uint16_t time) {
	(void)time;
}

void CoprocBatteryNew(void) {
}

void CoprocBatteryStatReset(void) {
}

void CoprocBatteryForceCharge(void) {
	g_coprocChargeStart = true;
	g_coprocChargeStartTime = HAL_GetTick();
}

void CoprocBatteryCurrentMax(uint16_t current) {
	g_coprocMaxCharge = current;
	if (current == 0) {
		g_coprocChargeStart = false;
	}
}
