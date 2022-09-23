#pragma once

#include <stdint.h>
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct {
	uint16_t x;
	uint16_t y;
	uint8_t color;
} Img1BytePixelHighres_t;

/* Creates a compressed image for a menu-interpreter GFX, if one of the three
custom colors with a fitting bit number and maximum 7 colorBits.
If colorBits is 8, an uncompressed GFX is created.

x, y: Size of the desired compressed graphic
colorBits: Bits used for color information, all other of the byte are used for compression
outBuffer: Buffer which must hold all the compressed data
outBufferLen: Length in bytes of outBuffer
backgroundColor: Default background color
drawPixels: Pixels to be included to the output graphic. This array gets written too.
            This is neccessary, to process them left to right and top to bottom.
            Pixels with the same coordinates are merged by doing a binary and operation to the colors.
            If this pointer is NULL, an valid empty (all bits set) output image is created.
numPixels: Number of pixels in drawPixels array. 0 creates an empty image.
outBufferUsed: Optional. If non NULL, the number of bytes used for the result are reported here.
returns:
    0: ok
    1: ok, but pixels chopped because buffer too small
    2: buffer too small for x * y, array may not be used
*/
uint8_t ImgCompressed1ByteHighres(uint16_t x, uint16_t y, uint8_t colorBits,
              uint8_t * outBuffer, size_t outBufferLen, uint8_t backgroundColor,
              Img1BytePixelHighres_t * drawPixels, uint16_t numPixel, size_t * outBufferUsed);

/* Converts dataIn to drawPixels, connecting between the Pixels with lines.
dataIn: Data to convert
dataPointsIn: Number of points in dataIn
drawPixels: Out, pixels converted
maxPixels: In, number of pixels in drawPixels
color: Color for the pixels
returns: Number of pixels used
*/
uint16_t ImgInterpolateToPixelsHighres(const uint16_t * dataIn, uint16_t dataPointsIn,
               Img1BytePixelHighres_t * drawPixels, uint16_t maxPixels, uint8_t color);

/* Generates a lineplot
data: Input, data to plot
dataPoints: Input, number of entries in data
x: Input, pixel size of ouput graphic dimension
y: Input, pixel size of output graphic dimension
flip: Input, if true, low values are at the bottom of the graphic (which has higher coordinates)
compressedGfxOut: Output, graphic
outGfxLen: Output, maximum length of compressedGfxOut
outGfxUsed: Output, if non NULL, used length of compressedGfxOut
tempBuffer: Optional processing buffer, for each pixel drawn, one Img1BytePixelHighres_t needs to be
            temporary stored. If NULL, y * 10 * sizeof(Img1BytePixelHighres_t) bytes are allocated on the stack.
tempBufferLen: Number of bytes buffer has. May be 0.
returns: 0: successful
         1: ok, but data are cut out due lack of memory
         2: compressedGfxOut too small, may not be used.
*/
uint8_t ImgCreateLineGfx1BitHighres(const uint16_t * data, uint16_t dataPoints, uint16_t x, uint16_t y, bool flip,
              uint8_t * compressedGfxOut, size_t outGfxLen, size_t * outGfxUsed, void * tempBuffer, size_t tempBufferLen);

/* Generates a multi lineplot
data: Input, data to plot
dataPoints: Input, number of entries for each line in data
colors: Input, color for each line in data
lines: Input, number of lines in data and colors
x: Input, pixel size of ouput graphic dimension
y: Input, pixel size of output graphic dimension
colorBits: Number of bits valid incolors
flip: Input, if true, low values are at the bottom of the graphic (which has higher coordinates)
compressedGfxOut: Output, graphic
outGfxLen: Output, maximum length of compressedGfxOut
outGfxUsed: Output, if non NULL, used length of compressedGfxOut
tempBuffer: Optional processing buffer, for each pixel drawn, one Img1BytePixelHighres_t needs to be
        temporary stored. If NULL, y * lines 10 * sizeof(Img1BytePixelHighres_t) bytes are allocated on the stack.
tempBufferLen: Number of bytes buffer has. May be 0.
returns: 0: successful
         1: ok, but data are cut out due lack of memory
         2: compressedGfxOut too small, may not be used.
*/
uint8_t ImgCreateLinesGfxHighres(const uint16_t ** data, uint16_t dataPoints, const uint8_t * colors, uint8_t lines,
              uint16_t x, uint16_t y, uint8_t colorBits, bool flip,
              uint8_t * compressedGfxOut, size_t outGfxLen, size_t * outGfxUsed, void * tempBuffer, size_t tempBufferLen);
