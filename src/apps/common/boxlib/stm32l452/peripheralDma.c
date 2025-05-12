/* Boxlib
(c) 2025 by Malte Marwedel

SPDX-License-Identifier: BSD-3-Clause
*/

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>

#include "boxlib/peripheralDma.h"

#include "boxlib/peripheral.h"
#include "boxlib/spiGeneric.h"
#include "spiPlatform.h"
#include "main.h"

static uint8_t g_spi2Started; //0: Stopped, 1 = tx only, 2 = rx + tx

//These values must fit together and are defined in the datasheet
#define SPIPORT SPI2

#define DMACHANNELTX DMA1_Channel5
#define DMACHANNELTXSELECTION (1 << DMA_CSELR_C5S_Pos)
#define DMACHANNELTXSELECTIONCLEAR DMA_CSELR_C5S_Msk
#define DMACHANNELTXCLEARFLAGS DMA_IFCR_CGIF5
#define DMACHANNELTXCOMPLETEFLAG DMA_ISR_TCIF5

#define DMACHANNELRX DMA1_Channel4
#define DMACHANNELRXSELECTION (1 << DMA_CSELR_C4S_Pos)
#define DMACHANNELRXSELECTIONCLEAR DMA_CSELR_C4S_Msk
#define DMACHANNELRXCLEARFLAGS DMA_IFCR_CGIF4
#define DMACHANNELRXCOMPLETEFLAG DMA_ISR_TCIF4


void PeripheralInit(void) {
	PeripheralBaseInit();
	__HAL_RCC_DMA1_CLK_ENABLE();
	uint32_t clearMask = DMACHANNELTXSELECTIONCLEAR | DMACHANNELRXSELECTIONCLEAR;
	uint32_t setMask = DMACHANNELTXSELECTION | DMACHANNELRXSELECTION;
	SpiPlatformInitDma(SPIPORT, DMACHANNELTX, DMACHANNELRX, clearMask, setMask);
}

void PeripheralTransferWaitDone(void) {
	if (g_spi2Started) {
		/*The nops are in place because I had some strange timing issues when using
		  the DMA with the ADC. There at least one NOP was needed, otherwise the bit
		  was set already on the first check. The second NOP is just to be sure.
		*/
		asm volatile ("nop");
		asm volatile ("nop");
		while (((DMA1->ISR) & DMACHANNELTXCOMPLETEFLAG) == 0);
		if (g_spi2Started == 2) {
			while (((DMA1->ISR) & DMACHANNELRXCOMPLETEFLAG) == 0);
			SpiPlatformDisableDma(DMACHANNELRX);
		}
		SpiPlatformDisableDma(DMACHANNELTX);
		SpiPlatformWaitDone(SPIPORT);
		SPIPORT->CR2 &= ~(SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN);
		g_spi2Started = 0;
	}
}

void PeripheralTransferBackground(const uint8_t * dataOut, uint8_t * dataIn, size_t len) {
	PeripheralTransferWaitDone();
	uint32_t clearMask = DMACHANNELTXCLEARFLAGS | DMACHANNELRXCLEARFLAGS;
	g_spi2Started = SpiPlatformTransferBackground(SPIPORT, DMACHANNELTX, DMACHANNELRX, clearMask, dataOut, dataIn, len);
}

void PeripheralTransferDma(const uint8_t * dataOut, uint8_t * dataIn, size_t len) {
	PeripheralTransferBackground(dataOut, dataIn, len);
	PeripheralTransferWaitDone();
}

void PeripheralTransfer(const uint8_t * dataOut, uint8_t * dataIn, size_t len) {
	PeripheralTransferDma(dataOut, dataIn, len);
}
