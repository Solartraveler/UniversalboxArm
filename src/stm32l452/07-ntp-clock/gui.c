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

#include "clockConfig.h"

#define TEXT_LEN_MAX 48

//with compression, two bytes can store 256 equal pixel
#define PREVIEW_FG_BUFFER_LEN ((MENU_GFX_SIZEX_COLORFGPREVIEW * MENU_GFX_SIZEY_COLORFGPREVIEW + 255)/ 256 * 2)
#define BACKGROUND_BUFFER_LEN ((MENU_GFX_SIZEX_BACKGROUND2 * MENU_GFX_SIZEY_BACKGROUND2 + 255)/ 256 * 2)

//This assert assumes the GFX elements are configured to use custom1 color.
_Static_assert((MENU_COLOR_CUSTOM1_RED_BITS + MENU_COLOR_CUSTOM1_GREEN_BITS + MENU_COLOR_CUSTOM1_BLUE_BITS) == 8, "Check buffer size above!");

#define REDSHIFT_IN   (MENU_COLOR_CUSTOM1_BLUE_BITS + MENU_COLOR_CUSTOM1_GREEN_BITS)
#define GREENSHIFT_IN (MENU_COLOR_CUSTOM1_BLUE_BITS)
#define BLUESHIFT_IN  (0)

#define REDMASK_IN   ((1<< MENU_COLOR_CUSTOM1_RED_BITS) - 1)
#define GREENMASK_IN ((1<< MENU_COLOR_CUSTOM1_GREEN_BITS) - 1)
#define BLUEMASK_IN  ((1<< MENU_COLOR_CUSTOM1_BLUE_BITS) - 1)





typedef struct {
	eDisplay_t type;
	uint16_t pixelX;
	uint16_t pixelY;
	char textbuffers[MENU_TEXT_MAX][TEXT_LEN_MAX];
	uint8_t previewBufferFg[PREVIEW_FG_BUFFER_LEN];
	uint8_t backgroundBuffer[BACKGROUND_BUFFER_LEN];
	uint8_t fgColor;
	uint8_t bgColor;
	bool leftPressed;
	bool rightPressed;
	bool upPressed;
	bool downPressed;
	uint32_t timestampUtc;
	uint32_t editPos;
	bool needRet;
} guiState_t;

guiState_t g_gui;

const char * g_days[7] = {"Mo", "Tu", "We", "Th", "Fr", "Sa", "Su"};

uint8_t menu_byte_get(MENUADDR addr) {
	if (addr < MENU_DATASIZE) {
		return menudata[addr];
	}
	return 0;
}


void GuiDrawTextMarker(char * text, size_t textMax, size_t pos) {
	if (pos < (textMax - 1)) {
		if (pos > 0) {
			memset(text, ' ', pos);
		}
		text[pos] = '^';
		text[pos + 1] = '\0';
	} else {
		text[0] = '\0';
	}
}

size_t GuiEdit(char * text, size_t pos, int16_t direction, char cmin, char cmax, bool allowDelete) {
	size_t l = strlen(text);
	if (pos < l) {
		char c = text[pos];
		c += direction;
		if (c > cmax) { //roll over
			c = cmin;
		} else if (c < cmin) {
			if ((allowDelete) && (l > 1) && (pos == (l - 1))) { //delete last char
				text[pos] = '\0';
				return pos - 1;
			} else {
				c = cmax; //roll under
			}
		}
		text[pos] = c;
	} else if (pos == 0) { //allow at least 1 char
		text[0] = '0';
		text[1] = '\0';
	}
	return pos;
}

size_t GuiEditChar(char * text, size_t pos, int16_t direction) {
	return GuiEdit(text, pos, direction, ' ', '~', true);
}

size_t GuiEditNext(char * text, size_t textMax, size_t pos) {
	size_t l = strlen(text);
	if ((pos + 2) < textMax) {
		pos++;
		if (pos >= l) { //add a char
			text[pos] = '0';
			text[pos + 1] = '\0';
			if (pos > l) {
				text[pos - 1] = '0';
			}
		}
	}
	return pos;
}

