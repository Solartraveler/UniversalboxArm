#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/*Comment out, if for some reason pulse audio makes problems. Then the app will work,
  just, there is nothing to hear.
*/
#define USE_PULSEAUDIO

#ifdef USE_PULSEAUDIO
#include <pulse/simple.h>
#endif

#include "sequenceToPwm.h"

#include "simulated.h"

static size_t g_seqSize;
static size_t g_seqFree;
static float g_seqConsumptionSecond;
static uint32_t g_seqLastCheck;

#ifdef USE_PULSEAUDIO
static pa_simple * g_pS;
#endif

void SeqStart(uint32_t pwmDivider, uint32_t seqMax, uint8_t * fifoBuffer, size_t fifoLen) {
	(void)pwmDivider;
	(void)fifoBuffer;
	g_seqSize = fifoLen;
	g_seqFree = fifoLen;
	//F_CPU is 32MHz
	g_seqConsumptionSecond = (32000000 / seqMax);
	g_seqLastCheck = HAL_GetTick();
	//Initialize pulseaudio output
#ifdef USE_PULSEAUDIO
	pa_sample_spec dataFormat;
	dataFormat.format = PA_SAMPLE_U8;
	dataFormat.channels = 1;
	dataFormat.rate = g_seqConsumptionSecond;
	g_pS = pa_simple_new(NULL, "Wav Player", PA_STREAM_PLAYBACK, NULL, "Wave file", &dataFormat, NULL, NULL, NULL);
	if (!g_pS) {
		printf("Error, opening pulse audio output failed\n");
	}
#endif
}

void SeqStop(void) {
	g_seqFree = g_seqSize;
#ifdef USE_PULSEAUDIO
	if (g_pS) {
		pa_simple_free(g_pS);
		g_pS = NULL;
	}
#endif
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
#ifdef USE_PULSEAUDIO
		if (g_pS) {
			int err = 0;
			if (pa_simple_write(g_pS, data, dataLen, &err)) {
				printf("Error, could not write data to pulse audio, errorcode %i\n", err);
			}
		}
#endif
	}
}

