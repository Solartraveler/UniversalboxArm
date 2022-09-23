/* Boxlib
(c) 2021 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause
*/

#include <stdbool.h>

#include "coproc.h"

#include "main.h"

#include "coprocCommands.h"
#include "mcu.h"

bool CoprocInGet(void) {
	if (HAL_GPIO_ReadPin(AvrSpiMiso_GPIO_Port, AvrSpiMiso_Pin) == GPIO_PIN_RESET) {
		return false;
	}
	return true;
}

static void CoprocClockSet(bool state) {
	if (state) {
		HAL_GPIO_WritePin(AvrSpiSck_GPIO_Port, AvrSpiSck_Pin, GPIO_PIN_SET);
	} else {
		HAL_GPIO_WritePin(AvrSpiSck_GPIO_Port, AvrSpiSck_Pin, GPIO_PIN_RESET);
	}
}

static void CoprocDataSet(bool state) {
	if (state) {
		HAL_GPIO_WritePin(AvrSpiMosi_GPIO_Port, AvrSpiMosi_Pin, GPIO_PIN_SET);
	} else {
		HAL_GPIO_WritePin(AvrSpiMosi_GPIO_Port, AvrSpiMosi_Pin, GPIO_PIN_RESET);
	}
}

static void CoprocCycleDelay(void) {
	//625µs minimum
	HAL_Delay(1);
	//McuDelayUs(700);
}

uint16_t CoprocSendCommand(uint8_t command, uint16_t data) {
	uint32_t dataIn = 0;
	uint32_t dataOut = (command << 16) | data;
	for (uint32_t i = 0; i < 24; i++) {
		if (dataOut & 0x800000) {
			CoprocDataSet(true);
		} else {
			CoprocDataSet(false);
		}
		dataOut <<= 1;
		CoprocCycleDelay();
		dataIn <<= 1;
		if (CoprocInGet()) {
			dataIn |= 1;
		}
		CoprocClockSet(true);
		CoprocCycleDelay();
		CoprocClockSet(false);
	}
	return dataIn & 0xFFFFFF;
}

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

int16_t CoprocReadBatteryTemperature(void) {
	int16_t temperature = CoprocSendCommand(CMD_BAT_TEMPERATURE, 0);
	return temperature;
}

uint16_t CoprocReadBatteryVoltage(void) {
	return CoprocSendCommand(CMD_BAT_VOLTAGE, 0);
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
	return CoprocSendCommand(CMD_BAT_CURRENT_MAX_R, 0);
}

//read back the time since starting of current charge in [s]
uint16_t CoprocReadBatteryChargeTime(void) {
	return CoprocSendCommand(CMD_BAT_TIME, 0);
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
