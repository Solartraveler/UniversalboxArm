/* Boxlib
(c) 2024 by Malte Marwedel

SPDX-License-Identifier: BSD-3-Clause

Thanks to get it working goes to
https://electronics.stackexchange.com/questions/408907/stm32-adcdma-occurring-only-once

*/


#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "boxlib/adcDma.h"

#include "main.h"

//Do not use for prints from within an ISR
void AdcPrintRegisters(const char * header) {
	printf("\r\n%s\r\n", header);
	printf("aSR: %x\r\n", (unsigned int)ADC1->SR);
	printf("aCR1: %x\r\n",(unsigned int)ADC1->CR1);
	printf("aCR2: %x\r\n", (unsigned int)ADC1->CR2);
	printf("aSQR1: %x\r\n", (unsigned int)ADC1->SQR1);
	printf("aSQR2: %x\r\n", (unsigned int)ADC1->SQR2);
	printf("aSQR3: %x\r\n", (unsigned int)ADC1->SQR3);
	printf("aDR: %x\r\n", (unsigned int)ADC1->DR);
	printf("aCCR: %x\r\n", (unsigned int)ADC->CCR);

	printf("dLISR: %x\r\n", (unsigned int)DMA2->LISR);
	printf("dCR: %x\r\n", (unsigned int)DMA2_Stream0->CR);
	printf("dNDTR: %x\r\n", (unsigned int)DMA2_Stream0->NDTR);
	printf("dPAR: %x\r\n", (unsigned int)DMA2_Stream0->PAR);
	printf("dM0AR: %x\r\n", (unsigned int)DMA2_Stream0->M0AR);
}

void AdcInit(bool div2, uint8_t prescaler) {
	(void)div2;
	if (prescaler == 0) {
		prescaler = 1; //if the CPU is clocked with more than 50MHz, a divion of 4 or higher must be used
	}
	__HAL_RCC_DMA2_CLK_ENABLE();
	__HAL_RCC_ADC1_CLK_ENABLE();

	//A ADC may only be disabled if the DMA is disabled first
	DMA2_Stream0->CR &= ~DMA_SxCR_EN;
	while(DMA2_Stream0->CR & DMA_SxCR_EN); //if a transfer is ongoing, this waits until it ends
	if (ADC1->CR2 & ADC_CR2_ADON) {
		ADC1->CR2 &= ~ADC_CR2_ADON; //first disable the ADC (from a possible previous init call)
	}
	ADC1_COMMON->CCR = prescaler << ADC_CCR_ADCPRE_Pos;
	ADC->CCR |= ADC_CCR_TSVREFE; //to measure our internal reference voltage on IN17
	AdcSampleTimeSet(7); //slowest
	ADC1->CR1 |= ADC_CR1_SCAN;
	ADC1->CR2 |= ADC_CR2_DMA | ADC_CR2_DDS;
	ADC1->CR2 |= ADC_CR2_ADON;
	//DMA2, Channel0, stream0 supports getting data from the ADC
	DMA2_Stream0->PAR = (uint32_t)(&(ADC1->DR));
	//high priority, 16Bit source, 16Bit destination pointer, increment memory pointer, no isr, peripheral->memory direction, ADC source
	DMA2_Stream0->CR = DMA_SxCR_PL_1 | DMA_SxCR_MSIZE_0 | DMA_SxCR_PSIZE_0 | DMA_SxCR_MINC;
}

void AdcInputsSet(const uint8_t * pAdcChannels, uint8_t numChannels) {
	ADC1->SQR1 = 0; //zero number of channels
	for (uint32_t i = 0; i < numChannels; i++) {
		if (i < 6) {
			ADC1->SQR3 |= pAdcChannels[i] << (ADC_SQR3_SQ1_Pos + i * 5);
		} else if (i < 12) {
			ADC1->SQR2 |= pAdcChannels[i] << (ADC_SQR2_SQ7_Pos + (i - 6) * 5);
		} else {
			ADC1->SQR1 |= pAdcChannels[i] << (ADC_SQR1_SQ13_Pos + (i - 12) * 5);
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
	//According to the datasheet: first the dma needs to be disabled, then the peripheral
	DMA2_Stream0->CR &= ~DMA_SxCR_EN;
	while(DMA2_Stream0->CR & DMA_SxCR_EN); //if a transfer is ongoing, this waits until it ends

	ADC1->CR2 &= ~ADC_CR2_ADON;

	ADC1->SR &= ~(ADC_SR_EOC | ADC_SR_STRT | ADC_SR_OVR); //clear end of conversion bit
	//AdcPrintRegisters("Start");
	ADC1->CR2 |= ADC_CR2_ADON;

	DMA2_Stream0->M0AR = (uint32_t)pOutput;
	DMA2_Stream0->NDTR = ((ADC1->SQR1 & ADC_SQR1_L_Msk) >> ADC_SQR1_L_Pos) + 1;
	DMA2->LIFCR = DMA_LIFCR_CTCIF0 | DMA_LIFCR_CHTIF0;
	while(DMA2->LISR & (DMA_LISR_TCIF0 | DMA_LISR_HTIF0));
	DMA2_Stream0->CR |= DMA_SxCR_EN;
	ADC1->CR2 |= ADC_CR2_SWSTART;
	//AdcPrintRegisters("Started");
}

bool AdcIsBusy(void) {
	/*There is no busy flag present in the ADC, and the end of conversion
	  flag might be already cleared because the DMA read it.
	*/
	if (ADC1->SR & ADC_SR_STRT) {
		if (!(ADC1->SR & ADC_SR_EOC)) {
			if (!(DMA2->LISR & DMA_LISR_TCIF0)) {
				return true;
			}
		}
	}
	return false;
}

bool AdcIsDone(void) {
	/*Without one nop, the register is read to have the flag set on the first call.
	  The second nop is just to be safe, as it is unknown how close to 'it works'
	  the first nop is.
	  A printf would also help...
	  A chain of
	  if (DMA2->LISR & DMA_LISR_TCIF0)
	    if (DMA2->LISR & DMA_LISR_TCIF0)
	      if (DMA2->LISR & DMA_LISR_TCIF0)
	        if (DMA2->LISR & DMA_LISR_TCIF0)
	 does not work either. But put a nop somewhere inbetween and then its working...
	*/
	asm volatile ("nop");
	asm volatile ("nop");
	if (DMA2->LISR & DMA_LISR_TCIF0) {
		//AdcPrintRegisters("Complete");
		return true;
	}
	//AdcPrintRegisters("Waiting");
	return false;
}

float AdcAvrefGet(void) {
	uint8_t input = 17;
	uint16_t result = 0;
	//Internal reference voltage: 1.21V typical (min 1.18V, max 1.24V)
	AdcInputsSet(&input, 1);
	AdcStartTransfer(&result);
	while (AdcIsDone() == false);
	if (result) {
		return 1.21f * 4095.0f / (float)result;
	}
	return 0;
}
