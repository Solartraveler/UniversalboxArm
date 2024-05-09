/* Boxlib
(c) 2024 by Malte Marwedel

SPDX-License-Identifier: BSD-3-Clause
*/


#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "boxlib/adcDma.h"

#include "main.h"

//Do not use for prints from within an ISR
void AdcPrintRegisters(const char * header) {
	printf("Nothing to print in the simulation...\n");
}

void AdcInit(bool div2, uint8_t prescaler) {
	(void)div2;
	(void)prescaler;
}

void AdcInputsSet(const uint8_t * pAdcChannels, uint8_t numChannels) {
	(void)pAdcChannels;
	(void)numChannels;
}

//sets the sample time for all the channels
void AdcSampleTimeSet(uint8_t sampleDelay) {
	(void)sampleDelay;
}

//pOutput must be an array of numChannels elements
void AdcStartTransfer(uint16_t * pOutput) {
	(void)pOutput;
}

bool AdcIsBusy(void) {
	return false;
}

bool AdcIsDone(void) {
	return true;
}

float AdcAvrefGet(void) {
	return 3.3;
}
