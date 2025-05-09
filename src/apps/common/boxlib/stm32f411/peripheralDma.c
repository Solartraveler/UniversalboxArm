#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>

#include "boxlib/peripheralDma.h"

#include "boxlib/peripheral.h"
#include "main.h"
#include "spiPlatform.h"

static uint8_t g_spi5Started; //0: Stopped, 1 = tx only started, 2 = rx + tx started

//These values must fit together and are defined in the datasheet
#define SPIPORT SPI5

#define DMASTREAMTX DMA2_Stream5
#define DMASTREAMTXCHANNEL 5
#define DMASTREAMTXCLEARFLAGS (DMA_HIFCR_CTCIF5 | DMA_HIFCR_CHTIF5 | DMA_HIFCR_CTEIF5 | DMA_HIFCR_CDMEIF5 | DMA_HIFCR_CFEIF5)
#define DMASTREAMTXCLEARREG (DMA2->HIFCR)
#define DMASTREAMTXCOMPLETEFLAG DMA_HISR_TCIF5
#define DMASTREAMTXCOMPLETEREG (DMA2->HISR)

#define DMASTREAMRX DMA2_Stream3
#define DMASTREAMRXCHANNEL 2
#define DMASTREAMRXCLEARFLAGS (DMA_LIFCR_CTCIF3 | DMA_LIFCR_CHTIF3 | DMA_LIFCR_CTEIF3 | DMA_LIFCR_CDMEIF3 | DMA_LIFCR_CFEIF3)
#define DMASTREAMRXCLEARREG (DMA2->LIFCR)
#define DMASTREAMRXCOMPLETEFLAG DMA_LISR_TCIF3
#define DMASTREAMRXCOMPLETEREG (DMA2->LISR)


void PeripheralInit(void) {
	PeripheralBaseInit();
	__HAL_RCC_DMA2_CLK_ENABLE();
	SpiPlatformInitDma(SPIPORT, DMASTREAMTX, DMASTREAMRX, DMASTREAMTXCHANNEL, DMASTREAMRXCHANNEL);
}

void PeripheralTransferWaitDone(void) {
	if (g_spi5Started) {
		/*The nops are in place because I had some strange timing issues when using
		  the DMA with the ADC. There at least one NOP was needed, otherwise the bit
		  was set already on the first check. The second NOP is just to be sure.
		*/
		asm volatile ("nop");
		asm volatile ("nop");
		while ((DMASTREAMTXCOMPLETEREG & DMASTREAMTXCOMPLETEFLAG) == 0);
		if (g_spi5Started == 2) {
			while ((DMASTREAMRXCOMPLETEREG & DMASTREAMRXCOMPLETEFLAG) == 0);
			SpiPlatformDisableDma(DMASTREAMRX);
		}
		SpiPlatformDisableDma(DMASTREAMTX);
		SpiPlatformWaitDone(SPIPORT);
	}
	g_spi5Started = 0;
}

void PeripheralTransferBackground(const uint8_t * dataOut, uint8_t * dataIn, size_t len) {
	PeripheralTransferWaitDone();
	g_spi5Started = SpiPlatformTransferBackground(SPIPORT, DMASTREAMTX, DMASTREAMRX,
	                &DMASTREAMTXCLEARREG, DMASTREAMTXCLEARFLAGS,
	                &DMASTREAMRXCLEARREG, DMASTREAMRXCLEARFLAGS, dataOut, dataIn, len);
}

void PeripheralTransferDma(const uint8_t * dataOut, uint8_t * dataIn, size_t len) {
	PeripheralTransferBackground(dataOut, dataIn, len);
	PeripheralTransferWaitDone();
}

void PeripheralTransfer(const uint8_t * dataOut, uint8_t * dataIn, size_t len) {
	PeripheralTransferDma(dataOut, dataIn, len);
}
