/* Boxlib emulation
(c) 2022 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause
*/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <pthread.h>

#ifdef COMPILE_WINDOWS
#include <windef.h>
#endif
#include <GL/gl.h>
#include <GL/glut.h>
#include <GL/freeglut.h>

#include "lcd.h"

#include "rs232debug.h"


#include "peripheral.h"
#include "main.h"

#ifndef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(P) (void)(P)
#endif

//2.0 -> screen goes from border to border.
//1.0 -> 0% space on each side, so the value must be above 1.0
#define SCREENFILL ((float)1.80)
#define SCREENOFFSET (SCREENFILL/2)

#define LCD_SCREEN_MAX_X 320
#define LCD_SCREEN_MAX_Y 240

/*TODO: Make configurable with a file. The default values are suitable for
  a 16 bit screen */

#ifndef LCD_COLOR_OUT_RED_BITS
#define LCD_COLOR_OUT_RED_BITS 5
#endif

#ifndef LCD_COLOR_OUT_GREEN_BITS
#define LCD_COLOR_OUT_GREEN_BITS 6
#endif

#ifndef LCD_COLOR_OUT_BLUE_BITS
#define LCD_COLOR_OUT_BLUE_BITS 5
#endif

#define LCD_COLOR_OUT_RED_LSB_POS (LCD_COLOR_OUT_GREEN_BITS + LCD_COLOR_OUT_BLUE_BITS)
#define LCD_COLOR_OUT_GREEN_LSB_POS (LCD_COLOR_OUT_BLUE_BITS)
#define LCD_COLOR_OUT_BLUE_LSB_POS (0)

#define LCD_COLOR_OUT_RED_MASK (((1<<LCD_COLOR_OUT_RED_BITS) -1) << (LCD_COLOR_OUT_GREEN_BITS + LCD_COLOR_OUT_BLUE_BITS))
#define LCD_COLOR_OUT_GREEN_MASK  (((1<<LCD_COLOR_OUT_GREEN_BITS) -1) << (LCD_COLOR_OUT_BLUE_BITS))
#define LCD_COLOR_OUT_BLUE_MASK ((1<<LCD_COLOR_OUT_BLUE_BITS) -1)


bool g_lcdEnabled;

eDisplay_t g_lcdType;

pthread_t g_guiThread;

pthread_mutex_t g_guiMutex;


//Pixels on the simulated LCD
uint32_t g_lcdWidth;
uint32_t g_lcdHeight;

//Pixels on the PC monitor
uint32_t g_dispx = 800;
uint32_t g_dispy = 600;

int g_redrawMax = 50; //redraw every x loops
int g_loopCycle = 0;
int g_loopMs = 10;
bool g_dataChanged;

float g_screen[LCD_SCREEN_MAX_Y][LCD_SCREEN_MAX_X][3];
bool g_backlightOn;

bool g_keyLeft;
bool g_keyRight;
bool g_keyUp;
bool g_keyDown;

//0 = off, 1 = red, 2 = green, 3 = yellow
//LED 0 is always off, because its from the coprocessor
#define LEDS_NUM 3
uint8_t g_ledState[LEDS_NUM];

bool KeyLeftPressed(void) {
	bool v;
	pthread_mutex_lock(&g_guiMutex);
	v = g_keyLeft;
	g_keyLeft = false;
	pthread_mutex_unlock(&g_guiMutex);
	return v;
}

bool KeyRightPressed(void) {
	bool v;
	pthread_mutex_lock(&g_guiMutex);
	v = g_keyRight;
	g_keyRight = false;
	pthread_mutex_unlock(&g_guiMutex);
	return v;
}

bool KeyUpPressed(void) {
	bool v;
	pthread_mutex_lock(&g_guiMutex);
	v = g_keyUp;
	g_keyUp = false;
	pthread_mutex_unlock(&g_guiMutex);
	return v;
}

bool KeyDownPressed(void) {
	bool v;
	pthread_mutex_lock(&g_guiMutex);
	v = g_keyDown;
	g_keyDown = false;
	pthread_mutex_unlock(&g_guiMutex);
	return v;
}


void LcdEnable(uint32_t clockPrescaler) {
	(void)clockPrescaler;
	PeripheralPowerOn();
	g_lcdEnabled = true;
}

void LcdDisable(void) {
	if (g_lcdEnabled) {
		g_lcdEnabled = false;
		if (g_guiThread) {
			pthread_join(g_guiThread, NULL);
			pthread_mutex_destroy(&g_guiMutex);
		}
	}
}

