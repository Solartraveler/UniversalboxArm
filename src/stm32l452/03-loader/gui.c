#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "gui.h"

#include "ff.h"
#include "boxlib/lcd.h"
#include "boxlib/keys.h"
#include "tarextract.h"
#include "json.h"
#include "menu-interpreter.h"
#include "menu-text.h"
#include "loader.h"
#include "framebufferLowmem.h"
#include "utility.h"
#include "filesystem.h"

#include "menudata.c"

#define FILELISTLEN 1024

#define FILENAMEMAX 64

#define BINTEXT 32

#define FILETEXT 12

#define READMETEXT 1024

//A compressed 320x240 image with 3bits per pixel needs at least 2328bytes
#define DRAWINGSIZE 7000

#define FRONTLEVEL_BG_DARK 100
#define FRONTLEVEL_BG_BRIGHT 257

void GuiLoadBinary(void);

typedef struct {
	eDisplay_t type;
	char fileList[FILELISTLEN];
	char binName[BINTEXT];
	char binVersion[BINTEXT];
	char binAuthor[BINTEXT];
	char binLicense[BINTEXT];
	char binDate[BINTEXT];
	char readme[READMETEXT];
	uint8_t drawing[DRAWINGSIZE];
	char fsSize[FILETEXT];
	char fsFree[FILETEXT];
	char fsSectors[FILETEXT];
	char fsClustersize[FILETEXT];
	uint16_t maxLinePixel;
	uint16_t pixelX;
	uint16_t pixelY;

	bool leftPressed;
	bool rightPressed;
	bool upPressed;
	bool downPressed;
} guiState_t;

guiState_t g_gui;

uint8_t menu_byte_get(MENUADDR addr) {
	if (addr < MENU_DATASIZE) {
		return menudata[addr];
	}
	return 0;
}

uint8_t menu_action(MENUACTION action) {
	//printf("Action %u called\r\n", action);
	if (action == MENU_ACTION_BINLOAD) {
		GuiLoadBinary();
	}
	if (action == MENU_ACTION_BINSAVEDELETE) {
		LoaderTarSaveDelete();
		return 1;
	}
	if (action == MENU_ACTION_BINEXEC) {
		LoaderProgramStart();
	}
	if (action == MENU_ACTION_BINWATCHDOG) {
		LoaderWatchdogEnforce(menu_checkboxstate[MENU_CHECKBOX_BINWATCHDOG] ? true : false);
		return 1;
	}
	if (action == MENU_ACTION_BINAUTOSTART) {
		LoaderAutostartSet(menu_checkboxstate[MENU_CHECKBOX_BINAUTOSTART] ? true : false);
		return 1;
	}
	if (action == MENU_ACTION_STORAGEFORMAT) {
		LoaderFormat();
		return 1;
	}
	if (action == MENU_ACTION_IMAGEENTER) {
		//Lets guess if the image has a black or white background
		uint8_t firstColor = g_gui.drawing[0] & 0x7;
		if (firstColor == 0) {
			//background black = background color, drawing = front color
			menu_screen_frontlevel(FRONTLEVEL_BG_DARK);
		} else {
			//drawing = background color, background white = front color
			menu_screen_frontlevel(FRONTLEVEL_BG_BRIGHT);
		}
		LoaderGfxUpdate();
		return 1;
	}
	if (action == MENU_ACTION_IMAGELEAVE) {
		menu_screen_frontlevel(FRONTLEVEL_BG_DARK); //a single color should be a front color
	}
	return 0;
}

#define BORDER_PIXELS 7

