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

#include "adcSample.h"
#include "boxlib/flash.h"
#include "boxlib/keys.h"
#include "boxlib/keysIsr.h"
#include "boxlib/lcd.h"
#include "femtoVsnprintf.h"
#include "filesystem.h"
#include "framebufferColor.h"
#include "guiPlatform.h"
#include "json.h"
#include "menu-interpreter.h"
#include "menu-text.h"
#include "utility.h"
#include "screenshot.h"

#define SCOPE_FILENAME "/etc/scope.json"

#define COMMONTEXT 12
#define LONGTEXT 128

#define CHANNELS 3

#define OFFSET_MV_MAX 4000
#define OFFSET_MV_MIN -1000

#define TRIGGER_MV_MAX 3500
#define TRIGGER_MV_MIN 0

//the scope gfx should be divided by this value in x and y direction without remainder
//unit: [pix/div]
#define GUI_PIXEL_SQUARE 20

_Static_assert((MENU_GFX_SIZEX_SCOPE % GUI_PIXEL_SQUARE) == 0, "Error, ratio not supported");
_Static_assert((MENU_GFX_SIZEY_SCOPE % GUI_PIXEL_SQUARE) == 0, "Error, ratio not supported");

typedef struct {
	eDisplay_t type;
	uint16_t pixelX;
	uint16_t pixelY;
	bool leftPressed;
	bool rightPressed;
	bool upPressed;
	bool downPressed;
	uint32_t gfxUpdateCnt;
	char textXAxis[COMMONTEXT];
	char textYAxis[COMMONTEXT];
	char textYOff[COMMONTEXT];
	char textTrigger[COMMONTEXT];
	char textVanalyze[LONGTEXT];
	MENU_GFX_FORMAT_SCOPE scope[MENU_GFX_SIZEX_SCOPE * MENU_GFX_SIZEY_SCOPE];
	uint8_t timeScaleIndex; //index for g_scaleTime
	uint8_t voltageScaleIndex; //index for g_scaleVoltage
	int16_t offsetMv; //[mV]
	int16_t triggerLevelMv; //[mV]
	int8_t activeChannels;
} guiState_t;


/*At 8 divs and 3.3Vref, little more than 6 blocks will spawn the full voltage
  rang at 500mV/div.
  The ADC will have a resolution of 0.000805V. Each div is 20pixel -> 16mv/div
  So there is no real use for anything below 10mV/div.
  unit: [mV/div]
*/
const textUnit_t g_scaleVoltage[] = {
{"1V",   1000.0},
{"0.7V", 700.0},
{"0.5V", 500.0}, //<- default value
{"0.3V", 300.0},
{"0.2V", 200.0},
{"0.15V", 150.0},
{"0.1V", 100.0},
{"70mV", 70.0},
{"50mV", 50.0},
{"30mV", 30.0},
{"20mV", 20.0},
{"15mV", 15.0},
{"10mV", 10.0},
};


guiState_t g_gui;

uint8_t menu_byte_get(MENUADDR addr) {
	if (addr < MENU_DATASIZE) {
		return menudata[addr];
	}
	return 0;
}

void GuiUpdateInputs(void) {
	uint8_t indexes[CHANNELS];
	uint8_t adc[CHANNELS];
	indexes[0] = menu_listindexstate[MENU_LISTINDEX_ADCRED];
	indexes[1] = menu_listindexstate[MENU_LISTINDEX_ADCGREEN];
	indexes[2] = menu_listindexstate[MENU_LISTINDEX_ADCBLUE];
	g_gui.activeChannels = 0;
	for (uint8_t i = 0; i < CHANNELS; i++) {
		uint8_t index = indexes[i];
		if (index > 0) {
			adc[g_gui.activeChannels] = index - 1;
			g_gui.activeChannels++;
		}
	}
	SampleInputsSet(adc, g_gui.activeChannels);
}

