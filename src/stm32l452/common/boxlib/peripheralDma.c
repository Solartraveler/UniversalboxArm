#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>

#include "peripheralDma.h"

#include "peripheral.h"
#include "main.h"

DMA_HandleTypeDef g_hdma_spi2_tx;
DMA_HandleTypeDef g_hdma_spi2_rx;


//defined in peripheral.c
extern SPI_HandleTypeDef g_hspi2;

volatile bool g_PeripheralDmaDone;

void DMA1_Channel5_IRQHandler(void) {
	HAL_DMA_IRQHandler(&g_hdma_spi2_tx);
}

void DMA1_Channel4_IRQHandler(void) {
	HAL_DMA_IRQHandler(&g_hdma_spi2_rx);
}

void PeripheralTransferComplete(SPI_HandleTypeDef *hspi) {
	g_PeripheralDmaDone = true;
}

void PeripheralInit(void) {
	PeripheralBaseInit(); //must set up g_hspi2 first

	//Copied from the STM cube generator output:
	__HAL_RCC_DMA1_CLK_ENABLE();

	HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 8, 0);
	HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);
	g_hdma_spi2_tx.Instance = DMA1_Channel5;
	g_hdma_spi2_tx.Init.Request = DMA_REQUEST_1;
	g_hdma_spi2_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
	g_hdma_spi2_tx.Init.PeriphInc = DMA_PINC_DISABLE;
	g_hdma_spi2_tx.Init.MemInc = DMA_MINC_ENABLE;
	g_hdma_spi2_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
	g_hdma_spi2_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
	g_hdma_spi2_tx.Init.Mode = DMA_NORMAL;
	g_hdma_spi2_tx.Init.Priority = DMA_PRIORITY_LOW;
	if (HAL_DMA_Init(&g_hdma_spi2_tx) != HAL_OK) {
		printf("Error, failed to init DMA\r\n");
	}

	HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 8, 0);
	HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);
	g_hdma_spi2_rx.Instance = DMA1_Channel4;
	g_hdma_spi2_rx.Init.Request = DMA_REQUEST_1;
	g_hdma_spi2_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
	g_hdma_spi2_rx.Init.PeriphInc = DMA_PINC_DISABLE;
	g_hdma_spi2_rx.Init.MemInc = DMA_MINC_ENABLE;
	g_hdma_spi2_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
	g_hdma_spi2_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
	g_hdma_spi2_rx.Init.Mode = DMA_NORMAL;
	g_hdma_spi2_rx.Init.Priority = DMA_PRIORITY_LOW;
	if (HAL_DMA_Init(&g_hdma_spi2_rx) != HAL_OK) {
		printf("Error, failed to init DMA\r\n");
	}
	__HAL_LINKDMA(&g_hspi2, hdmatx, g_hdma_spi2_tx);
	__HAL_LINKDMA(&g_hspi2, hdmarx, g_hdma_spi2_rx);
	HAL_SPI_RegisterCallback(&g_hspi2, HAL_SPI_TX_COMPLETE_CB_ID, &PeripheralTransferComplete);
	HAL_SPI_RegisterCallback(&g_hspi2, HAL_SPI_RX_COMPLETE_CB_ID, &PeripheralTransferComplete);
}

void PeripheralTransferBackground(const uint8_t * dataOut, uint8_t * dataIn, size_t len) {
	if ((dataIn == NULL) && (len > 7)) {
		g_PeripheralDmaDone = false;
		HAL_SPI_Transmit_DMA(&g_hspi2, (uint8_t*)dataOut, len);
	} else if ((dataOut == NULL) && (len > 7)) {
		g_PeripheralDmaDone = false;
		HAL_SPI_Receive_DMA(&g_hspi2, (uint8_t*)dataIn, len);
	} else {
		PeripheralTransfer(dataOut, dataIn, len);
	}
}

void PeripheralTransferWaitDone(void) {
	if (g_hspi2.State != HAL_SPI_STATE_READY) {
		while (!g_PeripheralDmaDone);
	}
}
