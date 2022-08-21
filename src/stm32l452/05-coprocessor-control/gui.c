#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "gui.h"

#include "boxlib/lcd.h"
#include "tarextract.h"
#include "menu-interpreter.h"
#include "menu-text.h"
#include "framebufferLowmem.h"
#include "utility.h"
#include "filesystem.h"
#include "boxlib/coproc.h"
#include "boxlib/keys.h"

#include "femtoVsnprintf.h"

#include "menudata.c"

#include "control.h"

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
	bool batview;
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
	if (action == MENU_ACTION_BATVIEW) {
		g_gui.batview = true;
	}
	if (action == MENU_ACTION_MAINVIEW) {
		g_gui.batview = false;
	}
	if (action == MENU_ACTION_POWERDOWN) {
		CoprocWritePowerdown();
	}
	if (action == MENU_ACTION_BATFORCECHARGE) {
		CoprocBatteryForceCharge();
	}
	if (action == MENU_ACTION_WAKEUP) {
		uint8_t selection = menu_radiobuttonstate[MENU_RBUTTON_WAKEUP];
		uint16_t time = 0;
		if (selection == 1) time =       10;
		if (selection == 2) time =     1*60;
		if (selection == 3) time =    10*60;
		if (selection == 4) time =  1*60*60;
		if (selection == 5) time = 12*60*60;
		CoprocWriteAlarm(time);
	}
	if (action == MENU_ACTION_MODE) {
		CoprocWritePowermode(menu_radiobuttonstate[MENU_RBUTTON_MODE]);
	}
	if (action == MENU_ACTION_BATNEW) {
		CoprocBatteryNew();
	}
	if (action == MENU_ACTION_BATRESETSTAT) {
		CoprocBatteryStatReset();
	}
	return 0;
}


#define BORDER_PIXELS 7

void GuiInit(void) {
	printf("Starting GUI\r\n");
	for (uint32_t i = 0; i < MENU_TEXT_MAX; i++) {
		menu_strings[i] = g_gui.textbuffers[i];
	}
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

void GuiScreenResolutionGet(uint16_t * x, uint16_t * y) {
	*x = g_gui.pixelX;
	*y = g_gui.pixelY;
}

void GuiUpdateBat(void) {
	char text[16];
	uint16_t batTemperature = CoprocReadBatteryTemperature();
	TemperatureToString(text, sizeof(text), batTemperature);
	femtoSnprintf(menu_strings[MENU_TEXT_BATTEMPERATURE], TEXT_LEN_MAX, "%s", text);

	uint16_t batVcc = CoprocReadBatteryVoltage();
	femtoSnprintf(menu_strings[MENU_TEXT_BATVOLTAGE], TEXT_LEN_MAX, "%umV", batVcc);

	uint16_t batI = CoprocReadBatteryCurrent();
	femtoSnprintf(menu_strings[MENU_TEXT_BATCURRENT], TEXT_LEN_MAX, "%umA", batI);

	uint8_t state = CoprocReadChargerState();
	if (state < STATES_MAX) {
		femtoSnprintf(menu_strings[MENU_TEXT_BATMODE], TEXT_LEN_MAX, "%u - %s", state, g_chargerState[state]);
	} else {
		femtoSnprintf(menu_strings[MENU_TEXT_BATMODE], TEXT_LEN_MAX, "%u - unknown", state);
	}

	uint8_t error = CoprocReadChargerError();
	if (error < ERRORS_MAX) {
		femtoSnprintf(menu_strings[MENU_TEXT_BATERROR], TEXT_LEN_MAX, "%u - %s", error, g_chargerError[error]);
	} else {
		femtoSnprintf(menu_strings[MENU_TEXT_BATERROR], TEXT_LEN_MAX, "%u - unknown", state);
	}

	uint16_t batCap = CoprocReadChargerAmount();
	femtoSnprintf(menu_strings[MENU_TEXT_BATCHARGED], TEXT_LEN_MAX, "%umAh", batCap);

	uint16_t batTotal = CoprocReadChargedTotal();
	femtoSnprintf(menu_strings[MENU_TEXT_BATTOTAL], TEXT_LEN_MAX, "%uAh", batTotal);

	uint16_t batChargecycles = CoprocReadChargedCycles();
	femtoSnprintf(menu_strings[MENU_TEXT_BATCYCLES], TEXT_LEN_MAX, "%u", batChargecycles);

	uint16_t batPrechargecycles = CoprocReadPrechargedCycles();
	femtoSnprintf(menu_strings[MENU_TEXT_BATPRECYCLES], TEXT_LEN_MAX, "%u", batPrechargecycles);

	uint16_t batPwm = CoprocReadChargerPwm();
	femtoSnprintf(menu_strings[MENU_TEXT_BATPWM], TEXT_LEN_MAX, "%u", batPwm);
}

void GuiUpdateMain(void) {
	uint16_t version = CoprocReadVersion();
	femtoSnprintf(menu_strings[MENU_TEXT_FIRMWARE], TEXT_LEN_MAX, "%u, version %u", version >> 8, version & 0xFF);

	uint16_t vcc = CoprocReadVcc();
	femtoSnprintf(menu_strings[MENU_TEXT_VCC], TEXT_LEN_MAX, "%umV", vcc);

	char text[16];
	int16_t cpuTemperature = CoprocReadCpuTemperature();
	TemperatureToString(text, sizeof(text), cpuTemperature);
	femtoSnprintf(menu_strings[MENU_TEXT_CPUTEMP], TEXT_LEN_MAX, "%s", text);

	uint16_t uptime = CoprocReadUptime();
	femtoSnprintf(menu_strings[MENU_TEXT_UPTIME], TEXT_LEN_MAX, "%uhours", uptime);

	uint16_t optime = CoprocReadOptime();
	femtoSnprintf(menu_strings[MENU_TEXT_OPTIME], TEXT_LEN_MAX, "%udays", optime);
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
	if (g_gui.cycle == 1000) {
		g_gui.cycle = 0;
		if (g_gui.batview) {
			GuiUpdateBat();
		} else {
			GuiUpdateMain();
		}
		menu_redraw();
	}
}

