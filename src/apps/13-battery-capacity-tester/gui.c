/* ADC scope
(c) 2024 by Malte Marwedel

SPDX-License-Identifier: GPL-3.0-or-later


*/

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "gui.h"

#include "menudata.c"

#include "boxlib/flash.h"
#include "boxlib/keys.h"
#include "boxlib/keysIsr.h"
#include "boxlib/lcd.h"
#include "capTester.h"
#include "femtoVsnprintf.h"
#include "filesystem.h"
#include "framebufferBwFast.h"
#include "json.h"
#include "menu-interpreter.h"
#include "menu-text.h"
#include "utility.h"

#define LONGTEXT 64

//In [mV]
#define VOLTAGE_MIN 750
#define VOLTAGE_MAX 16000

//In [mΩ]
#define RLOAD_MIN 500
#define RLOAD_MAX 20000


#define CAPTESTER_FILENAME "/etc/captester.json"

typedef struct {
	eDisplay_t type;
	uint16_t pixelX;
	uint16_t pixelY;
	bool leftPressed;
	bool rightPressed;
	bool upPressed;
	bool downPressed;
	uint32_t gfxUpdateCnt;
	char settings1[LONGTEXT];
	char settings2[LONGTEXT];
	char state1[LONGTEXT];
	char state2[LONGTEXT];
	uint32_t switchOff1Mv; //mV
	uint32_t switchOff2Mv; //mV
	uint32_t r1Mohm; //mOhm
	uint32_t r2Mohm; //mOhm
} guiState_t;

guiState_t g_gui;

uint8_t menu_byte_get(MENUADDR addr) {
	if (addr < MENU_DATASIZE) {
		return menudata[addr];
	}
	return 0;
}

static void GuiUpdateSelection(char * text, size_t bLen, uint32_t uOff, uint32_t r) {
	unsigned int v = uOff / 1000;
	unsigned int mv = uOff % 1000 / 10;
	unsigned int ohm = r / 1000;
	unsigned int mohm = r % 1000 / 100;
	//Assuming the shut down voltage is at 75% of the nominal voltage
	unsigned int vmax = uOff * 4 / 3;
	unsigned int imax = vmax * 1000 / r; //results in [mA]
	unsigned int pmax = imax * vmax / 1000; //results in [mW]
	snprintf(text, bLen, "lim=%u.%02uV, R=%u.%01uΩ, est Pmax=~%umW", v, mv, ohm, mohm, pmax);
}

static void GuiUpdateState(char * text, size_t bLen, bool enabled, uint32_t t, uint32_t u, uint32_t e, uint32_t r) {
	unsigned int h = t / (60 * 60);
	unsigned int m = (t / 60) % 60;
	unsigned int s = t % 60;
	unsigned int v = u / 1000;
	unsigned int mv = u % 1000 / 10;
	unsigned int ex = e / 1000;
	unsigned int exm = e % 1000 / 100;
	unsigned int im = 0;
	unsigned int pm = 0;
	if (enabled) {
		im = u * 1000UL / r;
		pm = u * u / r;
	}
	snprintf(text, bLen, "T=%u:%02u:%02u U=%u.%02uV\nI=%04umA P=%04umW\nE=%u.%02uWh", h, m, s, v, mv, im, pm, ex, exm);
}

static void GuiUpdateDynamic(void) {
	GuiUpdateSelection(menu_strings[MENU_TEXT_TEST1SETTINGS], LONGTEXT, g_gui.switchOff1Mv, g_gui.r1Mohm);
	GuiUpdateSelection(menu_strings[MENU_TEXT_TEST2SETTINGS], LONGTEXT, g_gui.switchOff2Mv, g_gui.r2Mohm);
	uint32_t t1, u1, e1, t2, u2, e2;
	bool en1 = CapDataGet(0, &t1, &u1, &e1);
	bool en2 = CapDataGet(1, &t2, &u2, &e2);
	GuiUpdateState(menu_strings[MENU_TEXT_TEST1STATE], LONGTEXT, en1, t1, u1, e1, g_gui.r1Mohm);
	GuiUpdateState(menu_strings[MENU_TEXT_TEST2STATE], LONGTEXT, en2, t2, u2, e2, g_gui.r2Mohm);
	if ((en1) || (en2)) {
		menu_strings[MENU_TEXT_ABORT] = "Abort";
	} else {
		menu_strings[MENU_TEXT_ABORT] = "Results";
	}
}

