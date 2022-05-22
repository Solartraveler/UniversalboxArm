#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "gui.h"

#include "ff.h"
#include "boxlib/lcd.h"
#include "tarextract.h"
#include "json.h"
#include "menu-interpreter.h"
#include "menu-text.h"
#include "forwarder.h"
#include "framebufferLowmem.h"
#include "utility.h"

#include "menudata.c"

#define CONFIGFILENAME "/etc/display.json"

#define HISTORYLISTLEN 512
#define LISTLEN (HISTORYLISTLEN * 2)

typedef struct {
	eDisplay_t type;
	char history[HISTORYLISTLEN];
	char brokendown[LISTLEN];
	uint16_t maxLinePixel;
	uint16_t pixelX;
	uint16_t pixelY;
} guiState_t;

guiState_t g_gui;

uint8_t menu_byte_get(MENUADDR addr) {
	if (addr < MENU_DATASIZE) {
		return menudata[addr];
	}
	return 0;
}

uint8_t menu_action(MENUACTION action) {
	return 0;
}

eDisplay_t ConfigReadLcd(void) {
	uint8_t displayconf[64] = {0};
	char lcdtype[32];
	FIL f;
	UINT r = 0;
	if (FR_OK == f_open(&f, CONFIGFILENAME, FA_READ)) {
		FRESULT res = f_read(&f, displayconf, sizeof(displayconf) - 1, &r);
		if (res != FR_OK) {
			printf("Warning, could not read display config file\r\n");
		}
		f_close(&f);
	} else {
		printf("Warning, no display configured\r\n");
		return NONE;
	}
	if (JsonValueGet(displayconf, r, "lcd", lcdtype, sizeof(lcdtype))) {
		if (strcmp(lcdtype, "ST7735_128x128") == 0) {
			printf("LCD 128x128 selected\r\n");
			return ST7735_128;
		}
		if (strcmp(lcdtype, "ST7735_160x128") == 0) {
			printf("LCD 128x160 selected\r\n");
			return ST7735_160;
		}
		if (strcmp(lcdtype, "ILI9341_320x240") == 0) {
			printf("LCD 320x240 selected\r\n");
			return ILI9341;
		}
		if (strcmp(lcdtype, "NONE") == 0) {
			printf("no LCD selected\r\n");
			return NONE;
		}
	}
	printf("Warning, could not get LCD value\r\n");
	return NONE;
}

#define BORDER_PIXELS 7

void GuiInit(void) {
	printf("Starting GUI\r\n");
	menu_strings[MENU_TEXT_UARTRX] = g_gui.brokendown;
	g_gui.type = ConfigReadLcd();
	uint16_t action = 0;
	if (g_gui.type != NONE) {
		LcdBacklightOn();
		LcdEnable(4); //8MHz
		LcdInit(g_gui.type);
	}
	menu_redraw();
	if (g_gui.type == ST7735_128) {
		g_gui.maxLinePixel = 128 - BORDER_PIXELS;
		g_gui.pixelX = 128;
		g_gui.pixelY = 128;
		action = 100;
	} else if (g_gui.type == ST7735_160) {
		g_gui.maxLinePixel = 160 - BORDER_PIXELS;
		g_gui.pixelX = 160;
		g_gui.pixelY = 128;
		action = 101;
	} else if (g_gui.type == ILI9341) {
		g_gui.maxLinePixel = 320 - BORDER_PIXELS;
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

void GuiScreenResolutionGet(uint16_t * x, uint16_t * y) {
	*x = g_gui.pixelX;
	*y = g_gui.pixelY;
}

void GuiFormatLinebreaks(char * textOut, size_t outMax, const char * textIn, uint16_t maxLinePixel) {
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
}

