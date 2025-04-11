#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/*Comment out, if for some reason pulse audio makes problems. Then the app will work,
  just, there is nothing to hear.
*/
#define USE_PULSEAUDIO

#include "boxlib/sequenceToPwm.h"

#include "main.h"
#include "simulated.h"

#ifndef F_CPU
#error "Define F_CPU within main.h or a -DFCPU=123456789 compiler parameter. Value is in Hz"
#endif


#ifdef USE_PULSEAUDIO

#include <unistd.h>
#include <pthread.h>
#include <pulse/simple.h>

#include "locklessfifo.h"

static FifoState_t g_fifo;

static volatile bool g_requestTerminate;

static pthread_t g_outputThread;


void * SeqOutputThread(void * arg) {
	pa_sample_spec * pDataFormat = (pa_sample_spec *)arg;
	pa_simple * pS = pa_simple_new(NULL, "Wav Player", PA_STREAM_PLAYBACK, NULL, "Wave file", pDataFormat, NULL, NULL, NULL);
	if (!pS) {
		printf("Error, opening pulse audio output failed\n");
		return NULL;
	}
	while (g_requestTerminate == false) {
		while ((FifoDataFree(&g_fifo) + 1) < g_fifo.bufferLen) {
			uint8_t data = FifoDataGet(&g_fifo);
			int err = 0;
			if (pa_simple_write(pS, &data, 1, &err)) {
				printf("Error, could not write data to pulse audio, errorcode %i\n", err);
			}
		}
		usleep(2000);
	}
	pa_simple_free(pS);
	return NULL;
}

void SeqStart(uint32_t pwmDivider, uint32_t seqMax, uint8_t * fifoBuffer, size_t fifoLen) {
	(void)pwmDivider;
	FifoInit(&g_fifo, fifoBuffer, fifoLen);
	//Initialize pulseaudio output
	static pa_sample_spec dataFormat;
	dataFormat.format = PA_SAMPLE_U8;
	dataFormat.channels = 1;
	dataFormat.rate = (F_CPU / seqMax);
	printf("Pulse audio rate %uHz\n", (unsigned int)dataFormat.rate);
	//start a thread for data processing
	g_requestTerminate = false;
	if (pthread_create(&g_outputThread, NULL, &SeqOutputThread, &dataFormat)) {
		printf("Error, starting thread for audio failed\n");
	}
}

void SeqStop(void) {
	g_requestTerminate = true;
	if (g_outputThread) {
		pthread_join(g_outputThread, NULL);
		g_outputThread = 0;
	}
}

size_t SeqFifoFree(void) {
	return FifoDataFree(&g_fifo);
}

void SeqFifoPut(const uint8_t * data, size_t dataLen) {
	FifoBufferPut(&g_fifo, data, dataLen);
}

#else

static size_t g_seqSize;
static size_t g_seqFree;
static float g_seqConsumptionSecond;
static uint32_t g_seqLastCheck;

void SeqStart(uint32_t pwmDivider, uint32_t seqMax, uint8_t * fifoBuffer, size_t fifoLen) {
	(void)pwmDivider;
	(void)fifoBuffer;
	g_seqSize = fifoLen;
	g_seqFree = fifoLen;
	g_seqConsumptionSecond = (F_CPU / seqMax);
	g_seqLastCheck = HAL_GetTick();
}

void SeqStop(void) {
	g_seqFree = g_seqSize;
}

size_t SeqFifoFree(void) {
	uint32_t t = HAL_GetTick();
	float deltaS = (t - g_seqLastCheck) / 1000.0;
	float consumed = deltaS * g_seqConsumptionSecond;
	uint32_t waiting = g_seqSize - g_seqFree;
	if (consumed < waiting) {
		g_seqFree += consumed;
	} else {
		g_seqFree = g_seqSize;
	}
	g_seqLastCheck = t;
	return g_seqFree;
}

void SeqFifoPut(const uint8_t * data, size_t dataLen) {
	if (SeqFifoFree() >= dataLen) {
		g_seqFree -= dataLen;
	}
}

#endif


