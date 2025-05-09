/* Boxlib
(c) 2025 by Malte Marwedel

SPDX-License-Identifier: BSD-3-Clause
*/
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>

#include "main.h"
#include "spiPlatform.h"
#include "boxlib/spiExternal.h"
#include "boxlib/spiExternalDma.h"

static uint8_t g_spi2Started; //0: Stopped, 1 = tx only started, 2 = rx + tx started

//These values must fit together and are defined in the datasheet
#define SPIPORT SPI2

#define DMASTREAMTX DMA1_Stream4
#define DMASTREAMTXCHANNEL 0
#define DMASTREAMTXCLEARFLAGS (DMA_HIFCR_CTCIF4 | DMA_HIFCR_CHTIF4 | DMA_HIFCR_CTEIF4 | DMA_HIFCR_CDMEIF4 | DMA_HIFCR_CFEIF4)
#define DMASTREAMTXCLEARREG (DMA1->HIFCR)
#define DMASTREAMTXCOMPLETEFLAG DMA_HISR_TCIF4
#define DMASTREAMTXCOMPLETEREG (DMA1->HISR)

#define DMASTREAMRX DMA1_Stream3
#define DMASTREAMRXCHANNEL 0
#define DMASTREAMRXCLEARFLAGS (DMA_LIFCR_CTCIF3 | DMA_LIFCR_CHTIF3 | DMA_LIFCR_CTEIF3 | DMA_LIFCR_CDMEIF3 | DMA_LIFCR_CFEIF3)
#define DMASTREAMRXCLEARREG (DMA1->LIFCR)
#define DMASTREAMRXCOMPLETEFLAG DMA_LISR_TCIF3
#define DMASTREAMRXCOMPLETEREG (DMA1->LISR)

void SpiExternalInit(void) {
	SpiExternalBaseInit();
	__HAL_RCC_DMA1_CLK_ENABLE();
	SpiPlatformInitDma(SPIPORT, DMASTREAMTX, DMASTREAMRX, DMASTREAMTXCHANNEL, DMASTREAMRXCHANNEL);
}

void SpiExternalTransferWaitDone(void) {
	if (g_spi2Started) {
		/*The nops are in place because I had some strange timing issues when using
		  the DMA with the ADC. There at least one NOP was needed, otherwise the bit
		  was set already on the first check. The second NOP is just to be sure.
		*/
		asm volatile ("nop");
		asm volatile ("nop");
		//According to the datasheet the stream needs to be disabled before the peripheral
		while ((DMASTREAMTXCOMPLETEREG & DMASTREAMTXCOMPLETEFLAG) == 0);
		if (g_spi2Started == 2) {
			while ((DMASTREAMRXCOMPLETEREG & DMASTREAMRXCOMPLETEFLAG) == 0);
			SpiPlatformDisableDma(DMASTREAMRX);
		}
		SpiPlatformDisableDma(DMASTREAMTX);
		SpiPlatformWaitDone(SPIPORT);
	}
	g_spi2Started = 0;
}

void SpiExternalTransferDma(const uint8_t * dataOut, uint8_t * dataIn, size_t len, uint8_t chipSelect, bool resetChipSelect) {
	SpiExternalChipSelect(chipSelect, true);
	g_spi2Started = SpiPlatformTransferBackground(SPIPORT, DMASTREAMTX, DMASTREAMRX,
	                &DMASTREAMTXCLEARREG, DMASTREAMTXCLEARFLAGS,
	                &DMASTREAMRXCLEARREG, DMASTREAMRXCLEARFLAGS, dataOut, dataIn, len);
	SpiExternalTransferWaitDone();
	if (resetChipSelect) {
		SpiExternalChipSelect(chipSelect, false);
	}
}

void SpiExternalTransfer(const uint8_t * dataOut, uint8_t * dataIn, size_t len, uint8_t chipSelect, bool resetChipSelect) {
	SpiExternalTransferDma(dataOut, dataIn, len, chipSelect, resetChipSelect);
}
