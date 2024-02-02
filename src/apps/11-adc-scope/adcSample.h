#pragma once

#include <stdbool.h>
#include <stdint.h>

#define ADC_MAX ((1<<12) - 1)

void SampleInit(void);

/*Time interval at which the three channels are sampled. Returns true if
  the requested interval is supported.
*/
bool SampleRateSet(uint32_t samplesEveryNs);

/*level: The ADC level for the trigger
  mode: 0 = level low, 1 = level high, 2 = falling edge, 3 = rising edge
  channel is 0 based. 0 = adc1, 1 = adc2, 2 = adc3
*/
void SampleTriggerSet(uint16_t level, uint8_t mode, uint8_t channel);

//Returns true if setting is supported
bool SampleInputsSet(uint8_t * adcChannels, uint8_t numChannels);

//0 = one shot, 1 = automatic
void SampleModeSet(uint8_t mode);

//start sampling with the next trigger (if not set to automatic anyway)
void SampleStart(void);

//start sampling now, regardless if the trigger condition is met
void SampleTriggerForce(void);

/*type = 0, last buffered, 1 = current incomplete one.
  The pBufferOut format is channel1 - channel 2 - channel 3 - channel1 etc.
  pBufferOut points to a static buffer, acquired since the last trigger catch.
  The trigger position is always in the middle of the data returned.
  returns: 0: last data as already requested, 1: data have changed
*/
uint8_t SampleGet(uint8_t type, const uint16_t ** pBufferOut, uint32_t * elementsReported);

//returns [V/digit] Depends on the bit resolution and ADC reference voltage.
float SampleVoltDigit(void);

/*Checks the performance of the ADC. After this call, all settings
  done before must be re-applied
*/
void SampleAdcPerformanceTest(void);