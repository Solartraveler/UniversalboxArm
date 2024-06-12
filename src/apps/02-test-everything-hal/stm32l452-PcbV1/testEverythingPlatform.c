#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "testEverythingPlatform.h"

#include "boxlib/mcu.h"
#include "boxlib/readLine.h"
#include "boxlib/rs232debug.h"
#include "boxlib/simpleadc.h"
#include "main.h"

bool g_usingHsi = true;

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

void ReadSensorsPlatform(void) {
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
}
