#pragma once
/* Boxlib
(c) 2025 by Malte Marwedel

SPDX-License-Identifier: BSD-3-Clause
*/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "main.h"

#define SPIQUEUEDEPTH 4

/*Initalizes the SPI to run in master mode with 8 bit data width, manual chip
  select and a divider of 64. The clock to the SPI has to be enabled before.
  pSpi: Give the desired SPI: SPI1, SPI2, SPI3 etc. Defined in the stm32******.h
*/
static inline void SpiPlatformInit(SPI_TypeDef * pSpi) {
	pSpi->CR1 = 0;
	pSpi->CR1 = SPI_CR1_MSTR | SPI_BAUDRATEPRESCALER_64 | SPI_CR1_SSM | SPI_CR1_SSI;
	pSpi->CR2 = SPI_CR2_DS_0 | SPI_CR2_DS_1 | SPI_CR2_DS_2| SPI_CR2_FRXTH; //8Bit, 8Bit RX threshold
	pSpi->CR1 |= SPI_CR1_SPE;
}

/*Initializes the SPI DMA. To be called after SpiPlatformInit.
  Currently only supports SPI1 and SPI2. SPI3 is on DMA2.
  Look up the correct values in the reference manual.
*/
static inline void SpiPlatformInitDma(SPI_TypeDef * pSpi, DMA_Channel_TypeDef * pDmaTx, DMA_Channel_TypeDef * pDmaRx, uint32_t clearMask, uint32_t setMask) {
	pDmaTx->CPAR = (uint32_t)(&(pSpi->DR));
	//low priority, 8 bit memory, 8 bit peripheral, memory increment, memory to peripheral
	pDmaTx->CCR = DMA_CCR_MINC | DMA_CCR_DIR;

	pDmaRx->CPAR = (uint32_t)(&(pSpi->DR));
	//low priority, 8 bit memory, 8 bit peripheral, memory increment, peripheral to memory
	pDmaRx->CCR = DMA_CCR_MINC;

	//Note: The read-modify-write might conflict with other functions using DMA in a multithreading app
	uint32_t cselr = DMA1_CSELR->CSELR;
	cselr &= ~clearMask;
	cselr |= setMask;
	DMA1_CSELR->CSELR = cselr;
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
	if (sr & SPI_SR_CRCERR) { printf("  CRCERR\r\n");}
	if (sr & SPI_SR_MODF) { printf("  MODF\r\n");}
	if (sr & SPI_SR_OVR) { printf("  OVER\r\n");}
	if (sr & SPI_SR_BSY) { printf("  BSY\r\n");}
	if (sr & SPI_SR_FRE) { printf("  FRE\r\n");}
	printf("  FRLVL %u\r\n", (unsigned int)((sr >> SPI_SR_FRLVL_Pos) & 3));
	printf("  FTLVL %u\r\n", (unsigned int)((sr >> SPI_SR_FTLVL_Pos) & 3));
}

static inline void SpiPlatformDmaRegisterPrint(DMA_Channel_TypeDef * pDmaTx, DMA_Channel_TypeDef * pDmaRx) {
	uint32_t itemsTx = pDmaTx->CNDTR;
	uint32_t itemsRx = pDmaRx->CNDTR;
	printf("Waiting tx: %u, rx: %u\r\n", (unsigned int)itemsTx, (unsigned int)itemsRx);
}

/*Starts a transfer using DMA.
  pSpi: Use some SPI1, SPI2. SPI3 needs adjustments as it is on DMA2...
  pDmaTx: Use the DMA channel supported by the selected SPI for transfer from buffer to peripheral, like DMA1_Channel5 for SPI2.
  pDmaRx: Use the DMA channel supported by the selected SPI for transfer from peripheral to buffer, like DMA1_Channel4 for SPI2.
  dmaClearMask: Use bitmask which clears the transfer complete or error state of the selected pDmaTx and pDmaRx.
                example: DMA_IFCR_CGIF4 | DMA_IFCR_CGIF5
  dataOut: Array to the data to send. May be NULL. In this case 0xFF is sent as data.
  dataIn: Array to the data to receive. May be NULL.
  len: Lenght of dataOut and dataIn. If 0, the function returns without starting a new transfer.
  returns: 0: No transfer started, 1 tx only transfer stared, 2, rx or tx + rx transfer started
*/
static inline uint8_t SpiPlatformTransferBackground(SPI_TypeDef * pSpi, DMA_Channel_TypeDef * pDmaTx, DMA_Channel_TypeDef * pDmaRx,
                        uint32_t dmaClearMask, const uint8_t * dataOut, uint8_t * dataIn, size_t len) {
	if (len == 0) {
		return 0;
	}
	static uint8_t dummyTransfer = 0xFF; //pattern to send if dataOut is NULL
	uint8_t result;
	//printf("Len %u-%u %u\r\n", dataOut ? 1 : 0, dataIn ? 1 : 0, len);
	pDmaTx->CNDTR = len;
	DMA1->IFCR = dmaClearMask;

	if (dataIn) {
		pSpi->CR2 |= SPI_CR2_RXDMAEN;
		pDmaRx->CMAR = (uint32_t)dataIn;
		pDmaRx->CNDTR = len;
		pDmaRx->CCR |= DMA_CCR_EN;
		result = 2;
	} else {
		result = 1;
	}
	pSpi->CR2 |= SPI_CR2_TXDMAEN;
	if (dataOut) {
		pDmaTx->CMAR = (uint32_t)dataOut;
		pDmaTx->CCR |= DMA_CCR_MINC;
	} else {
		pDmaTx->CMAR = (uint32_t)&dummyTransfer;
		pDmaTx->CCR &= ~DMA_CCR_MINC;
	}
	pDmaTx->CCR |= DMA_CCR_EN;
	//SpiPlatformRegisterPrint(pSpi);
	//SpiPlatformDmaRegisterPrint(pDmaTx, pDmaRx);
	return result;
}

/*Waits until an ongoing SPI transfer is stopped.
  pSpi: Use some SPI1, SPI2, SPI3
*/
static inline void SpiPlatformWaitDone(SPI_TypeDef * pSpi) {
	while (pSpi->SR & SPI_SR_FTLVL_Msk);
	while (pSpi->SR & SPI_SR_BSY);
	while (pSpi->SR & SPI_SR_FRLVL_Msk) {
		*(__IO uint8_t *)&(pSpi->DR);
	}
}

//Disables the DMA channel
static inline void SpiPlatformDisableDma(DMA_Channel_TypeDef * pDma) {
	pDma->CCR &= ~DMA_CCR_EN;
}
