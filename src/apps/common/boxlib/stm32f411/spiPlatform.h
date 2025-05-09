#pragma once
/* Boxlib
(c) 2025 by Malte Marwedel

SPDX-License-Identifier: BSD-3-Clause
*/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "main.h"

#define SPIQUEUEDEPTH 1

/*Initalizes the SPI to run in master mode with 8 bit data width, manual chip
  select and a divider of 64. The clock to the SPI has to be enabled before.
  pSpi: Give the desired SPI: SPI1, SPI2, SPI3 etc. Defined in the stm32******.h
*/
static inline void SpiPlatformInit(SPI_TypeDef * pSpi) {
	pSpi->CR1 = 0;
	pSpi->CR1 = SPI_CR1_MSTR | SPI_BAUDRATEPRESCALER_64 | SPI_CR1_SSM | SPI_CR1_SSI;
	pSpi->CR2 = 0;
	pSpi->CR1 |= SPI_CR1_SPE;
}

/*Initializes the SPI DMA. To be called after SpiPlatformInit.
  For STM32F411 SPI5 the values are:
  SpiPlatformInitDma(SPI5, DMA2_Stream5, DMA2_Stream3, 5, 2);
  Look up the correct values in the reference manual.
*/
static inline void SpiPlatformInitDma(SPI_TypeDef * pSpi, DMA_Stream_TypeDef * pDmaTx, DMA_Stream_TypeDef * pDmaRx, uint32_t txChannel, uint32_t rxChannel) {
	pDmaTx->PAR = (uint32_t)(&(pSpi->DR));
	//low priority, 8 bit memory, 8 bit peripheral, memory increment, memory to peripheral and channel
	pDmaTx->CR = DMA_SxCR_MINC | DMA_SxCR_DIR_0 | (txChannel << DMA_SxCR_CHSEL_Pos);

	pDmaRx->PAR = (uint32_t)(&(pSpi->DR));
	//low priority, 8 bit memory, 8 bit peripheral, memory increment, peripheral to memory and channel
	pDmaRx->CR = DMA_SxCR_MINC | (rxChannel << DMA_SxCR_CHSEL_Pos);
}

/*Print the registers of the SPI. Helps debugging.
*/
static inline void SpiPlatformRegisterPrint(SPI_TypeDef * pSpi) {
	uint32_t sr = pSpi->SR;
	printf("CR1: %08x\r\n", (unsigned int)pSpi->CR1);
	printf("CR2: %08x\r\n", (unsigned int)pSpi->CR2);
	printf("SR : %08x\r\n", (unsigned int)sr);
	if (sr & SPI_SR_RXNE) { printf("  RXNE\r\n");}
	if (sr & SPI_SR_TXE) { printf("  TXE\r\n");}
	if (sr & SPI_SR_UDR) { printf("  UDR\r\n");}
	if (sr & SPI_SR_CRCERR) { printf("  CRCERR\r\n");}
	if (sr & SPI_SR_MODF) { printf("  MODF\r\n");}
	if (sr & SPI_SR_OVR) { printf("  OVER\r\n");}
	if (sr & SPI_SR_BSY) { printf("  BSY\r\n");}
	if (sr & SPI_SR_FRE) { printf("  FRE\r\n");}
}

static inline void SpiPlatformDmaRegisterPrint(DMA_Stream_TypeDef * pDmaTx, DMA_Stream_TypeDef * pDmaRx) {
	uint32_t itemsTx = pDmaTx->NDTR;
	uint32_t itemsRx = pDmaRx->NDTR;
	printf("Waiting tx: %u, rx: %u\r\n", (unsigned int)itemsTx, (unsigned int)itemsRx);
}

