#pragma once

#include <stdbool.h>

void KeysInit(void);

/* The pressed value returns the current state, so if not polled fast enough,
events get lost. But it can be used for polling and checking multiple times.
Useful for waiting until two keys are pressed at the same time.
*/

bool KeyRightPressed(void);

bool KeyLeftPressed(void);

bool KeyUpPressed(void);

bool KeyDownPressed(void);