static void GuiLoadSettings(void) {
	uint8_t jsonFileBuffer[512];
	char value[10];
	size_t fileLen = 0;
	if (FilesystemReadFile(CAPTESTER_FILENAME, jsonFileBuffer, sizeof(jsonFileBuffer), &fileLen) == false) {
		return;
	}
	if (JsonValueGet(jsonFileBuffer, fileLen, "Enabled1", value, sizeof(value))) {
		if (strcmp(value, "true") == 0) {
			menu_checkboxstate[MENU_CHECKBOX_TEST1] = 1;
		}
		if (strcmp(value, "false") == 0) {
			menu_checkboxstate[MENU_CHECKBOX_TEST1] = 0;
		}
	}
	if (JsonValueGet(jsonFileBuffer, fileLen, "Enabled2", value, sizeof(value))) {
		if (strcmp(value, "true") == 0) {
			menu_checkboxstate[MENU_CHECKBOX_TEST2] = 1;
		}
		if (strcmp(value, "false") == 0) {
			menu_checkboxstate[MENU_CHECKBOX_TEST2] = 0;
		}
	}
	if (JsonValueGet(jsonFileBuffer, fileLen, "Uoff1", value, sizeof(value))) {
		uint32_t uoff = atoi(value);
		uoff -= uoff % 50;
		if ((uoff <= VOLTAGE_MAX) && (uoff >= VOLTAGE_MIN)) {
			g_gui.switchOff1Mv = uoff;
		}
	}
	if (JsonValueGet(jsonFileBuffer, fileLen, "Uoff2", value, sizeof(value))) {
		uint32_t uoff = atoi(value);
		uoff -= uoff % 50;
		if ((uoff <= VOLTAGE_MAX) && (uoff >= VOLTAGE_MIN)) {
			g_gui.switchOff2Mv = uoff;
		}
	}
	if (JsonValueGet(jsonFileBuffer, fileLen, "Rload1", value, sizeof(value))) {
		uint32_t rload = atoi(value);
		rload -= rload % 100;
		if ((rload <= RLOAD_MAX) && (rload >= RLOAD_MIN)) {
			g_gui.r1Mohm = rload;
		}
	}
	if (JsonValueGet(jsonFileBuffer, fileLen, "Rload2", value, sizeof(value))) {
		uint32_t rload = atoi(value);
		rload -= rload % 100;
		if ((rload <= RLOAD_MAX) && (rload >= RLOAD_MIN)) {
			g_gui.r2Mohm = rload;
		}
	}

}

static bool GuiSaveSettings(bool enabled1, bool enabled2) {
	char buffer[256];
	snprintf(buffer, sizeof(buffer), "{\n\
  \"Enabled1\": \"%s\",\n  \"Uoff1\": \"%u\",\n  \"Rload1\": \"%u\",\n\
  \"Enabled2\": \"%s\",\n  \"Uoff2\": \"%u\",\n  \"Rload2\": \"%u\"\n}\n",
	enabled1 ? "true" : "false", (unsigned int)g_gui.switchOff1Mv, (unsigned int)g_gui.r1Mohm,
	enabled2 ? "true" : "false", (unsigned int)g_gui.switchOff2Mv, (unsigned int)g_gui.r2Mohm);
	if (FilesystemWriteEtcFile(CAPTESTER_FILENAME, buffer, strlen(buffer))) {
		printf("Saved to %s\r\n", CAPTESTER_FILENAME);
		return true;
	} else {
		printf("Error, could not create file %s\r\n", CAPTESTER_FILENAME);
		return false;
	}
}

static void GuiDefaultSettings(void) {
	g_gui.switchOff1Mv = 900;
	g_gui.switchOff2Mv = 900;
	g_gui.r1Mohm = 2300;
	g_gui.r2Mohm = 2300;
	menu_checkboxstate[MENU_CHECKBOX_TEST1] = 1;
	menu_checkboxstate[MENU_CHECKBOX_TEST2] = 0;
}

