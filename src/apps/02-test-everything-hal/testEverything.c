/*
(c) 2021-2022 by Malte Marwedel

License: BSD-3-Clause
*/

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <ctype.h>

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
#include "boxlib/esp.h"
#include "boxlib/simpleadc.h"
#include "boxlib/boxusb.h"
#include "boxlib/mcu.h"
#include "boxlib/readLine.h"

#include "main.h"

#include "usbd_core.h"
#include "usb.h"

#include "utility.h"

void MainMenu(void) {
	printf("\r\nSelect operation:\r\n");
	printf("0: Read all inputs\r\n");
	printf("1: Set LEDs\r\n");
	printf("2: Set relays\r\n");
	printf("3: Check 32KHz crystal\r\n");
	printf("4: Check high speed crystal (8MHz)\r\n");
	printf("5: Increase CPU speed to 32MHz\r\n");
	printf("6: Increase CPU speed to 64MHz\r\n");
	printf("7: Toggle MCU flash prefetch\r\n");
	printf("8: Toggle MCU flash cache\r\n");
	printf("9: Measure bogomips\r\n");
	printf("a: Check SPI flash\r\n");
	printf("b: Set flash page size to 2^n\r\n");
	printf("c: Check ESP-01\r\n");
	printf("d: Toggle LCD backlight\r\n");
	printf("e: Init and write to the LCD (128x128 with ST7735)\r\n");
	printf("f: Init and write to the LCD (320x240 with ILI9341)\r\n");
	printf("g: Write a color pixel to the LCD\r\n");
	printf("m: Manual write data to the LCD\r\n");
	printf("h: This screen\r\n");
	printf("i: Check IR\r\n");
	printf("j: Peripheral powercycle (RS232, LCD, flash)\r\n");
	printf("k: Check coprocessor communication\r\n");
	printf("n: Manual coprocessor SPI voltage control\r\n");
	printf("l: Minimize power for 4 seconds\r\n");
	printf("p: Set analog, pull-up or pull down on pin\r\n");
	printf("r: Reboot with reset controller\r\n");
	printf("s: Jump to DFU bootloader\r\n");
	printf("t: Reboot to normal mode (needs coprocessor)\r\n");
	printf("u: Init USB device. 2. call disables again.\r\n");
	printf("z: Reboot to DFU mode (needs coprocessor)\r\n");
}

void AppInit(void) {
	LedsInit();
	Led1Red();
	HAL_Delay(100);
	Rs232Init(); //includes PeripheralPowerOn
	printf("Test everything %s\r\n", APPVERSION);
	CoprocInit();
	PeripheralInit();
	MainMenu();
}

#define PIN_NUM 11

typedef struct {
	GPIO_TypeDef * port;
	uint32_t pin;
	const char * name;
} pin_t;

const pin_t g_pins[PIN_NUM] = {
	{GPIOC, GPIO_PIN_1, "PC1, ADC2"},
	{GPIOC, GPIO_PIN_0, "PC0, ADC1"},
	{GPIOC, GPIO_PIN_3, "PC3, ADC4"},
	{GPIOA, GPIO_PIN_4, "PA4, ADC9, DAC1"},
	{GPIOB, GPIO_PIN_3, "PB3, SPI1SCK"},
	{GPIOB, GPIO_PIN_4, "PB4, SPI1MISO"},
	{GPIOB, GPIO_PIN_5, "PB5, SPI1MOSI"},
	{GPIOA, GPIO_PIN_3, "PA3, ADC8, UART2RX"},
	{GPIOA, GPIO_PIN_2, "PA2, ADC7, UART2TX"},
	{GPIOA, GPIO_PIN_1, "PA1, ADC6, Tim2Ch2"},
	{GPIOA, GPIO_PIN_0, "PA0, ADC5, Tim2Ch1"},
};

#define CHANNELS 19

const char * g_adcNames[CHANNELS] = {
	"Ref",
	"PC0",
	"PC1",
	"PC2",
	"PC3",
	"PA0",
	"PA1",
	"PA2",
	"PA3",
	"PA4",
	"PA5",
	"PA6",
	"PA7",
	"PC4",
	"PC5",
	"PB0",
	"PB1",
	"Tmp",
	"Bat"
};

