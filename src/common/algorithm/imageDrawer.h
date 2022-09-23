#pragma once

#include <stdint.h>
#include <ctype.h>
#include <stdbool.h>

/*
rangeOut: from 0 to rangeOut, the output will be scaled
data: in and output data
dataPoints: Number of data pointers in array
rangeMin: ignore data below this value. Points will get the value 0 the output.
rangeMax: ignore data above this value. Points will get the value range in the output.
          If >= rangeMin, both values will be ignored and determined from the data automatically
flip: If true, invert the scaled output. Useful if lower values should be higher coordinates.
rangeMinOut: minium value in the output
rangeMaxOut: maximum value in the output
*/
void ImgScale2Byte(uint16_t rangeOut, uint16_t * data, uint16_t dataPoints,
              uint16_t rangeMin, uint16_t rangeMax, bool flip, uint16_t * rangeMinOut, uint16_t * rangeMaxOut);


/*
start: start value of interpolation for the line
end: End value of interpolation for the line
dataOut: Linear interpolated line
dataPointsOut: Number of elements to interpolate in dataOut
*/
void ImgInterpolateSingleLine(uint16_t start, uint16_t end, uint16_t * out, uint16_t dataPointsOut);


/*
dataIn: Data to interpolate
dataPointsIn: Number of elements in dataIn, should be 2 for interpolation.
        If 1, the input data is copied to the output
        If 0, the output is set to all zero.
dataOut: Linear interpolated line
dataPointsOut: Number of elements to interpolate in dataOut, should be >= dataIn
*/
void ImgInterpolateLines(const uint16_t * dataIn, uint16_t dataPointsIn,
              uint16_t * dataOut, uint16_t dataPointsOut);

