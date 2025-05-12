#pragma once
/* Boxlib
(c) 2025 by Malte Marwedel

SPDX-License-Identifier: BSD-3-Clause
*/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "main.h"
//For SPIQUEUEDEPTH:
#include "spiPlatform.h"

/*Transfers data over SPI by polling. SPI needs to be initialized, the output
  pins configured and chip select properly set.
  pSpi: Give the desired SPI: SPI1, SPI2, SPI3 etc. Defined in the stm32******.h
  dataOut: Data to send. If not NULL, must have the length len.
  dataIn: Buffer to store the incoming data to. If not NULL, must have the length len.
  len: Number of bytes to send or receive.
  If dataOut is NULL, 0xFF will be send.
  If dataIn is NULL, the incoming data are just discarded.
  If dataOut and dataIn are NULL, just 0xFF is sent with the lenght len.
  Returns true if sending did not end in a timeout. Timeout is 100 ticks in most cases.
  (100 ticks are usually 100ms).
*/
static inline bool SpiGenericPolling(SPI_TypeDef * pSpi, const uint8_t * dataOut, uint8_t * dataIn, size_t len) {
	/*We can not simply fill data to the input buffer as long as there
	  is room free. Because if we have still data in the incoming buffer and then
	  then fill the outgoing buffer and then get a longer interrupt (by a interrupt
	  or a task scheduler) our input would be filled up by more bytes than the rx
	  buffer can handle and end with an overflow there.
	  So we can only fill as much data to the outgoing buffer as there is space in the incoming buffer too.
	  For the STM32F411, according to the datasheet there the buffers seems to support only one element.
	  For the STM32L452, the datasheet mentions 32bit of FIFO. This should be 4x 8bit data.
	  As result, there will be an interruption for the STM32F411 between one byte sent and the next one put
	  into the TX buffer, whereas the STM32L452 could work without interruption and reach the maximum possible
	  SPI speed.
	*/
	size_t inQueue = 0;
	const size_t inQueueMax = SPIQUEUEDEPTH; //defined by hardware
	bool success = true;
	bool timeout = false;
	size_t txLen = len;
	size_t rxLen = len;
	//printf("Len: %u\r\n", (unsigned int)len);
	uint32_t timeStart = HAL_GetTick();
	while ((rxLen) || (pSpi->SR & SPI_SR_BSY)) {
		//Because a set overflow bit is cleared by each read, so read an check only once in the loop
		uint32_t sr = pSpi->SR;
		if ((sr & SPI_SR_TXE) && (inQueue < inQueueMax) && (txLen)) {
			uint8_t data = 0xFF; //leave signal high if we are only want to receive
			if (dataOut) {
				data = *dataOut;
				dataOut++;
			}
			//printf("W\r\n");
			//Without the cast, 16bit are written, resulting in a second byte with value 0x0 send
			*(__IO uint8_t *)&(pSpi->DR) = data;
			inQueue++;
			txLen--;
			timeout = false;
		}
		if ((sr & SPI_SR_RXNE) && (inQueue) && (rxLen)) {
			//printf("R");
			//Without the cast, we might get up to two bytes at once
			uint8_t data = *(__IO uint8_t *)&(pSpi->DR);
			if (dataIn) {
				*dataIn = data;
				//printf("%x\r\n", data);
				dataIn++;
			}
			inQueue--;
			rxLen--;
			timeout = false;
		}
		if (sr & SPI_SR_OVR) {
			//printf("Overflow!\r\n");
			success = false;
			break;
		}
		if (timeout) {
			//printf("Timeout!\r\n");
			success = false;
			break;
		}
		if ((HAL_GetTick() - timeStart) >= 100) {
			/*If we have a multitasking system, the timeout might be just the cause
			  of the scheduler running a higher priority task for 100ms. So we need
			  to check if there is really nothing to do right now. So only if the
			  next loop still does not produce anything, we assume the reason of the
			  timeout is a non working SPI.
			*/
			//printf("Timeout?\r\n");
			timeout = true;
			timeStart = HAL_GetTick();
		}
	}
	return success;
}

/*Sets the prescaler of the SPI. Supported values are 2, 4, 8, 16 .... 256.
  Other values will be rounded to the next higher prescaler. Everything above
  256 will be handled as 256.
*/
static inline void SpiGenericPrescaler(SPI_TypeDef * pSpi, uint32_t prescaler) {
	uint32_t bits;
	if (prescaler <= 2) {
		bits = SPI_BAUDRATEPRESCALER_2;
	} else if (prescaler <= 4) {
		bits = SPI_BAUDRATEPRESCALER_4;
	} else if (prescaler <= 8) {
		bits = SPI_BAUDRATEPRESCALER_8;
	} else if (prescaler <= 16) {
		bits = SPI_BAUDRATEPRESCALER_16;
	} else if (prescaler <= 32) {
		bits = SPI_BAUDRATEPRESCALER_32;
	} else if (prescaler <= 64) {
		bits = SPI_BAUDRATEPRESCALER_64;
	} else if (prescaler <= 128) {
		bits = SPI_BAUDRATEPRESCALER_128;
	} else {
		bits = SPI_BAUDRATEPRESCALER_256;
	}
	uint32_t reg = pSpi->CR1;
	reg &= ~SPI_CR1_BR_Msk;
	reg |= bits;
	pSpi->CR1 = reg;
}
