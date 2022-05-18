#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>

#include "peripheralDma.h"

#include "peripheral.h"
#include "main.h"
#include "spi.h"

DMA_HandleTypeDef g_hdma_spi2_tx;

volatile bool g_PeripheralDmaDone;

void DMA1_Channel5_IRQHandler(void) {
	HAL_DMA_IRQHandler(&g_hdma_spi2_tx);
}

void PeripheralTransferComplete(SPI_HandleTypeDef *hspi) {
	g_PeripheralDmaDone = true;
}

void PeripheralInit(void) {
	//Copied from the STM cube generator output:
	__HAL_RCC_DMA1_CLK_ENABLE();
	HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 0, 0);
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
	__HAL_LINKDMA(&hspi2,hdmatx,g_hdma_spi2_tx);
	HAL_SPI_RegisterCallback(&hspi2, HAL_SPI_TX_COMPLETE_CB_ID, &PeripheralTransferComplete);
}

void PeripheralTransferBackground(const uint8_t * dataOut, uint8_t * dataIn, size_t len) {
	if ((dataIn == NULL) && (len > 7)) {
		g_PeripheralDmaDone = false;
		HAL_SPI_Transmit_DMA(&hspi2, (uint8_t*)dataOut, len);
	} else {
		PeripheralTransfer(dataOut, dataIn, len);
	}
}

void PeripheralTransferWaitDone(void) {
	if (hspi2.State != HAL_SPI_STATE_READY) {
		while (!g_PeripheralDmaDone);
	}
}
