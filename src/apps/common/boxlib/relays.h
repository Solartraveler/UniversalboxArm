#pragma once

#include <stdbool.h>

//Call once to init the GPIOs. After calling, all relays are off
void RelaysInit(void);

void Relay1Set(bool state);

void Relay2Set(bool state);

void Relay3Set(bool state);

void Relay4Set(bool state);

