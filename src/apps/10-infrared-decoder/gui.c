#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "gui.h"

#include "menudata.c"

#include "boxlib/keys.h"
#include "boxlib/keysIsr.h"
#include "boxlib/lcd.h"
#include "femtoVsnprintf.h"
#include "filesystem.h"
#include "framebufferBwFast.h"
#include "infrared.h"
#include "menu-interpreter.h"
#include "menu-text.h"
#include "utility.h"

#define HISTORYLISTLEN 768
#define LISTLEN (HISTORYLISTLEN + 50)

#define BORDER_PIXELS 7

typedef struct {
	eDisplay_t type;
	uint16_t pixelX;
	uint16_t pixelY;
	char history[HISTORYLISTLEN];
	char brokendown[LISTLEN];
	uint16_t maxLinePixel;
	bool leftPressed;
	bool rightPressed;
	bool upPressed;
	bool downPressed;
	uint32_t gfxUpdateCnt;
} guiState_t;

guiState_t g_gui;

uint8_t menu_byte_get(MENUADDR addr) {
	if (addr < MENU_DATASIZE) {
		return menudata[addr];
	}
	return 0;
}

uint8_t menu_action(MENUACTION action) {
	if (action == MENU_ACTION_CLEAR) {
		g_gui.history[0] = '\0';
		g_gui.brokendown[0] = '\0';
		GuiAppendString("");
		return 1;
	}
	return 0;
}

void GuiInit(void) {
	printf("Starting GUI\r\n");
	menu_strings[MENU_TEXT_IR] = g_gui.brokendown;
	g_gui.type = FilesystemReadLcd();
	uint16_t action = 0;
	if (g_gui.type != NONE) {
		LcdBacklightOn();
		LcdEnable(4); //8MHz
		LcdInit(g_gui.type);
	}
	menu_redraw();
	if (g_gui.type == ST7735_128) {
		g_gui.pixelX = 128;
		g_gui.pixelY = 128;
		action = 100;
	} else if (g_gui.type == ST7735_160) {
		g_gui.pixelX = 160;
		g_gui.pixelY = 128;
		action = 101;
	} else if (g_gui.type == ILI9341) {
		g_gui.pixelX = 320;
		g_gui.pixelY = 240;
		action = 102;
	} else {
		printf("No GUI enabled\r\n");
	}
	if (action) {
		g_gui.maxLinePixel = g_gui.pixelX - BORDER_PIXELS;
		menu_screen_size(g_gui.pixelX, g_gui.pixelY);
		menu_keypress(action);
	}
}

void GuiFormatLinebreaks(char * textOut, size_t outMax, const char * textIn, size_t maxLinePixel) {
	size_t pixelsInLine = 0;
	size_t charsUsed = 0;
	size_t textLen = strlen(textIn);
	for (size_t i = 0; i < textLen; i++) {
		char c = textIn[i];
		if ((c != '\n') && (c != '\r') && (charsUsed < (outMax - 1))) {
			textOut[charsUsed] = c;
			charsUsed++;
			//we draw offscreen, to get the width of the character
			uint8_t w = menu_char_draw(500, 0, 4, c, 1);
			//parts of utf-8 chars return 255 if they are incomplete
			if ((w != 255) && (w)) {
				pixelsInLine += w + 1;
			}
		}
		if (c == '\n') {
			if (charsUsed < (outMax - 1)) {
				textOut[charsUsed] = '\n';
				charsUsed++;
				pixelsInLine = 0;
			}
		}
		/*The word wrap is just a heuristic, it tries to wrap at word ending, but
		  on long words, the wrap will be in the middle of the word.*/
		if ((charsUsed < (outMax - 1)) &&
		    (((pixelsInLine >= maxLinePixel - 8)) ||
		     ((pixelsInLine >= maxLinePixel - 32) && (c == ' ')))) {
			textOut[charsUsed] = '\n';
			charsUsed++;
			pixelsInLine = 0;
		}
	}
	textOut[charsUsed] = '\0';
}

void GuiAppendString(const char * string) {
	size_t al = strlen(string);
	size_t bl = strlen(g_gui.history);
	if ((bl + al) >= HISTORYLISTLEN)
	{
		size_t leftshift = bl + al + 1 - HISTORYLISTLEN;
		memmove(g_gui.history, g_gui.history + leftshift, bl + 1);
		bl -= leftshift;
	}
	memcpy(g_gui.history + bl, string, al + 1);
	//now reformat with linebreaks
	GuiFormatLinebreaks(g_gui.brokendown, LISTLEN, g_gui.history, g_gui.maxLinePixel);
	size_t cl = strlen(g_gui.brokendown);
	size_t lines = 0;
	if (cl > 0) {
		for (size_t i = 0; i < (cl - 1); i++) {
			if (g_gui.brokendown[i] == '\n') {
				lines++;
			}
		}
	}
	menu_listindexstate[MENU_LISTINDEX_list1] = lines;
	//redraw the screen
	menu_redraw();
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
}

