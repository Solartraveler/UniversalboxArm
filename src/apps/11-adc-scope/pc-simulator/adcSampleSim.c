#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "adcSample.h"

#define NUM_CHANNELS 3

//Number of samples per channel
#define SAMPLES 320

uint8_t g_selectedInput[NUM_CHANNELS];
uint8_t g_activeInputs;
uint32_t g_sampleRate;
uint16_t g_triggerLevel;
uint8_t g_triggerMode; //0 = one shot, 1 = automatic
uint8_t g_triggerType; //0 = level low, 1 = level high, 2 = falling edge, 3 = rising edge
uint8_t g_triggerReady;
uint8_t g_triggerForce;
uint8_t g_triggerChannel;

void SampleInit(void) {
	memset(g_selectedInput, 0xFF, NUM_CHANNELS);
}

bool SampleRateSet(uint32_t samplesEveryNs) {
	g_sampleRate = samplesEveryNs;
	return true;
}

void SampleTriggerSet(uint16_t level, uint8_t type, uint8_t channel) {
	g_triggerLevel = level;
	g_triggerType = type;
	g_triggerChannel = channel;
}

bool SampleInputsSet(uint8_t * adcChannels, uint8_t numChannels) {
	memset(g_selectedInput, 0xFF, sizeof(g_selectedInput));
	if (numChannels > NUM_CHANNELS) {
		return false;
	}
	for (uint32_t i = 0; i < numChannels; i++) {
		g_selectedInput[i] = adcChannels[i];
	}
	g_activeInputs = numChannels;
	return true;
}

//0 = one shot, 1 = automatic
void SampleModeSet(uint8_t mode) {
	g_triggerMode = mode;
}

//start sampling with the next trigger (if not set to automatic anyway)
void SampleStart(void) {
	if (g_triggerMode == 0)
	{
		g_triggerReady = 1;
	}
}

//start sampling now
void SampleTrigger(void) {
	g_triggerForce = 1;
}

uint16_t SampleSawtooth(float timestamp) {
	timestamp *= 200.0;
	float increment = timestamp;
	uint32_t overflow = increment;
	increment -= overflow; //increment is within the range 0...0.99999
	uint32_t low = ((uint32_t)timestamp) & 1;
	if (low) {
		return increment * ADC_MAX;
	}
	return ADC_MAX - increment * ADC_MAX;
}

uint16_t SampleSine(float timestamp) {
	timestamp *= 2.0*M_PI;
	timestamp *= 100.0; //100Hz
	return ADC_MAX * ((sinf(timestamp) + 1.0) / 2.0);
}

uint16_t SampleRectangle(float timestamp) {
	timestamp *= 200.0;
	uint32_t low = ((uint32_t)timestamp) & 1;
	if (low) {
		return 0;
	}
	return ADC_MAX;
}

uint16_t SampleInput(uint8_t input, float timestamp) {
	uint16_t value = 0;
	if (input == 1) { //sawtooth 1kHz
		value = SampleSawtooth(timestamp);
	}
	if (input == 2) { //sine 1kHz
		value = SampleSine(timestamp);
	}
	if (input == 3) { //rectangle 1kHz
		value = SampleRectangle(timestamp);
	}
	return value;
}

//type: 0 = last buffered, 1 = current incomplete one
//The bufferOut format is channel1 - channel 2 - channel 3 - channel1 etc
//returns: 0: last data as already requested, 1: data have changed
uint8_t SampleGet(uint8_t type, const uint16_t ** pBufferOut, uint32_t * elementsReported) {
	float tIncrement = (float)g_sampleRate / 1000000000.0;
	float tRange = SAMPLES * tIncrement;
	float tStart = 0.0;
	static float tLastStart = 0.0;
	static uint16_t bufferOut[NUM_CHANNELS * SAMPLES];
	bool triggered = false;
	//1. seek time where the trigger is
	if (g_triggerForce) {
		tStart = (float)rand();
		triggered = true;
	} else if ((g_triggerReady) || (g_triggerMode == 1)) {
		float t = tRange / 2.0;
		float tEnd = t + 0.01; //only go through a little more than 10ms, as we simulate 100Hz signals which will repeat after this time
		uint16_t lastValue = 0; //undefined...
		if (g_triggerChannel < NUM_CHANNELS) {
			lastValue = SampleInput(g_selectedInput[g_triggerChannel], t);
		}
		while (t <= tEnd) {
			uint16_t value = 0;
			if (g_triggerChannel < NUM_CHANNELS) {
				value = SampleInput(g_selectedInput[g_triggerChannel], t);
			}
			if ((g_triggerType == 0) && (value <= g_triggerLevel)) {
				triggered = true;
				break;
			}
			if ((g_triggerType == 1) && (value >= g_triggerLevel)) {
				triggered = true;
				break;
			}
			if ((g_triggerType == 2) && (value <= g_triggerLevel) && (lastValue > g_triggerLevel)) {
				triggered = true;
				break;
			}
			if ((g_triggerType == 3) && (value >= g_triggerLevel) && (lastValue < g_triggerLevel)) {
				triggered = true;
				break;
			}
			t += tIncrement;
			lastValue = value;
		}
		if (triggered) {
			tStart = t - tRange / 2.0; //so the trigger is in the middle of the screen
		} else {
			tStart = tLastStart;
		}
	} else { //nothing to report...
		tStart = tLastStart;
	}
	tLastStart = tStart;
	//2. now generate the data based on the selected trigger
	float t = tStart;
	uint32_t elements = 0;
	if (g_activeInputs) {
		for (elements = 0; elements < (NUM_CHANNELS * SAMPLES); elements++) {
			uint8_t i = elements % g_activeInputs;
			uint16_t value = SampleInput(g_selectedInput[i], t);
			bufferOut[elements] = value;
			if ((i+1) == g_activeInputs) {
				t += tIncrement;
			}
		}
	}
	*elementsReported = elements;
	*pBufferOut = bufferOut;
	g_triggerReady = 0;
	g_triggerForce = 0;
	if (triggered) {
		return 1;
	}
	return 0;
}

float SampleVoltDigit(void) {
	return 3.0 / (ADC_MAX); //simulate 3V reference voltage
}

void SampleAdcPerformanceTest(void) {
}

