#pragma once

/* Locklessfifo
(c) 2025 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause

Implements a simple byte FIFO.
Thread safe with limitations:
FifoDataGet may be called from one thread (or interrupt)
and
FifoDataPut/FifoBufferPut may be called from another thread (or interrupt).
But when calling FifoDataGet from multiple threads, data may be lost.
The same is true for calling FifoDataPut/FifoBufferPut from different threads
at the same time.
FifoDataFree might return old data. So For checking if data can be put,
it should be used by the same thread as FifoDataPut/FifoBufferPut only.
Before any put or get may be called, FifoInit must be completed.

Optionally define a function for FIFO_DATA_GET_OK and/or FIFO_DATA_GET_FAIL before
including this header to get some callback if FifoDataGet is successful or not.
*/

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
	uint8_t * buffer;
	size_t bufferLen;
	size_t readIdx;
	size_t writeIdx;
} FifoState_t;

/*Initializes the given pFS struct. None of the pointer may be NULL.
  The useable FIFO will be one byte less, than provided by fifoLen.
  fifoBuffer: Some static buffer to use, needs to stay valid as long as the fifo is used.
  fifoLen: Length in bytes of fifoBuffer.
*/
static inline void FifoInit(FifoState_t * pFS, uint8_t * fifoBuffer, size_t fifoLen) {
	pFS->buffer = fifoBuffer;
	pFS->bufferLen = fifoLen;
	pFS->readIdx = 0;
	pFS->writeIdx = 0;
}

/*Gets a data byte from the FIFO. If none are available, 0 is returned.
  pFS may not be NULL.
*/
static inline uint8_t FifoDataGet(FifoState_t * pFS) {
	uint8_t out = 0;
	if ((pFS->buffer) && (pFS->readIdx != pFS->writeIdx)) {
		size_t ri = pFS->readIdx;
		out = pFS->buffer[ri];
		__sync_synchronize(); //the pointer increment may only be visible after the copy
		ri++;
		if (ri >= pFS->bufferLen) {
			ri = 0;
		}
		pFS->readIdx = ri;
#ifdef FIFO_DATA_GET_OK
		FIFO_DATA_GET_OK();
#endif
	} else {
#ifdef FIFO_DATA_GET_FAIL
		FIFO_DATA_GET_FAIL();
#endif
	}
	return out;
}

/*Puts a data byte to the FIFO. If the FIFO is full, the data is discharged.
  pFS may not be NULL.
  returns true if there was space in the FIFO. false otherwise.
*/
static inline bool FifoDataPut(FifoState_t * pFS, uint8_t out) {
	bool succeed = false;
	size_t writeThis = pFS->writeIdx;
	size_t writeNext = (writeThis + 1);
	if (writeNext == pFS->bufferLen) {
		writeNext = 0;
	}
	if (writeNext != pFS->readIdx) {
		pFS->buffer[writeThis] = out;
		pFS->writeIdx = writeNext;
		__sync_synchronize(); //just to make sure no nasty things like unifying two FifoDataPut could happen
		succeed = true;
	}
	return succeed;
}

/*Returns the number of bytes which can be put into the FIFO with FifoDataPut
  without getting false as return.
  The maximum value is one less than fifoLen given in FifoInit
*/
static inline size_t FifoDataFree(FifoState_t * pFS) {
	size_t rp = pFS->readIdx;
	size_t wp = pFS->writeIdx;
	__sync_synchronize();
	size_t unused;
	if (wp >= rp) {
		unused = (pFS->bufferLen - wp) + rp;
	} else {
		unused = (rp - wp);
	}
	return (unused - 1);
}

/*Puts a data array to the FIFO. If the FIFO is full, as much bytes as possible are
  put, all other are discharged.
  pFS and data may not be NULL.
  returns true if there was space in the FIFO for all bytes. false otherwise.
*/
static inline bool FifoBufferPut(FifoState_t * pFS, const uint8_t * data, size_t dataLen) {
	bool success = true;
	for (size_t i = 0; i < dataLen; i++) {
		success &= FifoDataPut(pFS, data[i]);
		if (!success) {
			//printf("Breaking at %u byte\r\n", (unsigned int)i);
			break;
		}
	}
	if (!success) {
		//printf("Error, FIFO overflow, wanting to put %u\r\n", (unsigned int)dataLen);
	}
	return success;
}
