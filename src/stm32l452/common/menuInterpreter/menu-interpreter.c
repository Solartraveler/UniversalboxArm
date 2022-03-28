/* MenuInterpreter
   Version 2.0
   (c) 2009-2010, 2012, 2014, 2016, 2019, 2020 by Malte Marwedel
   m DOT talk AT marwedels DOT de
   www.marwedels.de/malte
   menudesigner.sourceforge.net

   This Source Code Form is subject to the terms of the
   Mozilla Public License, v. 2.0.
   If a copy of the MPL was not distributed with this file,
   You can obtain one at https://mozilla.org/MPL/2.0/.
*/
//SPDX-License-Identifier: MPL-2.0


#include "menu-interpreter.h"
#include "menu-text.h"

#include <stdlib.h>
#include <stdint.h>

//#define DEBUG

#ifdef DEBUG
#include <stdio.h>
//use printf for debug messages
#define MENU_DEBUGMSG printf

#else
//ignore debug messages
#define MENU_DEBUGMSG(...)

#endif

#ifndef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(P) (void)(P)
#endif

#if (MENU_BYTECODE_VERSION != 5)
#error "Bytecode does not work with this interpreter. Version mismatch"
#endif

//macros for getting screen position and size
#ifdef LARGESCREEN

//creates two variables (+helper var) with the screen position from the bytecodes
#define MENU_SCREENPOS_PX_PY \
	SCREENPOS px = menu_byte_get_next(); \
	SCREENPOS py = menu_byte_get_next(); \
	uint8_t lpxy = menu_byte_get_next(); \
	px += (SCREENPOS)(lpxy & 0xF0) << 4; \
	py += (SCREENPOS)(lpxy & 0x0F) << 8;

//creates two variables (+helper var)with the size of the object from the bytecodes

#define MENU_SCREENPOS_SX_SY \
	SCREENPOS sx = menu_byte_get_next(); \
	SCREENPOS sy = menu_byte_get_next(); \
	uint8_t lsxy = menu_byte_get_next(); \
	sx += (SCREENPOS)(lsxy & 0xF0) << 4; \
	sy += (SCREENPOS)(lsxy & 0x0F) << 8;

#else

#define MENU_SCREENPOS_PX_PY \
	SCREENPOS px = menu_byte_get_next(); \
	SCREENPOS py = menu_byte_get_next();

#define MENU_SCREENPOS_SX_SY \
	SCREENPOS sx = menu_byte_get_next(); \
	SCREENPOS sy = menu_byte_get_next();

#endif



//variables for dynamic data
char * menu_strings[MENU_TEXT_MAX];
uint8_t menu_checkboxstate[MENU_CHECKBOX_MAX];
uint8_t menu_radiobuttonstate[MENU_RADIOBUTTON_MAX];
uint16_t menu_listindexstate[MENU_LIST_MAX];
uint8_t * menu_gfxdata[MENU_GFX_MAX];
#ifdef MENU_USE_MULTIGFX
uint16_t menu_gfxindexstate[MENU_MULTIGFX_MAX];
#endif



#ifdef MENU_USE_MULTILANGUAGE
uint8_t menu_language;

void menu_language_set(uint8_t id) {
	if (id < MENU_LANGUAGES_MAX) {
		menu_language = id;
	} else {
		MENU_DEBUGMSG("Error: language %i is out of range\n", id);
	}
}
#endif

//the state of the menu
//the adress of the current window
MENUADDR menu_window_init = 0; //the drawn window
#ifdef MENU_USE_SUBWINDOW
MENUADDR menu_subwindow_start = 0; //subwindow start adress, or 0 if no subwindow is shown
#endif
MENUADDR menu_window_start = 1; //the window which should be drawn
//the number of objects on the current window

#if (MENU_OBJECTS_MAX <= 255)
uint8_t menu_objects;
//the nuber of the object with the focus
uint8_t menu_focus;
uint8_t menu_focus_prime; //saves the focus of a window if the subwindo is active
#else
uint16_t menu_objects;
uint16_t menu_focus;
uint16_t menu_focus_prime;
#endif

uint8_t menu_focus_key_next, menu_focus_key_prev, menu_key_enter;
uint8_t menu_focus_restore; //set to 1 if focus should be resored
//the index of all objects in the list, 0 for objects which disallow a focus
MENUADDR menu_focus_objects[MENU_OBJECTS_MAX];

MENUADDR menu_pc; //shows on the byte to read next
static void menu_pc_set(MENUADDR addr) {
	menu_pc = addr;
}

static uint8_t menu_byte_get_next(void) {
	return menu_byte_get(menu_pc++);
}

static void menu_pc_skip(MENUADDR add) {
	menu_pc += add;
}

static MENUADDR menu_assemble_addr(void) {
#ifdef USE16BITADDR
	return menu_byte_get_next() + (menu_byte_get_next() <<8);
#else
	return menu_byte_get_next() + (menu_byte_get_next() <<8) + (menu_byte_get_next() << 16);
#endif
}

static MENUADDR menu_object_datasize(uint8_t object_id) {
	switch(object_id) {
		case MENU_BOX: return MENU_BOX_DATA;
		case MENU_LABEL: return MENU_LABEL_DATA;
		case MENU_BUTTON: return MENU_BUTTON_DATA;
		case MENU_GFX: return MENU_GFX_DATA;
		case MENU_LIST: return MENU_LIST_DATA;
		case MENU_CHECKBOX: return MENU_CHECKBOX_DATA;
		case MENU_RADIOBUTTON: return MENU_RADIOBUTTON_DATA;
		case MENU_SUBWINDOW: return MENU_SUBWINDOW_DATA;
		case MENU_WINDOW: return MENU_WINDOW_DATA;
		case MENU_SHORTCUT: return MENU_SHORTCUT_DATA;
		default: return 0;
	}
}

#if (defined(MENU_USE_LIST) || defined(MENU_USE_SUBWINDOW) || \
     defined(MENU_USE_BUTTON) || defined(MENU_USE_CHECKBOX) || \
     defined(MENU_USE_RADIOBUTTON) || defined(MENU_USE_BOX) || \
     defined(MENU_USE_GFX))

static void menu_draw_Xline(SCREENPOS px, SCREENPOS py,
                              SCREENPOS length, SCREENCOLOR color, uint8_t dotted) {
	SCREENPOS x;
	SCREENCOLOR colormod;
	for (x = px; x < px+length; x++) {
		if (dotted) //invert to save dotted state
			dotted = ~dotted;
		if (dotted == 1) {
			colormod = MENU_COLOR_BACKGROUND;
		} else {
			colormod = color;
		}
		menu_screen_set(x, py, colormod);
	}
}

static void menu_draw_Yline(SCREENPOS px, SCREENPOS py,
                              SCREENPOS length, SCREENCOLOR color, uint8_t dotted) {
	SCREENPOS y;
	SCREENCOLOR colormod;
	for (y = py; y < py+length; y++) {
		if (dotted) //invert to save dotted state
			dotted = ~dotted;
		if (dotted == 1) {
			colormod = MENU_COLOR_BACKGROUND;
		} else {
			colormod = color;
		}
		menu_screen_set(px, y, colormod);
	}
}

