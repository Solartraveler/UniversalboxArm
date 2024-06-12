#include <stdbool.h>
#include <stdio.h>

#include "testEverythingPlatform.h"

#include "boxlib/simpleadc.h"

bool g_usingHsi = true;

const char * g_adcNames[CHANNELS] = {
	"PA0",
	"PA1",
	"PA2",
	"PA3",
	"PA4",
	"PA5",
	"PA6",
	"PA7",
	"PB0",
	"PB1",
	"PC0",
	"PC1",
	"PC2",
	"PC3",
	"PC4",
	"PC5",
	"Tmp",
	"Ref",
	"Tmp"
};

const pin_t g_pins[PIN_NUM] = {
	//CN8
	{GPIOA, GPIO_PIN_0, "PA0, ADC0      "},
	{GPIOA, GPIO_PIN_1, "PA1, ADC1      "},
	{GPIOA, GPIO_PIN_4, "PA4, ADC4      "},
	{GPIOB, GPIO_PIN_0, "PB0, LCD SCK   "},
	{GPIOC, GPIO_PIN_1, "PC1, Key down  "},
	{GPIOC, GPIO_PIN_0, "PC0, Key right "},
	//CN9
	{GPIOB, GPIO_PIN_5, "PB5, "},
	{GPIOA, GPIO_PIN_3, "PA3, Usart RX  "},
	{GPIOA, GPIO_PIN_2, "PA2, Usart TX  "},
	{GPIOA, GPIO_PIN_10,"PA10, LCD MOSI "},
	{GPIOB, GPIO_PIN_3, "PB3, SWO       "},
	{GPIOB, GPIO_PIN_5, "PB5, LCD A0    "},
	{GPIOB, GPIO_PIN_4, "PB4, LCD reset "},
	{GPIOB, GPIO_PIN_10,"PB10, LCD CS   "},
	{GPIOA, GPIO_PIN_8, "PA8, Key up    "},
	//CN5
	{GPIOA, GPIO_PIN_9, "PA9, SD CS     "},
	{GPIOC, GPIO_PIN_7, "PC7, SD SCK    "},
	{GPIOB, GPIO_PIN_6, "PB6,           "},
	{GPIOA, GPIO_PIN_7, "PA7, ADC7      "},
	{GPIOA, GPIO_PIN_6, "PA6, ADC6      "},
	{GPIOA, GPIO_PIN_5, "PA5, LCD backli"},
	{GPIOB, GPIO_PIN_9, "PB9,           "},
	{GPIOB, GPIO_PIN_8, "PB8,           "},
	//other
	{GPIOC, GPIO_PIN_13, "PC13, Key left"},
};

void ClockToHsi(void) {
	printf("Error, %s not implemented\r\n", __func__);
}

void ClockToHse(void) {
	printf("Error, %s not implemented\r\n", __func__);
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

bool ClockToMsi(uint32_t frequency) {
	(void)frequency;
	printf("Error, there is no MSI on this CPU\r\n");
	return false;
}

void ManualSpiCoprocLevel(void) {
	printf("There is no coprocessor connected to the nucleo board\r\n");
}

void ReadSensorsPlatform(void) {
	for (uint32_t i = 0; i < CHANNELS; i++) {
		uint32_t val = AdcGet(i);
		printf("ADC input %u %s: %u\r\n", (unsigned int)i, g_adcNames[i], (unsigned int)val);
	}
}
