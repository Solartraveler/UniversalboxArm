

#include <stdint.h>

//The filename adc is already used by the HAL
#include "simpleadc.h"

#include "main.h"

void AdcInit(void) {
	__HAL_RCC_ADC_CLK_ENABLE();
	ADC1->CR &= ~ADC_CR_DEEPPWD; //disable power down
	HAL_Delay(1);
	ADC1->CR |= ADC_CR_ADVREGEN; //enable voltage regulator
	HAL_Delay(1);
 	//temperature sensor enabled, voltage reference enabled div by 4
	ADC1_COMMON->CCR = ADC_CCR_TSEN | ADC_CCR_VREFEN | ADC_CCR_CKMODE_1;
	ADC1->CR &= ~ADC_CR_ADCALDIF; //not calibrate differential mode
	ADC1->CR |= ADC_CR_ADCAL; //start calibration
	while (ADC1->CR & ADC_CR_ADCAL); //wait for calibration to end
	HAL_Delay(1); //errata workaround. TODO: Check if needed for STM32L too
	ADC1->CR |= ADC_CR_ADEN; //can not be done within 4 adc clock cycles, due errata
	while ((ADC1->ISR & ADC_ISR_ADRDY) == 0);
	/* Configure all channnels with 47.5 ADC clock cycles sampling time
	at 16MHz and a divider of 4 this results in a sample time of 11µs
	The temperature sensore needs at least 5µs sampling time, so this works
	up to 35MHz, otherwise the sampling time needs to be increased.
	*/
	ADC1->SMPR1 = ADC_SMPR1_SMP0_2 | ADC_SMPR1_SMP1_2 | ADC_SMPR1_SMP2_2 |
	              ADC_SMPR1_SMP3_2 | ADC_SMPR1_SMP4_2 | ADC_SMPR1_SMP5_2 |
	              ADC_SMPR1_SMP6_2 | ADC_SMPR1_SMP7_2 | ADC_SMPR1_SMP8_2 |
	              ADC_SMPR1_SMP9_2;
	ADC1->SMPR2 = ADC_SMPR2_SMP10_2 | ADC_SMPR2_SMP11_2 | ADC_SMPR2_SMP12_2 |
	              ADC_SMPR2_SMP13_2 | ADC_SMPR2_SMP14_2 | ADC_SMPR2_SMP15_2 |
	              ADC_SMPR2_SMP16_2 | ADC_SMPR2_SMP17_2 | ADC_SMPR2_SMP18_2;
}

uint16_t AdcGet(uint32_t channel) {
	if (channel >= 32) {
		return 0xFFFF; //impossible value
	}
	ADC1->ISR |= ADC_ISR_EOC; //clear end of conversion bit
	while (ADC1->CR & ADC_CR_ADSTART); //otherwise the channel can not be changed
	ADC1->SQR1 = channel << ADC_SQR1_SQ1_Pos; //all other bits stay zero
	ADC1->CR |= ADC_CR_ADSTART;
	while ((ADC1->ISR & ADC_ISR_EOC) == 0);
	uint16_t val = ADC1->DR;
	return val;
}

void AdcStop(void) {
	ADC1->CR &= ~ADC_CR_ADVREGEN;
	ADC1->CR |= ADC_CR_DEEPPWD;
	__HAL_RCC_ADC_CLK_DISABLE();
}