void ReadSensors(void) {
	KeysInit();
	bool right = KeyRightPressed();
	bool left = KeyLeftPressed();
	bool up = KeyUpPressed();
	bool down = KeyDownPressed();
	printf("\r\nright: %u, left: %u, up: %u, down: %u\r\n", right, left, up, down);
	bool avrIn = CoprocInGet();
	printf("Coprocessor pin: %u\r\n", avrIn);
	static bool adcInit = false;
	if (adcInit == false) {
		AdcInit();
		adcInit = true;
	}
	int32_t tsCal1 = *((uint16_t*)0x1FFF75A8); //30°C calibration value
	int32_t tsCal2 = *((uint16_t*)0x1FFF75CA); //130°C calibration value
	int32_t vrefint = *((uint16_t*)0x1FFF75AA); //voltage reference

	uint32_t vdda = 0;
	for (uint32_t i = 0; i < CHANNELS; i++) {
		uint32_t val = AdcGet(i);
		printf("ADC input %u %s: %u\r\n", (unsigned int)i, g_adcNames[i], (unsigned int)val);
		if ((i == 0) && (val > 0)) {
			vdda = 3000 * vrefint / val;
			printf("  VRef = %umV\r\n", (unsigned int)vdda);
		}
		if (i == 17) {
			/* The calibrated temperature values are for 3.0Vcc, so we need do scale
			   the adc value to the current vcc value
			*/
			val *= vdda;
			val /= 3000;
			int32_t temperatureMCelsius = 100000 / (tsCal2 - tsCal1) * (val - tsCal1) + 30000;
			int32_t temperature10thCelsius = temperatureMCelsius / 100;
			int32_t temperatureCelsius = temperature10thCelsius /10;
			int32_t diff = temperature10thCelsius - temperatureCelsius * 10;
			printf("  Temp = %u.%u°C\r\n", (int)temperatureCelsius, (int)diff);
		}
	}
	for (uint32_t i = 0; i < PIN_NUM; i++) {
		unsigned int set = 0;
		if (HAL_GPIO_ReadPin(g_pins[i].port, g_pins[i].pin) == GPIO_PIN_SET) {
			set = 1;
		}
		printf("%s: %u\r\n", g_pins[i].name, set);
	}
}

void ChangePullPin(void) {
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	static uint8_t pinMode[PIN_NUM] = {0};
	const char * modeName[4] = {"analog", "input", "input pull-up", "input pull-down"};
	printf("Select pin to change\r\n");
	for (uint32_t i = 0; i < PIN_NUM; i++) {
		const char * state = modeName[pinMode[i]];
		printf("%02i: %s = %s\r\n", (unsigned int)i, g_pins[i].name, state);
	}
	char buffer[8];
	ReadSerialLine(buffer, sizeof(buffer));
	unsigned int pin;
	sscanf(buffer, "%u", &pin);
	if (pin >= PIN_NUM) {
		printf("Pin unknown\r\n");
		return;
	}
	printf("Enter new mode: a: analog i: input floating, u: input + pull-up, d: input + pull-down\r\n");
	ReadSerialLine(buffer, sizeof(buffer));
	char c = buffer[0];
	uint32_t pull;
	uint32_t mode;
	if (c == 'a') {
		pinMode[pin] = 0; //Default after reset
		pull = GPIO_NOPULL;
		mode = GPIO_MODE_ANALOG;
	} else if (c == 'i') {
		pinMode[pin] = 1;
		pull = GPIO_NOPULL;
		mode = GPIO_MODE_INPUT;
	} else if (c == 'u') {
		pinMode[pin] = 2;
		pull = GPIO_PULLUP;
		mode = GPIO_MODE_INPUT;
	} else if (c == 'd') {
		pinMode[pin] = 3;
		pull = GPIO_PULLDOWN;
		mode = GPIO_MODE_INPUT;
	} else {
		printf("Mode unsupported\r\n");
		return;
	}
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = g_pins[pin].pin;
	GPIO_InitStruct.Mode = mode;
	GPIO_InitStruct.Pull = pull;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(g_pins[pin].port, &GPIO_InitStruct);
	printf("Done\r\n");
}

