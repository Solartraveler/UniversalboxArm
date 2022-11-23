#pragma once

#include <stdbool.h>

void KeysInit(void);

/* Released value returned, is provided by ISRs, so a release is never missed.
But once a true is returned, false will be returned until a user
press-and-releases the key again.
*/
bool KeyRightReleased(void);

bool KeyLeftReleased(void);

bool KeyUpReleased(void);

bool KeyDownReleased(void);