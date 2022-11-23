/*
(c) 2022 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause
*/

#include <stdint.h>
#include <ctype.h>
#include <stdlib.h>
#include <alloca.h>

#include "imageDrawer.h"

#include "utility.h"

void ImgScale2Byte(uint16_t rangeOut, uint16_t * data, uint16_t dataPoints,
              uint16_t rangeMin, uint16_t rangeMax, bool flip, uint16_t * rangeMinOut, uint16_t * rangeMaxOut) {
	if (rangeMin >= rangeMax) {
		rangeMin = 0xFFFF;
		rangeMax = 0;
		for (uint16_t i = 0; i < dataPoints; i++) {
			rangeMin = MIN(rangeMin, data[i]);
			rangeMax = MAX(rangeMax, data[i]);
		}
	} else { //do chropping
		for (uint16_t i = 0; i < dataPoints; i++) {
			data[i] = MAX(rangeMin, (MIN(rangeMax, data[i])));
		}
	}
	float scaler;
	float offset;
	if (rangeMax > rangeMin) {
		scaler = (float)rangeOut / ((float)(rangeMax - rangeMin));
		offset = 0.5; //proper rounding
	} else {
		scaler = 0.0; //does not matter, as it is multiplied by zero
		offset = rangeOut / 2; //makes a line in the middle
	}
	//let's scale
	uint16_t outMin = 0xFFFF;
	uint16_t outMax = 0;
	for (uint16_t i = 0; i < dataPoints; i++) {
		uint16_t out = ((float)(data[i] - rangeMin) * scaler + offset);
		if (flip) {
			out = rangeOut - out;
		}
		data[i] = out;
		outMin = MIN(outMin, out);
		outMax = MAX(outMax, out);
	}
	if (rangeMinOut) {
		*rangeMinOut = outMin;
	}
	if (rangeMaxOut) {
		*rangeMaxOut = outMax;
	}
}

void ImgInterpolateSingleLine(uint16_t start, uint16_t end, uint16_t * out, uint16_t dataPointsOut) {
	float delta  = (float)end - (float)start;
	if (dataPointsOut >= 2) {
		float scaler = delta / (dataPointsOut - 1);
		for (uint16_t i = 0; i < dataPointsOut; i++) {
			out[i] = (float)start + scaler * (float)i + 0.5f;
		}
	} else if (dataPointsOut == 1) {
		out[0] = start + delta * 0.5f + 0.5f; //use the average of the two dots
	}
}

void ImgInterpolateLines(const uint16_t * dataIn, uint16_t dataPointsIn,
              uint16_t * dataOut, uint16_t dataPointsOut) {
	if (dataPointsIn >= 2) {
		uint16_t linesOut = dataPointsIn - 1;
		//the length is rounded up, so if mapping is not exact, overlapping occurs
		uint16_t len = (dataPointsOut + (linesOut - 1)) / linesOut;
		float inc = (float)dataPointsOut / (float)linesOut;
		for (uint16_t i = 0; i < linesOut; i++) {
			uint16_t index = (float)i * inc;
			uint16_t limit = dataPointsOut - index;
			len = MIN(len, limit); //no buffer overflow please
			ImgInterpolateSingleLine(dataIn[i], dataIn[i + 1], &dataOut[index], len);
		}
	} else {
		uint16_t value = 0;
		if (dataPointsIn) {
			value = dataIn[0];
		}
		for (uint16_t i = 0; i < dataPointsOut; i++) {
			dataOut[i] = value;
		}
	}
}
