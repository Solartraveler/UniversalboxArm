#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <alloca.h>

#include "gui.h"

#include "boxlib/lcd.h"
#include "boxlib/leds.h"
#include "tarextract.h"
#include "menu-interpreter.h"
#include "menu-text.h"
#include "framebufferLowmem.h"
#include "utility.h"
#include "filesystem.h"
#include "boxlib/coproc.h"
#include "boxlib/keysIsr.h"
#include "imageDrawerLowres.h"
#include "clockMt.h"
#include "dateTime.h"

#include "femtoVsnprintf.h"

#include "menudata.c"

#include "ntp-clock.h"

#define TEXT_LEN_MAX 32

typedef struct {
	eDisplay_t type;
	uint16_t pixelX;
	uint16_t pixelY;
	char textbuffers[MENU_TEXT_MAX][TEXT_LEN_MAX];
	bool leftPressed;
	bool rightPressed;
	bool upPressed;
	bool downPressed;
	uint32_t timestampUtc;
} guiState_t;

guiState_t g_gui;

const char * g_days[7] = {"Mo", "Tu", "We", "Th", "Fr", "Sa", "Su"};

uint8_t menu_byte_get(MENUADDR addr) {
	if (addr < MENU_DATASIZE) {
		return menudata[addr];
	}
	return 0;
}

uint8_t menu_action(MENUACTION action) {

	return 0;
}

void GuiInit(void) {
	printf("Starting GUI\r\n");
	for (uint32_t i = 0; i < MENU_TEXT_MAX; i++) {
		menu_strings[i] = g_gui.textbuffers[i];
	}
	//menu_gfxdata[MENU_GFX_BATGRAPH] = g_gui.gfx;
	//memset(g_gui.gfx, 0xFE, GFX_MEMORY * sizeof(uint8_t));
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
		menu_screen_size(g_gui.pixelX, g_gui.pixelY);
		menu_keypress(action);
	}
}

void GuiUpdateClock(void) {
	uint32_t timestampUtc = ClockUtcGetMt(NULL);
	if (g_gui.timestampUtc == timestampUtc) {
		return;
	}
	g_gui.timestampUtc = timestampUtc;
	//time
	uint32_t timestampLocal = UtcToLocalTime(timestampUtc);
	uint16_t year, doy;
	uint8_t month, day, hour, minute, second;
	TimestampDecode(timestampLocal, &year, &month, &day, &doy, &hour, &minute, &second);
	uint8_t weekday = WeekdayFromDoy(doy, year);
	femtoSnprintf(menu_strings[MENU_TEXT_TIME], TEXT_LEN_MAX, "%2u:%02u:%02u", hour, minute, second);
	const char * weekdayStr = g_days[weekday];
	femtoSnprintf(menu_strings[MENU_TEXT_DATE], TEXT_LEN_MAX, "%s %2u.%02u.%u", weekdayStr, day + 1, month + 1, year);
	//last sync
	uint32_t lastSync;
	uint8_t mode = ClockSetTimestampGetMt(&lastSync, NULL);
	if (mode == 0) {
		femtoSnprintf(menu_strings[MENU_TEXT_SYNC], TEXT_LEN_MAX, "never set");
	} else {
		uint32_t lastSyncLocal = UtcToLocalTime(lastSync);
		const char * source = "man";
		if (mode == 2) {
			source = "NTP";
		}
		TimestampDecode(lastSyncLocal, &year, &month, &day, NULL, &hour, &minute, &second);
		femtoSnprintf(menu_strings[MENU_TEXT_SYNC], TEXT_LEN_MAX, "%s, %2u.%02u, %2u:%02u:%u", source, day + 1, month + 1, hour, minute, second);
	}
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

	GuiUpdateClock();
}

