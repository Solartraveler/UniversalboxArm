#pragma once

#include <stdint.h>
#include <stdbool.h>

#define PWM_MAX 65

void HardwareInit(void);

void ad_init(uint8_t prescal);

void LedOn(void);

void LedOff(void);

void ArmRun(void);

void ArmUserprog(void);