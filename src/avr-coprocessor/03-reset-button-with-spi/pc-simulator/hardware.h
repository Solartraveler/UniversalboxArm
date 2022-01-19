#pragma once

#include <stdint.h>
#include <stdbool.h>

void HardwareInit(void);

void LedOn(void);

void LedOff(void);

void LedOff(void);

void ArmRun(void);

void ArmReset(void);

void ArmBootload(void);

void ArmUserprog(void);

void ArmBatteryOn(void);

void SensorsOn(void);

void SensorsOff(void);

bool SpiSckLevel(void);

bool SpiDiLevel(void);

//in mV
uint16_t SensorsInputvoltageGet(void);

bool KeyPressedRight(void);

bool KeyPressedLeft(void);

void TimerInit(void);

bool TimerHasOverflown(void);

void WatchdogReset(void);