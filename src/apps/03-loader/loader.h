#pragma once

#include <stdbool.h>

void AppInit(void);

void AppCycle(void);

//Executes a program already loaded to the main buffer
//If the call returns, an error has happened
void LoaderProgramStart(void);

bool LoaderTarLoad(const char * filename);

//Deletes or saves a loaded tar file. If it is on disk it is deleted,
//otherwise it is saved.
bool LoaderTarSaveDelete(void);

//manualle enable or disable the watchdog. If no is defined, 5s will be used
bool LoaderWatchdogEnforce(bool enabled);

//true sets the bin as autostart (must be on the disk first)
bool LoaderAutostartSet(bool enabled);
//Gets the filename in the config file for autostarting
bool LoaderAutostartGet(char * filenameOut, size_t bufferLen);

//unmounts and formats the filesystem, then mounts again
bool LoaderFormat(void);

void LoaderGfxUpdate(void);