void SetLeds(void) {
	printf("\r\nToggle LEDs by entering 1...4. All other keys return\r\n");
	char c;
	bool valid;
	static bool state[4] = {false, false, false, false};
	do {
		valid = false;
		c = Rs232GetChar();
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

void SetRelays() {
	RelaysInit();
	printf("\r\nToggle relays by entering 1...4. All other keys return\r\n");
	char c;
	bool valid;
	static bool state[4] = {false, false, false, false};
	do {
		valid = false;
		c = Rs232GetChar();
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

void Check32kCrystal(void) {
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

static bool g_usingHsi = true;

void ClockToHsi(void) {
	Rs232Flush();
	//parts of the function are copied from the ST cube generator
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	if (g_usingHsi == false) {
		RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
		RCC_OscInitStruct.HSIState = RCC_HSI_ON;
		RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
		RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
		HAL_RCC_OscConfig(&RCC_OscInitStruct);
	}
	//reconfigure clock source
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
	                             | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
	//HSI is 16MHz, HSE is 8MHz, so peripheral clocks stays the same with div2
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
	HAL_StatusTypeDef result = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0);
	if (result != HAL_OK) {
		printf("Error, returned %u\r\n", (unsigned int)result);
		return;
	}
	//stop the other clocks
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_MSI;
	RCC_OscInitStruct.HSEState = RCC_HSE_OFF;
	RCC_OscInitStruct.MSIState = RCC_MSI_OFF;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
	HAL_RCC_OscConfig(&RCC_OscInitStruct);
	g_usingHsi = true;
	SystemCoreClockUpdate();
}

void ClockToHse(void) {
	//parts of the function are copied from the ST cube generator
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};

	//HSE on
	printf("\r\nStarting external high speed crystal\r\n");
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
	HAL_StatusTypeDef result = HAL_RCC_OscConfig(&RCC_OscInitStruct);
	if (result != HAL_OK) {
		printf("Error, returned %u\r\n", (unsigned int)result);
		return;
	}
	printf("Switching to external crystal\r\n");
	Rs232Flush();
	//switch clock source
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
	                             | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSE;
	//HSI is 16MHz, HSE is 8MHz, so peripheral clocks stays the same with div1
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
	result = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0);
	if (result != HAL_OK) {
		printf("Error, returned %u\r\n", (unsigned int)result);
		return;
	}
	g_usingHsi = false;
	SystemCoreClockUpdate();
	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) {
		return;
	}
	printf("Running with HSE clock source\r\n");
	//stop the other clocks
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_MSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_OFF;
	RCC_OscInitStruct.MSIState = RCC_MSI_OFF;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
	HAL_RCC_OscConfig(&RCC_OscInitStruct);
	printf("MSI and HSI turned off\r\n");
}

void CheckHseCrystal(void) {
	if (g_usingHsi == false) {
		printf("\r\nAlready running on external crystal. Switching back.\r\n");
		ClockToHsi();
		printf("Back running on HSI\r\n");
		return;
	}
	ClockToHse();
}

void ClockWithPll(uint32_t frequency, uint32_t apbDivider) {
	ClockToHsi();
	Rs232Flush();
	uint8_t result = McuClockToHsiPll(frequency, apbDivider);
	if (result == 1) {
		printf("Error, unsupported frequency\r\n");
	}
	if (result == 2) {
		printf("Error, failed to set flash latency\r\n");
	}
	if (result == 3) {
		printf("Error, failed to start PLL\r\n");
	}
	if (result == 4) {
		printf("Error, failed to set divider or latency\r\n");
	}
}

//debug prints may not work after changing. As the prescalers are not recalculated
bool ClockToMsi(uint32_t frequency) {
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	Rs232Flush();
	if (McuClockToMsi(frequency, RCC_HCLK_DIV1) == false) {
		printf("Error, no MSI\r\n");
		return false;
	}
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSEState = RCC_HSE_OFF;
	RCC_OscInitStruct.HSIState = RCC_HSI_OFF;
	HAL_RCC_OscConfig(&RCC_OscInitStruct);
	g_usingHsi = false;
	return true;
}

bool g_highSpeed;

void Speed32M(void) {
	if (!g_highSpeed) {
		printf("\r\nSwitching to 32MHz...\r\n");
		ClockWithPll(32000000, RCC_HCLK_DIV4);
		printf("Now running with 32MHz\r\n");
		g_highSpeed = true;
	} else {
		ClockToHsi();
		RCC_OscInitTypeDef RCC_OscInitStruct = {0};
		RCC_OscInitStruct.PLL.PLLState = RCC_PLL_OFF;
		HAL_RCC_OscConfig(&RCC_OscInitStruct);
		printf("\r\nBack running with 16MHz\r\n");
		g_highSpeed = false;
	}
}

void Speed64M(void) {
	if (!g_highSpeed) {
		printf("\r\nSwitching to 64MHz...\r\n");
		ClockWithPll(64000000, RCC_HCLK_DIV8);
		printf("Now running with 64MHz\r\n");
		g_highSpeed = true;
	} else {
		ClockToHsi();
		RCC_OscInitTypeDef RCC_OscInitStruct = {0};
		RCC_OscInitStruct.PLL.PLLState = RCC_PLL_OFF;
		HAL_RCC_OscConfig(&RCC_OscInitStruct);
		printf("\r\nBack running with 16MHz\r\n");
		g_highSpeed = false;
	}
}

void ToggleFlashPrefetch(void) {
	static bool enabled = false;
	if (!enabled) {
		__HAL_FLASH_PREFETCH_BUFFER_ENABLE();
		printf("\r\nPrefetch enabled\r\n");
	} else {
		__HAL_FLASH_PREFETCH_BUFFER_DISABLE();
		printf("\r\nPrefetch disabled\r\n");
	}
	enabled = !enabled;
}

void ToggleFlashCache(void) {
	static bool enabled = false;
	if (!enabled) {
		__HAL_FLASH_INSTRUCTION_CACHE_ENABLE();
		__HAL_FLASH_DATA_CACHE_ENABLE();
		printf("\r\nI+D cache enabled\r\n");
	} else {
		__HAL_FLASH_INSTRUCTION_CACHE_DISABLE();
		__HAL_FLASH_DATA_CACHE_DISABLE();
		printf("\r\nI+D cache disabled\r\n");
	}
	enabled = !enabled;
}

#define BLOCKSIZE_MAX 512

void CheckFlash(void) {
	FlashEnable(128);
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
			uint32_t blocksize = FlashBlocksizeGet();
			if (((density2 == 0x7) && (blocksize == 512)) ||
			    ((density2 == 0x8) && (blocksize == 256)))
			{
				uint32_t timestamp = HAL_GetTick();
				uint8_t bufferOut[BLOCKSIZE_MAX] = {0};
				uint8_t bufferIn[BLOCKSIZE_MAX] = {0};
				memcpy(bufferOut, &timestamp, sizeof(uint32_t));
				for (uint32_t i = 4; i < blocksize; i++) {
					bufferOut[i] = i;
				}
				FlashWrite(0, bufferOut, blocksize);
				FlashRead(0, bufferIn, blocksize);
				uint32_t timestamp2 = HAL_GetTick();
				uint32_t delta = timestamp2 - timestamp;
				if (memcmp(bufferOut, bufferIn, blocksize) == 0) {
					printf("Writing and reading back %ubyte successful\r\n", (unsigned int)blocksize);
					printf("Write+Read took %ums\r\n", (unsigned int)delta);
					for (uint32_t i = 128; i >= 2; i /= 2) {
						FlashEnable(i);
						memset(bufferIn, 0, blocksize);
						timestamp = HAL_GetTick();
						FlashRead(0, bufferIn, blocksize);
						timestamp2 = HAL_GetTick();
						delta = timestamp2 - timestamp;
						if (memcmp(bufferOut, bufferIn, blocksize) == 0) {
							printf("Reading with prescaler %u succeed. Time %ums\r\n", (unsigned int)i, (unsigned int)delta);
						} else {
							printf("Reading with prescaler %u failed\r\n", (unsigned int)i);
							break;
						}
					}
				} else {
					printf("Error, read back data mismatched:\r\n");
					PrintHex(bufferIn, blocksize);
					printf("And the sram buffer:\r\n");
					memset(bufferIn, 0, blocksize);
					FlashReadBuffer1(bufferIn, 0, blocksize);
					PrintHex(bufferIn, blocksize);
				}
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

void SetFlashPagesize(void) {
	printf("\r\nEnter p to set the page size to 512byte\r\n");
	char input = 0;
	do {
		input = Rs232GetChar();
		if (input == 'p') {
			printf("Ok...\r\n");
			FlashPagesizePowertwoSet();
			printf("Done\r\n");
		}
	} while (input == 0);
}

void CheckEspPrint(const char * buffer) {
	const uint32_t maxCharsLine = 80;
	uint32_t forceNewline = maxCharsLine;
	while (*buffer) {
		if (isprint(*buffer) || (*buffer == '\r') || (*buffer == '\n')) {
			if (*buffer == '\r') {
				forceNewline = maxCharsLine;
			}
			putchar(*buffer);
			forceNewline--;
		}
		buffer++;
		if (forceNewline == 0) {
			printf("\r\n");
			forceNewline = maxCharsLine;
		}
	}
}

void CheckEsp(void) {
	EspInit();
	char inBuffer[1024] = {0};
	size_t maxBuffer = sizeof(inBuffer);
	printf("\r\n==Enabling Esp==\r\n");
	EspEnable();
	EspCommand("", inBuffer, maxBuffer, 1000);
	CheckEspPrint(inBuffer);
	if ((strstr(inBuffer, "ready")) || (strstr(inBuffer, "Ai-Thinker"))) {

		EspCommand("AT+GMR\r\n", inBuffer, maxBuffer, 250); //request version
		printf("\r\n==Version==\r\n");
		CheckEspPrint(inBuffer);

		EspCommand("AT+CWMODE=?\r\n", inBuffer, maxBuffer, 250); //possible modes?
		printf("\r\n==Supported modes==\r\n");
		CheckEspPrint(inBuffer);

		EspCommand("AT+CWJAP?\r\n", inBuffer, maxBuffer, 250); //current mode?
		printf("\r\n==Current mode==\r\n");
		CheckEspPrint(inBuffer);

		EspCommand("AT+CWMODE_CUR=1\r\n", inBuffer, maxBuffer, 250); //lets become a client
		printf("\r\n==Now a client==\r\n");
		CheckEspPrint(inBuffer);

		//list available AP, dont know the right timeout. 1000 is too less
		EspCommand("AT+CWLAP\r\n", inBuffer, maxBuffer, 7000);
		printf("\r\n==Available APs==\r\n");
		CheckEspPrint(inBuffer);

		EspCommand("AT+CIPSTART=?\r\n", inBuffer, maxBuffer, 1000);
		printf("\r\n==Supported connections==\r\n");
		CheckEspPrint(inBuffer);

	} else {
		printf("Error, no valid answer from ESP detected\r\n");
	}
	EspStop();
	printf("\r\n==Esp disabled==\r\n");
}

void CheckIr(void) {
	IrInit();
	IrOn();
	printf("\r\nIR enabled, signal low will be printed every second until a key is pressed\r\n");
	char input = 0;
	do {
		input = Rs232GetChar();
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

void SetLcdBacklight(void) {
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

void WriteLcd(void) {
	LcdEnable(4);
	LcdInit(ST7735_128);
	LcdTestpattern();
}

void WriteLcdBig(void) {
	LcdEnable(4);
	LcdInit(ILI9341);
	LcdTestpattern();
}

void WritePixelLcd(void) {
	printf("\r\nEnter 2x3 decimal digits for x+y, then 6 hexadecimal digits for color, separated with a space\r\n");
	char buffer[16] = {0};
	ReadSerialLine(buffer, sizeof(buffer));
	unsigned int x, y, color;
	sscanf(buffer, "%u %u %x", &x, &y, &color);
	PeripheralPrescaler(2);
	LcdWritePixel(x, y, color);
	printf("Written(%u,%u) = 0x%x\r\n", x, y, color);
}

void ManualLcdCmd(void) {
	printf("\r\nFormat (hex): Parameters command data1, data2, data3, data4, data5, data6, data7, data8\r\n");
	char buffer[128];
	ReadSerialLine(buffer, sizeof(buffer));
	printf("\r\n");
	unsigned int vars[10] = {0};
	sscanf(buffer, "%x %x %x %x %x %x %x %x %x %x",
	       vars, vars+1, vars+2, vars+3, vars+4, vars+5, vars+6, vars+7, vars+8, vars+9);
	size_t len = vars[0];
	if (len > 8) {
		return;
	}
	uint8_t parameters[8];
	uint8_t readback[8];
	for (uint32_t i = 0; i < 8; i++) {
		parameters[i] = vars[i+2];
	}
	LcdCommandData(vars[1], parameters, readback, len);
	PrintHex(readback, len);
	printf("Done\r\n");
}

void PeripheralPowercycle(void) {
	printf("\r\nPower off for 4 sec\r\n");
	Rs232Flush();
	PeripheralPowerOff();
	HAL_Delay(2000);
	printf("If you see this message, the power off test failed\r\n");
	HAL_Delay(2000);
	PeripheralPowerOn();
	printf("Power back on. There should be no message printed between this and the power off message\r\n");
}

void CheckCoprocComm(void) {
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

void ManualSpiCoprocLevel(void) {
	printf("Enter number 0...3. Low bit will be the clock, high bit will be the data level\r\n");
	char buffer[128];
	ReadSerialLine(buffer, sizeof(buffer));
	printf("\r\n");
	unsigned int val = 0;
	sscanf(buffer, "%x", &val);
	if (val & 2) {
		HAL_GPIO_WritePin(AvrSpiMosi_GPIO_Port, AvrSpiMosi_Pin, GPIO_PIN_SET);
		printf("Data pin now high\r\n");
	} else {
		HAL_GPIO_WritePin(AvrSpiMosi_GPIO_Port, AvrSpiMosi_Pin, GPIO_PIN_RESET);
		printf("Data pin now low\r\n");
	}
	if (val & 1) {
		HAL_GPIO_WritePin(AvrSpiSck_GPIO_Port, AvrSpiSck_Pin, GPIO_PIN_SET);
		printf("Clock pin now high\r\n");
	} else {
		HAL_GPIO_WritePin(AvrSpiSck_GPIO_Port, AvrSpiSck_Pin, GPIO_PIN_RESET);
		printf("Clock pin now low\r\n");
	}
}

void RebootToDfu(void) {
	CoprocWriteReboot(2); //2 for dfu bootloader
}

void RebootToNormal(void) {
	CoprocWriteReboot(1); //1 for normal boot
}

typedef void (ptrFunction_t)(void);

//See https://stm32f4-discovery.net/2017/04/tutorial-jump-system-memory-software-stm32/
void JumpDfu(void) {
	uint32_t dfuStart = 0x1FFF0000;
	Led1Green();
	printf("\r\nDirectly jump to the DFU bootloader\r\n");
	volatile uint32_t * pStackTop = (uint32_t *)dfuStart;
	volatile uint32_t * pProgramStart = (uint32_t *)(dfuStart + 4);
	printf("This function is at 0x%x. New stack will be at 0x%x\r\n", (unsigned int)&JumpDfu, (unsigned int)(*pStackTop));
	printf("Program start will be at 0x%x\r\n", (unsigned int)(*pProgramStart));
	/*The bootloader seems not to reset the GPIO ports, so we can lock the pin for
	  SPI2 MISO and prevent it becoming a high level output
	  but to use our peripherals again, we might need a system reset or at least
	  a GPIO port reset
	*/
	McuLockCriticalPins();

	Led2Off();
	//first all peripheral clocks should be disabled
	UsbStop(); //stops USB clock
	EspStop(); //stops UART3 clock
	AdcStop(); //stops ADC clock
	Rs232Flush();
	PeripheralPowerOff(); //stops SPI2 clock, also RS232 level converter will stop
	Rs232Stop(); //stops UART1 clock
	McuStartOtherProgram((void *)dfuStart, true); //usually does not return
}

void Bogomips(void) {
	uint32_t timeout = HAL_GetTick() + 1000;
	uint32_t ips = 0;
	while (timeout > HAL_GetTick()) {
		ips++;
	}
	printf("\r\nIPS: %u\r\n", (unsigned int)ips);
}

//================== code for USB testing ==============

usbd_device g_usbDev;

/* The PID used here is reserved for general test purpose.
See: https://pid.codes/1209/
*/
uint8_t g_deviceDescriptor[] = {
	0x12,       //length of this struct
	0x01,       //always 1
	0x10,0x01,  //usb version
	0xFF,       //device class
	0xFF,       //subclass
	0xFF,       //device protocol
	0x20,       //maximum packet size
	0x09,0x12,  //vid
	0x02,0x00,  //pid
	0x00,0x01,  //revision
	0x1,        //manufacturer index
	0x2,        //product name index
	0x3,        //serial number index
	0x01        //number of configurations
};

uint8_t g_DeviceConfiguration[] = {
	9,     //length of this entry
	0x2,   //device configuration
	18, 0, //total length of this struct
	0x1,   //number of interfaces
	0x1,   //this config
	0x0,   //descriptor of this config index, not used
	0x80, //bus powered
	50,   //100mA
	//interface descriptor follows
	9,    //length of this descriptor
	0x4,  //interface descriptor
	0x0,  //interface number
	0x0,  //alternate settings
	0x0,  //number of endpoints without ep 0
	0xFF, //class code -> vendor specific
	0x0,  //subclass code
	0x0,  //protocol code
	0x0   //string index for interface descriptor
};

static struct usb_string_descriptor g_lang_desc     = USB_ARRAY_DESC(USB_LANGID_ENG_US);
static struct usb_string_descriptor g_manuf_desc_en = USB_STRING_DESC("marwedels.de");
static struct usb_string_descriptor g_prod_desc_en  = USB_STRING_DESC("UniversalboxARM");
static struct usb_string_descriptor g_serial_desc   = USB_STRING_DESC("TestEverything");

void UsbIrqOnEnter(void) {
	Led1Red();
}

void UsbIrqOnLeave(void) {
	Led1Off();
}

static usbd_respond usbGetDesc(usbd_ctlreq *req, void **address, uint16_t *length) {
	const uint8_t dtype = req->wValue >> 8;
	const uint8_t dnumber = req->wValue & 0xFF;
	void* desc = NULL;
	uint16_t len = 0;
	usbd_respond result = usbd_fail;
	switch (dtype) {
		case USB_DTYPE_DEVICE:
			desc = g_deviceDescriptor;
			len = sizeof(g_deviceDescriptor);
			result = usbd_ack;
			break;
		case USB_DTYPE_CONFIGURATION:
			desc = g_DeviceConfiguration;
			len = sizeof(g_DeviceConfiguration);
			result = usbd_ack;
			break;
		case USB_DTYPE_STRING:
			if (dnumber < 4) {
				struct usb_string_descriptor * pStringDescr = NULL;
				if (dnumber == 0) {
					pStringDescr = &g_lang_desc;
				}
				if (dnumber == 1) {
					pStringDescr = &g_manuf_desc_en;
				}
				if (dnumber == 2) {
					pStringDescr = &g_prod_desc_en;
				}
				if (dnumber == 3) {
					pStringDescr = &g_serial_desc;
				}
				desc = pStringDescr;
				len = pStringDescr->bLength;
				result = usbd_ack;
			}
			break;
	}
	*address = desc;
	*length = len;
	return result;
}

static usbd_respond usbSetConf(usbd_device *dev, uint8_t cfg) {
	usbd_respond result = usbd_fail;
	switch (cfg) {
		case 0:
			//deconfig
			break;
		case 1:
			//set config
			result = usbd_ack;
			break;
	}
	return result;
}

static usbd_respond usbControl(usbd_device *dev, usbd_ctlreq *req, usbd_rqc_callback *callback) {
	//Printing can be done here as long it is buffered. Otherwise it might be too slow
	if ((req->bmRequestType & (USB_REQ_TYPE | USB_REQ_RECIPIENT)) == (USB_REQ_VENDOR | USB_REQ_DEVICE)) {
		//use req->bRequest to implement some custom control command
		//return usbd_ack; if the command was valid
	}
	return usbd_fail;
}

bool g_usbEnabled;

void TestUsb(void) {
	if (g_usbEnabled == true) {
		printf("\r\nStopping USB\r\n");
		UsbStop();
		printf("USB disconnected\r\n");
		g_usbEnabled = false;
		return;
	}
	printf("\r\nStarting USB\r\n");
	int32_t result = UsbStart(&g_usbDev, &usbSetConf, &usbControl, &usbGetDesc);
	if (result == -1) {
		printf("Error, failed to start 48MHz clock. Error: %u\r\n", (unsigned int)result);
		return;
	}
	if (result == -2) {
		printf("Error, failed to set USB clock source\r\n");
		return;
	}
	uint32_t laneState = result;
	if (laneState == usbd_lane_unk) {
		printf("Connection state: Unknown charger\r\n");
	}
	if (laneState == usbd_lane_dsc) {
		printf("Connection state: disconnected\r\n");
	}
	if (laneState == usbd_lane_sdp) {
		printf("Connection state: standard downstream port\r\n");
	}
	if (laneState == usbd_lane_cdp) {
		printf("Connection state: charging downstream port\r\n");
	}
	if (laneState == usbd_lane_dcp) {
		printf("Connection state: dedicated charging port\r\n");
	}
	g_usbEnabled = true;
	HAL_Delay(100);
	uint32_t info = usbd_getinfo(&g_usbDev);
	printf("There should now be an USB device connected. Status 0x%x decoding as:\r\n", (unsigned int)info);
	if (info == USBD_HW_ADDRFST) {
		printf("  Set address\r\n");
	}
	if (info & USBD_HW_BC) {
		printf("  Battery charging\r\n");
	}
	if (info & USBD_HW_ENABLED) {
		printf("  USB HW enabled\r\n");
	}
	if ((info & USBD_HW_ENUMSPEED) == USBD_HW_SPEED_HS) {
		printf("  USB high speed\r\n"); //not supported anyway
	} else if ((info & USBD_HW_ENUMSPEED) == USBD_HW_SPEED_FS) {
		printf("  USB full speed\r\n");
	} else if ((info & USBD_HW_ENUMSPEED) == USBD_HW_SPEED_LS) {
		printf("  USB low speed\r\n");
	} else {
		printf("  USB not connected\r\n");
	}
	printf("The USB traffic is signaled by the red LED. The bogo MIPS should jitter now.\r\n");
	printf("A second call disconnects again\r\n");
}

void MinPower(void) {
	printf("\r\nMinimize power for 4 seconds\r\n");
	UsbStop();
	g_usbEnabled = false;
	Led1Green();
	PeripheralPowerOff();
	AdcStop();
	EspStop();
	if (!ClockToMsi(1000000)) { //1MHz
		return;
	}
	HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE2);
	Led1Off();
	Led2Off();
	//no we are slow with 1MHz
	uint32_t timeout = HAL_GetTick() + 1000 * 4;
	while (timeout > HAL_GetTick())
	{
		__WFI();
	}
	HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);
	ClockToHsi();
	PeripheralPowerOn();
	printf("Power back on\r\n");
}

void AppCycle(void) {
	Led2Green();
	HAL_Delay(250);
	Led2Off();
	HAL_Delay(250);
	char input = Rs232GetChar();
	if (input) {
		printf("%c", input);
	}
	switch (input) {
		case '0': ReadSensors(); break;
		case '1': SetLeds(); break;
		case '2': SetRelays(); break;
		case '3': Check32kCrystal(); break;
		case '4': CheckHseCrystal(); break;
		case '5': Speed32M(); break;
		case '6': Speed64M(); break;
		case '7': ToggleFlashPrefetch(); break;
		case '8': ToggleFlashCache(); break;
		case '9': Bogomips(); break;
		case 'a': CheckFlash(); break;
		case 'b': SetFlashPagesize(); break;
		case 'c': CheckEsp(); break;
		case 'd': SetLcdBacklight(); break;
		case 'e': WriteLcd(); break;
		case 'f': WriteLcdBig(); break;
		case 'g': WritePixelLcd(); break;
		case 'h': MainMenu(); break;
		case 'i': CheckIr(); break;
		case 'j': PeripheralPowercycle(); break;
		case 'k': CheckCoprocComm(); break;
		case 'l': MinPower(); break;
		case 'm': ManualLcdCmd(); break;
		case 'n': ManualSpiCoprocLevel(); break;
		case 'p': ChangePullPin(); break;
		case 'r': NVIC_SystemReset(); break;
		case 's': JumpDfu(); break;
		case 't': RebootToNormal(); break;
		case 'u': TestUsb(); break;
		case 'z': RebootToDfu(); break;
		default: break;
	}
}