void GuiColorText(char * text, size_t textMax, uint8_t color) {
	unsigned int b = (color >> BLUESHIFT_IN) & BLUEMASK_IN;
	unsigned int g = (color >> GREENSHIFT_IN) & GREENMASK_IN;
	unsigned int r = (color >> REDSHIFT_IN) & REDMASK_IN;
	femtoSnprintf(text, textMax, "R:%u G:%u B:%u", r, g, b);
}

void GuiSetPreviewBuffer(uint8_t * buffer, size_t len, uint8_t color) {
	for (size_t i = 0; i < (len / 2); i++) {
		buffer[i * 2] = color;
		buffer[i * 2 + 1] = 0xFF; //repeats = 256
	}
}

uint8_t GuiEditColor(uint32_t textIndex, size_t pos, uint8_t color, uint8_t * previewBuffer, size_t previewBufferLen, int16_t direction) {
	//update color
	uint8_t b = (color >> BLUESHIFT_IN) & BLUEMASK_IN;
	uint8_t g = (color >> GREENSHIFT_IN) & GREENMASK_IN;
	uint8_t r = (color >> REDSHIFT_IN) & REDMASK_IN;
	if (pos == 0) {
		r = (r + direction) & REDMASK_IN;
	}
	if (pos == 4) {
		g = (g + direction) & GREENMASK_IN;
	}
	if (pos == 8) {
		b = (b + direction) & BLUEMASK_IN;
	}
	color = (b << BLUESHIFT_IN) | (g << GREENSHIFT_IN) | (r << REDSHIFT_IN);
	GuiColorText(menu_strings[textIndex], TEXT_LEN_MAX, color);
	GuiSetPreviewBuffer(previewBuffer, previewBufferLen, color);
	return color;
}