void LcdBacklightOn(void) {
	g_backlightOn = true;
}

void LcdBacklightOff(void) {
	g_backlightOn = false;
}

static void update_window_size(int width, int height) {
	if ((g_dispx != width) || (g_dispy != height)) {
		glViewport(0, 0, width, height);
		glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
		g_dispx = width;
		g_dispy = height;
	}
}

static void CalcScaleAndOffset(float x, float y, float * xoffset, float * yoffset, float * xscale, float * yscale) {
	float screenstart = 1.0 - SCREENFILL;
	float screenwidth = SCREENFILL - (2.0 - SCREENFILL);
	*xscale = screenwidth/(float)g_lcdWidth/2.0;
	*yscale = screenwidth/(float)g_lcdHeight/2.0; //div by 2 because its a radius
	*xoffset = screenstart + screenwidth/(float)g_lcdWidth * x + (screenwidth/2.0)/(float)g_lcdWidth;
	*yoffset = screenstart + screenwidth/(float)g_lcdHeight * y + (screenwidth/2.0)/(float)g_lcdHeight;
	*yoffset *= -1.0;
}

static void drawRoundDot(float x, float y, float r, float g, float b) {
	float xoffset, yoffset, xscale, yscale;
	glPushMatrix();
	glColor3f(r,g,b);
	CalcScaleAndOffset(x, y, &xoffset, &yoffset, &xscale, &yscale);
	xscale *= 8.00;
	yscale *= 8.00;
	glBegin(GL_TRIANGLE_FAN);
	glVertex2f(xoffset, yoffset);
	for (float i = 0; i < ((M_PI * 2.0) + 0.0001); i += (M_PI/8.0))
	{
		float c = cosf(i);
		float s = sinf(i);
		glVertex2f(xoffset + xscale * s, yoffset + yscale * c);
	}
	glEnd();
	glPopMatrix();
}

static void DrawRectangle(float x, float y, float r, float g, float b) {
	float xoffset, yoffset, xscale, yscale;
	glPushMatrix();
	glColor3f(r,g,b);
	CalcScaleAndOffset(x, y, &xoffset, &yoffset, &xscale, &yscale);
	glBegin(GL_TRIANGLE_FAN);
	glVertex2f(xoffset + xscale, yoffset - yscale);
	glVertex2f(xoffset - xscale, yoffset - yscale);
	glVertex2f(xoffset - xscale, yoffset + yscale);
	glVertex2f(xoffset + xscale, yoffset + yscale);
	glVertex2f(xoffset - xscale, yoffset + yscale);
	glEnd();
	glPopMatrix();
}

static void redraw(int param) {
	UNREFERENCED_PARAMETER(param);
	g_loopCycle++;
	pthread_mutex_lock(&g_guiMutex);
	bool changed = false;
	if ((g_dataChanged) || (g_loopCycle >= g_redrawMax))
	{
		changed = true;
		glClear(GL_COLOR_BUFFER_BIT);
		for (uint32_t y = 0; y < g_lcdHeight; y++) {
			for (uint32_t x = 0; x < g_lcdWidth; x++) {
				float r = g_screen[y][x][0];
				float g = g_screen[y][x][1];
				float b = g_screen[y][x][2];
				if (g_backlightOn == false) {
					r /= 2.0;
					g /= 2.0;
					b /= 2.0;
				}
				DrawRectangle(x, y, r, g, b);
			}
		}
		for (uint32_t i = 0; i < LEDS_NUM; i++) {
			float r, g, b;
			switch (g_ledState[i]) {
				case 1:  r = 1.0; g = 0.0; b = 0.0; break;
				case 2:  r = 0.0; g = 1.0; b = 0.0; break;
				case 3:  r = 1.0; g = 1.0; b = 0.0; break;
				default: r = 0.2; g = 0.2; b = 0.2; break;
			}
			drawRoundDot(g_lcdWidth - LEDS_NUM * 16.0 + i * 16.0, g_lcdHeight + 8.0, r, g, b);
		}
		g_dataChanged = false;
		g_loopCycle = 0;
	}
	pthread_mutex_unlock(&g_guiMutex);
	if (changed) {
		glutSwapBuffers();
		glFlush();
	}
	if (g_lcdEnabled) {
		glutTimerFunc(g_loopMs, redraw, 0);
	} else {
		glutLeaveMainLoop();
	}
}