uint8_t menu_action(MENUACTION action) {
	if (action == MENU_ACTION_TEST1UINC) {
		if (g_gui.switchOff1Mv < VOLTAGE_MAX) {
			g_gui.switchOff1Mv += 50;
		}
		GuiUpdateDynamic();
		return 1;
	}
	if (action == MENU_ACTION_TEST1UDEC) {
		if (g_gui.switchOff1Mv > VOLTAGE_MIN) {
			g_gui.switchOff1Mv -= 50;
		}
		GuiUpdateDynamic();
		return 1;
	}
	if (action == MENU_ACTION_TEST1RINC) {
		if (g_gui.r1Mohm < RLOAD_MAX) {
			g_gui.r1Mohm += 100;
		}
		GuiUpdateDynamic();
		return 1;
	}
	if (action == MENU_ACTION_TEST1RDEC) {
		if (g_gui.r1Mohm > RLOAD_MIN) {
			g_gui.r1Mohm -= 100;
		}
		GuiUpdateDynamic();
		return 1;
	}
	if (action == MENU_ACTION_TEST2UINC) {
		if (g_gui.switchOff2Mv < VOLTAGE_MAX) {
			g_gui.switchOff2Mv += 50;
		}
		GuiUpdateDynamic();
		return 1;
	}
	if (action == MENU_ACTION_TEST2UDEC) {
		if (g_gui.switchOff2Mv > VOLTAGE_MIN) {
			g_gui.switchOff2Mv -= 50;
		}
		GuiUpdateDynamic();
		return 1;
	}
	if (action == MENU_ACTION_TEST2RINC) {
		if (g_gui.r2Mohm < RLOAD_MAX) {
			g_gui.r2Mohm += 100;
		}
		GuiUpdateDynamic();
		return 1;
	}
	if (action == MENU_ACTION_TEST2RDEC) {
		if (g_gui.r2Mohm > RLOAD_MIN) {
			g_gui.r2Mohm -= 100;
		}
		GuiUpdateDynamic();
		return 1;
	}
	if (action == MENU_ACTION_START) {
		bool enabled1 = menu_checkboxstate[MENU_CHECKBOX_TEST1] ? true : false;
		bool enabled2 = menu_checkboxstate[MENU_CHECKBOX_TEST2] ? true : false;
		CapStart(0, enabled1, g_gui.r1Mohm, g_gui.switchOff1Mv);
		CapStart(1, enabled2, g_gui.r2Mohm, g_gui.switchOff2Mv);
		GuiSaveSettings(enabled1, enabled2);
	}
	if (action == MENU_ACTION_ABORT) {
		CapStop();
	}
	if (action == MENU_ACTION_SAVERESULTS) {
		CapSave();
	}
	return 0;
}

void GuiInit(void) {
	printf("Starting GUI\r\n");
	menu_strings[MENU_TEXT_TEST1SETTINGS] = g_gui.settings1;
	menu_strings[MENU_TEXT_TEST2SETTINGS] = g_gui.settings2;
	menu_strings[MENU_TEXT_TEST1STATE] = g_gui.state1;
	menu_strings[MENU_TEXT_TEST2STATE] = g_gui.state2;
	menu_strings[MENU_TEXT_ABORT] = "Abort";
	if (FlashReady()) {
		g_gui.type = FilesystemReadLcd();
	}
	if (g_gui.type == NONE) {
		printf("No LCD type configured, assuming ILI9341 LCD!\r\n");
		g_gui.type = ILI9341;
	}
	LcdBacklightOn();
	LcdEnable(2); //8MHz
	LcdInit(g_gui.type);
	if (g_gui.type == ILI9341) {
		g_gui.pixelX = 320;
		g_gui.pixelY = 240;
	} else if (g_gui.type == ST7735_160) {
		g_gui.pixelX = 160;
		g_gui.pixelY = 128;
	} else if (g_gui.type == ST7735_128) {
		g_gui.pixelX = 128;
		g_gui.pixelY = 128;
	} else {
		printf("Configured LCD type not supported\r\n");
	}
	GuiDefaultSettings();
	GuiLoadSettings();
	GuiUpdateDynamic();
	menu_redraw();
}

void GuiCycle(char key) {
	bool state = KeyLeftReleased();
	static uint32_t cycle = 0;
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

	cycle++;
	if (cycle == 500) { //update GFX every 500ms
		cycle = 0;
		GuiUpdateDynamic();
		menu_redraw();
	}
}

