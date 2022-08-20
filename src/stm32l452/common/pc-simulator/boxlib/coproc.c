/* Boxlib emulation
(c) 2022 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause
*/

#include <stdbool.h>
#include <unistd.h>

#include "coproc.h"

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
	return 3210;
}


int16_t CoprocReadCpuTemperature(void) {
	return 405;
}

uint16_t CoprocReadUptime(void) {
	return 3;
}


uint16_t CoprocReadOptime(void) {
	return 4;
}

int16_t CoprocReadBatteryTemperature(void) {
	return 351;
}

uint16_t CoprocReadBatteryVoltage(void) {
	return 3341;
}

uint16_t CoprocReadBatteryCurrent(void) {
	return 0;
}

uint8_t CoprocReadChargerState(void) {
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
	return 0;
}

void CoprocWriteReboot(uint8_t mode) {
	(void)mode;
}

void CoprocWriteLed(uint8_t mode) {
	(void)mode;
}

void CoprocWatchdogCtrl(uint16_t timeout) {
	(void)timeout;
}

void CoprocWatchdogReset(void) {
}


void CoprocWritePowermode(uint8_t powermode) {
	(void)powermode;
}

void CoprocWriteAlarm(uint16_t alarm) {
	(void)alarm;
}

void CoprocWritePowerdown(void) {
}

void CoprocBatteryNew(void) {
}

void CoprocBatteryStatReset(void) {
}

void CoprocBatteryForceCharge(void) {
}

void CoprocBatteryCurrentMax(uint16_t current) {
	(void)current;
}