uint8_t menu_action(MENUACTION action) {
	//wifi password
	if (action == MENU_ACTION_PWINC) {
		g_gui.editPos = GuiEditChar(menu_strings[MENU_TEXT_WIFIPW], g_gui.editPos, 1);
		GuiDrawTextMarker(menu_strings[MENU_TEXT_WIFIPWPOS], TEXT_LEN_MAX, g_gui.editPos);
		return 1;
	}
	if (action == MENU_ACTION_PWDEC) {
		g_gui.editPos = GuiEditChar(menu_strings[MENU_TEXT_WIFIPW], g_gui.editPos, -1);
		GuiDrawTextMarker(menu_strings[MENU_TEXT_WIFIPWPOS], TEXT_LEN_MAX, g_gui.editPos);
		return 1;
	}
	if (action == MENU_ACTION_PWNEXT) {
		g_gui.editPos = GuiEditNext(menu_strings[MENU_TEXT_WIFIPW], TEXT_LEN_MAX, g_gui.editPos);
		GuiDrawTextMarker(menu_strings[MENU_TEXT_WIFIPWPOS], TEXT_LEN_MAX, g_gui.editPos);
		return 1;
	}
	if (action == MENU_ACTION_PWPREV) {
		if (g_gui.editPos > 0) {
			g_gui.editPos--;
			GuiDrawTextMarker(menu_strings[MENU_TEXT_WIFIPWPOS], TEXT_LEN_MAX, g_gui.editPos);
			return 1;
		} else {
			PasswordSet(menu_strings[MENU_TEXT_WIFIPW]);
			g_gui.needRet = true;
			return 0;
		}
	}
	//wifi name
	if (action == MENU_ACTION_NAMEINC) {
		g_gui.editPos = GuiEditChar(menu_strings[MENU_TEXT_WIFINAME], g_gui.editPos, 1);
		GuiDrawTextMarker(menu_strings[MENU_TEXT_WIFINAMEPOS], TEXT_LEN_MAX, g_gui.editPos);
		return 1;
	}
	if (action == MENU_ACTION_NAMEDEC) {
		g_gui.editPos = GuiEditChar(menu_strings[MENU_TEXT_WIFINAME], g_gui.editPos, -1);
		GuiDrawTextMarker(menu_strings[MENU_TEXT_WIFINAMEPOS], TEXT_LEN_MAX, g_gui.editPos);
		return 1;
	}
	if (action == MENU_ACTION_NAMENEXT) {
		g_gui.editPos = GuiEditNext(menu_strings[MENU_TEXT_WIFINAME], TEXT_LEN_MAX, g_gui.editPos);
		GuiDrawTextMarker(menu_strings[MENU_TEXT_WIFINAMEPOS], TEXT_LEN_MAX, g_gui.editPos);
		return 1;
	}
	if (action == MENU_ACTION_NAMEPREV) {
		if (g_gui.editPos > 0) {
			g_gui.editPos--;
			GuiDrawTextMarker(menu_strings[MENU_TEXT_WIFINAMEPOS], TEXT_LEN_MAX, g_gui.editPos);
			return 1;
		} else {
			ApSet(menu_strings[MENU_TEXT_WIFINAME]);
			g_gui.needRet = true;
			return 0;
		}
	}
	//ntp name
	if (action == MENU_ACTION_NTPINC) {
		g_gui.editPos = GuiEditChar(menu_strings[MENU_TEXT_NTPNAME], g_gui.editPos, 1);
		GuiDrawTextMarker(menu_strings[MENU_TEXT_NTPNAMEPOS], TEXT_LEN_MAX, g_gui.editPos);
		return 1;
	}
	if (action == MENU_ACTION_NTPDEC) {
		g_gui.editPos = GuiEditChar(menu_strings[MENU_TEXT_WIFINAME], g_gui.editPos, -1);
		GuiDrawTextMarker(menu_strings[MENU_TEXT_NTPNAMEPOS], TEXT_LEN_MAX, g_gui.editPos);
		return 1;
	}
	if (action == MENU_ACTION_NTPNEXT) {
		g_gui.editPos = GuiEditNext(menu_strings[MENU_TEXT_WIFINAME], TEXT_LEN_MAX, g_gui.editPos);
		GuiDrawTextMarker(menu_strings[MENU_TEXT_NTPNAMEPOS], TEXT_LEN_MAX, g_gui.editPos);
		return 1;
	}
	if (action == MENU_ACTION_NTPPREV) {
		if (g_gui.editPos > 0) {
			g_gui.editPos--;
			GuiDrawTextMarker(menu_strings[MENU_TEXT_NTPNAMEPOS], TEXT_LEN_MAX, g_gui.editPos);
			return 1;
		} else {
			TimeserverSet(menu_strings[MENU_TEXT_WIFINAME]);
			g_gui.needRet = true;
			return 0;
		}
	}

	//FG color
	if (action == MENU_ACTION_COLORFGINC) {
		g_gui.fgColor = GuiEditColor(MENU_TEXT_COLORFG, g_gui.editPos, g_gui.fgColor, g_gui.previewBufferFg, PREVIEW_FG_BUFFER_LEN, 1);
		return 1;
	}
	if (action == MENU_ACTION_COLORFGDEC) {
		g_gui.fgColor = GuiEditColor(MENU_TEXT_COLORFG, g_gui.editPos, g_gui.fgColor, g_gui.previewBufferFg, PREVIEW_FG_BUFFER_LEN, -1);
		return 1;
	}
	if (action == MENU_ACTION_COLORFGNEXT) {
		if (g_gui.editPos < 8) {
			g_gui.editPos += 4;
			GuiDrawTextMarker(menu_strings[MENU_TEXT_COLORFGPOS], TEXT_LEN_MAX, g_gui.editPos);
			return 1;
		}
	}
	if (action == MENU_ACTION_COLORFGPREV) {
		if (g_gui.editPos >= 4) {
			g_gui.editPos -= 4;
			GuiDrawTextMarker(menu_strings[MENU_TEXT_COLORFGPOS], TEXT_LEN_MAX, g_gui.editPos);
			return 1;
		} else {
			g_gui.needRet = true;
			return 0;
		}
	}
	//BG color
	if (action == MENU_ACTION_COLORBGINC) {
		g_gui.bgColor = GuiEditColor(MENU_TEXT_COLORBG, g_gui.editPos, g_gui.bgColor, g_gui.backgroundBuffer, BACKGROUND_BUFFER_LEN, 1);
		return 1;
	}
	if (action == MENU_ACTION_COLORBGDEC) {
		g_gui.bgColor = GuiEditColor(MENU_TEXT_COLORBG, g_gui.editPos, g_gui.bgColor, g_gui.backgroundBuffer, BACKGROUND_BUFFER_LEN, -1);
		return 1;
	}
	if (action == MENU_ACTION_COLORBGNEXT) {
		if (g_gui.editPos < 8) {
			g_gui.editPos += 4;
			GuiDrawTextMarker(menu_strings[MENU_TEXT_COLORBGPOS], TEXT_LEN_MAX, g_gui.editPos);
			return 1;
		}
	}
	if (action == MENU_ACTION_COLORBGPREV) {
		if (g_gui.editPos >= 4) {
			g_gui.editPos -= 4;
			GuiDrawTextMarker(menu_strings[MENU_TEXT_COLORBGPOS], TEXT_LEN_MAX, g_gui.editPos);
			return 1;
		} else {
			g_gui.needRet = true;
			return 0;
		}
	}
	if (action == MENU_ACTION_SAVE) {
		ConfigSaveWifi();
		ConfigSaveClock();
	}
	return 0;
}

