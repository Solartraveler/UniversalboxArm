#pragma once

#include <stdint.h>
#include <stdbool.h>

#define PWM_MAX 65

void HardwareInit(void);

void PinsInit(void);

void ad_init(uint8_t prescal);

void LedOn(void);

void LedOff(void);

void ArmRun(void);

void ArmReset(void);

void ArmBootload(void);

void ArmUserprog(void);

void ArmBatteryOn(void);

void ArmBatteryOff(void);

void SensorsOn(void);

void SensorsOff(void);

bool SpiSckLevel(void);

bool SpiDiLevel(void);

//in 0.1°C units
int16_t SensorsBatterytemperatureGet(void);

//in mV
uint16_t SensorsInputvoltageGet(void);


//directly the AD converter value
uint16_t VccRaw(void);

//directly the AD converter value
uint16_t DropRaw(void);

//in mV
uint16_t SensorsBatteryvoltageGet(void);

//in mA
uint16_t SensorsBatterycurrentGet(void);

//in 0.1°C units
int16_t SensorsChiptemperatureGet(void);

void PwmBatterySet(uint8_t val);

bool KeyPressedRight(void);

bool KeyPressedLeft(void);

void TimerInit(bool useIsr);

bool TimerHasOverflown(void);

bool TimerHasOverflownIsr(void);

void TimerStop(void);

void WatchdogReset(void);

void WatchdogDisable(void);

void WaitForInterrupt(void);

void WaitForExternalInterrupt(void);
