#pragma once

void AppInit(void);

void AppCycle(void);

/*Opens a file. If playback is true, it is also played.
*/
void PlayerStart(const char * filepath, bool playback);

void PlayerStop(void);

void PlayerFileGetMeta(char * text, size_t maxLen);

void PlayerFileGetState(char * text, size_t maxLen);