/*Starts a transfer using DMA. The previous transfer must have been finished.
  pSpi: Use SPI1, ... SPI5
  pDmaTx: Use the DMA channel supported by the selected SPI for transfer from buffer to peripheral, like DMA2_Stream5 for SPI5.
  pDmaRx: Use the DMA channel supported by the selected SPI for transfer from peripheral to buffer, like DMA2_Stream3 for SPI5.
  dmaTxClearRegister: Pointer to DMA1 or DMA2 register LIFCR or HIFCR
  dmaTxClearMask:     Use bitmask which clears the transfer complete or error state of the selected pDmaTx.
  dmaRxClearRegister: Pointer to DMA1 or DMA2 register LIFCR or HIFCR
  dmaRxClearMask:     Use bitmask which clears the transfer complete or error state of the selected pDmaRx.
  dataOut: Array to the data to send. May be NULL. In this case 0xFF is sent as data.
  dataIn: Array to the data to receive. May be NULL.
  len: Length of dataOut and dataIn. If 0, the function returns without starting a new transfer.
  returns: 0: No transfer started, 1 tx only transfer stared, 2, rx or tx + rx transfer started
*/
static inline uint8_t SpiPlatformTransferBackground(SPI_TypeDef * pSpi, DMA_Stream_TypeDef * pDmaTx, DMA_Stream_TypeDef * pDmaRx,
                        __IO uint32_t * dmaTxClearRegister, uint32_t dmaTxClearMask,
                        __IO uint32_t * dmaRxClearRegister, uint32_t dmaRxClearMask,
                        const uint8_t * dataOut, uint8_t * dataIn, size_t len) {
	if (len == 0) {
		return 0;
	}
	static uint8_t dummyTransfer = 0xFF; //pattern to send if dataOut is NULL
	uint8_t result;
	//printf("Len %u-%u %u\r\n", dataOut ? 1 : 0, dataIn ? 1 : 0, len);
	pDmaTx->NDTR = len;
	*(__IO uint32_t *)dmaTxClearRegister = dmaTxClearMask;
	*(__IO uint32_t *)dmaRxClearRegister = dmaRxClearMask;
	/*Analysis shows:
	  1. The SPI starts transferring as soon as it is enabled and the TX DMA is
	     enabled too.
	  2. When we disable and re-enable the SPI, there is a spike on the SPI clock
	     which would be interpreded as clock cycle if the chip select is not disabled.
	     Since we need to chain multiple transfers without disabling chip select,
	     we may not disable the SPI in between.
	  3. Setting up an RX transfer does not start anything, looks like the DMA is waiting
	     for an rising edge of the RXNE bit. Having the bit already set to 1 when enabling
	     RX DMA, does not start a transfer.
	  -> So the SPI needs to be enabled all the time, and the RX be set up before
	     the TX. Then transfer starts as soon as the TX DMA is enabled.
	*/
	if (dataIn) {
		pSpi->CR2 |= SPI_CR2_RXDMAEN;
		pDmaRx->M0AR = (uint32_t)dataIn;
		pDmaRx->NDTR = len;
		pDmaRx->CR |= DMA_SxCR_EN;
		result = 2;
	} else {
		result = 1;
	}
	pSpi->CR2 |= SPI_CR2_TXDMAEN;
	if (dataOut) {
		pDmaTx->M0AR = (uint32_t)dataOut;
		pDmaTx->CR |= DMA_SxCR_MINC;
	} else {
		pDmaTx->M0AR = (uint32_t)&dummyTransfer;
		pDmaTx->CR &= ~DMA_SxCR_MINC;
	}
	pDmaTx->CR |= DMA_SxCR_EN;
	//SpiPlatformRegisterPrint(pSpi);
	//SpiPlatformDmaRegisterPrint(pDmaTx, pDmaRx);
	return result;
}

static inline void SpiPlatformWaitDone(SPI_TypeDef * pSpi) {
	while ((pSpi->SR & SPI_SR_RXNE)) {
		//data are expected here for TX only transfers
		*(__IO uint8_t *)&(pSpi->DR);
	}
	while ((pSpi->SR & SPI_SR_TXE) == 0);
	while (pSpi->SR & SPI_SR_BSY);
	while ((pSpi->SR & SPI_SR_RXNE)) {
		*(__IO uint8_t *)&(pSpi->DR);
	}
	pSpi->CR2 &= ~(SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN);
}

//Disables the DMA stream
static inline void SpiPlatformDisableDma(DMA_Stream_TypeDef * pDma) {
	pDma->CR &= ~DMA_SxCR_EN;
	while(pDma->CR & DMA_SxCR_EN);
}
