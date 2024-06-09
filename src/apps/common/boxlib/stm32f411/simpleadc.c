

#include <stdint.h>

//The filename adc is already used by the HAL
#include "boxlib/simpleadc.h"

#include "main.h"

void AdcInit(void) {
	__HAL_RCC_ADC1_CLK_ENABLE();
 	//temperature sensor enabled, voltage reference enabled div by 4
	ADC1_COMMON->CCR = ADC_CCR_TSVREFE | ADC_CCR_ADCPRE_0;
	/* Configure all channnels with 84 ADC clock cycles sampling time.
	*/
	ADC1->SMPR1 = ADC_SMPR1_SMP18_2 | ADC_SMPR1_SMP17_2 | ADC_SMPR1_SMP16_2 |
	              ADC_SMPR1_SMP15_2 | ADC_SMPR1_SMP14_2 | ADC_SMPR1_SMP13_2 |
	              ADC_SMPR1_SMP12_2 | ADC_SMPR1_SMP11_2 | ADC_SMPR1_SMP10_2;
	ADC1->SMPR2 = ADC_SMPR2_SMP9_2 | ADC_SMPR2_SMP8_2 | ADC_SMPR2_SMP7_2 |
	              ADC_SMPR2_SMP6_2 | ADC_SMPR2_SMP5_2 | ADC_SMPR2_SMP4_2 |
	              ADC_SMPR2_SMP3_2 | ADC_SMPR2_SMP2_2 | ADC_SMPR2_SMP1_2 | ADC_SMPR2_SMP0_2;
}

uint16_t AdcGet(uint32_t channel) {
	if (channel >= 32) {
		return 0xFFFF; //impossible value
	}
	ADC1->CR2 &= ~ADC_CR2_ADON;
	ADC1->SR &= ~(ADC_SR_EOC | ADC_SR_STRT | ADC_SR_OVR); //clear end of conversion bit
	ADC1->SQR3 = channel << ADC_SQR3_SQ1_Pos;
	ADC1->CR2 |= ADC_CR2_ADON;
	ADC1->CR2 |= ADC_CR2_SWSTART;
	while ((ADC1->SR & ADC_SR_EOC) == 0);
	uint16_t val = ADC1->DR;
	return val;
}

void AdcStop(void) {
	ADC1->CR2 &= ~ADC_CR2_ADON;
	__HAL_RCC_ADC1_CLK_DISABLE();
}
