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
#include "boxlib/peripheral.h"

#include "main.h"

void mainMenu(void) {
	printf("\r\nSelect operation:\r\n");
	printf("0: Read all inputs\r\n");
	printf("1: Set LEDs\r\n");
	printf("2: Set relays\r\n");
	printf("3: Check 32KHz crystal\r\n");
	printf("4: Check high speed crystal\r\n");
	printf("5: Check SPI flash\r\n");
	printf("6: Set flash page size to 2^n\r\n");
	printf("7: Check ESP-01\r\n");
	printf("8: Toggle LCD backlight\r\n");
	printf("9: Init and write to the LCD\r\n");
	printf("a: Write a color pixel to the LCD\r\n");
	printf("b: Check IR\r\n");
	printf("c: Peripheral powercycle (RS232, LCD, flash)\r\n");
	printf("d: Check coprocessor communication\r\n");
	printf("e: Reboot to DFU mode\r\n");
	printf("f: Reboot to normal mode\r\n");
	printf("r: Reboot without coprocessor\r\n");
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
	printf("\r\nStarting external low speed crystal\r\n");
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSE;
	RCC_OscInitStruct.LSEState = RCC_LSE_ON;
	HAL_StatusTypeDef result = HAL_RCC_OscConfig(&RCC_OscInitStruct);
	if (result != HAL_OK) {
		printf("Error, returned %u\r\n", (unsigned int)result);
		return;
	}
	printf("Running\r\n");
}

void check16MCrystal(void) {
	//parts of the function are copied from the ST cube generator
	static bool switched = false;
	if (switched) {
		printf("\r\nAlready running on external crystal\r\n");
		return;
	}
	printf("\r\nStarting external high speed crystal\r\n");
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
	HAL_StatusTypeDef result = HAL_RCC_OscConfig(&RCC_OscInitStruct);
	if (result != HAL_OK) {
		printf("Error, returned %u\r\n", (unsigned int)result);
		return;
	}
	printf("Switching to external crystal\r\n");
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSE;
	//MSI is 4MHz, HSE is 8MHz, so all clocks stays the same with div2.
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV2;
	result = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0);
	if (result != HAL_OK) {
		printf("Error, returned %u\r\n", (unsigned int)result);
		return;
	}
	switched = true;
	printf("Running with HSE clock source\r\n");
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
}

void readSerialLine(char * input, size_t len) {
	memset(input, 0, len);
	size_t i = 0;
	while (i < (len - 1)) {
		char c = rs232GetChar();
		if (c != 0) {
			input[i] = c;
			i++;
			printf("%c", c);
		}
		if ((c == '\r') || (c == '\n'))
		{
			break;
		}
	}
}

void writePixelLcd(void) {
	printf("\r\nEnter 2x3 decimal digits for x+y, then 6 hexadecimal digits for color, separated with a space\r\n");
	char buffer[16] = {0};
	readSerialLine(buffer, sizeof(buffer));
	unsigned int x, y, color;
	sscanf(buffer, "%u %u %x", &x, &y, &color);
	LcdWritePixel(x, y, color);
	printf("Written(%u,%u) = 0x%x\r\n", x, y, color);
}

void PeripheralPowercycle(void) {
	printf("\r\nPower off for 2 sec\r\n");
	PeripheralPowerOff();
	HAL_Delay(1000);
	printf("If you see this message, the power off test failed\r\n");
	HAL_Delay(1000);
	PeripheralPowerOn();
	printf("Power back on. There should be no message printed between this and the power off message\r\n");
}

void checkCoprocComm(void) {
	printf("\r\nCheck coprocessor communication\r\n");
	uint16_t pattern = CoprocReadTestpattern();
	uint16_t version = CoprocReadVersion();
	uint16_t vcc = CoprocReadVcc();

	printf("Pattern: 0x%x, version: 0x%x, Vcc: %umV\r\n", pattern, version, vcc);
	if (pattern == 0xF055) {
		printf("Looks good\r\n");
	} else {
		printf("Error, pattern does not fit\r\n");
	}
}

void rebootToDfu(void) {
	CoprocWriteReboot(2); //2 for dfu bootloader
}

void rebootToNormal(void) {
	CoprocWriteReboot(1); //1 for normal boot
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
		case 'a': writePixelLcd(); break;
		case 'b': checkIr(); break;
		case 'c': PeripheralPowercycle(); break;
		case 'd': checkCoprocComm(); break;
		case 'e': rebootToDfu(); break;
		case 'f': rebootToNormal(); break;
		case 'r': NVIC_SystemReset(); break;
		case 'h': mainMenu(); break;
		default: break;
	}
}