static void GuiReloadConfig(void) {
	ApGet(menu_strings[MENU_TEXT_WIFINAME], TEXT_LEN_MAX);
	PasswordGet(menu_strings[MENU_TEXT_WIFIPW], TEXT_LEN_MAX);
	TimeserverGet(menu_strings[MENU_TEXT_NTPNAME], TEXT_LEN_MAX);
}

void GuiInit(void) {
	printf("Starting GUI\r\n");
	for (uint32_t i = 0; i < MENU_TEXT_MAX; i++) {
		menu_strings[i] = g_gui.textbuffers[i];
	}
	GuiReloadConfig();
	//update wifi name
	menu_strings[MENU_TEXT_WIFINAMEPOS][0] = '^';
	//update wifi password
	menu_strings[MENU_TEXT_WIFIPWPOS][0] = '^';
	//update server name
	menu_strings[MENU_TEXT_NTPNAMEPOS][0] = '^';
	//update fg color
	g_gui.fgColor = ColorFgGet();
	menu_gfxdata[MENU_GFX_COLORFGPREVIEW] = g_gui.previewBufferFg;
	GuiSetPreviewBuffer(g_gui.previewBufferFg, PREVIEW_FG_BUFFER_LEN, g_gui.fgColor);
	GuiColorText(menu_strings[MENU_TEXT_COLORFG], TEXT_LEN_MAX, g_gui.fgColor);
	menu_strings[MENU_TEXT_COLORFGPOS][0] = '^';
	//update bg color
	g_gui.bgColor = ColorBgGet();
	menu_gfxdata[MENU_GFX_COLORBGPREVIEW] = g_gui.backgroundBuffer;
	menu_gfxdata[MENU_GFX_BACKGROUND1] = g_gui.backgroundBuffer;
	menu_gfxdata[MENU_GFX_BACKGROUND2] = g_gui.backgroundBuffer;
	GuiSetPreviewBuffer(g_gui.backgroundBuffer, BACKGROUND_BUFFER_LEN, g_gui.bgColor);
	GuiColorText(menu_strings[MENU_TEXT_COLORBG], TEXT_LEN_MAX, g_gui.bgColor);
	menu_strings[MENU_TEXT_COLORBGPOS][0] = '^';
	//prepare first draw
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

	if (key == 'u') {
		GuiReloadConfig();
	}

	GuiUpdateClock();

	if (g_gui.needRet) {
		g_gui.needRet = false;
		menu_keypress(50);
	}
}

