#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "boxlib/lcd.h"

typedef struct {
	const char text[7];
	float unit;
} textUnit_t;

//needs the filesystem running, to get the display type
void GuiInit(void);

void GuiAppendString(const char * string);

//call periodically to react to key presses
//key can be from the serial input. 0 for doing nothing.
void GuiCycle(char key);
