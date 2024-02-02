#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <alloca.h>

#include "gui.h"

#include "boxlib/lcd.h"
#include "boxlib/lcdBacklight.h"
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
#include "screenshot.h"

#include "femtoVsnprintf.h"

#include "menudata.c"

#include "ntp-clock.h"

#include "clockConfig.h"

/* All the digits as bitmaps.
The headers are generated on the fly by make from .ppm files,
so do not try to find them before calling make.
*/

#include "0.h"
#include "1.h"
#include "2.h"
#include "3.h"
#include "4.h"
#include "5.h"
#include "6.h"
#include "7.h"
#include "8.h"
#include "9.h"
#include "colon.h"
#include "0Xxl.h"
#include "1Xxl.h"
#include "2Xxl.h"
#include "3Xxl.h"
#include "4Xxl.h"
#include "5Xxl.h"
#include "6Xxl.h"
#include "7Xxl.h"
#include "8Xxl.h"
#include "9Xxl.h"
#include "colonXxl.h"



/*must be larger or equal than any value in the g_bitmapsLen array, which is
 currently 230 for the nomal digits and 558 for the xxl digits.
*/
#define DIGITS_LEN_MAX 560

#define TEXT_LEN_MAX 48

#define BACKLIGHT_MAX 1000

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
	uint8_t hourHBuffer[DIGITS_LEN_MAX];
	uint8_t hourLBuffer[DIGITS_LEN_MAX];
	uint8_t minuteHBuffer[DIGITS_LEN_MAX];
	uint8_t minuteLBuffer[DIGITS_LEN_MAX];
	uint8_t colonBuffer[DIGITS_LEN_MAX];
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

#define BITMAPS_NUM 22

const unsigned char * g_bitmaps[BITMAPS_NUM] = {
build_0_8bit, build_1_8bit, build_2_8bit, build_3_8bit,
build_4_8bit, build_5_8bit, build_6_8bit, build_7_8bit,
build_8_8bit, build_9_8bit, build_colon_8bit,
build_0Xxl_8bit, build_1Xxl_8bit, build_2Xxl_8bit, build_3Xxl_8bit,
build_4Xxl_8bit, build_5Xxl_8bit, build_6Xxl_8bit, build_7Xxl_8bit,
build_8Xxl_8bit, build_9Xxl_8bit, build_colonXxl_8bit,

};

const unsigned int g_bitmapsLen[BITMAPS_NUM] = {
build_0_8bit_len, build_1_8bit_len, build_2_8bit_len, build_3_8bit_len,
build_4_8bit_len, build_5_8bit_len, build_6_8bit_len, build_7_8bit_len,
build_8_8bit_len, build_9_8bit_len, build_colon_8bit_len,
build_0Xxl_8bit_len, build_1Xxl_8bit_len, build_2Xxl_8bit_len, build_3Xxl_8bit_len,
build_4Xxl_8bit_len, build_5Xxl_8bit_len, build_6Xxl_8bit_len, build_7Xxl_8bit_len,
build_8Xxl_8bit_len, build_9Xxl_8bit_len, build_colonXxl_8bit_len,

};


uint8_t menu_byte_get(MENUADDR addr) {
	if (addr < MENU_DATASIZE) {
		return menudata[addr];
	}
	return 0;
}

static void GuiCopyColorizeBitmap(uint8_t * target, size_t targetLen, const uint8_t * source, size_t sourceLen, uint8_t bgColor, uint8_t fgColor) {
	if (targetLen < sourceLen) {
		printf("Error, bitmap too large\r\n");
		return;
	}
	for (size_t i = 0; i < sourceLen; i += 2) {
		if (source[i] == 0) {
			target[i] = bgColor;
		} else {
			target[i] = fgColor;
		}
		target[i + 1] = source[i + 1];
	}
}