static void menu_draw_border(SCREENPOS px, SCREENPOS py, SCREENPOS sx,
                      SCREENPOS sy, SCREENCOLOR color, uint8_t hasfocus) {
	menu_draw_Xline(px, py, sx, color, hasfocus);
	uint8_t dotted = 0;
	if (hasfocus) {
		if (sx & 1) {
			dotted = 1;
		} else {
			dotted = 0xFE;
		}
	}
	menu_draw_Yline(px+sx-1, py, sy, color, dotted);
	menu_draw_Yline(px, py, sy, color, hasfocus);
	if (hasfocus) {
		if (sy & 1) {
			dotted = 1;
		} else {
			dotted = 0xFE;
		}
	}
	menu_draw_Xline(px, py+sy-1, sx, color, dotted);
}

#endif

#if (defined(MENU_USE_LIST) || defined(MENU_USE_BOX) || defined(MENU_USE_SUBWINDOW))

static void menu_draw_box(SCREENPOS px, SCREENPOS py, SCREENPOS sx,
                   SCREENPOS sy, SCREENCOLOR color) {
	SCREENPOS x, y;
	SCREENPOS ex = px+sx;
	SCREENPOS ey = py+sy;
	for (y = py; y < ey; y++) {
		for (x = px; x < ex; x++) {
			menu_screen_set(x, y, color);
		}
	}
}

#endif

#if defined(MENU_USE_MULTILANGUAGE) || defined(MENU_USE_MULTIGFX)

static MENUADDR menu_assembleaddr_direct(MENUADDR baseaddr) {
#ifdef USE16BITADDR
	return menu_byte_get(baseaddr) + (menu_byte_get(baseaddr + 1) <<8);
#else
	return menu_byte_get(baseaddr) + (menu_byte_get(baseaddr + 1) <<8) + (menu_byte_get(baseaddr + 2) << 16);
#endif
}

#endif

#if defined(MENU_USE_BUTTON) || \
    defined(MENU_USE_CHECKBOX) || \
    defined(MENU_USE_RADIOBUTTON) || \
    defined(MENU_USE_LABEL) || \
    defined(MENU_USE_LIST)

//storage: lowest bit: static/dynamic, 2.bit: single/multi language
static char menu_text_byte_get(MENUADDR baseaddr, uint16_t index,
                                 uint8_t storage) {
	char c = '\0';
	if ((storage & 1) == 0) {
#ifdef MENU_USE_MULTILANGUAGE
		if (storage & 2) {
			baseaddr += menu_language*MENU_ADDR_BYTES;
			baseaddr = menu_assembleaddr_direct(baseaddr);
			MENU_DEBUGMSG("Multilang %i base address changed to: %i\n", menu_language, baseaddr);
		}
#endif
		c = menu_byte_get(baseaddr+index);
	} else {
#if (MENU_TEXT_MAX > 0)
		if (baseaddr < MENU_TEXT_MAX) {
			if (menu_strings[baseaddr] != NULL) {
#ifdef MENU_USE_MULTILANGUAGE
				if (storage & 2) {
					baseaddr += menu_language;
				}
#endif
				c = menu_strings[baseaddr][index];
			}
		}
#endif
	}
	return c;
}

static void menu_text_draw_base(SCREENPOS x, SCREENPOS y, uint8_t font,
                         uint8_t options, MENUADDR baseaddr,
                         uint16_t offset, SCREENPOS maxpix) {
	char cdraw;
	uint16_t index = offset;
	maxpix += x;
	uint8_t storage = (options>>MENU_OPTIONS_STORAGE) & 3;
	uint8_t transparency = (options>>MENU_OPTIONS_TRANSPARENCY) & 1;
	while (index < 10000) {
		cdraw = menu_text_byte_get(baseaddr, index, storage);
		index++;
		if ((cdraw == '\0') || (cdraw == '\n') || (x >= maxpix)) {
			break;
		}
		SCREENPOS ox = x;
		uint8_t width = menu_char_draw(x, y, font, cdraw, transparency);
		if (width != 255) { //value 255 is used to draw no char, instead use char for UTF-8 state machine, more bytes to follow
			x += width + 1;
		}
		if (ox > x) {
			break; //prevent overflow on the right side of the screen back to the left
		}
	}
}

#endif

#if defined(MENU_USE_BUTTON) || \
    defined(MENU_USE_CHECKBOX) || \
    defined(MENU_USE_RADIOBUTTON) || \
    defined(MENU_USE_LABEL)

static void menu_text_draw(SCREENPOS x, SCREENPOS y, uint8_t font,
 uint8_t options, MENUADDR baseaddr) {
	menu_text_draw_base(x, y, font, options, baseaddr, 0, MENU_SCREEN_X - x);
}

#endif


#if defined(MENU_USE_BOX) && (MENU_COLOR_OUT_RED_MSB_POS > 7)
static SCREENCOLOR menu_rgb332_Output(uint8_t colorin) {
	SCREENCOLOR c;
	uint8_t r = (colorin & 0xE0);
	if (r == 0xE0) { //not linear scaling, but allows white color and fast calculation
		r = 0xFF;
	}
	uint8_t g = (colorin & 0x1C) << 3;
	if (g == 0xE0) {
		g = 0xFF;
	}
	uint8_t b = (colorin & 0x3) << 6;
	if (b == 0xC0) {
		b = 0xFF;
	}
	SCREENCOLOR rout = r >> (8 - MENU_COLOR_OUT_RED_BITS);
	SCREENCOLOR gout = g >> (8 - MENU_COLOR_OUT_GREEN_BITS);
	SCREENCOLOR bout = b >> (8 - MENU_COLOR_OUT_BLUE_BITS);
	c = (rout << MENU_COLOR_OUT_RED_LSB_POS) | (gout << MENU_COLOR_OUT_GREEN_LSB_POS) | (bout << MENU_COLOR_OUT_BLUE_LSB_POS);
	return c;
}
#endif

static void menu_box(uint8_t hasfocus) {
#ifdef MENU_USE_BOX
	MENU_SCREENPOS_PX_PY
	MENU_SCREENPOS_SX_SY
	menu_pc_skip(MENU_ACTION_BYTES + MENU_OPTION_BYTES + MENU_ADDR_BYTES); //options + action + windowaddr
	SCREENCOLOR color = menu_byte_get_next();
#if (MENU_COLOR_OUT_RED_MSB_POS > 7)
	color = menu_rgb332_Output(color); //332 format if output format has more than 8 bits
#endif
	MENU_DEBUGMSG("Drawing box %i;%i with size %i;%i, color %i, focus %i\n", px, py, sx, sy, color, hasfocus);
	menu_draw_box(px, py, sx, sy, color);
	if (hasfocus) //append dotted border
		menu_draw_border(px, py, sx, sy, color, hasfocus);
#else
	UNREFERENCED_PARAMETER(hasfocus);
	MENU_DEBUGMSG("Error: Box used, but not compiled in\n");
#endif
}

