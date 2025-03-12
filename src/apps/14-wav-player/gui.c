#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <alloca.h>

#include "gui.h"

#include "menudata.c"

#include "boxlib/lcd.h"
#include "boxlib/leds.h"
#include "boxlib/coproc.h"
#include "boxlib/keys.h"
#include "boxlib/keysIsr.h"
#include "femtoVsnprintf.h"
#include "ff.h"
#include "filesystem.h"
#include "framebufferBwFast.h"
#include "menu-interpreter.h"
#include "menu-text.h"
#include "utility.h"
#include "wavplayer.h"

#define TEXT_LEN_MAX 64
#define FILEPATH_MAX 256
#define FILELIST_MAX 2048

typedef struct {
	eDisplay_t type;
	uint16_t pixelX;
	uint16_t pixelY;
	char fileselect[FILELIST_MAX];
	char filename[TEXT_LEN_MAX];
	char attributes[TEXT_LEN_MAX];
	char state[TEXT_LEN_MAX];
	char filepath[FILEPATH_MAX];
	bool leftPressed;
	bool rightPressed;
	bool upPressed;
	bool downPressed;
	uint32_t cycle;
} guiState_t;

guiState_t g_gui;

uint8_t menu_byte_get(MENUADDR addr) {
	if (addr < MENU_DATASIZE) {
		return menudata[addr];
	}
	return 0;
}

void GuiCreateFullPath(char * fullPath, size_t maxLen, const char * filename) {
	if (strlen(g_gui.filepath) > 1) {
		snprintf(fullPath, maxLen, "%s/%s", g_gui.filepath, filename);
	} else {
		snprintf(fullPath, maxLen, "/%s", filename);
	}
}

void GuiAppendFilelist(const char * filename) {
	size_t lenAppend = strlen(filename);
	size_t lenUsed = strlen(g_gui.fileselect);
	if ((FILELIST_MAX - lenUsed) > lenAppend) {
		memcpy(g_gui.fileselect + lenUsed, filename, lenAppend + 1);
	}
}

void GuiUpdateFilelist(void) {
	DIR d;
	FILINFO fi;
	g_gui.fileselect[0] = '\0';
	bool elementBefore = false;
	if (strlen(g_gui.filepath) > 1) {
		GuiAppendFilelist(".."); //if not root directory, allow to go one level up
		elementBefore = true;
	}
	printf("Content of %s:\r\n", g_gui.filepath);
	if (f_opendir(&d, g_gui.filepath) == FR_OK) {
		while (f_readdir(&d, &fi) == FR_OK) {
			if (fi.fname[0]) {
				printf("%s\r\n", fi.fname);
				if ((EndsWith(fi.fname, ".wav")) || (fi.fattrib & AM_DIR)) {
					if (elementBefore) {
						GuiAppendFilelist("\n");
					}
					GuiAppendFilelist(fi.fname);
					elementBefore = true;
				}
			} else {
				break;
			}
		}
		f_closedir(&d);
	}
}

void GuiUpdateText(void) {
	PlayerFileGetMeta(g_gui.attributes, sizeof(g_gui.attributes));
	PlayerFileGetState(g_gui.state, sizeof(g_gui.state));
}