static void GuiLargeDigitsUpdate(uint8_t hour, uint8_t minute) {
	uint8_t hourH = hour / 10;
	uint8_t hourL = hour % 10;
	uint8_t minuteH = minute / 10;
	uint8_t minuteL = minute % 10;
	uint8_t bgColor = g_gui.bgColor;
	uint8_t fgColor = g_gui.fgColor;
	uint8_t offset = 0;
	if (g_gui.type == ILI9341) {
		offset = 11; //the xxl digits are later in the array
	}
	GuiCopyColorizeBitmap(g_gui.hourHBuffer, DIGITS_LEN_MAX, g_bitmaps[hourH + offset], g_bitmapsLen[hourH + offset], bgColor, fgColor);
	GuiCopyColorizeBitmap(g_gui.hourLBuffer, DIGITS_LEN_MAX, g_bitmaps[hourL + offset], g_bitmapsLen[hourL + offset], bgColor, fgColor);
	GuiCopyColorizeBitmap(g_gui.minuteHBuffer, DIGITS_LEN_MAX, g_bitmaps[minuteH + offset], g_bitmapsLen[minuteH + offset], bgColor, fgColor);
	GuiCopyColorizeBitmap(g_gui.minuteLBuffer, DIGITS_LEN_MAX, g_bitmaps[minuteL + offset], g_bitmapsLen[minuteL + offset], bgColor, fgColor);
	//The colon is the entry after the ten digits in the array
	GuiCopyColorizeBitmap(g_gui.colonBuffer, DIGITS_LEN_MAX, g_bitmaps[10 + offset], g_bitmapsLen[10 + offset], bgColor, fgColor);
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

static void UpdateInterval(void) {
	uint16_t interval = TimeserverRefreshGet();
	femtoSnprintf(menu_strings[MENU_TEXT_NTPINTERVAL], TEXT_LEN_MAX, "%uh", interval);
}

static void UpdateTz(void) {
	int16_t offset = UtcOffsetGet();
	char pref = '+';
	if (offset < 0) {
		pref = '-';
		offset = -offset;
	}
	uint16_t h = offset / 60;
	uint16_t m = offset % 60;
	femtoSnprintf(menu_strings[MENU_TEXT_TZOFFSET], TEXT_LEN_MAX, "%c%u:%02u", pref, h, m);
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
	//ntp interval
	if (action == MENU_ACTION_NTPINTERVALINC) {
		uint16_t interval = TimeserverRefreshGet();
		if (interval < 222) { //1h less than the maximum supported by DerivationPPB()
			interval++;
		}
		TimeserverRefreshSet(interval);
		UpdateInterval();
		return 1;
	}
	if (action == MENU_ACTION_NTPINTERVALDEC) {
		uint16_t interval = TimeserverRefreshGet();
		if (interval > 8) { //minimum supported by DerivationPPB()
			interval--;
		}
		TimeserverRefreshSet(interval);
		UpdateInterval();
		return 1;
	}
	//timezone delta
	if (action == MENU_ACTION_TZDELTAINC) {
		int16_t delta = UtcOffsetGet();
		if (delta < (60*14)) {
			delta += 30;
		}
		UtcOffsetSet(delta);
		UpdateTz();
		return 1;
	}
	if (action == MENU_ACTION_TZDELTADEC) {
		int16_t delta = UtcOffsetGet();
		if (delta > (-60*14)) {
			delta -= 30;
		}
		UtcOffsetSet(delta);
		UpdateTz();
		return 1;
	}


	if (action == MENU_ACTION_SAVE) {
		ConfigSaveWifi();
		ConfigSaveClock();
	}
	if (action == MENU_ACTION_LEDFLASHING) {
		bool flashing = menu_checkboxstate[MENU_CHECKBOX_LEDFLASHING] ? true : false;
		LedFlashSet(flashing);
	}
	if (action == MENU_ACTION_BACKLIGHT) {
		const uint32_t blMax = BACKLIGHT_MAX;
		uint32_t bl;
		uint8_t blIndex = menu_radiobuttonstate[MENU_RBUTTON_BACKLIGHT];
		switch(blIndex) {
			case 1: bl = blMax * 50 / 100; break;
			case 2: bl = blMax * 25 / 100; break;
			case 3: bl = blMax * 12 / 100; break;
			case 4: bl = blMax * 6 / 100; break;
			case 5: bl = blMax * 3 / 100; break;
			case 6: bl = blMax * 2 / 100; break;
			case 7: bl = blMax * 1 / 100; break;
			default: bl = blMax;
		}
		BacklightSet(bl);
		LcdBacklightSet(bl);
	}
	if (action == MENU_ACTION_SYNC) {
		SyncNow();
	}
	if (action == MENU_ACTION_SUMMERTIME) {
		bool state = menu_checkboxstate[MENU_CHECKBOX_SUMMERTIME] ? true : false;
		SummertimeSet(state);
	}
	return 0;
}

static void GuiReloadConfig(void) {
	ApGet(menu_strings[MENU_TEXT_WIFINAME], TEXT_LEN_MAX);
	PasswordGet(menu_strings[MENU_TEXT_WIFIPW], TEXT_LEN_MAX);
	TimeserverGet(menu_strings[MENU_TEXT_NTPNAME], TEXT_LEN_MAX);
	UpdateInterval();
	UpdateTz();
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
	//prepare large digits
	menu_gfxdata[MENU_GFX_DIGITHOURH] = g_gui.hourHBuffer;
	menu_gfxdata[MENU_GFX_DIGITHOURH2] = g_gui.hourHBuffer;
	menu_gfxdata[MENU_GFX_DIGITHOURL] = g_gui.hourLBuffer;
	menu_gfxdata[MENU_GFX_DIGITHOURL2] = g_gui.hourLBuffer;
	menu_gfxdata[MENU_GFX_DIGITMINUTEH] = g_gui.minuteHBuffer;
	menu_gfxdata[MENU_GFX_DIGITMINUTEH2] = g_gui.minuteHBuffer;
	menu_gfxdata[MENU_GFX_DIGITMINUTEL] = g_gui.minuteLBuffer;
	menu_gfxdata[MENU_GFX_DIGITMINUTEL2] = g_gui.minuteLBuffer;
	menu_gfxdata[MENU_GFX_DIGITDOT] = g_gui.colonBuffer;
	menu_gfxdata[MENU_GFX_DIGITDOT2] = g_gui.colonBuffer;
	/*Fill all arrays with white dummy data, so should a digit not fit into the
	  the buffer, and therefore is not drawn, a white image is drawn instead
	*/
	memset(g_gui.hourHBuffer, 0xFF, DIGITS_LEN_MAX);
	memset(g_gui.hourLBuffer, 0xFF, DIGITS_LEN_MAX);
	memset(g_gui.minuteHBuffer, 0xFF, DIGITS_LEN_MAX);
	memset(g_gui.minuteLBuffer, 0xFF, DIGITS_LEN_MAX);
	memset(g_gui.colonBuffer, 0xFF, DIGITS_LEN_MAX);
	//update led flashing
	if (LedFlashGet()) {
		menu_checkboxstate[MENU_CHECKBOX_LEDFLASHING] = 1;
	}
	if (SummertimeGet()) {
		menu_checkboxstate[MENU_CHECKBOX_SUMMERTIME] = 1;
	}
	//update backlight strength
	const uint32_t blMax = BACKLIGHT_MAX;
	uint32_t bl = BacklightGet();
	uint8_t blIndex = 0;
	if (bl <= (blMax * 1 / 100)) {
		blIndex = 7;
	} else if (bl <= (blMax * 2 / 100)) {
		blIndex = 6;
	} else if (bl <= (blMax * 3 / 100)) {
		blIndex = 5;
	} else if (bl <= (blMax * 6 / 100)) {
		blIndex = 4;
	} else if (bl <= (blMax * 12 / 100)) {
		blIndex = 3;
	} else if (bl <= (blMax * 25 / 100)) {
		blIndex = 2;
	} else if (bl <= (blMax * 50 / 100)) {
		blIndex = 1;
	}
	menu_radiobuttonstate[MENU_RBUTTON_BACKLIGHT] = blIndex;
	//prepare first draw
	g_gui.timestampUtc = 0xFFFFFFFF;
	g_gui.type = FilesystemReadLcd();
	uint16_t action = 0;
	if (g_gui.type != NONE) {
		LcdBacklightOn();
		LcdEnable(2); //8MHz
		LcdInit(g_gui.type);
		LcdBacklightInit();
		LcdBacklightSet(bl);
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
	GuiLargeDigitsUpdate(hour, minute);
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
	if (key == 'x') {
		Screenshot();
	}

	GuiUpdateClock();

	if (g_gui.needRet) {
		g_gui.needRet = false;
		menu_keypress(50);
	}
}