static void menu_label(void) {
#ifdef MENU_USE_LABEL
	MENU_SCREENPOS_PX_PY
	uint8_t options = menu_byte_get_next();
	MENUADDR textaddr = menu_assemble_addr();
	uint8_t fonts = menu_byte_get_next();
	MENU_DEBUGMSG("Drawing label %i, %i with font %i, options 0x%x, addr %i\n", px, py, fonts, options, textaddr);
	menu_text_draw(px, py, fonts, options, textaddr);
#else
	MENU_DEBUGMSG("Error: Label used, but not compiled in\n");
#endif
}

#if defined(MENU_USE_BUTTON) || \
    defined(MENU_USE_CHECKBOX) || \
    defined(MENU_USE_RADIOBUTTON)

static void menu_basicbutton(uint8_t hasfocus, uint8_t offx, uint8_t offy) {
	MENU_SCREENPOS_PX_PY
	MENU_SCREENPOS_SX_SY
	uint8_t options = menu_byte_get_next();
	menu_pc_skip(MENU_ACTION_BYTES + MENU_ADDR_BYTES); //action + windowaddr
	MENUADDR textaddr = menu_assemble_addr();
	uint8_t fonts = menu_byte_get_next();
	MENU_DEBUGMSG("Drawing button %i, %i with font %i, options 0x%x, addr %i\n", px, py, fonts, options, textaddr);
	if (hasfocus) {
		fonts = fonts >>4;
	} else
		fonts = fonts & 0x0f;
	menu_text_draw(px+offx, py+offy, fonts, options, textaddr);
	if (options & (1 << MENU_OPTIONS_RECTANGLE))
		menu_draw_border(px, py, sx, sy, MENU_COLOR_BORDER, hasfocus);
}

#endif

static void menu_button(uint8_t hasfocus) {
#ifdef MENU_USE_BUTTON
	//possible improvement: calculate second two parameters so that the text is centered
	menu_basicbutton(hasfocus, 2, 2);
#else
	UNREFERENCED_PARAMETER(hasfocus);
	MENU_DEBUGMSG("Error: Button used, but not compiled in\n");
#endif
}

static void menu_checkbox(uint8_t hasfocus) {
#ifdef MENU_USE_CHECKBOX
	SCREENPOS px = menu_byte_get(menu_pc);
	SCREENPOS py = menu_byte_get(menu_pc+1);
#ifdef LARGESCREEN
	uint8_t lpxy = menu_byte_get(menu_pc+2);
	px += (SCREENPOS)(lpxy & 0xF0) << 4;
	py += (SCREENPOS)(lpxy & 0x0F) << 8;
#endif
	menu_basicbutton(hasfocus, 10, 0);
	uint8_t ckbnumber = menu_byte_get_next();
	menu_draw_border(px, py, 8, 7, MENU_COLOR_BORDER, 0);
	SCREENCOLOR color = MENU_COLOR_BACKGROUND;
	if (hasfocus) {
		color = MENU_COLOR_BORDER;
	}
	menu_draw_Xline(px, py+7, 8, color, 0);
	color = MENU_COLOR_BACKGROUND;
	if ((ckbnumber < MENU_CHECKBOX_MAX) && (menu_checkboxstate[ckbnumber])) {
		color = MENU_COLOR_BORDER;
	}
	MENU_DEBUGMSG("Drawing checkbox %i with state %i\n", ckbnumber, color);
	//TODO: make the pattern available as gfx instead of hard coded values
	menu_screen_set(px+1, py+4, color);
	menu_screen_set(px+2, py+5, color);
	menu_screen_set(px+3, py+4, color);
	menu_screen_set(px+4, py+3, color);
	menu_screen_set(px+5, py+2, color);
	menu_screen_set(px+6, py+1, color);
#else
	UNREFERENCED_PARAMETER(hasfocus);
	MENU_DEBUGMSG("Error: Checkbox used, but not compiled in\n");
#endif
}

static void menu_radiobutton(uint8_t hasfocus) {
#ifdef MENU_USE_RADIOBUTTON
	SCREENPOS px = menu_byte_get(menu_pc);
	SCREENPOS py = menu_byte_get(menu_pc+1);
#ifdef LARGESCREEN
	uint8_t lpxy = menu_byte_get(menu_pc+2);
	px += (SCREENPOS)(lpxy & 0xF0) << 4;
	py += (SCREENPOS)(lpxy & 0x0F) << 8;
#endif
	menu_basicbutton(hasfocus, 10, 0);
	uint8_t radionumber = menu_byte_get_next();
	uint8_t radioselect = radionumber >> 4;
	radionumber &= 0x0f;
	menu_draw_border(px, py, 8, 7, MENU_COLOR_BORDER, 0);
	SCREENCOLOR color = MENU_COLOR_BACKGROUND;
	if (hasfocus)
		color = MENU_COLOR_BORDER;
	menu_draw_Xline(px, py+7, 8, color, 0);
	color = MENU_COLOR_BACKGROUND;
	if ((radionumber < MENU_RADIOBUTTON_MAX) && (menu_radiobuttonstate[radionumber] == radioselect)) {
		color = MENU_COLOR_BORDER;
	}
	MENU_DEBUGMSG("Drawing radiobutton of group %i, checked on %i. Tableentry: %i\n", radionumber, radioselect, menu_checkboxstate[radionumber]);
	//TODO: make the pattern available as gfx instead of hard coded values
	menu_screen_set(px+3, py+2, color);
	menu_screen_set(px+4, py+2, color);
	menu_screen_set(px+2, py+3, color);
	menu_screen_set(px+3, py+3, color);
	menu_screen_set(px+4, py+3, color);
	menu_screen_set(px+5, py+3, color);
	menu_screen_set(px+3, py+5, color);
	menu_screen_set(px+4, py+5, color);
	menu_screen_set(px+2, py+4, color);
	menu_screen_set(px+3, py+4, color);
	menu_screen_set(px+4, py+4, color);
	menu_screen_set(px+5, py+4, color);
#else
	UNREFERENCED_PARAMETER(hasfocus);
	MENU_DEBUGMSG("Error: Radiobutton used, but not compiled in\n");
#endif
}

#ifdef MENU_USE_GFX

#if ((defined(MENU_USE_GFXFORMAT_2)) && (!defined(MENU_SCREEN_COLORCUSTOM1))) || \
    ((defined(MENU_USE_GFXFORMAT_3)) && (!defined(MENU_SCREEN_COLORCUSTOM2))) || \
    ((defined(MENU_USE_GFXFORMAT_4)) && (!defined(MENU_SCREEN_COLORCUSTOM3)))

static inline SCREENCOLOR menu_gfx_rgb_to_screencolor(uint8_t r, uint8_t g, uint8_t b)
{
	SCREENCOLOR c;
#if defined(MENU_SCREEN_MONOCHROME)
	uint16_t c16 = r + g + b;
	if (c16 > 382) {
		c = 1;
	} else {
		c = 0;
	}
#elif defined(MENU_SCREEN_GREYSCALE4BIT)
	uint16_t c16 = r + g + b;
	c = c16 / 48; //(255*3)/48 < 16
#else
	SCREENCOLOR rout = r >> (8 - MENU_COLOR_OUT_RED_BITS);
	SCREENCOLOR gout = g >> (8 - MENU_COLOR_OUT_GREEN_BITS);
	SCREENCOLOR bout = b >> (8 - MENU_COLOR_OUT_BLUE_BITS);
	c = (rout << MENU_COLOR_OUT_RED_LSB_POS) | (gout << MENU_COLOR_OUT_GREEN_LSB_POS) | (bout << MENU_COLOR_OUT_BLUE_LSB_POS);
#endif
	return c;
}

