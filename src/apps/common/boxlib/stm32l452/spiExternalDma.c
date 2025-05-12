/* Boxlib
(c) 2021 by Malte Marwedel

SPDX-License-Identifier: BSD-3-Clause
*/

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "boxlib/spiExternal.h"
#include "boxlib/spiGeneric.h"
#include "spiPlatform.h"

#include "main.h"

static uint8_t g_spi1Started; //0: Stopped, 1 = tx only, 2 = rx + tx

//These values must fit together and are defined in the datasheet
#define SPIPORT SPI1

#define DMACHANNELTX DMA1_Channel3
#define DMACHANNELTXSELECTION (1 << DMA_CSELR_C3S_Pos)
#define DMACHANNELTXSELECTIONCLEAR DMA_CSELR_C3S_Msk
#define DMACHANNELTXCLEARFLAGS DMA_IFCR_CGIF3
#define DMACHANNELTXCOMPLETEFLAG DMA_ISR_TCIF3

#define DMACHANNELRX DMA1_Channel2
#define DMACHANNELRXSELECTION (1 << DMA_CSELR_C2S_Pos)
#define DMACHANNELRXSELECTIONCLEAR DMA_CSELR_C2S_Msk
#define DMACHANNELRXCLEARFLAGS DMA_IFCR_CGIF2
#define DMACHANNELRXCOMPLETEFLAG DMA_ISR_TCIF2

void SpiExternalInit(void) {
	SpiExternalBaseInit();
	__HAL_RCC_DMA1_CLK_ENABLE();
	uint32_t clearMask = DMACHANNELTXSELECTIONCLEAR | DMACHANNELRXSELECTIONCLEAR;
	uint32_t setMask = DMACHANNELTXSELECTION | DMACHANNELRXSELECTION;
	SpiPlatformInitDma(SPIPORT, DMACHANNELTX, DMACHANNELRX, clearMask, setMask);
}

void SpiExternalTransferWaitDone(void) {
	if (g_spi1Started) {
		/*The nops are in place because I had some strange timing issues when using
		  the DMA with the ADC. There at least one NOP was needed, otherwise the bit
		  was set already on the first check. The second NOP is just to be sure.
		*/
		asm volatile ("nop");
		asm volatile ("nop");
		while (((DMA1->ISR) & DMACHANNELTXCOMPLETEFLAG) == 0);
		if (g_spi1Started == 2) {
			while (((DMA1->ISR) & DMACHANNELRXCOMPLETEFLAG) == 0);
			SpiPlatformDisableDma(DMACHANNELRX);
		}
		SpiPlatformDisableDma(DMACHANNELTX);
		SpiPlatformWaitDone(SPIPORT);
		SPIPORT->CR2 &= ~(SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN);
		g_spi1Started = 0;
	}
}

void SpiExternalTransferBackground(const uint8_t * dataOut, uint8_t * dataIn, size_t len) {
	SpiExternalTransferWaitDone();
	uint32_t clearMask = DMACHANNELTXCLEARFLAGS | DMACHANNELRXCLEARFLAGS;
	g_spi1Started = SpiPlatformTransferBackground(SPIPORT, DMACHANNELTX, DMACHANNELRX, clearMask, dataOut, dataIn, len);true;
}

void SpiExternalTransferDma(const uint8_t * dataOut, uint8_t * dataIn, size_t len, uint8_t chipSelect, bool resetChipSelect) {
	SpiExternalChipSelect(chipSelect, true);
	SpiExternalTransferBackground(dataOut, dataIn, len);
	SpiExternalTransferWaitDone();
	if (resetChipSelect) {
		SpiExternalChipSelect(chipSelect, false);
	}
}

void SpiExternalTransfer(const uint8_t * dataOut, uint8_t * dataIn, size_t len, uint8_t chipSelect, bool resetChipSelect) {
	SpiExternalTransferDma(dataOut, dataIn, len, chipSelect, resetChipSelect);
}
