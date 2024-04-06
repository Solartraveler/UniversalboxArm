#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "adcSample.h"

#include "boxlib/adcDma.h"
#include "boxlib/leds.h"
#include "boxlib/timer16Bit.h"
#include "main.h"
#include "utility.h"

#define SAMPLES 320

#define CHANNELS 3

typedef struct {
	uint8_t activeChannels;
	bool forceStart;
	bool triggerStart;
	bool samplingDone;

	uint16_t sampleActive[SAMPLES * CHANNELS];
	bool sampleActiveHasTriggered;
	uint16_t sampleActiveTriggerIdx;
	uint16_t sampleActiveWriteIdx;
	uint16_t sampleActiveLeft; //counts down from the trigger condition
	uint16_t sampleActiveNoTrigger; //count down until start of trigger condition (minimum amount of samples before the trigger)

	uint16_t sampleShadow[SAMPLES * CHANNELS];
	uint16_t sampleShadowLeft;

	uint8_t inputs[CHANNELS];

	uint16_t triggerLevel;
	uint8_t triggerType; //0 = low level, 1 = high level, 2 = falling edge, 3 = rising edge
	uint8_t triggerMode; //0 = continuous, 1 = single shot
	uint8_t triggerChannel; //0...(activeChannels-1)

	float avcc; //typically 3.3V

	uint16_t timeExceed; //debug value to detect bad ISR setupt
	uint32_t error; //debug value
} adcState_t;

adcState_t g_adcState;

void SampleStopAdc(void) {
	TIM2->CR1 &= ~TIM_CR1_CEN;
	while (AdcIsBusy());
}

void SampleStartAdc(void) {
	TIM2->CNT = 0;
	TIM2->CR1 |= TIM_CR1_CEN;
}

static void SampleInputsRestore(void) {
	if (g_adcState.activeChannels) {
		AdcInputsSet(g_adcState.inputs, g_adcState.activeChannels);
	}
}

void SampleAvccCalib(void) {
	//printf("Start avc calib\r\n");
	g_adcState.avcc = AdcAvrefGet();
	//printf("Avcc: %umV\r\n", (unsigned int)(g_adcState.avcc * 1000.0f));
	SampleInputsRestore();
}

