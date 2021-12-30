/*
(c) 2021 by Malte Marwedel

License: BSD-3-Clause
*/

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>

#include "testEverything.h"

#include "stm32l4xx_hal.h"

#include "boxlib/keys.h"
#include "boxlib/leds.h"
#include "boxlib/relays.h"
#include "boxlib/rs232debug.h"
#include "boxlib/lcd.h"
#include "boxlib/flash.h"
#include "boxlib/coproc.h"
#include "boxlib/ir.h"

#include "main.h"

void mainMenu(void) {
	printf("\r\nSelect operation:\r\n");
	printf("0: Read all inputs\r\n");
	printf("1: Set LEDs\r\n");
	printf("2: Set relays\r\n");
	printf("3: Check 32KHz crystal\r\n");
	printf("4: Check 16MHz crystal\r\n");
	printf("5: Check SPI flash\r\n");
	printf("6: Set flash page size to 2^n\r\n");
	printf("7: Check ESP-01\r\n");
	printf("8: Toggle LCD backlight\r\n");
	printf("9: Write to LCD\r\n");
	printf("a: Check IR\r\n");
	printf("r: Reboot\r\n");
	printf("h: This screen\r\n");
}

void printHex(const uint8_t * data, size_t len) {
	for (uint32_t i = 0; i < len; i++) {
		printf("%02X ", data[i]);
		if (((i % 8) == 7) || (i == len -1)) {
			printf("\r\n");
		}
	}
}

void testInit(void) {
	Led1Red();
	HAL_Delay(100);
	rs232Init();
	printf("Test everything 0.1\r\n");
	mainMenu();
}

void readSensors() {
	bool right = KeyRighPressed();
	bool left = KeyLeftPressed();
	bool up = KeyUpPressed();
	bool down = KeyDownPressed();
	printf("\n\rright: %u, left: %u, up: %u, down: %u\r\n", right, left, up, down);
	bool avrIn = CoprocInGet();
	printf("Coprocessor pin: %u\r\n", avrIn);
}

void setLeds() {
	printf("\n\rToggle LEDs by entering 1...4. All other keys return\n\r");
	char c;
	bool valid;
	static bool state[4] = {false, false, false, false};
	do {
		valid = false;
		c = rs232GetChar();
		uint8_t i = c - '1';
		if ((i >= 0) && (i <= 3)) {
			valid = true;
			state[i] = !state[i];
			if ((state[0]) && (state[1])) {
				Led1Yellow();
			} else if (state[0]) {
				Led1Red();
			} else if (state[1]) {
				Led1Green();
			} else {
				Led1Off();
			}
			if ((state[2]) && (state[3])) {
				Led2Yellow();
			} else if (state[2]) {
				Led2Red();
			} else if (state[3]) {
				Led2Green();
			} else {
				Led2Off();
			}
			printf("LED states: %u, %u, %u, %u\r\n", state[0], state[1], state[2], state[3]);
			continue;
		}
	} while((c == 0) || (valid));
}

void setRelays() {
	printf("\n\rToggle relays by entering 1...4. All other keys return\n\r");
	char c;
	bool valid;
	static bool state[4] = {false, false, false, false};
	do {
		valid = false;
		c = rs232GetChar();
		uint8_t i = c - '1';
		if ((i >= 0) && (i <= 3)) {
			valid = true;
			state[i] = !state[i];
			Relay1Set(state[0]);
			Relay2Set(state[1]);
			Relay3Set(state[2]);
			Relay4Set(state[3]);
			printf("Relay states: %u, %u, %u, %u\r\n", state[0], state[1], state[2], state[3]);
			continue;
		}
	} while((c == 0) || (valid));
}

void check32kCrystal(void) {
	printf("\r\nTODO\r\n");
}

void check16MCrystal(void) {
	printf("\r\nTODO\r\n");
}

