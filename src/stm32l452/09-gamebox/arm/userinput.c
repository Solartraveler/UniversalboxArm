/*
   Gamebox
    Copyright (C) 2004-2006  by Malte Marwedel
    m.talk AT marwedels dot de

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdbool.h>

#include "main.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "boxlib/keys.h"
#include "boxlib/keysIsr.h"
#include "boxlib/simpleadc.h"
#include "utility.h"
#include "filesystem.h"
#include "json.h"

#define JOYSTICK_FILENAME "/etc/joystick.json"
#define FILE_MAX 256

#define INPUT_PORT GPIOC
#define INPUT_KEY_PIN GPIO_PIN_3
#define INPUT_X_PIN GPIO_PIN_0
#define INPUT_X_ADC 1
#define INPUT_Y_PIN GPIO_PIN_1
#define INPUT_Y_ADC 2

#define INP_LINEARIZE 4095000
#define INP_SCALE 200
#define INP_LIM 127
#define INP_SNAP_IN 96
#define INP_SNAP_OUT 32



struct userinputstruct userin;
struct userinputcalibstruct calib_x;
struct userinputcalibstruct calib_y;

const char input_calib_text1[] PROGMEM = "All sides move, press key in center";
const char input_calib_text2[] PROGMEM = "Cali:";

SemaphoreHandle_t g_inputSemaphore;
StaticSemaphore_t g_inputSemaphoreState;


void InputLock(void) {
	while (!xSemaphoreTake(g_inputSemaphore, 1000));
}

void InputUnlock(void) {
	xSemaphoreGive(g_inputSemaphore);
}

#if modul_calib_save

const char input_calib_save1[] PROGMEM = "Save joystick calib in file?";
const char input_calib_no[] PROGMEM = "No";
const char input_calib_yes[] PROGMEM = "Yes";
const char input_calib_save2[] PROGMEM = "Values saved";

void calib_load(void) {
	uint8_t jsonfile[FILE_MAX] = {0};
	size_t r = 0;
	char value[8];
	uint8_t num = 0;
	if (FilesystemReadFile(JOYSTICK_FILENAME, jsonfile, sizeof(jsonfile) - 1, &r)) {
		InputLock();
		if (JsonValueGet(jsonfile, r, "centerX1", value, sizeof(value))) {
			calib_x.zero = MAX(atoi(value), 1);
			num++;
		}
		if (JsonValueGet(jsonfile, r, "minX1", value, sizeof(value))) {
			calib_x.min = MAX(atoi(value), 1);
			num++;
		}
		if (JsonValueGet(jsonfile, r, "maxX1", value, sizeof(value))) {
			calib_x.max = MAX(atoi(value), 1);
			num++;
		}
		if (JsonValueGet(jsonfile, r, "center1", value, sizeof(value))) {
			calib_y.zero = MAX(atoi(value), 1);
			num++;
		}
		if (JsonValueGet(jsonfile, r, "minY1", value, sizeof(value))) {
			calib_y.min = MAX(atoi(value), 1);
			num++;
		}
		if (JsonValueGet(jsonfile, r, "maxY1", value, sizeof(value))) {
			calib_y.max = MAX(atoi(value), 1);
			num++;
		}
		InputUnlock();
		printf("%u calibration values loaded\r\n", num);
	}
}

void calib_save(void) {
	u08 accepted = 0;
	clear_screen();
	load_text(input_calib_save1);
	scrolltext(0, 0x03, 0, 120);
	load_text(input_calib_no);
	draw_string(1, 8, 0x31, 0, 1);
	while (userin_press() == 0) {
		if ((userin_right()) && (accepted == 0)) { //Yes
			accepted = 1;
			load_text(input_calib_yes);
			draw_box(0, 8, 16, 8, 0x00, 0x00);
		}
		if (userin_left() || userin_up() || userin_down()) { //No
			accepted = 0;
			load_text(input_calib_no);
			draw_box(0, 8, 16, 8, 0x00, 0x00);
		}
		draw_string(1, 8, 0x31, 0, 1);
	}
	if (accepted == 1) { //save to filesystem
		char buffer[FILE_MAX];
		snprintf(buffer, sizeof(buffer), "{\
		\n  \"centerX1\": \"%u\",\n  \"minX1\": \"%u\",\n  \"maxX1\": \"%u\",\n\
		  \"centerY1\": \"%u\",\n  \"minY1\": \"%u\",\n  \"maxY1\": \"%u\",\n}",
		         calib_x.zero, calib_x.min, calib_x.max,
		         calib_y.zero, calib_y.min, calib_y.max);
		if (FilesystemWriteEtcFile(JOYSTICK_FILENAME, buffer, strlen(buffer))) {
			printf("Saved to %s\r\n", JOYSTICK_FILENAME);
			load_text(input_calib_save2);
			scrolltext(3,0x13,0,120);
		} else {
			printf("Error, could not create file %s\r\n", JOYSTICK_FILENAME);
		}
		waitms(500);
		userin_flush();
	}
}

#endif

void input_calib(void) {
	uint16_t min_x = 4096, min_y = 4096;
	uint16_t max_x = 1, max_y = 1;
	uint16_t medium_x,medium_y;
	uint16_t temp;
	static uint8_t input_calib_ignoretext;

	if (input_calib_ignoretext == 0) {
		input_calib_ignoretext = 1;
		load_text(input_calib_text1);
		scrolltext(0, 0x03, 0, 100);
		waitms(250);
	}
	load_text(input_calib_text2);
	draw_box(0, 0, 16, 8, 0x00, 0x00); //clear the text
	draw_string(0, 0, 0x03, 0, 1);
	InputLock();
	while (HAL_GPIO_ReadPin(INPUT_PORT, INPUT_KEY_PIN) == GPIO_PIN_SET) {
		//Calib of X
		temp = AdcGet(INPUT_X_ADC);
		showbin(9, temp, 0x30);
		if (temp < min_x) {
			min_x = temp;
		}
		if (temp > max_x) {
			max_x = temp;
		}
		//Calib of Y
		temp = AdcGet(INPUT_Y_ADC);
		showbin(13, temp, 0x30);
		if (temp < min_y) {
			min_y = temp;
		}
		if (temp > max_y) {
			max_y = temp;
		}
		//Show as binary
		showbin(8 , min_x, 0x03);
		showbin(10, max_x, 0x03);
		showbin(12, min_y, 0x03);
		showbin(14, max_y, 0x03);
	}
	//Measure the centre
	medium_x = AdcGet(INPUT_X_ADC);
	medium_y = AdcGet(INPUT_Y_ADC);;
	//No value may be zero, as it will be used as divider
	if (min_x == 0) {
		min_x = 1;
	}
	if (min_y == 0) {
		min_y = 1;
	}
	if (medium_x == 0) {
		medium_x = 1;
	}
	if (medium_y == 0) {
		medium_y = 1;
	}
	//calc calibration values
	calib_x.zero = INP_LINEARIZE/medium_x;
	calib_y.zero = INP_LINEARIZE/medium_y;
	calib_x.max = (INP_LINEARIZE/max_x - calib_x.zero) *(-1); // *(-1) for positive value
	calib_y.max = (INP_LINEARIZE/max_y - calib_y.zero) *(-1);
	calib_x.min = INP_LINEARIZE/min_x - calib_x.zero;
	calib_y.min = INP_LINEARIZE/min_y - calib_y.zero;
	//Input thread should work again
	InputUnlock();
}

void input_select(void) {

}

void JoystickInit(void) {
	if (g_inputSemaphore == NULL) {
		g_inputSemaphore = xSemaphoreCreateMutexStatic(&g_inputSemaphoreState);
	}
	AdcInit();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	//For the x and y axis
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = INPUT_X_PIN | INPUT_Y_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(INPUT_PORT, &GPIO_InitStruct);
	//for the button
	GPIO_InitStruct.Pin = INPUT_KEY_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(INPUT_PORT, &GPIO_InitStruct);
	//some simple default values
	calib_x.min = 500;
	calib_x.zero = 1800;
	calib_x.max = 500;
	calib_y.min = 500;
	calib_y.zero = 1800;
	calib_y.max = 500;
	InputUnlock();
}

void InputThread(void * param) {
	(void)param;
	uint8_t snapKey = 0, snapX = 0, snapY = 0;
	bool doublePress = false;
	while(1) {
		InputLock();
		if (HAL_GPIO_ReadPin(INPUT_PORT, INPUT_KEY_PIN) == GPIO_PIN_RESET) {
			if (snapKey == 0) {
				userin.press = 1;
				snapKey = 1;
			}
		} else {
			snapKey = 0;
		}
		uint16_t x = AdcGet(INPUT_X_ADC);
		int16_t xLin = 0;
		if (x) {
			xLin = (INP_LINEARIZE / x); //make linear
			xLin -= calib_x.zero;
			if (xLin < 0) {
				xLin = xLin * INP_SCALE / calib_x.min;
			} else if (xLin > 0) {
				xLin = xLin * INP_SCALE / calib_x.max;
			}
			xLin = MAX(-INP_LIM, MIN(INP_LIM, xLin));
			userin.x = xLin;
		}
		if ((xLin > INP_SNAP_IN) && (snapX == 0)) {
			snapX = 1;
			userin.right = 1;
		}
		if ((xLin < -INP_SNAP_IN) && (snapX == 0)) {
			snapX = 1;
			userin.left = 1;
		}
		if ((xLin < INP_SNAP_OUT) && (xLin > -INP_SNAP_OUT)) {
			snapX = 0;
		}

		uint16_t y = AdcGet(INPUT_Y_ADC);
		int16_t yLin = 0;
		if (y) {
			yLin = (INP_LINEARIZE / y); //make linear
			yLin -= calib_y.zero;
			if (xLin < 0) {
				yLin = yLin * INP_SCALE / calib_y.min;
			} else if (xLin > 0) {
				yLin = yLin * INP_SCALE / calib_y.max;
			}
			yLin = MAX(-INP_LIM, MIN(INP_LIM, yLin));
		}
		userin.y = yLin;
		if ((yLin > INP_SNAP_IN) && (snapY == 0)) {
			snapY = 1;
			userin.down = 1;
		}
		if ((yLin < -INP_SNAP_IN) && (snapY == 0)) {
			snapY = 1;
			userin.up = 1;
		}
		if ((yLin < INP_SNAP_OUT) && (yLin > -INP_SNAP_OUT)) {
			snapY = 0;
		}
		InputUnlock();
		//printf("X: %u -> %i, Y: %u -> %i\r\n", x,userin.x, y, userin.y);
		//If the keypad instead of the joystick is used
		if ((KeyUpPressed()) && (KeyDownPressed())) {
			doublePress = true;
		}
		if (doublePress == false) {
			if (KeyRightReleased()) {
				userin.right = 1;
				userin.x = 100;
				vTaskDelay(100);
				userin.x = 0;
			}
			if (KeyLeftReleased()) {
				userin.left = 1;
				userin.x = -100;
				vTaskDelay(100);
				userin.x = 0;
			}
			if (KeyUpReleased()) {
				userin.up = 1;
				userin.y = -100;
				vTaskDelay(100);
				userin.y = 0;
			}
			if (KeyDownReleased()) {
				userin.down = 1;
				userin.y = 100;
				vTaskDelay(100);
				userin.y = 0;
			}
		} else {
			if ((KeyRightPressed() == false) && (KeyLeftPressed() == false) &&
			    (KeyUpPressed() == false) && (KeyDownPressed() == false)) {
				doublePress = false;
				userin.press = 1;
			}
		}
		vTaskDelay(1);
	}
}

void InputDebug(char c) {
	/* Yes, this results in a long wait while calibration is ongoing
	   and there the watchdog might get triggered.
	*/
	InputLock();
	if (c == 'w') {
		userin.up = 1;
		userin.y = -100;
		vTaskDelay(100);
		userin.y = 0;
	}
	if (c == 'a') {
		userin.left = 1;
		userin.x = -100;
		vTaskDelay(100);
		userin.x = 0;
	}
	if (c == 's') {
		userin.down = 1;
		userin.y = 100;
		vTaskDelay(100);
		userin.y = 0;
	}
	if (c == 'd') {
		userin.right = 1;
		userin.x = 100;
		vTaskDelay(100);
		userin.x = 0;
	}
	if (c == ' ') {
		userin.press = 1;
	}
	InputUnlock();
}

void reduceCPU(void) {
	vTaskDelay(1);
}

u08 userin_left(void) {
	reduceCPU();
	if (userin.left) {
		userin.left = 0;
		return 1;
	}
	return 0;
}

u08 userin_right(void) {
	reduceCPU();
	if (userin.right) {
		userin.right = 0;
		return 1;
	}
	return 0;
}

u08 userin_up(void) {
	reduceCPU();
	if (userin.up) {
		userin.up = 0;
		return 1;
	}
	return 0;
}

u08 userin_down(void) {
	reduceCPU();
	if (userin.down) {
		userin.down = 0;
		return 1;
	}
	return 0;
}

u08 userin_press(void) {
	reduceCPU();
	if (userin.press) {
		userin.press = 0;
		return 1;
	}
	return 0;
}

void userin_flush(void) {
	userin.left = 0;
	userin.right = 0;
	userin.up = 0;
	userin.down = 0;
	userin.press = 0;
}
