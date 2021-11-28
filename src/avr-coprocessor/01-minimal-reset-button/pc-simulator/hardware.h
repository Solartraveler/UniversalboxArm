#pragma once

#include <stdint.h>
#include <stdbool.h>

void PinsInit(void);

void LedOn(void);

void LedOff(void);

void LedOff(void);

void ArmRun(void);

void ArmReset(void);

void ArmBootload(void);

void ArmUserprog(void);

void ArmBatteryOn(void);

bool KeyPressedRight(void);

bool KeyPressedLeft(void);

void TimerInit(void);

bool TimerHasOverflown(void);

