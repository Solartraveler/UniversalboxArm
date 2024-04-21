/* Boxlib
(c) 2021 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause
*/

#include <stdbool.h>

#include "boxlib/coproc.h"

#include "coprocCommands.h"
#include "coprocPlatform.h"

uint16_t CoprocReadTestpattern(void) {
	uint16_t pattern = CoprocSendCommand(CMD_TESTREAD, 0);
	return pattern;
}

uint16_t CoprocReadVersion(void) {
	uint16_t version = CoprocSendCommand(CMD_VERSION, 0);
	return version;
}

uint16_t CoprocReadVcc(void) {
	uint16_t vcc = CoprocSendCommand(CMD_VCC, 0);
	return vcc;
}

int16_t CoprocReadCpuTemperature(void) {
	int16_t temperature = CoprocSendCommand(CMD_CPU_TEMPERATURE, 0);
	return temperature;
}

uint16_t CoprocReadUptime(void) {
	return CoprocSendCommand(CMD_UPTIME, 0);
}


uint16_t CoprocReadOptime(void) {
	return CoprocSendCommand(CMD_OPTIME, 0);
}

uint8_t CoprocReadLed(void) {
	return CoprocSendCommand(CMD_LED_READ, 0);
}

uint16_t CoprocReadWatchdogCtrl(void) {
	return CoprocSendCommand(CMD_WATCHDOG_CTRL_READ, 0);
}

uint8_t CoprocReadPowermode(void) {
	return CoprocSendCommand(CMD_POWERMODE_READ, 0);
}

uint16_t CoprocReadAlarm(void) {
	return CoprocSendCommand(CMD_ALARM_READ, 0);
}

int16_t CoprocReadBatteryTemperature(void) {
	int16_t temperature = CoprocSendCommand(CMD_BAT_TEMPERATURE, 0);
	return temperature;
}

uint16_t CoprocReadBatteryVoltage(void) {
	return CoprocSendCommand(CMD_BAT_VOLTAGE, 0);
}

uint16_t CoprocReadBatteryMinVoltage(void) {
	return CoprocSendCommand(CMD_BAT_MIN_VOLTAGE, 0);
}

uint16_t CoprocReadBatteryCurrent(void) {
	return CoprocSendCommand(CMD_BAT_CURRENT, 0);
}

uint8_t CoprocReadChargerState(void) {
	return CoprocSendCommand(CMD_BAT_CHARGE_STATE, 0);
}

uint8_t CoprocReadChargerError(void) {
	return CoprocSendCommand(CMD_BAT_CHARGE_ERR, 0);
}

uint16_t CoprocReadChargerAmount(void) {
	return CoprocSendCommand(CMD_BAT_CHARGED, 0);
}

uint16_t CoprocReadChargedTotal(void) {
	return CoprocSendCommand(CMD_BAT_CHARGED_TOT, 0);
}

uint16_t CoprocReadChargedCycles(void) {
	return CoprocSendCommand(CMD_BAT_CHARGE_CYC, 0);
}

uint16_t CoprocReadPrechargedCycles(void) {
	return CoprocSendCommand(CMD_BAT_PRECHARGE_CYC, 0);
}

uint16_t CoprocReadChargerPwm(void) {
	return CoprocSendCommand(CMD_BAT_PWM, 0);
}

uint16_t CoprocReadBatteryCurrentMax(void) {
	return CoprocSendCommand(CMD_BAT_CURRENT_MAX_READ, 0);
}

//read back the time since starting of current charge in [s]
uint16_t CoprocReadBatteryChargeTime(void) {
	return CoprocSendCommand(CMD_BAT_TIME, 0);
}

uint8_t CoprocReadCpuLoad(void) {
	return CoprocSendCommand(CMD_CPU_LOAD, 0);
}

uint16_t CoprocReadChargeNoCurrentVolt(void) {
	return CoprocSendCommand(CMD_BAT_CHARGE_NOCURR_VOLT, 0);
}

void CoprocWriteReboot(uint8_t mode) {
	CoprocSendCommand(CMD_REBOOT, 0xA600 | mode);
}

void CoprocWriteLed(uint8_t mode) {
	CoprocSendCommand(CMD_LED, mode);
}

void CoprocWatchdogCtrl(uint16_t timeout) {
	CoprocSendCommand(CMD_WATCHDOG_CTRL, timeout);
}

void CoprocWatchdogReset(void) {
	CoprocSendCommand(CMD_WATCHDOG_RESET, 0x0042);
}

void CoprocWritePowermode(uint8_t powermode) {
	CoprocSendCommand(CMD_POWERMODE, powermode);
}

void CoprocWriteAlarm(uint16_t alarm) {
	CoprocSendCommand(CMD_ALARM, alarm);
}

void CoprocWritePowerdown(void) {
	CoprocSendCommand(CMD_POWERDOWN, 0x1122);
}

void CoprocWriteKeyPressTime(uint16_t time) {
	CoprocSendCommand(CMD_KEYPRESSTIME, time);
}

void CoprocBatteryNew(void) {
	CoprocSendCommand(CMD_BAT_NEW, 0x1291);
}

void CoprocBatteryStatReset(void) {
	CoprocSendCommand(CMD_BAT_STAT_RESET, 0x3344);
}

void CoprocBatteryForceCharge(void) {
	CoprocSendCommand(CMD_BAT_FORCE_CHARGE, 0x0);
}

void CoprocBatteryCurrentMax(uint16_t current) {
	CoprocSendCommand(CMD_BAT_CURRENT_MAX, current);
}

