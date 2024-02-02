#pragma once

#include <stdbool.h>

//Call for setting up the GPIOs
void IrInit(void);

void IrOn(void);

void IrOff(void);

bool IrPinSignal(void);