void input_key_special(int key, int x, int y) {
	UNREFERENCED_PARAMETER(x);
	UNREFERENCED_PARAMETER(y);
	pthread_mutex_lock(&g_guiMutex);
	if (key == GLUT_KEY_LEFT) {
		g_keyLeft = true;
		//printf("left key pressed\n");
	}
	if (key == GLUT_KEY_RIGHT) {
		g_keyRight = true;
		//printf("right key pressed\n");

	}
	if (key == GLUT_KEY_UP) {
		g_keyUp = true;
		//printf("up key pressed\n");
	}
	if (key == GLUT_KEY_DOWN) {
		g_keyDown = true;
		//printf("down key pressed\n");
	}
	pthread_mutex_unlock(&g_guiMutex);
}

void GlutWindowClosed(void) {
	printf("Window closed, terminating application\n");
	Rs232Stop(); //needs to reset the terminal
}


void * GlutGui(void * parameter) {
	int argc = 1;
	char * argv = "";
	glutInit(&argc, &argv);

	g_dispx = g_lcdWidth * 4;
	g_dispy = g_lcdHeight * 4;

	unsigned int displaymode = GLUT_RGBA | GLUT_DOUBLE | GLUT_MULTISAMPLE;
	glutInitDisplayMode(displaymode);
	glEnable(GL_MULTISAMPLE);
	glutInitWindowSize(g_dispx, g_dispy);
	glutInitWindowPosition(100,20);
	char title[128];
	snprintf(title, sizeof(title), "LCD simulation %ux%u", (unsigned int)g_lcdWidth, (unsigned int)g_lcdHeight);
	glutCreateWindow(title);
	glutReshapeFunc(update_window_size);
	glutSpecialFunc(input_key_special);
	glutCloseFunc(&GlutWindowClosed);
	glutTimerFunc(g_loopMs, redraw, 0);
	glClearColor(0.0,0.0,0.0,0.0);
	glutMainLoop(); //does not return
	pthread_exit(0);
	return NULL;
}

void LcdInit(eDisplay_t lcdType) {
	if (!g_lcdEnabled) {
		printf("Error, LCD must be enabled before init can be called\n");
		return;
	}
	g_lcdType = lcdType;
	//g_lcdWidht may not exceed LCD_SCREEN_MAX_X
	//g_lcdHeight may not exceed LCD_SCREEN_MAX_Y
	if (g_lcdType == ST7735_128) {
		g_lcdWidth = 128;
		g_lcdHeight = 128;
	}
	if (g_lcdType == ST7735_160) {
		g_lcdWidth = 160;
		g_lcdHeight = 128;
	}
	if (g_lcdType == ILI9341) {
		g_lcdWidth = 320;
		g_lcdHeight = 240;
	}
	if (pthread_mutex_init(&g_guiMutex, NULL) == 0) {
		pthread_create(&g_guiThread, NULL, &GlutGui, NULL);
		LcdTestpattern();
	} else {
		printf("Error, internal\n");
	}
}

void LcdWritePixel(uint16_t x, uint16_t y, uint16_t color) {
		//the physical LCD needs the colors in msb first, so we have to convert back here
		color = (color << 8) | (color >> 8);
		if ((x < LCD_SCREEN_MAX_X) && (y < LCD_SCREEN_MAX_Y)) {
		float r = 0.0, g = 0.0, b = 0.0;
		uint32_t ri = (color & LCD_COLOR_OUT_RED_MASK) >> LCD_COLOR_OUT_RED_LSB_POS;
		uint32_t gi = (color & LCD_COLOR_OUT_GREEN_MASK) >> LCD_COLOR_OUT_GREEN_LSB_POS;
		uint32_t bi = (color & LCD_COLOR_OUT_BLUE_MASK) >> LCD_COLOR_OUT_BLUE_LSB_POS;
		float rmax = (1<<LCD_COLOR_OUT_RED_BITS) -1;
		float gmax = (1<<LCD_COLOR_OUT_GREEN_BITS) -1;
		float bmax = (1<<LCD_COLOR_OUT_BLUE_BITS) -1;
		//printf("%x %i %f %u %x\n", color, ri, rmax, MENU_COLOR_OUT_RED_LSB_POS, MENU_COLOR_OUT_RED_MASK);
		if (rmax) {
			r = 1.0/rmax*(float)ri;
		}
		if (gmax) {
			g = 1.0/gmax*(float)gi;
		}
		if (bmax) {
			b = 1.0/bmax*(float)bi;
		}
		pthread_mutex_lock(&g_guiMutex);
		g_screen[y][x][0] = r;
		g_screen[y][x][1] = g;
		g_screen[y][x][2] = b;
		g_dataChanged = true;
		pthread_mutex_unlock(&g_guiMutex);
	}
}