#endif

#if defined(MENU_USE_GFXFORMAT_1)
static void menu_gfx_4Bit_set(SCREENPOS x, SCREENPOS y, uint8_t color4Bit) {
	SCREENCOLOR c;
#if defined(MENU_SCREEN_MONOCHROME)
	c = 0;
	if (color4Bit & 0x8) {
		c = 1;
	}
#elif defined(MENU_SCREEN_GREYSCALE4BIT)
	c = color4Bit;
#else
	color4Bit <<= 4;
	SCREENCOLOR rout = color4Bit >> (8 - MENU_COLOR_OUT_RED_BITS);
	SCREENCOLOR gout = color4Bit >> (8 - MENU_COLOR_OUT_GREEN_BITS);
	SCREENCOLOR bout = color4Bit >> (8 - MENU_COLOR_OUT_BLUE_BITS);
	c = (rout << MENU_COLOR_OUT_RED_LSB_POS) | (gout << MENU_COLOR_OUT_GREEN_LSB_POS) | (bout << MENU_COLOR_OUT_BLUE_LSB_POS);
#endif
	menu_screen_set(x, y, c);
}
#endif

#define MENU_GFX_CUSTOMCOLOR_TO_SCREENCOLOR(ID) \
	uint8_t r = (colorin & MENU_COLOR_CUSTOM##ID##_RED_MASK) >> MENU_COLOR_CUSTOM##ID##_RED_LSB_POS;\
	uint8_t g = (colorin & MENU_COLOR_CUSTOM##ID##_GREEN_MASK) >> MENU_COLOR_CUSTOM##ID##_GREEN_LSB_POS;\
	uint8_t b = (colorin & MENU_COLOR_CUSTOM##ID##_BLUE_MASK) >> MENU_COLOR_CUSTOM##ID##_BLUE_LSB_POS;\
	r = r << (8 - MENU_COLOR_CUSTOM##ID##_RED_BITS);\
	g = g << (8 - MENU_COLOR_CUSTOM##ID##_GREEN_BITS);\
	b = b << (8 - MENU_COLOR_CUSTOM##ID##_BLUE_BITS);\
	c = menu_gfx_rgb_to_screencolor(r, g, b);



#if defined(MENU_USE_GFXFORMAT_2)
static void menu_gfx_Custom1_set(SCREENPOS x, SCREENPOS y, COLORCUSTOM3 colorin) {
	SCREENCOLOR c;
#if defined(MENU_SCREEN_COLORCUSTOM1)
	c = colorin;
#else
	MENU_GFX_CUSTOMCOLOR_TO_SCREENCOLOR(1)
#endif
	menu_screen_set(x, y, c);
}
#endif


#if defined(MENU_USE_GFXFORMAT_3)
static void menu_gfx_Custom2_set(SCREENPOS x, SCREENPOS y, COLORCUSTOM3 colorin) {
	SCREENCOLOR c;
#if defined(MENU_SCREEN_COLORCUSTOM2)
	c = colorin;
#else
	MENU_GFX_CUSTOMCOLOR_TO_SCREENCOLOR(2)
#endif
	menu_screen_set(x, y, c);
}
#endif


#if defined(MENU_USE_GFXFORMAT_4)
static void menu_gfx_Custom3_set(SCREENPOS x, SCREENPOS y, COLORCUSTOM3 colorin) {
	SCREENCOLOR c;
#if defined(MENU_SCREEN_COLORCUSTOM3)
	c = colorin;
#else
	MENU_GFX_CUSTOMCOLOR_TO_SCREENCOLOR(3)
#endif
	menu_screen_set(x, y, c);
}
#endif

inline static uint8_t menu_gfx_1byte_fetch(MENUADDR gfxaddr, MENUADDR gfxinc, uint8_t storage) {
	uint8_t data = 0;
	if ((storage & 1) == 0) { //internal one
		data = menu_byte_get(gfxaddr+gfxinc);
	} else {//if within the RAM
		data = menu_gfxdata[gfxaddr][gfxinc];
	}
	return data;
}

#if ((defined(MENU_USE_GFXFORMAT_2) && (MENU_GFX_CUSTOM1_BYTES == 2)) ||\
    (defined(MENU_USE_GFXFORMAT_3) && (MENU_GFX_CUSTOM2_BYTES == 2)) ||\
    (defined(MENU_USE_GFXFORMAT_4) && (MENU_GFX_CUSTOM3_BYTES == 2)))
inline static uint16_t menu_gfx_2byte_fetch(MENUADDR gfxaddr, MENUADDR gfxinc, uint8_t storage) {
	uint16_t color =  menu_gfx_1byte_fetch(gfxaddr, gfxinc, storage);
	color |= menu_gfx_1byte_fetch(gfxaddr, gfxinc + 1, storage) << 8;
	return color;
}
#endif


#if ((defined(MENU_USE_GFXFORMAT_2) && (MENU_GFX_CUSTOM1_BYTES == 3)) ||\
    (defined(MENU_USE_GFXFORMAT_3) && (MENU_GFX_CUSTOM2_BYTES == 3)) ||\
    (defined(MENU_USE_GFXFORMAT_4) && (MENU_GFX_CUSTOM3_BYTES == 3)))
inline static uint32_t menu_gfx_3byte_fetch(MENUADDR gfxaddr, MENUADDR gfxinc, uint8_t storage) {
	uint32_t color =  menu_gfx_1byte_fetch(gfxaddr, gfxinc, storage);
	color |= menu_gfx_1byte_fetch(gfxaddr, gfxinc + 1, storage) << 8;
	color |= (uint32_t)menu_gfx_1byte_fetch(gfxaddr, gfxinc + 2, storage) << 16;
	return color;
}
#endif

#define MENU_GFX_COMPRESSION_REPEATS_EXTRA_BYTE \
  havedata = menu_gfx_1byte_fetch(gfxaddr, gfxinc, storage) + 1; \
  gfxinc++;

#define MENU_GFX_COMPRESSION_REPEATS_INCLUDED(ID) \
  havedata = (color >> (MENU_COLOR_CUSTOM##ID##_RED_MSB_POS + 1)) + 1; \
  color &= (1 << (MENU_COLOR_CUSTOM##ID##_RED_MSB_POS + 1)) -1;

#if (MENU_GFX_CUSTOM1_COMPRESSION_BITS == 8)
#define MENU_GFX_CUSTOM1_COMPRESSION_REPEATS MENU_GFX_COMPRESSION_REPEATS_EXTRA_BYTE

#else
#define MENU_GFX_CUSTOM1_COMPRESSION_REPEATS MENU_GFX_COMPRESSION_REPEATS_INCLUDED(1)

#endif

#if (MENU_GFX_CUSTOM2_COMPRESSION_BITS == 8)
#define MENU_GFX_CUSTOM2_COMPRESSION_REPEATS MENU_GFX_COMPRESSION_REPEATS_EXTRA_BYTE

#else
#define MENU_GFX_CUSTOM2_COMPRESSION_REPEATS MENU_GFX_COMPRESSION_REPEATS_INCLUDED(2)

#endif

#if (MENU_GFX_CUSTOM3_COMPRESSION_BITS == 8)
#define MENU_GFX_CUSTOM3_COMPRESSION_REPEATS MENU_GFX_COMPRESSION_REPEATS_EXTRA_BYTE

#else
#define MENU_GFX_CUSTOM3_COMPRESSION_REPEATS MENU_GFX_COMPRESSION_REPEATS_INCLUDED(3)

#endif


//MENU_GFX_CUSTOM##ID##_FETCH(gfxaddr, gfxinc, storage);

#define MENU_GFX_CUSTOM_DRAW_MACRO2(FORMAT, ID, BYTES) \
	if (imageformat == FORMAT) { \
		COLORCUSTOM##ID color = 0;\
		for (y = py; y < py+sy; y++) {\
			for (x = px; x < px+sx; x++) {\
				if (havedata == 0) { \
					havedata = 1;\
					color = menu_gfx_##BYTES##byte_fetch(gfxaddr, gfxinc, storage);\
					gfxinc += BYTES;\
					if (compressed) {\
						MENU_GFX_CUSTOM##ID##_COMPRESSION_REPEATS\
					}\
				}\
				havedata--;\
				menu_gfx_Custom##ID##_set(x, y, color);\
			}\
		}\
	}

#define MENU_GFX_CUSTOM_DRAW_MACRO1(FORMAT, ID, BYTES) MENU_GFX_CUSTOM_DRAW_MACRO2(FORMAT, ID, BYTES)
#define MENU_GFX_CUSTOM_DRAW(FORMAT, ID) MENU_GFX_CUSTOM_DRAW_MACRO1(FORMAT, ID, MENU_GFX_CUSTOM##ID##_BYTES)

#endif


static void menu_gfx(uint8_t hasfocus) {
#ifdef MENU_USE_GFX
	MENU_SCREENPOS_PX_PY
	MENU_SCREENPOS_SX_SY
	uint8_t options = menu_byte_get_next();
	menu_pc_skip(MENU_ACTION_BYTES + MENU_ADDR_BYTES); //action + windowaddr
	MENUADDR gfxaddr = menu_assemble_addr();
	MENUADDR gfxinc = 0;
	uint8_t compressed = (options >> MENU_OPTIONS_COMPRESSED) & 1;
	uint8_t storage = (options >> MENU_OPTIONS_STORAGE) & 3;
	uint8_t havedata = 0;
	SCREENPOS x, y;
#ifdef MENU_USE_MULTIGFX
	if ((options >> MENU_OPTIONS_MULTIGFX) & 1) {
		uint16_t images = menu_byte_get(gfxaddr) + (menu_byte_get(gfxaddr+1) << 8);
		uint8_t arrindex = menu_byte_get(gfxaddr+2);
		uint16_t index = menu_gfxindexstate[arrindex];
		MENU_DEBUGMSG("images: %i, arrindex: %i, index: %i\r\n", images, arrindex, index);
		if (index >= images) { //allows simple increment before each draw call
			menu_gfxindexstate[arrindex] = index = 0;
		}
		//get new gfxaddr from table (table has 3 bytes config data at beginning)
		gfxaddr = menu_assembleaddr_direct(gfxaddr+3+index*MENU_ADDR_BYTES);
	}
#endif
	if ((storage & 1) && (menu_gfxdata[gfxaddr] == NULL)) { //user forgot to add a memory -> avoid crash
		return;
	}
	uint8_t imageformat = (options >> MENU_OPTIONS_IMAGEFORMAT);
	MENU_DEBUGMSG("Drawing gfx at %i, %i, compressed: %i, storage: %i, address: %i, format: %i\n", px, py, compressed, storage, gfxaddr, imageformat);
#if defined(MENU_USE_GFXFORMAT_0)
	if (imageformat == 0) {
		SCREENCOLOR color = 0;
		uint8_t data = 0;
		for (y = py; y < py+sy; y++) {
			for (x = px; x < px+sx; x++) {
				//check if we have source data
				if (havedata == 0) { //fetch a next byte
					data = menu_gfx_1byte_fetch(gfxaddr, gfxinc, storage);
					gfxinc++;
					if (compressed == 1) {
						havedata = data & 0x7F;
						if (data >> 7) {
							color = MENU_COLOR_MAX;
						} else {
							color = 0;
						}
					} else
						havedata = 8;
				}
				if (compressed == 0) {
					if (data & 0x80) {
						color = MENU_COLOR_MAX;
					} else {
						color = 0;
					}
					data = data << 1;
				}
				havedata--;
				menu_screen_set(x, y, color);
			}
		}
  }
#endif
#if defined(MENU_USE_GFXFORMAT_1)
	if (imageformat == 1) { //4bit greyscale
		uint8_t color = 0;
		uint8_t data = 0;
		for (y = py; y < py+sy; y++) {
			for (x = px; x < px+sx; x++) {
				//check if we have source data
				if (havedata == 0) { //fetch a next byte
					data = menu_gfx_1byte_fetch(gfxaddr, gfxinc, storage);
					gfxinc++;
					if (compressed == 1) {
						havedata = (data & 0xF) + 1;
						color = data >> 4;
					} else {
						havedata = 2;
					}
				}
				if (compressed == 0) {
					color = data & 0xF;
					data = data >> 4;
				}
				havedata--;
				menu_gfx_4Bit_set(x, y, color);
			}
		}
	}
#endif
#if defined(MENU_USE_GFXFORMAT_2)
	MENU_GFX_CUSTOM_DRAW(2, 1)
#endif
#if defined(MENU_USE_GFXFORMAT_3)
	MENU_GFX_CUSTOM_DRAW(3, 2)
#endif
#if defined(MENU_USE_GFXFORMAT_4)
	MENU_GFX_CUSTOM_DRAW(4, 3)
#endif
	if ((hasfocus) || (options & (1<<MENU_OPTIONS_RECTANGLE))) //append dotted border
		menu_draw_border(px, py, sx, sy, MENU_COLOR_BORDER, hasfocus);
#else
	UNREFERENCED_PARAMETER(hasfocus);
	MENU_DEBUGMSG("Error: Gfx used, but not compiled in\n");
#endif
}

#ifdef MENU_USE_LIST

static uint16_t menu_list_lines(MENUADDR addr, uint8_t storage) {
	uint16_t lines = 0;
	uint16_t i = 0;
	char c;
	while ((c = menu_text_byte_get(addr, i++, storage)) != '\0') {
		if (c == '\n')
			lines++;
	}
	if (i) //count single string as one line too
		lines++;
	return lines;
}

static uint16_t menu_list_line_seek(MENUADDR addr, uint8_t storage, uint16_t line) {
	uint16_t i = 0;
	char c;
	while (line > 0) {
		c = menu_text_byte_get(addr, i, storage);
		if (c == '\n')
			line--;
		if (c == '\0')
			break;
		i++;
	};
	return i;
}

static uint16_t menu_list_line_next(MENUADDR baseaddr, uint16_t textpos, uint8_t storage) {
	char c;
	while(1) {
		c = menu_text_byte_get(baseaddr, textpos, storage);
		if (c == '\n') {
			textpos++;
			break;
		}
		if (c == '\0')
			break;
		textpos++;
	};
	return textpos;
}

#endif

static void menu_list(uint8_t hasfocus) {
#ifdef MENU_USE_LIST
	//this is a very complex object
	MENU_SCREENPOS_PX_PY
	MENU_SCREENPOS_SX_SY
	uint8_t options = menu_byte_get_next();
	menu_pc_skip(MENU_ACTION_BYTES + MENU_ADDR_BYTES); //action + windowaddr
	MENUADDR baseaddr  = menu_assemble_addr();
	uint8_t fonts = menu_byte_get_next();
	uint8_t listnumber = menu_byte_get_next();
	menu_pc_skip(4); //the four keys
	uint8_t storage = (options>>MENU_OPTIONS_STORAGE) & 3;
	SCREENPOS x, y;
	MENU_DEBUGMSG("Drawing list %i;%i with size %i;%i\n", px, py, sx, sy);
	//draw the text
	uint16_t textlines = menu_list_lines(baseaddr, storage); //lines of the list to display
	uint16_t selectedline = menu_listindexstate[listnumber];
	SCREENPOS fontheight = menu_font_heigth(fonts & 0x0f)+1;
	SCREENPOS linesonscreen = (sy - 3)/ (fontheight); //maximum lines which could be displayed
	/*remove the -1 if on lists which can display even number of lines, should put 3 unselected lines
	 *before* the selected one, not after
	*/
	uint16_t beginningline = selectedline - ((linesonscreen-1) / 2);
	if ((selectedline >= textlines) && (textlines)) {
			/* if textlines is zero, the selected line is not important anyway */
		 menu_listindexstate[listnumber] = selectedline = textlines;
	}
	if ((beginningline+linesonscreen) > textlines) { //crop lower end
		if (textlines > linesonscreen) { //only crop, if more lines than there is to be drawn
			beginningline = textlines-linesonscreen;
		} else //draw from the beginning, if more screen lines than lines to draw
			beginningline = 0;
	}
	if (selectedline < ((unsigned short)(linesonscreen-1) / 2)) { //for the -1 see text above
		beginningline = 0;
	}
	MENU_DEBUGMSG("textlines %i, selectedline %i, fontheigth %i, on screen %i, beginningline %i\n",
	  textlines, selectedline, fontheight, linesonscreen, beginningline);
	uint16_t textpos = menu_list_line_seek(baseaddr, storage, beginningline);
	y = py+2;
	uint16_t t;
	for (t = beginningline; t < (beginningline+linesonscreen); t++) { //now draw each line
		x = px +2;
		uint8_t font = fonts & 0x0f;
		if (t == selectedline)
			font = fonts >> 4;
		if (menu_text_byte_get(baseaddr, textpos, storage) == '\0') {
			break;
		}
		MENU_DEBUGMSG("textpos %i\n", textpos);
		menu_text_draw_base(x, y, font, options, baseaddr, textpos, sx-9);
		textpos = menu_list_line_next(baseaddr, textpos, storage);
		y += menu_font_heigth(font)+1;
	}
	//draw the surrounding
	menu_draw_border(px, py, sx, sy, MENU_COLOR_BORDER, hasfocus);
	menu_draw_Yline(px+sx-6, py+1, sy-2, MENU_COLOR_BORDER, 0);
	menu_draw_box(px+sx-5, py+1, 4, sy-2, MENU_COLOR_BACKGROUND); //clear overlapped text
	//calculate right side scroller size and position
	SCREENPOS relpos, height;
	if (textlines > 0) {
		//wont overflow, because textlines > beginningline
		//+textlines/2 just helps to reduce rounding errors
		relpos = (((uint16_t)(sy-2))*beginningline+textlines/2)/textlines;
		uint16_t bheight = (((uint16_t)(sy-2))*linesonscreen+textlines/2)/textlines;
		if (bheight > (uint16_t)sy-2) {//no lager bars, than list height
			height = sy-2;
		} else
			height = bheight; //height may be only 8 bit width
	} else {
		relpos = 0;
		height = sy-2;
	}
	if (height == 0) //make at least a scrollbar with one pixel size
		height = 1;
	MENU_DEBUGMSG("relpos %i, height %i\n", relpos, height);
	SCREENPOS abspos = py+1+relpos;
	if ((abspos+height) > (py+sy-1)) //limit lowest scrollbar position
		abspos = py+sy-1-height;
	menu_draw_box(px+sx-5, abspos, 4, height, MENU_COLOR_BORDER);
#else
	UNREFERENCED_PARAMETER(hasfocus);
	MENU_DEBUGMSG("Error: List used, but not compiled in\n");
#endif
}

static void menu_shortcut(void) {
	//non visible object
	menu_pc_skip(MENU_SHORTCUT_DATA);
}

static void menu_new_window(MENUADDR addr) {
	menu_pc_set(addr);
	uint8_t token = menu_byte_get_next();
#ifdef MENU_USE_SUBWINDOW
	if (token == MENU_SUBWINDOW) {
		menu_pc_skip(MENU_CORDSKIP); //the coordinates
		//save old focus, if last window was a normal one
		if (menu_byte_get(menu_window_init) == MENU_WINDOW)
			menu_focus_prime = menu_focus;
	}
#endif
	if ((token == MENU_WINDOW) || (token == MENU_SUBWINDOW)) {
		menu_focus_key_prev = menu_byte_get_next();
		menu_focus_key_next = menu_byte_get_next();
		menu_key_enter = menu_byte_get_next();
		uint16_t index = 0;
		uint8_t options;
		while (index < MENU_OBJECTS_MAX) {
			token = menu_byte_get_next();
			MENUADDR t = menu_pc + menu_object_datasize(token);
			menu_focus_objects[index] = 0; //for LABEL and shortcut
			switch(token) {
				case MENU_BOX:
				case MENU_BUTTON:
				case MENU_GFX:
				case MENU_LIST:
				case MENU_CHECKBOX:
				case MENU_RADIOBUTTON:
						options = menu_byte_get(menu_pc+MENU_CORDSKIP); //offset for the options in all objects
						if (options & (1<<MENU_OPTIONS_FOCUSABLE)) { //if focusable
							menu_focus_objects[index] = menu_pc-1; //start addr of object
						} else
							menu_focus_objects[index] = 0;
 						break;
				case MENU_WINDOW:
				case MENU_SUBWINDOW:
				case MENU_INVALID: goto windowend; //sorry, no double break possible
				default: ; //do nothing for the label
			}
			menu_pc_set(t);
			index++; //count amounts of objects
			//define SCHEDULE if you use a multi tasking OS and the drawing take too long
			MENU_SCHEDULE
		}
		windowend:
		menu_objects = index;
		MENU_DEBUGMSG("Objects in Window: %i\n", menu_objects);
		//check if old focus should be used
		if ((menu_focus_restore) && (menu_focus_objects[menu_focus_prime])) {
			menu_focus = menu_focus_prime;
			menu_focus_restore = 0;
		} else {
			//set up focus on first focusabel object
			menu_focus = 0;
			for (index = 0; index < menu_objects; index++) {
				if (menu_focus_objects[index] != 0) {
					menu_focus = index;
					break;
				}
			}
		}
	} else {
		MENU_DEBUGMSG("Error: token %i is invalid for a new window\n", token);
	}
	MENU_DEBUGMSG("new_window called: First focus: %i\n", menu_focus);
	menu_window_init = addr;
}

//draws two times in the case of a window with a sub window
static void menu_draw_windowcontent(uint8_t focusallow) {
	uint16_t index = 0;
	while (index <= MENU_OBJECTS_MAX) {
		uint8_t token = menu_byte_get_next();
		uint8_t hasfocus = 0;
		if ((index == menu_focus) && (menu_focus_objects[index]) && (focusallow)) {
			hasfocus = 1;
		}
		switch(token) {
			case MENU_BOX: menu_box(hasfocus); break;
			case MENU_LABEL: menu_label(); break;
			case MENU_BUTTON: menu_button(hasfocus); break;
			case MENU_GFX: menu_gfx(hasfocus); break;
			case MENU_LIST: menu_list(hasfocus); break;
			case MENU_CHECKBOX: menu_checkbox(hasfocus); break;
			case MENU_RADIOBUTTON: menu_radiobutton(hasfocus); break;
			case MENU_SHORTCUT: menu_shortcut(); break;
			default: return;
		}
		index++; //count amounts of objects
	}
}

void menu_redraw(void) {
	//skip over possible global shortcuts if menu_window_init = 0
	if (menu_window_init == 0) { //first start
		uint8_t token = menu_byte_get(menu_window_start);
		while (token == MENU_SHORTCUT) {
			//should only happen at the beginning for global shortcuts
			menu_window_start += menu_object_datasize(MENU_SHORTCUT)+1;
			token = menu_byte_get(menu_window_start);
		}
	}
	menu_screen_clear();
#ifdef MENU_USE_SUBWINDOW
	MENU_DEBUGMSG("window init: %i, subwindow start: %i, window start: %i\n", menu_window_init, menu_subwindow_start, menu_window_start);
	if (menu_subwindow_start) {
		if (menu_window_init != menu_subwindow_start) {
			menu_new_window(menu_subwindow_start);
		}
	} else if (menu_window_init != menu_window_start) {
		menu_new_window(menu_window_start);
	}
#else
	MENU_DEBUGMSG("window init: %i, window start: %i\n", menu_window_init, menu_window_start);
	if (menu_window_init != menu_window_start) {
		menu_new_window(menu_window_start);
	}
#endif
	//draw window
	menu_pc_set(menu_window_start);
	uint8_t token = menu_byte_get_next();
	if (token == MENU_WINDOW) {
		uint8_t allowfocus = 1;
#ifdef MENU_USE_SUBWINDOW
		if (menu_subwindow_start) {
			allowfocus = 0;
		}
#endif
		menu_pc_skip(MENU_WINDOW_DATA);
		menu_draw_windowcontent(allowfocus);
	} else {
		MENU_DEBUGMSG("Error: Not a window\n");
	}
#ifdef MENU_USE_SUBWINDOW
	//draw subwindow, if enabled
	if (menu_subwindow_start) {
		menu_pc_set(menu_subwindow_start);
		token = menu_byte_get_next();
		if (token == MENU_SUBWINDOW) { //checking could be optimized out
			MENU_SCREENPOS_PX_PY
			MENU_SCREENPOS_SX_SY
			menu_draw_box(px, py, sx, sy, MENU_COLOR_SUBWINDOW_BACKGROUND);
			menu_draw_border(px, py, sx, sy, MENU_COLOR_SUBWINDOW_BORDER, 0);
			menu_pc_skip(3); //three byte for the keys
			menu_draw_windowcontent(1);
		} else {
			MENU_DEBUGMSG("Error: Not a subwindow\n");
		}
	}
#endif
	menu_screen_flush();
}

static void menu_run_action(MENUADDR addr) {
	menu_pc_set(addr);
	MENU_DEBUGMSG("Reading action from adress %i\n", addr);
	MENUACTION action = menu_byte_get_next();
#if (MENU_ACTION_BYTES == 2)
  action |= (menu_byte_get_next() << 8);
#endif
	MENUADDR news = menu_assemble_addr();
	uint8_t wishredraw = menu_action(action);
	if (news != 0) { //if new window switch
#ifdef MENU_USE_SUBWINDOW
		if (news == MENU_SUBWINDOW_RET) { //special value used for subwindow close
			menu_subwindow_start = 0;
			menu_focus_restore = 1;
		} else {
			uint8_t wtype = menu_byte_get(news);
			if (wtype == MENU_SUBWINDOW) { //look if new window is a subwindow or normal one
				menu_subwindow_start = news;
			} else {
				menu_window_start = news;
				menu_subwindow_start = 0;
			}
		}
#else
		menu_window_start = news;
#endif
	}
	if (news || wishredraw)
		menu_redraw();
}

#ifdef MENU_USE_CHECKBOX

static void menu_handle_checkbox(MENUADDR addr) {
	uint8_t index = menu_byte_get(addr + MENU_CKRAD_OFF);
	if (index < MENU_CHECKBOX_MAX)
		menu_checkboxstate[index] = 1-menu_checkboxstate[index];
	menu_redraw(); //Improvement: May be faster if only the checkbox gets redrawn
}

#endif

#ifdef MENU_USE_RADIOBUTTON

static void menu_handle_radiobutton(MENUADDR addr) {
	uint8_t groupindex = menu_byte_get(addr + MENU_CKRAD_OFF);
	uint8_t value = groupindex >> 4;
	groupindex &= 0x0f;
	if (groupindex < MENU_RADIOBUTTON_MAX)
		menu_radiobuttonstate[groupindex] = value;
	menu_redraw(); //Improvement: May be faster if only the radiobutton gets redrawn
}

#endif

#ifdef MENU_USE_LIST

static void menu_handle_listbox(MENUADDR addr, uint8_t key) {
	menu_pc_set(addr+MENU_LISTPOS_OFF);
	SCREENPOS sy = menu_byte_get_next();
#ifdef LARGESCREEN
	uint8_t lsxy = menu_byte_get_next();
	sy += (SCREENPOS)(lsxy & 0x0F) << 8;
#endif
	uint8_t options = menu_byte_get_next();
	uint8_t storage = (options>>MENU_OPTIONS_STORAGE) & 3;
	MENUACTION action = menu_byte_get_next();
#if (MENU_ACTION_BYTES >= 2)
	action |= menu_byte_get_next() << 8;
#endif
	menu_pc_skip(MENU_ADDR_BYTES); //windowaddr
	MENUADDR baseaddr  = menu_assemble_addr();
	uint8_t fonts = menu_byte_get_next();
	uint16_t listindex = menu_byte_get_next();
	SCREENPOS fontheight = menu_font_heigth(fonts & 0x0f)+1;
	SCREENPOS linesonscreen = (sy - 3)/ (fontheight);
	if (listindex < MENU_LIST_MAX) { //if index is valid
		unsigned short nvalue = menu_listindexstate[listindex];
		unsigned short ovalue = nvalue;
		if (menu_byte_get_next() == key) {
			nvalue++;
		}
		if (menu_byte_get_next() == key) {
			nvalue--;
		}
		if (menu_byte_get_next() == key) { //page down key
			nvalue += linesonscreen;
		}
		if (menu_byte_get_next() == key) { //page up key
			nvalue -= linesonscreen;
		}
		if (ovalue != nvalue) { //a quick pre-checking
			uint16_t textlines = menu_list_lines(baseaddr, storage);
			if (nvalue > 32000) //indicates an underflow
				nvalue = 0;
			if (nvalue >= textlines) //indicaes an overflow
				nvalue = textlines-1;
			if (nvalue != menu_listindexstate[listindex]) {
				menu_listindexstate[listindex] = nvalue;
				if (action)
				{
					menu_action(action + 1); //the index change action is always +1 of the normal enter action
				}
				menu_redraw();
			}
		}
	}
}

#endif

void menu_keypress(uint8_t key) {
	if (key == 0)
		return;
	//look if key is the enter key
	MENUADDR focusaddr = menu_focus_objects[menu_focus];
	if (focusaddr) { //if there is at least one object with a focus
#if defined(MENU_USE_LIST) || defined(MENU_USE_CHECKBOX) || defined(MENU_USE_RADIOBUTTON)
		uint8_t obj = menu_byte_get(focusaddr);
#endif
		//look if this is a listbox
#ifdef MENU_USE_LIST
		if (obj == MENU_LIST) {
			menu_handle_listbox(focusaddr, key);
		}
#endif
		if (key == menu_key_enter) {
#ifdef MENU_USE_CHECKBOX
			if (obj == MENU_CHECKBOX)
				menu_handle_checkbox(focusaddr);
#endif
#ifdef MENU_USE_RADIOBUTTON
			if (obj == MENU_RADIOBUTTON)
				menu_handle_radiobutton(focusaddr);
#endif
			menu_run_action(focusaddr+MENU_ACTION_OFF); //offset for all focusable objects where action and windows are starting
			return;
		}
	}
	//look if key swiches the focus
	if (key == menu_focus_key_prev) {
		uint8_t i;
		for (i = 0; i < menu_objects; i++) { //seek next object which is focusable
			menu_focus--;
			if (menu_focus >= menu_objects)
				menu_focus = menu_objects-1;
			if (menu_focus_objects[menu_focus] != 0) //found one
				break;
		}
		MENU_DEBUGMSG("New focus: %i\n", menu_focus);
		menu_redraw();
		return;
	}
	if (key == menu_focus_key_next) {
		uint8_t i;
		for (i = 0; i < menu_objects; i++) { //seek next object which is focusable
			menu_focus++;
			if (menu_focus >= menu_objects)
				menu_focus = 0;
			if (menu_focus_objects[menu_focus] != 0) //found one
				break;
		}
		MENU_DEBUGMSG("New focus: %i\n", menu_focus);
		menu_redraw();
		return;
	}
	//look if key is part of a shortcut
	uint8_t windowtoken = menu_byte_get(menu_window_init);
	MENUADDR p = menu_window_init+menu_object_datasize(windowtoken)+1;
	uint8_t token;
	while (1) {
		token = menu_byte_get(p);
		if (token == MENU_SHORTCUT) {
			if (key == menu_byte_get(p+1)) {
				MENU_DEBUGMSG("Found proper shortcut at %i, key: %i\n", p, key);
				menu_run_action(p+2);
				return;
			} //else
				//printf("Found other shortcut at %i with key %i\n", p, menu_byte_get(p+1));
		} else if ((token == MENU_WINDOW) || (token == MENU_SUBWINDOW) || (token == MENU_INVALID)) {
			break;
		}
		p += menu_object_datasize(token)+1;
	}
  //look if key is part of a global shortcut
	p = 1;
	while (1) {
		token = menu_byte_get(p);
		if (token == MENU_SHORTCUT) {
			if (key == menu_byte_get(p+1)) {
				MENU_DEBUGMSG("Found proper global shortcut at %i, key: %i\n", p, key);
				menu_run_action(p+2);
				return;
			} //else
				//printf("Found other shortcut at %i with key %i\n", p, menu_byte_get(p+1));
		} else {
			break;
		}
		p += menu_object_datasize(token)+1;
	}
}

#ifdef MENU_MOUSE_SUPPORT

void menu_mouse(SCREENPOS x, SCREENPOS y, uint8_t key) {
	MENUADDR elemmatch = 0;
	MENUADDR seekpos = 0;
	//check if sub window active or not
	seekpos = menu_window_start;
#ifdef MENU_USE_SUBWINDOW
	if (menu_subwindow_start) {
		seekpos = menu_subwindow_start;
	}
#endif
	//jump over first window definitions
	seekpos += menu_object_datasize(menu_byte_get(seekpos))+1;
	//check objects, remember last found object
	while(1) {
		uint8_t obj = menu_byte_get(seekpos);
		if ((obj == MENU_BOX) || (obj == MENU_BUTTON) || (obj == MENU_GFX) ||
		    (obj == MENU_LIST) || (obj == MENU_CHECKBOX) || (obj == MENU_RADIOBUTTON)) {
			MENUADDR spos2 = seekpos+1;
			SCREENPOS px = menu_byte_get(spos2++);
			SCREENPOS py = menu_byte_get(spos2++);
#ifdef LARGESCREEN
			uint8_t lpxy = menu_byte_get(spos2++);
			px += (SCREENPOS)(lpxy & 0xF0) << 4;
			py += (SCREENPOS)(lpxy & 0x0F) << 8;
#endif
			SCREENPOS sx = menu_byte_get(spos2++);
			SCREENPOS sy = menu_byte_get(spos2++);
#ifdef LARGESCREEN
			uint8_t lsxy = menu_byte_get(spos2++);
			sx += (SCREENPOS)(lsxy & 0xF0) << 4;
			sy += (SCREENPOS)(lsxy & 0x0F) << 8;
#endif
			uint8_t focusable = menu_byte_get(spos2) & 1;
			if (focusable) {
				//compare if position fits
				if ((x >= px) && (x < (px+sx)) && (y >= py) && (y < (py+sy))) {
					elemmatch = seekpos;
				}
			}
		} else if ((obj != MENU_LABEL) && (obj != MENU_SHORTCUT)) {
				break;
		}
		seekpos += menu_object_datasize(obj)+1;
	}
	//run action
	if (elemmatch) {
#if defined(MENU_USE_LIST) || defined(MENU_USE_CHECKBOX) || defined(MENU_USE_RADIOBUTTON)
		uint8_t obj = menu_byte_get(elemmatch);
#endif
#ifdef MENU_USE_LIST
		if (obj == MENU_LIST) {
			menu_handle_listbox(elemmatch, key);
		}
#endif
		if (key == menu_key_enter) {
#ifdef MENU_USE_CHECKBOX
			if (obj == MENU_CHECKBOX)
				menu_handle_checkbox(elemmatch);
#endif
#ifdef MENU_USE_RADIOBUTTON
			if (obj == MENU_RADIOBUTTON)
				menu_handle_radiobutton(elemmatch);
#endif
			menu_run_action(elemmatch+MENU_ACTION_OFF); //offset for all focusable objects where action and windows are starting
			return;
		}
	}
}

#endif