static bool GuiSaveSettings(void) {
	//lets gather everything worth saving
	uint8_t timeScaleIndex = g_gui.timeScaleIndex;
	uint8_t voltageScaleIndex = g_gui.voltageScaleIndex;
	int16_t offsetMv = g_gui.offsetMv;
	uint8_t redChannelIndex = menu_listindexstate[MENU_LISTINDEX_ADCRED];
	uint8_t greenChannelIndex = menu_listindexstate[MENU_LISTINDEX_ADCGREEN];
	uint8_t blueChannelIndex = menu_listindexstate[MENU_LISTINDEX_ADCBLUE];
	int16_t triggerLevelMv = g_gui.triggerLevelMv;
	uint8_t triggerMode = menu_radiobuttonstate[MENU_RBUTTON_TRIGGERMODE];
	uint8_t triggerType = menu_radiobuttonstate[MENU_RBUTTON_TRIGGERTYPE];
	uint8_t triggerSource = menu_radiobuttonstate[MENU_RBUTTON_TRIGGERSOURCE];
	uint8_t analyzeRed = (menu_checkboxstate[MENU_CHECKBOX_REDMAX] << 2) | (menu_checkboxstate[MENU_CHECKBOX_REDMIN] << 1) | menu_checkboxstate[MENU_CHECKBOX_REDAVG];
	uint8_t analyzeGreen = (menu_checkboxstate[MENU_CHECKBOX_GREENMAX] << 2) | (menu_checkboxstate[MENU_CHECKBOX_GREENMIN] << 1) | menu_checkboxstate[MENU_CHECKBOX_GREENAVG];
	uint8_t analyzeBlue = (menu_checkboxstate[MENU_CHECKBOX_BLUEMAX] << 2) | (menu_checkboxstate[MENU_CHECKBOX_BLUEMIN] << 1) | menu_checkboxstate[MENU_CHECKBOX_BLUEAVG];
	char buffer[512];
	snprintf(buffer, sizeof(buffer), "{\n  \"XScaleI\": \"%u\",\n  \"YScaleI\": \"%u\",\n  \"YOff\": \"%i\",\n\
  \"IRed\": \"%u\",\n  \"IGreen\": \"%u\",\n  \"IBlue\": \"%u\",\n\
  \"TLevel\": \"%i\",\n  \"TMode\": \"%u\",\n  \"TType\": \"%u\",\n  \"TSource\": \"%u\",\n\
  \"ARed\": \"%u\",\n  \"AGreen\": \"%u\",\n  \"ABlue\": \"%u\"\n\
}\n",
	          timeScaleIndex, voltageScaleIndex, offsetMv,
	          redChannelIndex, greenChannelIndex, blueChannelIndex,
	          triggerLevelMv, triggerMode, triggerType, triggerSource,
	          analyzeRed, analyzeGreen, analyzeBlue);
	if (FilesystemWriteEtcFile(SCOPE_FILENAME, buffer, strlen(buffer))) {
		printf("Saved to %s\r\n", SCOPE_FILENAME);
		return true;
	} else {
		printf("Error, could not create file %s\r\n", SCOPE_FILENAME);
		return false;
	}
}


static inline void GuiSetScopePixel(uint16_t x, int16_t y, MENU_GFX_FORMAT_SCOPE color) {
	if ((x < MENU_GFX_SIZEX_SCOPE) && (y < MENU_GFX_SIZEY_SCOPE) && (y >= 0)) {
		uint8_t oldColor = g_gui.scope[MENU_GFX_SIZEX_SCOPE * y + x];
		if ((oldColor != 0x7) && (oldColor != 0x0)) {
			color |= oldColor;
		}
		g_gui.scope[MENU_GFX_SIZEX_SCOPE * y + x] = color;
	}
}

//resulting value saturates
static int16_t GuiCalcPixelY(int16_t value, int16_t digitsOffset, float pixelPerDigit) {
	value -= digitsOffset;
	float v = value;
	float f = v * pixelPerDigit;
	f = (float)(MENU_GFX_SIZEY_SCOPE - 1U) - f;
	if (f > INT16_MAX) {
		return INT16_MAX;
	}
	if (f < INT16_MIN) {
		return INT16_MIN;
	}
	return (f + 0.5f);
}

