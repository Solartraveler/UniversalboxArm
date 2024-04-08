/* Boxlib
(c) 2024 by Malte Marwedel

SPDX-License-Identifier: BSD-3-Clause
*/


#include <stdbool.h>
#include <stdint.h>

#include "boxlib/adcDma.h"

#include "main.h"

void AdcInit(bool div2, uint8_t prescaler) {
	//prepare ADC
	__HAL_RCC_ADC_CLK_ENABLE();
	if (ADC1->CR & ADC_CR_ADEN) {
		ADC1->CR |= ADC_CR_ADDIS; //first disable the ADC (from a possible previous init call)
		while (ADC1->CR & ADC_CR_ADDIS);
	}
	ADC1->CR &= ~ADC_CR_DEEPPWD; //disable power down
	HAL_Delay(1);
	ADC1->CR |= ADC_CR_ADVREGEN; //enable voltage regulator
	HAL_Delay(1);
	uint32_t ccr = ADC_CCR_VREFEN | (prescaler << ADC_CCR_PRESC_Pos);
	if (div2) {
		ccr |= ADC_CCR_CKMODE_1;
	} else {
		ccr |= ADC_CCR_CKMODE_0;
	}
	ADC1_COMMON->CCR = ccr;
	ADC1->CR &= ~ADC_CR_ADCALDIF; //not calibrate differential mode
	ADC1->CR |= ADC_CR_ADCAL; //start calibration
	while (ADC1->CR & ADC_CR_ADCAL); //wait for calibration to end
	HAL_Delay(1); //errata workaround. TODO: Check if needed for STM32L too
	ADC1->CR |= ADC_CR_ADEN; //can not be done within 4 adc clock cycles, due errata
	while ((ADC1->ISR & ADC_ISR_ADRDY) == 0);
	AdcSampleTimeSet(7); //slowest
	ADC1->CFGR |= ADC_CFGR_DMAEN; //single shot DMA mode
	//prepare DMA
	__HAL_RCC_DMA1_CLK_ENABLE();
	//DMA1 Channel1 supports getting data from the ADC
	DMA1_Channel1->CPAR = (uint32_t)(&(ADC1->DR));
	//high priority, 16Bit source, 16Bit destination pointer, increment memory pointer, no isr, peripheral->memory direction
	DMA1_Channel1->CCR = DMA_CCR_PL_1 | DMA_CCR_MSIZE_0 | DMA_CCR_PSIZE_0 | DMA_CCR_MINC;
	DMA1_CSELR->CSELR &= ~DMA_CSELR_C1S_Msk; //select ADC
}

void AdcInputsSet(const uint8_t * pAdcChannels, uint8_t numChannels) {
	while (ADC1->CR & ADC_CR_ADSTART); //otherwise the channel can not be changed
	ADC1->SQR1 = 0; //zero number of channels
	for (uint32_t i = 0; i < numChannels; i++) {
		if (i < 4) {
			ADC1->SQR1 |= pAdcChannels[i] << (ADC_SQR1_SQ1_Pos + i * 6);
		} else if (i < 9) {
			ADC1->SQR2 |= pAdcChannels[i] << (ADC_SQR2_SQ5_Pos + (i - 3) * 6);
		} else if (i < 14) {
			ADC1->SQR3 |= pAdcChannels[i] << (ADC_SQR3_SQ10_Pos + (i - 8) * 6);
		} else {
			ADC1->SQR4 |= pAdcChannels[i] << (ADC_SQR4_SQ15_Pos + (i - 13) * 6);
		}
	}
	ADC1->SQR1 |= (numChannels - 1) << ADC_SQR1_L_Pos;
}

//sets the sample time for all the channels
void AdcSampleTimeSet(uint8_t sampleDelay) {
	uint32_t smpr = 0;
	for (uint32_t i = 0; i < 10; i++) {
		smpr |= sampleDelay << (i * 3);
	}
	ADC1->SMPR1 = smpr;
	ADC1->SMPR2 = smpr & 0x7FFFFFF;
}

//pOutput must be an array of numChannels elements
void AdcStartTransfer(uint16_t * pOutput) {
	ADC1->ISR |= ADC_ISR_EOC; //clear end of conversion bit
	DMA1_Channel1->CCR &= ~DMA_CCR_EN;
	DMA1_Channel1->CMAR = (uint32_t)pOutput;
	DMA1_Channel1->CNDTR = (ADC1->SQR1 & ADC_SQR1_L_Msk) + 1;
	DMA1->IFCR = DMA_IFCR_CTCIF1 | DMA_IFCR_CHTIF1;
	DMA1_Channel1->CCR |= DMA_CCR_EN;
	ADC1->CR |= ADC_CR_ADSTART;
}

bool AdcIsBusy(void) {
	if (ADC1->CR & ADC_CR_ADSTART) {
		return true;
	}
	return false;
}

bool AdcIsDone(void) {
	/* There is at least one nop needed, otherwise the flag seems to be set
	   immediately. To be safe, we use two nops. Very similar to the behaviour
	   observed with the STM32F411. When the AdcAvrefGet() function was in another
	   compilation unit, no nop was needed. Looks like here it is inlined and
	   then the timing problems start.
	*/
	asm volatile("nop");
	asm volatile("nop");
	if (DMA1->ISR & DMA_ISR_TCIF1) {
		return true;
	}
	return false;
}

float AdcAvrefGet(void) {
	uint8_t input = 0;
	uint16_t result = 0;
	AdcInputsSet(&input, 1);
	AdcStartTransfer(&result);
	while (AdcIsDone() == false);
	if (result) {
		uint16_t cal = AdcCalibGet();
		return 3.0f * (float)cal/(float)result;
	}
	return 0.0;
}
