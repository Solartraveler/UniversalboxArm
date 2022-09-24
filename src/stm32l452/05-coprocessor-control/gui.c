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

#include "control.h"

#define TEXT_LEN_MAX 32

//store data for 4 hours
#define GRAPH_DATAPOINTS 120

#define GFX_MEMORY 4096

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
	uint32_t gfxUpdateCnt;
	uint16_t voltageHistory[GRAPH_DATAPOINTS];
	uint16_t currentHistory[GRAPH_DATAPOINTS];
	uint16_t historyUsed;
	uint8_t gfx[GFX_MEMORY];
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
	if (action == MENU_ACTION_BATMAX) {
		uint8_t selection = menu_radiobuttonstate[MENU_RBUTTON_BATMAX];
		uint16_t ma = 0;
		if (selection == 1) ma =  40;
		if (selection == 2) ma =  70;
		if (selection == 3) ma = 100;
		if (selection == 4) ma = 150;
		CoprocBatteryCurrentMax(ma);
	}
	if (action == MENU_ACTION_CONFIRMSPI) {
		uint8_t selection = menu_radiobuttonstate[MENU_RBUTTON_CONFIRMSPI];
		CoprocWriteLed(selection);
	}
	return 0;
}


void GuiInit(void) {
	printf("Starting GUI\r\n");
	for (uint32_t i = 0; i < MENU_TEXT_MAX; i++) {
		menu_strings[i] = g_gui.textbuffers[i];
	}
	uint16_t charingCurrentMax = CoprocReadBatteryCurrentMax();
	uint8_t selection = 0;
	if (charingCurrentMax >= 40) selection = 1;
	if (charingCurrentMax >= 70) selection = 2;
	if (charingCurrentMax >= 100) selection = 3;
	if (charingCurrentMax >= 150) selection = 4;
	menu_radiobuttonstate[MENU_RBUTTON_BATMAX] = selection;
	menu_radiobuttonstate[MENU_RBUTTON_CONFIRMSPI] = 1; //default is confirm
	menu_gfxdata[MENU_GFX_BATGRAPH] = g_gui.gfx;
	memset(g_gui.gfx, 0xFE, GFX_MEMORY * sizeof(uint8_t));
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

void GuiUpdateGraph(void) {
	uint16_t batVcc = CoprocReadBatteryVoltage();
	uint16_t batI = CoprocReadBatteryCurrent();
	memmove(&g_gui.voltageHistory[0], &g_gui.voltageHistory[1], sizeof(uint16_t) * (GRAPH_DATAPOINTS - 1));
	memmove(&g_gui.currentHistory[0], &g_gui.currentHistory[1], sizeof(uint16_t) * (GRAPH_DATAPOINTS - 1));
	g_gui.voltageHistory[GRAPH_DATAPOINTS - 1] = batVcc;
	g_gui.currentHistory[GRAPH_DATAPOINTS - 1] = batI;
	g_gui.historyUsed = MIN(g_gui.historyUsed + 1, GRAPH_DATAPOINTS);
	const uint16_t * data[2];
	data[0] = g_gui.voltageHistory + GRAPH_DATAPOINTS - g_gui.historyUsed;
	data[1] = g_gui.currentHistory + GRAPH_DATAPOINTS - g_gui.historyUsed;
	uint8_t colors[2] = {0x1, 0x4};
	const size_t bufferLen = sizeof(Img1BytePixelLowres_t) * 2 * MENU_GFX_SIZEX_BATGRAPH * 30;
	void * buffer = alloca(bufferLen);
	ImgCreateLinesGfxLowres(data, g_gui.historyUsed, colors, 2, MENU_GFX_SIZEX_BATGRAPH, MENU_GFX_SIZEY_BATGRAPH,
	                                        3, true, g_gui.gfx, GFX_MEMORY, NULL, buffer, bufferLen);
	uint16_t minVoltage = 0xFFFF;
	uint16_t maxVoltage = 0x0;
	uint16_t minCurrent = 0xFFFF;
	uint16_t maxCurrent = 0x0;
	for (uint16_t i = 0; i < g_gui.historyUsed; i++) {
		uint16_t index = GRAPH_DATAPOINTS - g_gui.historyUsed + i;
		minVoltage = MIN(minVoltage, g_gui.voltageHistory[index]);
		maxVoltage = MAX(maxVoltage, g_gui.voltageHistory[index]);
		minCurrent = MIN(minCurrent, g_gui.currentHistory[index]);
		maxCurrent = MAX(maxCurrent, g_gui.currentHistory[index]);
	}
	femtoSnprintf(menu_strings[MENU_TEXT_BATVOLTAGEMINMAX], TEXT_LEN_MAX, "%umV - %umV", minVoltage, maxVoltage);
	femtoSnprintf(menu_strings[MENU_TEXT_BATCURRENTMINMAX], TEXT_LEN_MAX, "%umA - %umA", minCurrent, maxCurrent);
	femtoSnprintf(menu_strings[MENU_TEXT_GRAPHTIME], TEXT_LEN_MAX, "showing %umin", g_gui.historyUsed * 2);
}

static void GuiUpdateBat(uint32_t subcycle) {
	if (subcycle == 1) {
		char text[16];
		uint16_t batTemperature = CoprocReadBatteryTemperature();
		TemperatureToString(text, sizeof(text), batTemperature);
		femtoSnprintf(menu_strings[MENU_TEXT_BATTEMPERATURE], TEXT_LEN_MAX, "%s", text);
	}

	if (subcycle == 2) {
		uint16_t batVcc = CoprocReadBatteryVoltage();
		femtoSnprintf(menu_strings[MENU_TEXT_BATVOLTAGE], TEXT_LEN_MAX, "%umV", batVcc);
	}

	if (subcycle == 3) {
		uint16_t batI = CoprocReadBatteryCurrent();
		femtoSnprintf(menu_strings[MENU_TEXT_BATCURRENT], TEXT_LEN_MAX, "%umA", batI);
	}

	if (subcycle == 4) {
		uint8_t state = CoprocReadChargerState();
		if (state < STATES_MAX) {
			femtoSnprintf(menu_strings[MENU_TEXT_BATMODE], TEXT_LEN_MAX, "%u - %s", state, g_chargerState[state]);
		} else {
			femtoSnprintf(menu_strings[MENU_TEXT_BATMODE], TEXT_LEN_MAX, "%u - unknown", state);
		}
	}

	if (subcycle == 5) {
		uint8_t error = CoprocReadChargerError();
		if (error < ERRORS_MAX) {
			femtoSnprintf(menu_strings[MENU_TEXT_BATERROR], TEXT_LEN_MAX, "%u - %s", error, g_chargerError[error]);
		} else {
			femtoSnprintf(menu_strings[MENU_TEXT_BATERROR], TEXT_LEN_MAX, "%u - unknown", error);
		}
	}

	if (subcycle == 6) {
		uint16_t batCap = CoprocReadChargerAmount();
		femtoSnprintf(menu_strings[MENU_TEXT_BATCHARGED], TEXT_LEN_MAX, "%umAh", batCap);
	}

	if (subcycle == 7) {
		uint16_t batTotal = CoprocReadChargedTotal();
		femtoSnprintf(menu_strings[MENU_TEXT_BATTOTAL], TEXT_LEN_MAX, "%uAh", batTotal);
	}

	if (subcycle == 8) {
		uint16_t batChargecycles = CoprocReadChargedCycles();
		femtoSnprintf(menu_strings[MENU_TEXT_BATCYCLES], TEXT_LEN_MAX, "%u", batChargecycles);
	}

	if (subcycle == 9) {
		uint16_t batPrechargecycles = CoprocReadPrechargedCycles();
		femtoSnprintf(menu_strings[MENU_TEXT_BATPRECYCLES], TEXT_LEN_MAX, "%u", batPrechargecycles);
	}

	if (subcycle == 10) {
		uint16_t batPwm = CoprocReadChargerPwm();
		femtoSnprintf(menu_strings[MENU_TEXT_BATPWM], TEXT_LEN_MAX, "%u", batPwm);
	}

	if (subcycle == 11) {
		uint16_t batTime = CoprocReadBatteryChargeTime();
		uint16_t min = batTime / 60;
		uint16_t sec = batTime % 60;
		femtoSnprintf(menu_strings[MENU_TEXT_BATTIME], TEXT_LEN_MAX, "%u:%02u", min, sec);
	}
}

static void GuiUpdateMain(uint32_t subcycle) {
	if (subcycle == 1) {
		uint16_t version = CoprocReadVersion();
		femtoSnprintf(menu_strings[MENU_TEXT_FIRMWARE], TEXT_LEN_MAX, "%u, version %u", version >> 8, version & 0xFF);
	}

	if (subcycle == 2) {
		uint16_t vcc = CoprocReadVcc();
		femtoSnprintf(menu_strings[MENU_TEXT_VCC], TEXT_LEN_MAX, "%umV", vcc);
	}

	if (subcycle == 3) {
		char text[16];
		int16_t cpuTemperature = CoprocReadCpuTemperature();
		TemperatureToString(text, sizeof(text), cpuTemperature);
		femtoSnprintf(menu_strings[MENU_TEXT_CPUTEMP], TEXT_LEN_MAX, "%s", text);
	}

	if (subcycle == 4) {
		uint16_t uptime = CoprocReadUptime();
		femtoSnprintf(menu_strings[MENU_TEXT_UPTIME], TEXT_LEN_MAX, "%uhours", uptime);
	}

	if (subcycle == 5) {
		uint16_t optime = CoprocReadOptime();
		femtoSnprintf(menu_strings[MENU_TEXT_OPTIME], TEXT_LEN_MAX, "%udays", optime);
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
		if (g_gui.gfxUpdateCnt == 0) { //run every 2min
			GuiUpdateGraph();
			g_gui.gfxUpdateCnt = 1200;
		}
		g_gui.gfxUpdateCnt--;
		if (g_gui.batview) {
			GuiUpdateBat(subcycle);
		} else {
			GuiUpdateMain(subcycle);
		}
		if (g_gui.cycle == 2000) {
			Led1Green();
			/* This takes a long time @ 16MHz, Os optimization:
			  Mainscreen @ 320x240: 116ms menu processig + 98ms frame flush = 214ms total
			  Batteryscreen @ 320x240: 375ms menu processing + 243ms frame flush = 618ms total
			  Processing times without menu_screen_set: Mainscreen: 55ms, Batteryscreen: 157ms
				With Og optimization:
			  Mainscreen @ 320x240: 187ms total
			  Battery screen @ 320x240: 555ms total
				With O2 optimization:
			  Mainscreen @ 320x240: 175ms total
			  Battery screen @ 320x240: 510ms total
			  With O3 optimization:
			  Mainscreen @ 320x240: 168ms total
			  Battery screen @ 320x240: 494ms total
			*/
			//uint32_t timeStart = HAL_GetTick();
			menu_redraw();
			//uint32_t timeStop = HAL_GetTick();
			//printf("menu_redraw took %uticks\r\n", (unsigned int)(timeStop - timeStart));
			Led1Off();
			g_gui.cycle = 0;
		}
	}
}

