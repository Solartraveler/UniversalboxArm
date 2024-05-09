#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>

#include "boxlib/peripheralDma.h"

#include "boxlib/peripheral.h"
#include "main.h"

DMA_HandleTypeDef g_hdma_spi5_rx;

DMA_HandleTypeDef g_hdma_spi5_tx;

//defined in peripheral.c
extern SPI_HandleTypeDef g_hspi5;

volatile bool g_PeripheralDmaDone;

void DMA2_Stream3_IRQHandler(void) {
  HAL_DMA_IRQHandler(&g_hdma_spi5_rx);
}

void DMA2_Stream5_IRQHandler(void) {
	HAL_DMA_IRQHandler(&g_hdma_spi5_tx);
}

void PeripheralTransferComplete(SPI_HandleTypeDef * hspi) {
	(void)hspi;
	g_PeripheralDmaDone = true;
}

void PeripheralInit(void) {
	PeripheralBaseInit(); //must set up g_hspi5 first

	//Copied from the STM cube generator output:
	__HAL_RCC_DMA2_CLK_ENABLE();

	HAL_NVIC_SetPriority(DMA2_Stream3_IRQn, 8, 0);
	HAL_NVIC_EnableIRQ(DMA2_Stream3_IRQn);

	g_hdma_spi5_rx.Instance = DMA2_Stream3;
	g_hdma_spi5_rx.Init.Channel = DMA_CHANNEL_2;
	g_hdma_spi5_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
	g_hdma_spi5_rx.Init.PeriphInc = DMA_PINC_DISABLE;
	g_hdma_spi5_rx.Init.MemInc = DMA_MINC_ENABLE;
	g_hdma_spi5_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
	g_hdma_spi5_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
	g_hdma_spi5_rx.Init.Mode = DMA_NORMAL;
	g_hdma_spi5_rx.Init.Priority = DMA_PRIORITY_LOW;
	g_hdma_spi5_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
	if (HAL_DMA_Init(&g_hdma_spi5_rx) != HAL_OK) {
		printf("Error, failed to init DMA\r\n");
	}

	HAL_NVIC_SetPriority(DMA2_Stream5_IRQn, 8, 0);
	HAL_NVIC_EnableIRQ(DMA2_Stream5_IRQn);

	/* SPI5_TX Init */
	g_hdma_spi5_tx.Instance = DMA2_Stream5;
	g_hdma_spi5_tx.Init.Channel = DMA_CHANNEL_5;
	g_hdma_spi5_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
	g_hdma_spi5_tx.Init.PeriphInc = DMA_PINC_DISABLE;
	g_hdma_spi5_tx.Init.MemInc = DMA_MINC_ENABLE;
	g_hdma_spi5_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
	g_hdma_spi5_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
	g_hdma_spi5_tx.Init.Mode = DMA_NORMAL;
	g_hdma_spi5_tx.Init.Priority = DMA_PRIORITY_LOW;
	g_hdma_spi5_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
	if (HAL_DMA_Init(&g_hdma_spi5_tx) != HAL_OK) {
		printf("Error, failed to init DMA\r\n");
	}

	__HAL_LINKDMA(&g_hspi5, hdmatx, g_hdma_spi5_tx);
	__HAL_LINKDMA(&g_hspi5, hdmarx, g_hdma_spi5_rx);
	HAL_SPI_RegisterCallback(&g_hspi5, HAL_SPI_TX_COMPLETE_CB_ID, &PeripheralTransferComplete);
	HAL_SPI_RegisterCallback(&g_hspi5, HAL_SPI_RX_COMPLETE_CB_ID, &PeripheralTransferComplete);
}


void PeripheralTransferBackground(const uint8_t * dataOut, uint8_t * dataIn, size_t len) {
	if ((dataIn == NULL) && (len > 7)) {
		g_PeripheralDmaDone = false;
		HAL_SPI_Transmit_DMA(&g_hspi5, (uint8_t*)dataOut, len);
	} else if ((dataOut == NULL) && (len > 7)) {
		g_PeripheralDmaDone = false;
		HAL_SPI_Receive_DMA(&g_hspi5, (uint8_t*)dataIn, len);
	} else {
		PeripheralTransfer(dataOut, dataIn, len);
	}
}

void PeripheralTransferWaitDone(void) {
	if (g_hspi5.State != HAL_SPI_STATE_READY) {
		while (!g_PeripheralDmaDone);
	}
}

