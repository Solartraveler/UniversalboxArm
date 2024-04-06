#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "adcPlatform.h"

/*Set div2 if the ADC is not clocked by a div1 from the base clock.
  Resulting divider: (div2 + 1)* 2 ^ prescaler.
*/
void AdcInit(bool div2, uint8_t prescaler);

/*Set the adc inputs. ADC 0 is the internal bandgap reference.
  Up to 16 channels may be set.
*/
void AdcInputsSet(const uint8_t * pAdcChannels, uint8_t numChannels);

/*Sets the sample time for all the channels. Number of ADC cycles:
  0: 2.5
  1: 6.5
  2: 12.5
  3: 24.5
  4: 47.5
  5: 92.5
  6: 247.5
  7: 640.5
*/
void AdcSampleTimeSet(uint8_t adcCycles);

//pOutput must be an array of numChannels (AdcInputsSet()) elements
void AdcStartTransfer(uint16_t * pOutput);

//Returns true if the ADC is currently active
bool AdcIsBusy(void);

/*Returns true if the last ADC value written into the memory. This might
  be a little bit later than the AdcIsBusy switches from true to false.
*/
bool AdcIsDone(void);

/*Measures the reference voltage. AdcInit needs to be called before.
  This function should only be called if no conversion is ongoing or could
  start while the function is called.
  returns [V]
*/
float AdcAvrefGet(void);