void GuiInit(void) {
	printf("Starting GUI\r\n");
	menu_strings[MENU_TEXT_BINLIST] = g_gui.fileList;
	menu_strings[MENU_TEXT_BINNAME] = g_gui.binName;
	menu_strings[MENU_TEXT_BINVERSION] = g_gui.binVersion;
	menu_strings[MENU_TEXT_BINAUTHOR] = g_gui.binAuthor;
	menu_strings[MENU_TEXT_BINLICENSE] = g_gui.binLicense;
	menu_strings[MENU_TEXT_BINDATE] = g_gui.binDate;
	menu_strings[MENU_TEXT_BININFO] = g_gui.readme;
	menu_strings[MENU_TEXT_DISKSIZE] = g_gui.fsSize;
	menu_strings[MENU_TEXT_DISKFREE] = g_gui.fsFree;
	menu_strings[MENU_TEXT_DISKSECTORS] = g_gui.fsSectors;
	menu_strings[MENU_TEXT_DISKCLUSTER] = g_gui.fsClustersize;
	menu_gfxdata[MENU_GFX_BINDRAWING] = g_gui.drawing;
	//we must provide data which prevent a buffer overflow
	memset(g_gui.drawing, 0xFF, DRAWINGSIZE);
	g_gui.type = FilesystemReadLcd();
	if (g_gui.type != NONE) {
		LcdBacklightOn();
		//HAL_Delay(1000);
		LcdEnable(4); //8MHz
		LcdInit(g_gui.type);
	}
	menu_screen_frontlevel(FRONTLEVEL_BG_DARK);
	menu_redraw();
	GuiUpdateFilelist();
	uint16_t action = 0;
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

void GuiLcdSet(eDisplay_t type) {
	if (type == ST7735_128) {
		FilesystemWriteLcd("ST7735_128x128");
	} else if (type == ST7735_160) {
		FilesystemWriteLcd("ST7735_160x128");
	} else if (type == ILI9341) {
		FilesystemWriteLcd("ILI9341_320x240");
	} else {
		FilesystemWriteLcd("NONE");
	}
}

void GuiLoadBinary(void) {
	uint32_t index = menu_listindexstate[MENU_LISTINDEX_BININDEX];
	const char * textStart = g_gui.fileList;
	const char * listStart = g_gui.fileList;
	while ((*listStart) && (index)) {
		if (*listStart == '\n') {
			index--;
			textStart = listStart + 1;
		}
		listStart++;
	}
	size_t len = 0;
	while ((textStart[len]) && (textStart[len] != '\n')) {
		len++;
	}
	char filename[FILENAMEMAX] = "/bin/";
	size_t l = strlen(filename);
	if ((len + l) < FILENAMEMAX) {
		strncpy(filename + l, textStart, len);
		printf("Selected name: %s\r\n", filename);
		LoaderTarLoad(filename);
	}
}

void GuiTransferStart(void) {
	menu_keypress(103); //show subwindow
}

void GuiTransferDone(void) {
	menu_keypress(50); //close subwindow
}

void GuiJumpBinScreen(void) {
	menu_keypress(104); //go to app window
}

void GuiUpdateFilelist(void) {
	DIR d;
	FILINFO fi;
	size_t offset = 0;
	g_gui.fileList[0] = '\0';
	if (f_opendir(&d, "/bin") == FR_OK) {
		while (f_readdir(&d, &fi) == FR_OK) {
			if ((fi.fname[0]) && (FILELISTLEN > offset)) {
				snprintf(g_gui.fileList + offset, FILELISTLEN - offset, "%s%s", offset ? "\n" : "", fi.fname);
				if (offset) {
					offset++;
				}
				offset += strlen(fi.fname);
			} else {
				break;
			}
		}
		f_closedir(&d);
	}
	menu_listindexstate[MENU_LISTINDEX_BININDEX] = 0;
}

void GuiShowBinData(const char * name, const char * version, const char * author,
     const char * license, const char * date, bool watchdog,
     bool autostart, bool saved) {
	strncpy(g_gui.binName, name, BINTEXT - 1);
	strncpy(g_gui.binVersion, version, BINTEXT - 1);
	strncpy(g_gui.binAuthor, author, BINTEXT - 1);
	strncpy(g_gui.binLicense, license, BINTEXT - 1);
	strncpy(g_gui.binDate, date, BINTEXT - 1);
	menu_checkboxstate[MENU_CHECKBOX_BINWATCHDOG] = watchdog ? 1 : 0;
	menu_checkboxstate[MENU_CHECKBOX_BINAUTOSTART] = autostart ? 1 : 0;
	menu_strings[MENU_TEXT_SAVEDELETE] = saved ? "Delete" : "Save";
}

void GuiShowInfoData(const char * text, size_t textLen) {
	size_t maxLinePixel = g_gui.maxLinePixel;
	size_t pixelsInLine = 0;
	size_t charsUsed = 0;
	bool wasNewline = false;
	for (size_t i = 0; i < textLen; i++) {
		char c = text[i];
		if ((isprint(c)) && (charsUsed < (READMETEXT - 1))) {
			g_gui.readme[charsUsed] = c;
			charsUsed++;
			//we draw offscreen, to get the width of the character
			uint8_t w = menu_char_draw(500, 0, 4, c, 1);
			if (w) { //parts of utf-8 chars return 0 and do not consume width
				pixelsInLine += w + 1;
			}
		}
		if (c == '\n') {
			if ((wasNewline) && (charsUsed < (READMETEXT - 1))) {
				g_gui.readme[charsUsed] = '\n';
				charsUsed++;
				wasNewline = false;
				pixelsInLine = 0;
			} else {
				wasNewline = true;
			}
		} else {
			wasNewline = false;
		}
		/*The word wrap is just a heuristic, it tries to wrap at word ending, but
		  on long words, the wrap will be in the middle of the word.*/
		if ((charsUsed < (READMETEXT - 1)) &&
		    (((pixelsInLine >= maxLinePixel - 8)) ||
		     ((pixelsInLine >= maxLinePixel - 32) && (c == ' ')))) {
			g_gui.readme[charsUsed] = '\n';
			charsUsed++;
			wasNewline = false;
			pixelsInLine = 0;
		}
	}
	g_gui.readme[charsUsed] = '\0';
	menu_listindexstate[MENU_LISTINDEX_BININFO] = 0;
}

//x and y must be even numbers, otherwise border calculation would be wrong.
void GuiShowGfxData(const uint8_t * data, size_t imageLen, uint16_t x, uint16_t y) {
	memset(g_gui.drawing, 0xFF, DRAWINGSIZE); //empty white image
	//when the input format is smaller than our display, we need to add borders
	uint32_t wptr = 0;
	uint32_t requiredPixels = g_gui.pixelX * g_gui.pixelY;
	uint32_t generatedPixels = 0;
	if ((x <= g_gui.pixelX) && (y <= g_gui.pixelY)) {
		uint16_t borderX = (g_gui.pixelX - x) / 2;
		uint16_t borderY = (g_gui.pixelY - y) / 2;
		uint8_t firstColor = data[0] & 0x7;
		//fill top border
		uint32_t areaPixels = g_gui.pixelX * borderY;
		while ((areaPixels) && (wptr < DRAWINGSIZE)) {
			uint8_t thisPixels = MIN(areaPixels, 32);
			g_gui.drawing[wptr] = firstColor | ((thisPixels - 1) << 3);
			wptr++;
			areaPixels -= thisPixels;
			generatedPixels += thisPixels;
		}
		//re-compress the graphic data
		areaPixels = g_gui.pixelX * y;
		uint32_t inputRepeatLeft = 0;
		uint32_t inputIndex = 0;
		uint8_t inputColor = 0;
		uint16_t wx = 0;
		uint8_t colorLast = 0xFF;
		uint32_t repeatLast = 0;
		while ((areaPixels) && (wptr < DRAWINGSIZE)) {
			//printf("Pixels: %u, wptr: %u, input %u, x %u\n", generatedPixels, wptr, inputIndex, wx);
			//renew input
			if ((inputRepeatLeft == 0) && (inputIndex < imageLen)) {
				inputColor = data[inputIndex] & 0x7;
				inputRepeatLeft = (data[inputIndex] >> 3) + 1;
				inputIndex++;
			}
			//calculate
			uint8_t color = 0;
			uint16_t colorRepeat = 0;
			if (wx < borderX) { //append left border
				color = firstColor;
				colorRepeat = borderX;
			} else if (wx >= (borderX + x)) { //append right border
				color = firstColor;
				colorRepeat = borderX;
			} else if (inputRepeatLeft) { //append input data
				color = inputColor;
				colorRepeat = MIN(inputRepeatLeft, x + borderX - wx);
				inputRepeatLeft -= colorRepeat;
			} else { //fallback, should the input provide too little data
				color = firstColor;
				colorRepeat = 1;
			}
			wx += colorRepeat;
			if (wx == g_gui.pixelX) {
				wx = 0;
			}
			//put to output
			if ((color != colorLast) || (repeatLast == areaPixels)) {
 				while ((repeatLast) && (wptr < DRAWINGSIZE)) {
					uint8_t repeat = MIN(32, repeatLast);
					g_gui.drawing[wptr] = colorLast | ((repeat - 1) << 3);
					repeatLast -= repeat;
					wptr++;
					generatedPixels += repeat;
					areaPixels -= repeat;
				}
			}
			colorLast = color;
			repeatLast += colorRepeat;
		}
		//fill bottom border
		areaPixels = g_gui.pixelX * borderY;
		while ((areaPixels) && (wptr < DRAWINGSIZE)) {
			uint8_t thisPixels = MIN(areaPixels, 32);
			g_gui.drawing[wptr] = firstColor | ((thisPixels - 1) << 3);
			wptr++;
			areaPixels -= thisPixels;
			generatedPixels += thisPixels;
		}
		//if we need more memory than we have reserved, we need to corrupt the
		//graphic until we reached the required pixel number
		if (generatedPixels < requiredPixels) {
			printf("Warning, need to cut GFX. Missing %u pixels\n", (unsigned int)(requiredPixels - generatedPixels));
		}
		wptr = DRAWINGSIZE - 1;
		while ((generatedPixels < requiredPixels) && (wptr)) {
			uint8_t used = (g_gui.drawing[wptr] >> 3) + 1;
			g_gui.drawing[wptr] = firstColor | ((32 - 1) << 3);
			generatedPixels += 32 - used; //update delta
			wptr--;
		}
	}
}

void GuiShowFsData(uint32_t totalBytes, uint32_t freeBytes, uint32_t sectors, uint32_t clustersize) {
	snprintf(g_gui.fsSize, FILETEXT, "%uKiB", (unsigned int)(totalBytes / 1024));
	snprintf(g_gui.fsFree, FILETEXT, "%uKiB", (unsigned int)(freeBytes / 1024));
	snprintf(g_gui.fsSectors, FILETEXT, "%u", (unsigned int)sectors);
	snprintf(g_gui.fsClustersize, FILETEXT, "%uByte", (unsigned int)clustersize);
}

/*
Normally, the menu can be controlled with the 4 hardware keys and w-a-s-d over
the serial port.
If no display is selected, up will start 320x240, right 160x128 and down 128x128
*/
void GuiCycle(char key) {
	bool guiEnabled = false;

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
		} else {
			GuiLcdSet(ST7735_160);
			guiEnabled = true;
		}
	}
	g_gui.rightPressed = state;

	state = KeyUpPressed();
	if (((g_gui.upPressed == false) && (state)) || (key == 'w')) {
		if (g_gui.type != NONE) {
			menu_keypress(3);
		} else {
			GuiLcdSet(ILI9341);
			guiEnabled = true;
		}
	}
	g_gui.upPressed = state;

	state = KeyDownPressed();
	if (((g_gui.downPressed == false) && (state)) || (key == 's')) {
		if (g_gui.type != NONE) {
			menu_keypress(4);
		} else {
			GuiLcdSet(ST7735_128);
			guiEnabled = true;
		}
	}
	g_gui.downPressed = state;

	if (guiEnabled) {
		GuiInit(); //safe to call a 2. time
	}
}

