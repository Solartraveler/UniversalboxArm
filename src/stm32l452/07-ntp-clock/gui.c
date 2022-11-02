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
#include "boxlib/keys.h"
#include "imageDrawerLowres.h"

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
	uint32_t cycle;
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

void GuiCycle(char key) {
	bool state = KeyLeftPressed();
	if (((g_gui.leftPressed == false) && (state)) || (key == 'a')) {
		if (g_gui.type != NONE) {
			menu_keypress(2);
		}
	}
	g_gui.leftPressed = state;

	state = KeyRightPressed();
	if (((g_gui.rightPressed == false) && (state)) || (key == 'd')) {
		if (g_gui.type != NONE) {
			menu_keypress(1);
		}
	}
	g_gui.rightPressed = state;

	state = KeyUpPressed();
	if (((g_gui.upPressed == false) && (state)) || (key == 'w')) {
		if (g_gui.type != NONE) {
			menu_keypress(3);
		}
	}
	g_gui.upPressed = state;

	state = KeyDownPressed();
	if (((g_gui.downPressed == false) && (state)) || (key == 's')) {
		if (g_gui.type != NONE) {
			menu_keypress(4);
		}
	}
	g_gui.downPressed = state;

	g_gui.cycle++;
	uint32_t subcycle = g_gui.cycle / 100;
	if (g_gui.cycle % 100 == 0) { //run every 100ms
		g_gui.cycle = 0;
	}
}

