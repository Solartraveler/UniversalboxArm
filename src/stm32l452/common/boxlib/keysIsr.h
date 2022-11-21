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

/* Released value returned, is provided by ISRs, so a release is never missed.
But once a true is returned, false will be returned until a user
press-and-releases the key again.
*/
bool KeyRightReleased(void);

bool KeyLeftReleased(void);

bool KeyUpReleased(void);

bool KeyDownReleased(void);