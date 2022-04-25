#pragma once

#include <stdbool.h>

#include "boxlib/lcd.h"

//needs the filesystem running, to get the display type
void GuiInit(void);

//writes the display type to the filesystem
void GuiLcdSet(eDisplay_t type);

void GuiTransferStart(void);

void GuiTransferDone(void);

void GuiJumpBinScreen(void);

void GuiUpdateFilelist(void);

void GuiShowBinData(const char * name, const char * version, const char * author,
     const char * license, const char * date, bool watchdog,
     bool autostart, bool saved);

//the lenght specifier allows non \0 terminated strings
void GuiShowInfoData(const char * text, size_t textLen);

//Updates the GFX. Only images with a even x and y size are supported.
//Moreover the GFX dimension must be smaller or equal the resolution of the screen.
void GuiShowGfxData(const uint8_t * data, size_t dataLen, uint16_t x, uint16_t y);

void GuiShowFsData(uint32_t totalBytes, uint32_t freeBytes, uint32_t sectors, uint32_t clustersize);

//Fills the variables with the currently selected screen resolution, might be zero.
void GuiScreenResolutionGet(uint16_t * x, uint16_t * y);

//call periodically to react to key presses
//key can be from the serial input. 0 for doing nothing.
void GuiCycle(char key);