static void GuiDrawChannel(const uint16_t * adcValues, uint8_t indexOffset, uint16_t elementsPerChannel,
                           int16_t digitsOffset, float pixelPerDigit, MENU_GFX_FORMAT_SCOPE color) {
	int16_t yLast = 0;
	for (uint32_t i = 0; i < elementsPerChannel; i++) {
		int16_t y = GuiCalcPixelY(adcValues[i * g_gui.activeChannels + indexOffset], digitsOffset, pixelPerDigit);
		GuiSetScopePixel(i, y, color);
		//Disable to only draw the pixel, and do not draw a line from datapoint to datapoint
#if 1
		if ((i > 0) && (yLast != y)) {
			//interpolate between the two lines
			int32_t dir;
			int32_t distance;
			if (yLast < y) {
				dir = 1;
				distance = y - yLast;
			} else {
				dir = -1;
				distance = yLast - y;
			}
			int32_t mid = distance / 2;
			if (distance > 1) {
				for (int32_t j = 0; j < distance; j++) {
					if (j < mid) {
						GuiSetScopePixel(i - 1, yLast, color);
					} else {
						GuiSetScopePixel(i, yLast, color);
					}
					yLast += dir;
				}
			}
		}
		yLast = y;
#endif
	}
}

static void GuiAppendAnalyzedValue(char * outString, size_t outSize, const char * prefix, float value) {
	uint16_t mv = value * 1000.0f;
	size_t used = strlen(outString);
	size_t spaceLeft = outSize - used;
	snprintf(outString + used, spaceLeft, " %s: %umV", prefix, mv);
}

static void GuiAppendAnalyzeData(char * outString, size_t outSize, const char * prefix,
               const uint16_t * adcValues, uint8_t indexOffset, uint16_t elementsPerChannel,
               uint8_t useMaxIdx, uint8_t useMinIdx, uint8_t useAvgIdx) {
	uint8_t useMax = menu_checkboxstate[useMaxIdx];
	uint8_t useMin = menu_checkboxstate[useMinIdx];
	uint8_t useAvg = menu_checkboxstate[useAvgIdx];
	if ((useMax == 0) && (useMin == 0) && (useAvg == 0)) {
		return;
	}
	size_t used = strlen(outString);
	if ((used >= outSize) || (elementsPerChannel == 0)) {
		return;
	}
	uint32_t digitMax = 0;
	uint32_t digitMin = ADC_MAX;
	uint32_t digitSum = 0;
	for (uint32_t i = 0; i < elementsPerChannel; i++) {
		uint16_t d = adcValues[i * g_gui.activeChannels + indexOffset];
		digitMax = MAX(digitMax, d);
		digitMin = MIN(digitMin, d);
		digitSum += d;
	}
	float voltPerDigit = SampleVoltDigit(); //result unit: [V/digit]
	float vMax = (float)digitMax * voltPerDigit;
	float vMin = (float)digitMin * voltPerDigit;
	float vAvg = (float)digitSum * voltPerDigit / elementsPerChannel;
	size_t spaceLeft = outSize - used;
	snprintf(outString + used, spaceLeft, " %s", prefix);
	if (useMax) {
		GuiAppendAnalyzedValue(outString, outSize, "max", vMax);
	}
	if (useMin) {
		GuiAppendAnalyzedValue(outString, outSize, "min", vMin);
	}
	if (useAvg) {
		GuiAppendAnalyzedValue(outString, outSize, "avg", vAvg);
	}
}