void SampleInit(void) {

	HAL_NVIC_DisableIRQ(TIM2_IRQn);
	AdcInit(false, 0);
	SampleAvccCalib();
	__HAL_RCC_TIM2_CLK_ENABLE();
	TIM2->CR1 = 0; //all stopped
	TIM2->CR2 = 0;
	TIM2->CNT = 0;
	TIM2->SR = 0;
	TIM2->DIER = TIM_DIER_UIE;
	HAL_NVIC_SetPriority(TIM2_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(TIM2_IRQn);
}

#pragma GCC push_options
#pragma GCC optimize ("-O3")

//See performance results in comment at benchmark function
void TIM2_IRQHandler(void) {
	Led1Off();
	TIM2->SR = 0;
	NVIC_ClearPendingIRQ(TIM2_IRQn);
	uint32_t tick0 = TIM2->CNT;
	if (AdcIsBusy() == false) {
		if (g_adcState.samplingDone == false) {
			if (g_adcState.sampleActiveHasTriggered == false) {
				Led1Green();
				//check if trigger condition is met
				bool triggerCond = g_adcState.forceStart;
				if ((triggerCond == false) && (g_adcState.triggerStart) && (g_adcState.sampleActiveNoTrigger == 0)) {
					uint16_t lastData = g_adcState.sampleActive[g_adcState.sampleActiveWriteIdx * g_adcState.activeChannels + g_adcState.triggerChannel];
					if (g_adcState.triggerType == 0) { //level low
						if (lastData < g_adcState.triggerLevel) {
							triggerCond = true;
						}
					} else if (g_adcState.triggerType == 1) { //level high
						if (lastData > g_adcState.triggerLevel) {
							triggerCond = true;
						}
					} else {
						uint16_t sampleLastLastIdx = g_adcState.sampleActiveWriteIdx - 1;
						if (sampleLastLastIdx >= SAMPLES) { //handle underflow
							sampleLastLastIdx = SAMPLES - 1;
						}
						uint16_t lastLastData = g_adcState.sampleActive[sampleLastLastIdx * g_adcState.activeChannels + g_adcState.triggerChannel];
						if (g_adcState.triggerType == 2) { //falling edge
							if ((lastLastData >= g_adcState.triggerLevel) && (lastData < g_adcState.triggerLevel)) {
								triggerCond = true;
							}
						} else if (g_adcState.triggerType == 3) { //rising edge
							if ((lastLastData <= g_adcState.triggerLevel) && (lastData > g_adcState.triggerLevel)) {
								triggerCond = true;
							}
						}
					}
				} else if (g_adcState.sampleActiveNoTrigger) {
					g_adcState.sampleActiveNoTrigger--;
				}
				if (triggerCond) {
					Led1Red();
					g_adcState.sampleActiveHasTriggered = true;
					g_adcState.sampleActiveTriggerIdx = g_adcState.sampleActiveWriteIdx;
					g_adcState.forceStart = false;
					g_adcState.triggerStart = false;
					g_adcState.sampleActiveLeft = SAMPLES / 2 - 2;
				}
			} else {
				if (g_adcState.sampleActiveLeft == 0) {
					g_adcState.samplingDone = true;
				} else {
					Led1Red();
					g_adcState.sampleActiveLeft--;
				}
			}
		}
		if (g_adcState.samplingDone == false) {
			//start next sample
			g_adcState.sampleActiveWriteIdx++;
			if (g_adcState.sampleActiveWriteIdx == SAMPLES) {
				g_adcState.sampleActiveWriteIdx = 0;
			}
			AdcStartTransfer(&g_adcState.sampleActive[g_adcState.sampleActiveWriteIdx * g_adcState.activeChannels]);
		}
	} else {
		while(AdcIsBusy()) {
			if (TIM2->CNT < tick0) {
				g_adcState.error++;
				break; //endless loop detected!
			}
		}
		uint32_t tick1 = TIM2->CNT;
		uint32_t delta = tick1 - tick0;
		g_adcState.timeExceed = MAX(delta, g_adcState.timeExceed);
	}
	Led1Off();
}

#pragma GCC pop_options

 //worst case of ticks measured by the the benchmark below
#define ISRTIME_MAX 394

void SampleRecalcSetupTime() {
	SampleStopAdc();
	uint32_t arr = TIM2->ARR;
	//The timer2 runs with the same clock as the ADC
	int32_t sampleIdx;
	/*The fixed 12.5 ADC clocks per 12bit sample are already added:
	uint32_t convertTime[8] = {15, 19, 25, 37, 40, 105, 260, 653};
	but these values do not work!, Instead the benchmark indicates some slower timings need
	to be used:
	*/
	const uint32_t convertTime[8]       = {20, 20, 40, 40, 60, 120, 260, 660};
	const uint32_t convertTimeOffset[8] = {60, 60, 80, 80, 80, 80, 80, 60};
	for (sampleIdx = 7; sampleIdx >= 0; sampleIdx--) {
		float requiredTime = convertTimeOffset[sampleIdx] + convertTime[sampleIdx] * g_adcState.activeChannels + ISRTIME_MAX;
		if (requiredTime < arr) {
			break;
		}
	}
	sampleIdx = MAX(sampleIdx, 0);
	printf("Arr: %u, ADC time: %u\r\n", (unsigned int)arr, (unsigned int)(convertTime[sampleIdx]));
	AdcSampleTimeSet(sampleIdx);
	SampleStartAdc();
}

bool SampleRateSet(uint32_t samplesEveryNs) {
	uint32_t divider10 = 1000000000 / (SystemCoreClock / 10);
	uint32_t arr = (samplesEveryNs * 10) / divider10;
	if (arr > ISRTIME_MAX) { //worse case CPU cycles to process one timer ISR
		SampleStopAdc();
		if (TIM2->ARR != arr) {
			memset(g_adcState.sampleActive, 0, SAMPLES * CHANNELS * sizeof(uint16_t));
			memset(g_adcState.sampleShadow, 0, SAMPLES * CHANNELS * sizeof(uint16_t));
			TIM2->ARR = arr;
		}
		SampleRecalcSetupTime();
		return true;
	}
	return false;
}

void SampleTriggerSet(uint16_t level, uint8_t type, uint8_t channel) {
	g_adcState.triggerType = type;
	g_adcState.triggerLevel = level;
	g_adcState.triggerChannel = channel;
}

bool SampleInputsSet(uint8_t * adcChannels, uint8_t numChannels) {
	if (numChannels > CHANNELS) {
		return false;
	}
	for (uint32_t i = 0; i < numChannels; i++) {
		g_adcState.inputs[i] = adcChannels[i];
	}
	SampleStopAdc();
	g_adcState.activeChannels = numChannels;
	memset(g_adcState.sampleActive, 0, SAMPLES * CHANNELS * sizeof(uint16_t));
	SampleInputsRestore();
	if (numChannels) {
		SampleRecalcSetupTime();
	}
	memset(g_adcState.sampleShadow, 0, SAMPLES * CHANNELS * sizeof(uint16_t));
	return true;
}

void SampleModeSet(uint8_t mode) {
	g_adcState.triggerMode = mode;
	if (mode == 1) {
		SampleStart();
	}
}

void SampleStart(void) {
	g_adcState.sampleActiveHasTriggered = false;
	g_adcState.triggerStart = true;
	g_adcState.sampleActiveNoTrigger = SAMPLES / 2;
	__sync_synchronize();
	g_adcState.samplingDone = false;
	__sync_synchronize();
}

void SampleTriggerForce(void) {
	g_adcState.sampleActiveHasTriggered = false;
	g_adcState.forceStart = true;
	g_adcState.sampleActiveNoTrigger = SAMPLES / 2;
	__sync_synchronize();
	g_adcState.samplingDone = false;
	__sync_synchronize();
}

uint8_t SampleGet(uint8_t type, const uint16_t ** pBufferOut, uint32_t * elementsReported) {
	uint8_t ac = g_adcState.activeChannels;
	if (ac == 0) {
		*elementsReported = 0;
		return 0;
	}
	uint8_t dataNew = 0;
	uint16_t * sampleShadow = g_adcState.sampleShadow;
	if ((g_adcState.samplingDone) || ((type == 1) && (g_adcState.sampleActiveHasTriggered))) {
		//yes, when type == 1, we simply copy the data "in flight"
		__sync_synchronize();
		g_adcState.sampleShadowLeft = g_adcState.sampleActiveLeft;
		__sync_synchronize();
		/*The trigger index is somewhere in the data, so we have to move the data in the right position
		*/
		uint32_t triggerIndex = g_adcState.sampleActiveTriggerIdx;
		uint16_t indexStart = (triggerIndex + SAMPLES / 2) % SAMPLES;
		uint16_t sizeEnd = SAMPLES - indexStart;
		memcpy(sampleShadow, g_adcState.sampleActive + indexStart * ac, sizeof(uint16_t) * sizeEnd * ac);
		if (indexStart > 0) {
			memcpy(sampleShadow + sizeEnd * ac, g_adcState.sampleActive, sizeof(uint16_t) * indexStart * ac);
		}
		__sync_synchronize();
		if ((g_adcState.samplingDone) && (g_adcState.triggerMode == 1)) {
			//restart next data sampling
			SampleAvccCalib();
			SampleStart();
		}
		__sync_synchronize();
		dataNew = 1;
	}
	*pBufferOut = sampleShadow;
	*elementsReported = (SAMPLES  - g_adcState.sampleShadowLeft) * ac;

	//debug information
	if ((g_adcState.timeExceed) || (g_adcState.error)) {
		__disable_irq();
		uint32_t exceed = g_adcState.timeExceed;
		uint32_t error = g_adcState.error;
		g_adcState.timeExceed = 0;
		g_adcState.error = 0;
		__enable_irq();
		printf("Time exceed %u ticks, error %u\r\n", (unsigned int)exceed, (unsigned int)error);
	}
	return dataNew;
}

float SampleVoltDigit(void) {
	return g_adcState.avcc / (float)ADC_MAX;
}

#define TESTINPUTMAX 8

//needs setup and restore the g_adsState before and after the call
static uint32_t SampleAdcTriggerPerformanceTest(void) {
	uint16_t output[CHANNELS];
	AdcStartTransfer(output);
	while (AdcIsDone() == false); //otherwise the ISRs would just report an error
	Timer16BitStop();
	Timer16BitReset();
	__disable_irq();
	Timer16BitStart();
	TIM2_IRQHandler();
	uint32_t ticks = Timer16BitGet();
	__enable_irq();
	return ticks;
}

/* When executing from RAM with 80MHz, the results are
For optimization -Os:
Ticks for low level trigger no cond: 302
Ticks for low level trigger: 372
Ticks for high level trigger: 380
Ticks for falling edge trigger: 392
Ticks for rising edge trigger: 396
Ticks for continue sampling: 286

For optimization -O3:
Ticks for low level trigger no cond: 305
Ticks for low level trigger: 372
Ticks for high level trigger: 380
Ticks for falling edge trigger: 392
Ticks for rising edge trigger: 394
Ticks for continue sampling: 279

Running from flash is expected to be ~20% faster.
*/
void SampleAdcPerformanceTest(void) {
	SampleStopAdc();
	Timer16BitInit(0);
	const uint8_t inputs[TESTINPUTMAX] = {1, 1, 1, 1, 1, 1, 1, 1};
	uint16_t output[TESTINPUTMAX];
	//1. testing sample time
	for (uint32_t j = 0; j <= 7; j++) {
		printf("Testing sample time %u\r\n", (unsigned int)j);
		AdcSampleTimeSet(j);
		uint32_t ticksLast = 0;
		for (uint32_t i = 1; i <= TESTINPUTMAX; i++) {
			AdcInputsSet(inputs, i);
			Timer16BitStop();
			Timer16BitReset();
			__disable_irq();
			Timer16BitStart();
			AdcStartTransfer(output);
			while (AdcIsDone() == false);
			uint32_t ticks = Timer16BitGet();
			__enable_irq();
			printf("Sampling %u channels took %u ticks, (+%u)\r\n", (unsigned int)i, (unsigned int)ticks, (unsigned int)(ticks - ticksLast));
			ticksLast = ticks;
		}
	}
	//2. testing trigger performance
	//2.1 save current settings
	uint16_t tLOld = g_adcState.triggerLevel;
	uint16_t tTOld = g_adcState.triggerType;
	uint8_t tCOld = g_adcState.triggerChannel;
	uint8_t aCOld = g_adcState.activeChannels;

	//2.2 setup common settings
	_Static_assert(TESTINPUTMAX >= CHANNELS, "fix the buffer overflow!");
	AdcInputsSet(inputs, CHANNELS);
	g_adcState.forceStart = false;
	g_adcState.triggerLevel = ADC_MAX / 2;
	g_adcState.activeChannels = CHANNELS;
	g_adcState.triggerChannel = 0;

	//3.1 measure ticks for low level trigger - condition not met
	g_adcState.samplingDone = false;
	g_adcState.triggerStart = true;
	g_adcState.sampleActiveHasTriggered = false;
	g_adcState.sampleActiveNoTrigger = 0;
	g_adcState.sampleActiveWriteIdx = 0;
	g_adcState.sampleActive[0] = ADC_MAX;
	g_adcState.triggerType = 0;
	uint32_t ticksLowlevelNo = SampleAdcTriggerPerformanceTest();
	printf("Ticks for low level trigger no cond: %u\r\n", (unsigned int)ticksLowlevelNo);
	printf("  Check: Trg: %s\r\n", g_adcState.sampleActiveHasTriggered ? "fail" : "ok");

	//3.1 measure ticks for low level trigger
	g_adcState.samplingDone = false;
	g_adcState.triggerStart = true;
	g_adcState.sampleActiveHasTriggered = false;
	g_adcState.sampleActiveNoTrigger = 0;
	g_adcState.sampleActiveWriteIdx = 0;
	g_adcState.sampleActive[0] = 0;
	g_adcState.triggerType = 0;
	uint32_t ticksLowlevel = SampleAdcTriggerPerformanceTest();
	printf("Ticks for low level trigger: %u\r\n", (unsigned int)ticksLowlevel);
	printf("  Check: Trg: %s\r\n", g_adcState.sampleActiveHasTriggered ? "ok" : "fail");

	//3.1 measure ticks for high level trigger
	g_adcState.samplingDone = false;
	g_adcState.triggerStart = true;
	g_adcState.sampleActiveHasTriggered = false;
	g_adcState.sampleActiveNoTrigger = 0;
	g_adcState.sampleActiveWriteIdx = 0;
	g_adcState.sampleActive[0] = ADC_MAX;
	g_adcState.triggerType = 1;
	uint32_t ticksHighlevel = SampleAdcTriggerPerformanceTest();
	printf("Ticks for high level trigger: %u\r\n", (unsigned int)ticksHighlevel);
	printf("  Check: Trg: %s\r\n", g_adcState.sampleActiveHasTriggered ? "ok" : "fail");

	//3.3 measure ticks for falling edge trigger
	g_adcState.samplingDone = false;
	g_adcState.triggerStart = true;
	g_adcState.sampleActiveHasTriggered = false;
	g_adcState.sampleActiveNoTrigger = 0;
	g_adcState.sampleActiveWriteIdx = 1;
	g_adcState.sampleActive[0] = ADC_MAX;
	g_adcState.sampleActive[0 + CHANNELS] = 0;
	g_adcState.triggerType = 2;
	uint32_t ticksFalling = SampleAdcTriggerPerformanceTest();
	printf("Ticks for falling edge trigger: %u\r\n", (unsigned int)ticksFalling);
	printf("  Check: Trg: %s\r\n", g_adcState.sampleActiveHasTriggered ? "ok" : "fail");

	//3.4 measure ticks for rising edge trigger
	g_adcState.samplingDone = false;
	g_adcState.triggerStart = true;
	g_adcState.sampleActiveHasTriggered = false;
	g_adcState.sampleActiveNoTrigger = 0;
	g_adcState.sampleActiveWriteIdx = 1;
	g_adcState.sampleActive[0] = 0;
	g_adcState.sampleActive[0 + CHANNELS] = ADC_MAX;
	g_adcState.triggerType = 3;
	uint32_t ticksRising = SampleAdcTriggerPerformanceTest();
	printf("Ticks for rising edge trigger: %u\r\n", (unsigned int)ticksRising);
	printf("  Check: Trg: %s\r\n", g_adcState.sampleActiveHasTriggered ? "ok" : "fail");

	//3.4 measure ticks for continue sampling
	uint32_t ticksSampling = SampleAdcTriggerPerformanceTest();
	printf("Ticks for continue sampling: %u\r\n", (unsigned int)ticksSampling);
	printf("  Check: Write: %s\r\n", g_adcState.sampleActiveWriteIdx == 3 ? "ok" : "fail");

	//4. restore settings
	Timer16BitDeinit();
	g_adcState.triggerLevel = tLOld;
	g_adcState.triggerType = tTOld;
	g_adcState.triggerChannel = tCOld;
	g_adcState.activeChannels = aCOld;

	SampleInputsRestore();
	SampleRecalcSetupTime();
}