void GuiFileSelect(uint16_t index) {
	//seek the line start
	const char * pStart = g_gui.fileselect;
	while(index) {
		if (*pStart == '\n') {
			index--;
		}
		if (*pStart == '\0') {
			return; //error case
		}
		pStart++;
	}
	//seek line end
	const char * pEnd = pStart;
	while(*pEnd != '\0') {
		if (*pEnd == '\n') {
			break;
		}
		pEnd++;
	}
	//copy to buffer
	char filename[TEXT_LEN_MAX] = {0};
	char filepath[FILEPATH_MAX] = {0};
	memcpy(filename, pStart, pEnd - pStart);
	if (strcmp(filename, "..")) {
		GuiCreateFullPath(filepath, sizeof(filepath), filename);
	} else {
		const char * indexSlash = strrchr(g_gui.filepath, '/');
		if ((indexSlash != g_gui.filepath) && (indexSlash != NULL)) {
			memcpy(filepath, g_gui.filepath, indexSlash - g_gui.filepath - 1);
		} else {
			filepath[0] = '/';
		}
	}
	printf("Selected %s\r\n", filepath);
	//If it is a directory, update list, otherwise open the file for playing
	bool isDirectory = false;
	if (strlen(filepath) > 1) { //f_stat does not work for the root directory
		FILINFO fno;
		if (f_stat(filepath, &fno) == FR_OK) {
			if (fno.fattrib & AM_DIR) {
				isDirectory = true;
			}
		} else {
			printf("Error, could not get info from path\r\n");
		}
	} else {
		isDirectory = true;
	}
	if (isDirectory) {
		strlcpy(g_gui.filepath, filepath, sizeof(g_gui.filepath));
		GuiUpdateFilelist();
		menu_listindexstate[MENU_LISTINDEX_FILEINDEX] = 0;
	} else {
		strlcpy(g_gui.filename, filename, sizeof(g_gui.filename));
		PlayerStart(filepath, false);
		GuiUpdateText();
		menu_keypress(50); //go to the play screen
	}
}

uint8_t menu_action(MENUACTION action) {
	if (action == MENU_ACTION_FILESELECT) {
		GuiFileSelect(menu_listindexstate[MENU_LISTINDEX_FILEINDEX]);
		return 1;
	}
	if (action == MENU_ACTION_PLAY) {
		char filepath[FILEPATH_MAX];
		GuiCreateFullPath(filepath, sizeof(filepath), g_gui.filename);
		PlayerStart(filepath, true);
		GuiUpdateText();
		return 1;
	}
	if (action == MENU_ACTION_STOP) {
		PlayerStop();
		return 1;
	}
	return 0;
}

void GuiInit(void) {
	printf("Starting GUI\r\n");
	menu_strings[MENU_TEXT_FILESELECT] = g_gui.fileselect;
	menu_strings[MENU_TEXT_FILENAME] = g_gui.filename;
	menu_strings[MENU_TEXT_ATTRIBUTES] = g_gui.attributes;
	menu_strings[MENU_TEXT_STATE] = g_gui.state;
	g_gui.filepath[0] = '/';
	GuiUpdateFilelist();
	g_gui.type = FilesystemReadLcd();
	uint16_t action = 0;
	if (g_gui.type != NONE) {
		LcdBacklightOn();
		LcdEnable(2); //8MHz
		LcdInit(g_gui.type);
	}
	menu_redraw();
	if (g_gui.type == ST7735_128) {
		g_gui.pixelX = 128;
		g_gui.pixelY = 128;
	} else if (g_gui.type == ST7735_160) {
		g_gui.pixelX = 160;
		g_gui.pixelY = 128;
	} else if (g_gui.type == ILI9341) {
		g_gui.pixelX = 320;
		g_gui.pixelY = 240;
		action = 102;
	} else {
		printf("No GUI enabled\r\n");
	}
	if (action) {
		menu_screen_size(g_gui.pixelX, g_gui.pixelY);
		menu_keypress(action);
	}
}

void GuiCycle(char key) {
	bool state = KeyLeftReleased();
	if (((g_gui.leftPressed == false) && (state)) || (key == 'a')) {
		if (g_gui.type != NONE) {
			menu_keypress(2);
		}
	}
	g_gui.leftPressed = state;

	state = KeyRightReleased();
	if (((g_gui.rightPressed == false) && (state)) || (key == 'd')) {
		if (g_gui.type != NONE) {
			menu_keypress(1);
		}
	}
	g_gui.rightPressed = state;

	state = KeyUpReleased();
	if (((g_gui.upPressed == false) && (state)) || (key == 'w')) {
		if (g_gui.type != NONE) {
			menu_keypress(3);
		}
	}
	g_gui.upPressed = state;

	state = KeyDownReleased();
	if (((g_gui.downPressed == false) && (state)) || (key == 's')) {
		if (g_gui.type != NONE) {
			menu_keypress(4);
		}
	}
	g_gui.downPressed = state;

	g_gui.cycle++;
	if (g_gui.cycle == 500) { //run every 500ms
		GuiUpdateText();
		menu_redraw();
		g_gui.cycle = 0;
	}
}

