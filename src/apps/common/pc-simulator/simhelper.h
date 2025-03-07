#pragma once

#include <stdbool.h>

void CatchSignal(int sig);

/*Enables the simlauted flash and formats the filensystem.
  Then the file /etc/display.json is created.
  display may be one of the supported strings "128x128", "160x128", "320x240"
  and "NONE".
  The filesystem is left in an unmounted state.
*/
void CreateFilesystem(const char * display);

/*Copies the content found on the host system in the filenameSource
  to the internal filesystem at the place filenameDest.
  The filensystem should be available, but unmonuted. It is left in an unmounted state.
  Returns true if copying is successful.
*/
bool CopyFileToFilesystem(const char * filenameSource, const char * filenameDest);
