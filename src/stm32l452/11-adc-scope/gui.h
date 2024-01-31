#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "boxlib/lcd.h"

//needs the filesystem running, to get the display type
void GuiInit(void);

void GuiAppendString(const char * string);

//call periodically to react to key presses
//key can be from the serial input. 0 for doing nothing.
void GuiCycle(char key);