void checkFlash(void) {
	FlashEnable();
	uint16_t status = FlashGetStatus();
	uint16_t density = (status >> 10) & 0xF;
	uint16_t comp = (status >> 14) & 1;
	uint16_t protection = (status >> 9) & 1;
	uint16_t pageSizeB = (status >> 8) & 1;
	uint16_t error = (status >> 5) & 1;
	printf("\r\nFlash status: 0x%04x\r\n  -> density: 0x%x\r\n", status, density);
	printf("  -> compare result: %u\r\n", comp);
	printf("  -> protection: %u\r\n", protection);
	printf("  -> page size: 2^n%s\r\n", pageSizeB ? "" : "+n");
	printf("  -> program/erase error: %u\r\n", error);
	uint8_t manufacturer;
	uint16_t device;
	FlashGetId(&manufacturer, &device);
	printf("Manufacturer: 0x%02x, device: 0x%04x\r\n", manufacturer, device);
	if (manufacturer == 0x1F) {
		printf("  -> From Adesto\r\n");
		uint8_t family = device >> 13;
		if (family == 0x1) {
			printf("  -> AT45D. Looks good\r\n");
			uint8_t density2 = (device >> 8) & 0x1F;
			if (((density2 == 0x7) && (AT45PAGESIZE == 512)) ||
			    ((density2 == 0x8) && (AT45PAGESIZE == 256)))
			{
				uint32_t timestamp = HAL_GetTick();
				uint8_t bufferOut[AT45PAGESIZE] = {0};
				uint8_t bufferIn[AT45PAGESIZE] = {0};
				memcpy(bufferOut, &timestamp, sizeof(uint32_t));
				for (uint32_t i = 4; i < AT45PAGESIZE; i++) {
					bufferOut[i] = i;
				}
				FlashWrite(0, bufferOut, sizeof(bufferOut));
				FlashRead(0, bufferIn, sizeof(bufferIn));
				uint32_t timestamp2 = HAL_GetTick();
				if (memcmp(bufferOut, bufferIn, sizeof(bufferOut)) == 0) {
					printf("Writing and reading back %ubyte successful\r\n", AT45PAGESIZE);
				} else {
					printf("Error, read back data mismatched:\r\n");
					printHex(bufferIn, sizeof(bufferIn));
					printf("And the sram buffer:\r\n");
					memset(bufferIn, 0, sizeof(bufferIn));
					FlashReadBuffer1(bufferIn, 0, sizeof(bufferIn));
					printHex(bufferIn, sizeof(bufferIn));
				}
				printf("Write+Read took %ums\r\n", (unsigned int)(timestamp2 - timestamp));
			} else {
				printf("Error, device ID does not fit to the pagesize\r\n");
			}
		} else {
			printf("Family unknown\r\n");
		}
	} else {
		printf("Manufacturer unknown\r\n");
	}
}

void setFlashPagesize(void) {
	printf("\r\nEnter p to set the page size to 512byte\r\n");
	char input = 0;
	do {
		input = rs232GetChar();
		if (input == 'p') {
			printf("Ok...\r\n");
			FlashPagesizePowertwo();
			printf("Done\r\n");
		}
	} while (input == 0);
}

void checkEsp(void) {
	printf("\r\nTODO\r\n");
}

void checkIr(void) {
	IrOn();
	printf("\r\nIR enabled, signal low will be printed every second until a key is pressed\r\n");
	char input = 0;
	do {
		input = rs232GetChar();
		uint32_t timeEnd = HAL_GetTick() + 1000;
		float sig = 0;
		float noSig = 0;
		while (HAL_GetTick() < timeEnd) {
			if (IrPinSignal()) {
				sig++;
			} else {
				noSig++;
			}
		}
		if ((sig + noSig) > 1) {
			float perc = sig * 1000.0 / (sig + noSig);
			unsigned int a = perc / 10;
			unsigned int b = perc - a;
			printf("Signal %u.%u%%\r\n", a, b);
		}
	} while (input == 0);
	IrOff();
	printf("Ir disabled\r\n");
}

void setLcdBacklight(void) {
	static bool state = false;
	state = !state;
	if (state) {
		printf("\r\nBacklight on\r\n");
		LcdBacklightOn();
	} else {
		printf("\r\nBacklight off\r\n");
		LcdBacklightOff();
	}
}

void writeLcd(void) {
	LcdEnable();
	LcdInit();
	LcdTestpattern();
	printf("\r\nTODO\r\n");
}

void testCycle(void) {
	Led2Green();
	HAL_Delay(100);
	Led2Off();
	HAL_Delay(100);
	char input = rs232GetChar();
	if (input) {
		printf("%c", input);
	}
	switch (input) {
		case '0': readSensors(); break;
		case '1': setLeds(); break;
		case '2': setRelays(); break;
		case '3': check32kCrystal(); break;
		case '4': check16MCrystal(); break;
		case '5': checkFlash(); break;
		case '6': setFlashPagesize(); break;
		case '7': checkEsp(); break;
		case '8': setLcdBacklight(); break;
		case '9': writeLcd(); break;
		case 'a': checkIr(); break;
		case 'r': NVIC_SystemReset(); break;
		case 'h': mainMenu(); break;
		default: break;
	}
}