void LcdDrawHLine(uint16_t color, uint16_t x, uint16_t y, uint16_t length) {
	for (uint32_t i = 0; i < length; i++) {
		LcdWritePixel(x + i, y, color);
	}
}

void LcdDrawVLine(uint16_t color, uint16_t x, uint16_t y, uint16_t length) {
	for (uint32_t i = 0; i < length; i++) {
		LcdWritePixel(x, y + i, color);
	}
}

void LcdWriteRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint8_t * data, size_t len) {
	for (uint32_t yi = y; yi < (y + height); yi++) {
		for (uint32_t xi = x; xi < (x + width); xi++) {
			uint16_t color = (*data) + ((*(data + 1)) << 8);
			LcdWritePixel(xi, yi, color);
			if (len >= 2) {
				data += 2;
				len -= 2;
			}
		}
	}
}

void LcdWaitBackgroundDone(void) {
}

//shows colored lines, a black square in the upper left and a box around it
void LcdTestpattern(void) {
	uint16_t height = g_lcdHeight;
	uint16_t width = g_lcdWidth;
	for (uint16_t y = 0; y < height; y++) {
		uint16_t rgb = 1 << (y % 16);
		LcdDrawHLine(rgb, 0, y, width);
	}
	//black square marks the top left
	for (uint16_t y = 0; y < 48; y++) {
		LcdDrawHLine(0, 0, y, 48);
	}
	//next to it a red sqare
	for (uint16_t y = 0; y < 48; y++) {
		LcdDrawHLine(0xF800, 48, y, 16);
	}
	//a green one
	for (uint16_t y = 0; y < 48; y++) {
		LcdDrawHLine(0x07E0, 64, y, 16);
	}
	//a blue one
	for (uint16_t y = 0; y < 48; y++) {
		LcdDrawHLine(0x001F, 80, y, 16);
	}
	//fading bars
	for (uint16_t i = 0; i < 64; i++) {
		uint16_t r = (i >> 1) << 11; //5bit red
		uint16_t g = (i >> 0) << 5; //6bit green
		uint16_t b = (i >> 1); //5bit red
		LcdDrawVLine(r, 2 + i, 48, 4);
		LcdDrawVLine(g, 2 + i, 52, 4);
		LcdDrawVLine(b, 2 + i, 56, 4);
	}
	//white-dark green border marks the outer layer
	LcdDrawHLine(0xFFFF, 0, 0, width);
	LcdDrawHLine(0xFFFF, 0, height - 1, width);
	LcdDrawVLine(0xFFFF, 0, 0, height);
	LcdDrawVLine(0xFFFF, width - 1, 0, height);
	LcdDrawHLine(0x07E0, 1, 1, width - 2);
	LcdDrawHLine(0x07E0, 1, height - 2, width - 2);
	LcdDrawVLine(0x07E0, 1, 1, height - 2);
	LcdDrawVLine(0x07E0, width - 2, 1, height - 2);
}

static void GuiNotifyChange(void) {
	if (pthread_mutex_lock(&g_guiMutex) == 0) {
		g_dataChanged = true;
		pthread_mutex_unlock(&g_guiMutex);
	}
}

void Led1Red(void) {
	g_ledState[1] = 1;
	GuiNotifyChange();
}

void Led1Green(void) {
	g_ledState[1] = 2;
	GuiNotifyChange();
}

void Led1Yellow(void) {
	g_ledState[1] = 3;
	GuiNotifyChange();
}

void Led1Off(void) {
	g_ledState[1] = 0;
	GuiNotifyChange();
}

void Led2Red(void) {
	g_ledState[2] = 1;
	GuiNotifyChange();
}

void Led2Green(void) {
	g_ledState[2] = 2;
	GuiNotifyChange();
}

void Led2Yellow(void) {
	g_ledState[2] = 3;
	GuiNotifyChange();
}

void Led2Off(void) {
	g_ledState[2] = 0;
	GuiNotifyChange();
}