static bool GuiRedrawGraph(bool userSettingsChanged) {
	const MENU_GFX_FORMAT_SCOPE colorWhite = 7;
	const MENU_GFX_FORMAT_SCOPE colorYellow = 6;
	const MENU_GFX_FORMAT_SCOPE colorRed = 4;
	const MENU_GFX_FORMAT_SCOPE colorGreen = 2;
	const MENU_GFX_FORMAT_SCOPE colorBlue = 1;

	//get the last data
	uint8_t type = 0;
	if (g_scaleTime[g_gui.timeScaleIndex].unit >= 10000000.0f) {
		//If there is only one sample every 10ms, do not wait for the buffer to be filled up
		type = 1;
	}
	const uint16_t * pAdcValues;
	uint32_t elementsReported = 0;
	uint8_t newData = SampleGet(type, &pAdcValues, &elementsReported);
	if ((newData == 0) && (userSettingsChanged == false)) {
		return false;
	}

	//first clear all pixel
	memset(g_gui.scope, 0, sizeof(MENU_GFX_FORMAT_SCOPE) * MENU_GFX_SIZEX_SCOPE * MENU_GFX_SIZEY_SCOPE);
	//draw white border around
	for (uint32_t i = 0; i < MENU_GFX_SIZEX_SCOPE; i++) {
		GuiSetScopePixel(i, 0, colorWhite);
		GuiSetScopePixel(i, MENU_GFX_SIZEY_SCOPE - 1, colorWhite);
	}
	for (uint32_t i = 0; i < MENU_GFX_SIZEY_SCOPE; i++) {
		GuiSetScopePixel(0, i, colorWhite);
		GuiSetScopePixel(MENU_GFX_SIZEX_SCOPE - 1, i, colorWhite);
	}
	//draw cords in the middle
	uint32_t inc = 4;
	for (uint32_t y = GUI_PIXEL_SQUARE; y < MENU_GFX_SIZEY_SCOPE; y += GUI_PIXEL_SQUARE) {
		for (uint32_t x = 0; x < MENU_GFX_SIZEX_SCOPE; x += inc) {
			GuiSetScopePixel(x, y - 1, colorWhite);
		}
	}
	for (uint32_t x = GUI_PIXEL_SQUARE; x < MENU_GFX_SIZEX_SCOPE; x += GUI_PIXEL_SQUARE) {
		for (uint32_t y = 0; y < MENU_GFX_SIZEY_SCOPE; y += inc) {
			GuiSetScopePixel(x, y - 1, colorWhite);
		}
	}
	//draw the trigger lines
	float voltPerDigit = SampleVoltDigit(); //result unit: [V/digit]
	int16_t digitsOffset = g_gui.offsetMv / (voltPerDigit * 1000.0f); //result unit: [digit]
	float voltPerPixel = g_scaleVoltage[g_gui.voltageScaleIndex].unit / (GUI_PIXEL_SQUARE * 1000.0f); //result unit: [V/pix]
	float pixelPerDigit = voltPerDigit / voltPerPixel; //result unit: [pixel/digit]
	float trigger = g_gui.triggerLevelMv - g_gui.offsetMv;
	trigger /= 1000.0f; //result unit: [V]
	int16_t triggerLine = trigger / voltPerPixel; //result unit: [pixel]
	triggerLine = (MENU_GFX_SIZEY_SCOPE - 1) - triggerLine;
	for (uint32_t x = 0; x < MENU_GFX_SIZEX_SCOPE; x += 2) {
		GuiSetScopePixel(x, triggerLine, colorYellow); //yellow horizontal line
	}
	for (uint32_t y = 1; y < (MENU_GFX_SIZEY_SCOPE - 1); y += 2) {
		GuiSetScopePixel(MENU_GFX_SIZEX_SCOPE / 2, y, colorYellow); //yellow vertical line
	}
	if (g_gui.activeChannels) {
		uint16_t elementsPerChannel = elementsReported / g_gui.activeChannels;
		uint8_t offset = 0;
		if (menu_listindexstate[MENU_LISTINDEX_ADCRED]) {
			GuiDrawChannel(pAdcValues, offset, elementsPerChannel, digitsOffset, pixelPerDigit, colorRed);
			offset++;
		}
		if (menu_listindexstate[MENU_LISTINDEX_ADCGREEN]) {
			GuiDrawChannel(pAdcValues, offset, elementsPerChannel, digitsOffset, pixelPerDigit, colorGreen);
			offset++;
		}
		if (menu_listindexstate[MENU_LISTINDEX_ADCBLUE]) {
			GuiDrawChannel(pAdcValues, offset, elementsPerChannel, digitsOffset, pixelPerDigit, colorBlue);
		}
		//analyze the data
		menu_strings[MENU_TEXT_VANALYZE][0] = '\0'; //reset string
		GuiAppendAnalyzeData(menu_strings[MENU_TEXT_VANALYZE], LONGTEXT, "red", pAdcValues, 0, elementsPerChannel, MENU_CHECKBOX_REDMAX, MENU_CHECKBOX_REDMIN, MENU_CHECKBOX_REDAVG);
		GuiAppendAnalyzeData(menu_strings[MENU_TEXT_VANALYZE], LONGTEXT, "green", pAdcValues, 1, elementsPerChannel, MENU_CHECKBOX_GREENMAX, MENU_CHECKBOX_GREENMIN, MENU_CHECKBOX_GREENAVG);
		GuiAppendAnalyzeData(menu_strings[MENU_TEXT_VANALYZE], LONGTEXT, "blue", pAdcValues, 2, elementsPerChannel, MENU_CHECKBOX_BLUEMAX, MENU_CHECKBOX_BLUEMIN, MENU_CHECKBOX_BLUEAVG);
	}
	return true;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"

/* We know the text might be longer than the buffer when numbers are high.
But: 1. The numbers possible are no more than 4 digits.
     2. If not truncated here, the GUI will do the truncation by overwriting the
       pixel with the next element anyway.
So why waste memory for solving compiler warnings? They are not buffer overflows.
*/
static void GuiUpdateUserSelections(void) {
	snprintf(menu_strings[MENU_TEXT_YAXIS], COMMONTEXT, "%s/div", g_scaleVoltage[g_gui.voltageScaleIndex].text);
	int16_t mv = g_gui.offsetMv;
	char prefix = '+';
	if (mv < 0) {
		prefix = '-';
		mv = -mv;
	}
	uint32_t ov = mv / 1000;
	uint32_t omv = (mv % 1000 ) / 10;
	snprintf(menu_strings[MENU_TEXT_YOFF], COMMONTEXT, "%c%u.%02uV", prefix, (unsigned int)ov, (unsigned int)omv);
	snprintf(menu_strings[MENU_TEXT_XAXIS], COMMONTEXT, "%s/div", g_scaleTime[g_gui.timeScaleIndex].text);
	uint8_t triggerType = menu_radiobuttonstate[MENU_RBUTTON_TRIGGERTYPE];
	const char * signal = NULL;
	if (triggerType == 0) {
		signal = "L";
	}
	if (triggerType == 1) {
		signal = "H";
	}
	if (triggerType == 2) {
		signal = "Fa";
	}
	if (triggerType == 3) {
		signal = "Ri";
	}
	uint8_t triggerMode = menu_radiobuttonstate[MENU_RBUTTON_TRIGGERMODE];
	uint32_t tv = g_gui.triggerLevelMv / 1000;
	uint32_t tmv = (g_gui.triggerLevelMv % 1000) / 10;
	char mode = 'A';
	if (triggerMode == 0) {
		mode = 'M';
	}
	snprintf(menu_strings[MENU_TEXT_TRIGGER], COMMONTEXT, "%c%s%u.%02uV", mode, signal, (unsigned int)tv, (unsigned int)tmv);
	GuiRedrawGraph(true);
}

#pragma GCC diagnostic pop

static void GuiUpdateTrigger(void) {
	if (g_gui.triggerLevelMv > TRIGGER_MV_MAX) {
		g_gui.triggerLevelMv = TRIGGER_MV_MAX;
	}
	if (g_gui.triggerLevelMv < TRIGGER_MV_MIN) {
		g_gui.triggerLevelMv = TRIGGER_MV_MIN;
	}
	uint16_t level = (float)g_gui.triggerLevelMv / 1000.0f / SampleVoltDigit();
	SampleTriggerSet(level, menu_radiobuttonstate[MENU_RBUTTON_TRIGGERTYPE], menu_radiobuttonstate[MENU_RBUTTON_TRIGGERSOURCE]);
	SampleModeSet(menu_radiobuttonstate[MENU_RBUTTON_TRIGGERMODE]);
	GuiUpdateUserSelections();
}


static void GuiLoadSettings(void) {
	uint8_t jsonFileBuffer[512];
	char value[10];
	size_t fileLen = 0;
	if (FilesystemReadFile(SCOPE_FILENAME, jsonFileBuffer, sizeof(jsonFileBuffer), &fileLen) == false) {
		return;
	}
	if (JsonValueGet(jsonFileBuffer, fileLen, "XScaleI", value, sizeof(value))) {
		uint32_t index = atoi(value);
		if (index < (sizeof(g_scaleTime)/(sizeof(textUnit_t)))) {
			g_gui.timeScaleIndex = index;
		}
	}
	if (JsonValueGet(jsonFileBuffer, fileLen, "YScaleI", value, sizeof(value))) {
		uint32_t index = atoi(value);
		if (index < (sizeof(g_scaleVoltage)/(sizeof(textUnit_t)))) {
			g_gui.voltageScaleIndex = index;
		}
	}
	if (JsonValueGet(jsonFileBuffer, fileLen, "YOff", value, sizeof(value))) {
		int32_t yOff = atoi(value);
		if ((yOff >= OFFSET_MV_MIN) && (yOff <= OFFSET_MV_MAX)) {
			g_gui.offsetMv = yOff;
		}
	}
	if (JsonValueGet(jsonFileBuffer, fileLen, "IRed", value, sizeof(value))) {
		uint32_t index = atoi(value);
		menu_listindexstate[MENU_LISTINDEX_ADCRED] = index;
	}
	if (JsonValueGet(jsonFileBuffer, fileLen, "IGreen", value, sizeof(value))) {
		uint32_t index = atoi(value);
		menu_listindexstate[MENU_LISTINDEX_ADCGREEN] = index;
	}
	if (JsonValueGet(jsonFileBuffer, fileLen, "IBlue", value, sizeof(value))) {
		uint32_t index = atoi(value);
		menu_listindexstate[MENU_LISTINDEX_ADCBLUE] = index;
	}
	if (JsonValueGet(jsonFileBuffer, fileLen, "TLevel", value, sizeof(value))) {
		int32_t tLevel = atoi(value);
		if ((tLevel >= TRIGGER_MV_MIN) && (tLevel <= TRIGGER_MV_MAX)) {
			g_gui.triggerLevelMv = tLevel;
		}
	}
	if (JsonValueGet(jsonFileBuffer, fileLen, "TMode", value, sizeof(value))) {
		uint32_t tMode = atoi(value);
		if (tMode < 2) {
			menu_radiobuttonstate[MENU_RBUTTON_TRIGGERMODE] = tMode;
		}
	}
	if (JsonValueGet(jsonFileBuffer, fileLen, "TType", value, sizeof(value))) {
		uint32_t tType = atoi(value);
		if (tType < 4) {
			menu_radiobuttonstate[MENU_RBUTTON_TRIGGERTYPE] = tType;
		}
	}
	if (JsonValueGet(jsonFileBuffer, fileLen, "TSource", value, sizeof(value))) {
		uint32_t tSource = atoi(value);
		if (tSource < CHANNELS) {
			menu_radiobuttonstate[MENU_RBUTTON_TRIGGERSOURCE] = tSource;
		}
	}
	if (JsonValueGet(jsonFileBuffer, fileLen, "ARed", value, sizeof(value))) {
		uint32_t analyzeRed = atoi(value);
		menu_checkboxstate[MENU_CHECKBOX_REDAVG] = (analyzeRed & 1) ? 1 : 0;
		menu_checkboxstate[MENU_CHECKBOX_REDMIN] = (analyzeRed & 2) ? 1 : 0;
		menu_checkboxstate[MENU_CHECKBOX_REDMAX] = (analyzeRed & 4) ? 1 : 0;
	}
	if (JsonValueGet(jsonFileBuffer, fileLen, "AGreen", value, sizeof(value))) {
		uint32_t analyzeGreen = atoi(value);
		menu_checkboxstate[MENU_CHECKBOX_GREENAVG] = (analyzeGreen & 1) ? 1 : 0;
		menu_checkboxstate[MENU_CHECKBOX_GREENMIN] = (analyzeGreen & 2) ? 1 : 0;
		menu_checkboxstate[MENU_CHECKBOX_GREENMAX] = (analyzeGreen & 4) ? 1 : 0;
	}
	if (JsonValueGet(jsonFileBuffer, fileLen, "ABlue", value, sizeof(value))) {
		uint32_t analyzeBlue = atoi(value);
		menu_checkboxstate[MENU_CHECKBOX_BLUEAVG] = (analyzeBlue & 1) ? 1 : 0;
		menu_checkboxstate[MENU_CHECKBOX_BLUEMIN] = (analyzeBlue & 2) ? 1 : 0;
		menu_checkboxstate[MENU_CHECKBOX_BLUEMAX] = (analyzeBlue & 4) ? 1 : 0;
	}
	GuiUpdateUserSelections();
	SampleRateSet(g_scaleTime[g_gui.timeScaleIndex].unit);
	GuiUpdateTrigger();
}

static void GuiDefaultSettings(void) {
	//input settings
	menu_listindexstate[MENU_LISTINDEX_ADCRED] = INPUT_RED_DEFAULT;
	menu_listindexstate[MENU_LISTINDEX_ADCGREEN] = INPUT_GREEN_DEFAULT;
	menu_listindexstate[MENU_LISTINDEX_ADCBLUE] = INPUT_BLUE_DEFAULT;
	//trigger settings
	menu_radiobuttonstate[MENU_RBUTTON_TRIGGERMODE] = 1; //automatic
	menu_radiobuttonstate[MENU_RBUTTON_TRIGGERTYPE] = 0; //low level
	menu_radiobuttonstate[MENU_RBUTTON_TRIGGERSOURCE] = 0; //red channel
	g_gui.triggerLevelMv = 1500;
	//view settings
	g_gui.timeScaleIndex = SCALETIME_INDEX_DEFAULT;
	g_gui.voltageScaleIndex = 2;
	g_gui.offsetMv = 0;
	//analyze settings
	menu_checkboxstate[MENU_CHECKBOX_REDMAX] = 1;
	menu_checkboxstate[MENU_CHECKBOX_REDMIN] = 1;
	menu_checkboxstate[MENU_CHECKBOX_REDAVG] = 1;
	menu_checkboxstate[MENU_CHECKBOX_GREENMAX] = 0;
	menu_checkboxstate[MENU_CHECKBOX_GREENMIN] = 0;
	menu_checkboxstate[MENU_CHECKBOX_GREENAVG] = 0;
	menu_checkboxstate[MENU_CHECKBOX_BLUEMAX] = 0;
	menu_checkboxstate[MENU_CHECKBOX_BLUEMIN] = 0;
	menu_checkboxstate[MENU_CHECKBOX_BLUEAVG] = 0;
	//forwared required
	SampleRateSet(g_scaleTime[g_gui.timeScaleIndex].unit);
	GuiUpdateTrigger();
}

uint8_t menu_action(MENUACTION action) {
	if (action == MENU_ACTION_XINC) {
		if ((g_gui.timeScaleIndex + 1U) < (sizeof(g_scaleTime)/(sizeof(textUnit_t)))) {
			g_gui.timeScaleIndex++;
		}
		SampleRateSet(g_scaleTime[g_gui.timeScaleIndex].unit);
		GuiUpdateUserSelections();
		return 1;
	}
	if (action == MENU_ACTION_XDEC) {
		if (g_gui.timeScaleIndex) {
			g_gui.timeScaleIndex--;
		}
		SampleRateSet(g_scaleTime[g_gui.timeScaleIndex].unit);
		GuiUpdateUserSelections();
		return 1;
	}
	if (action == MENU_ACTION_YINC) {
		if ((g_gui.voltageScaleIndex + 1U) < (sizeof(g_scaleVoltage)/(sizeof(textUnit_t)))) {
			g_gui.voltageScaleIndex++;
		}
		GuiUpdateUserSelections();
		return 1;
	}
	if (action == MENU_ACTION_YDEC) {
		if (g_gui.voltageScaleIndex) {
			g_gui.voltageScaleIndex--;
		}
		GuiUpdateUserSelections();
		return 1;
	}
	if (action == MENU_ACTION_YOFFINC) {
		g_gui.offsetMv += g_scaleVoltage[g_gui.voltageScaleIndex].unit;
		if (g_gui.offsetMv > OFFSET_MV_MAX) {
			g_gui.offsetMv = OFFSET_MV_MAX;
		}
		GuiUpdateUserSelections();
		return 1;
	}
	if (action == MENU_ACTION_YOFFDEC) {
		g_gui.offsetMv -= g_scaleVoltage[g_gui.voltageScaleIndex].unit;
		if (g_gui.offsetMv < OFFSET_MV_MIN) {
			g_gui.offsetMv = OFFSET_MV_MIN;
		}
		GuiUpdateUserSelections();
		return 1;
	}
	if (action == MENU_ACTION_TRIGGERSTART) {
		SampleStart();
		return 0;
	}
	if (action == MENU_ACTION_SCREENSHOT) {
		if (Screenshot()) {
			menu_keypress(100);
		}
		return 1;
	}
	if (action == MENU_ACTION_CONFIGSAVE) {
		if (GuiSaveSettings()) {
			menu_keypress(100);
		}
		return 1;
	}
	if (action == MENU_ACTION_CONFIGDEFAULT) {
		GuiDefaultSettings();
		GuiUpdateTrigger();
		return 1;
	}
	if (action == MENU_ACTION_TRIGGERINC500) {
		g_gui.triggerLevelMv += 500;
		GuiUpdateTrigger();
		return 1;
	}
	if (action == MENU_ACTION_TRIGGERINC50) {
		g_gui.triggerLevelMv += 50;
		GuiUpdateTrigger();
		return 1;
	}
	if (action == MENU_ACTION_TRIGGERDEC50) {
		g_gui.triggerLevelMv -= 50;
		GuiUpdateTrigger();
		return 1;
	}
	if (action == MENU_ACTION_TRIGGERDEC500) {
		g_gui.triggerLevelMv -= 500;
		GuiUpdateTrigger();
		return 1;
	}
	if (action == MENU_ACTION_TRIGGERUPDATE) {
		GuiUpdateTrigger();
		return 1;
	}
	if (action == MENU_ACTION_INDEXCHANGE_UPDATEINPUT) {
		GuiUpdateInputs();
		return 1;
	}
	return 0;
}

void GuiInit(void) {
	printf("Starting GUI\r\n");
	menu_strings[MENU_TEXT_INPUTSLIST] = g_inputsList;
	menu_strings[MENU_TEXT_XAXIS] = g_gui.textXAxis;
	menu_strings[MENU_TEXT_YAXIS] = g_gui.textYAxis;
	menu_strings[MENU_TEXT_YOFF] = g_gui.textYOff;
	menu_strings[MENU_TEXT_TRIGGER] = g_gui.textTrigger;
	menu_strings[MENU_TEXT_VANALYZE] = g_gui.textVanalyze;
	menu_gfxdata[MENU_GFX_SCOPE] = g_gui.scope;
	if (FlashReady()) {
		g_gui.type = FilesystemReadLcd();
	} else {
		printf("No filesystem, assuming ILI9341 LCD!\r\n");
		g_gui.type = ILI9341;
	}
	if (g_gui.type != NONE) {
		LcdBacklightOn();
		//At 40MHz: The SPI transfer takes 73ms, at 20MHz: 103ms
		LcdEnable(2); //40MHz
		LcdInit(g_gui.type);
	}
	if (g_gui.type == ILI9341) {
		g_gui.pixelX = 320;
		g_gui.pixelY = 240;
	} else {
		printf("No GUI enabled, or LCD not supported\r\n");
	}
	GuiDefaultSettings();
	GuiLoadSettings();
	//GuiUpdateTrigger and GuiUpdateUserSelection() are part of GuiDefaultSettings() and GuiLoadSettings()
	GuiUpdateInputs();
	GuiRedrawGraph(true);
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
	if (cycle == 250) { //update GFX every 250ms
		cycle = 0;
		if (GuiRedrawGraph(false)) {
			menu_redraw();
		}
	}
}

