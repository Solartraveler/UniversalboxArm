#pragma once

/* Stateless TGA writer

(c) 2022 by Malte Marwedel

SPDX-License-Identifier: BSD-3-Clause

Idea is to first call ImgTgaStart.
If colorMapEntries was not zero, then call ImgTgaColormap16 or ImgTgaColormap24
or append a custom color map by ImgTgaAppendDirect.
Then append pixel data by one of the three functions ImgTgaAppendDirect or
ImgTgaAppend32To24 or if compressed to be true, ImgTgaAppendCompress1Byte.

Note: Not all settings possible in the .tga format are actually be supported by
some common programs like GIMP or Irfan View.

It is highly reccommend to use some sort of buffer within outputWriteFunc_t
if the underlying operating system does not provide one.
*/

#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>



typedef bool (outputWriteFunc_t)(const void * buffer, size_t len, void * param);

/* Writes the 18 byte header of a .tga image
sizeX: Width of the image in pixels
sizeY: Height of the image in pixels
colorMapEntries: If not 0, an indexed image is created.
                 A value of 1 does make little sense - the image would just have
                 one color all over.
                 A value above 256 should work, and then requires 2 byte per
                 pixel data. But neither GIMP, nor Irfan View can open the
                 generated images.
colorBits: Number of color bits used for the colorMapEntries OR the
           pixel data itself.
           While the tga format would support anything between 1 and 32, most
           programs can not open these or display garbage, so only the values
           8 for monochrome images
           and 16 or 24 for color images are supported.
           For 16 bits, 1bit is reserved, resulting in 5bits per color.
compression: If true, the pixel data needs the compression format
pWriter:     Function to write everything to the output., void * param
param:       Is forwarded to the outputWriteFunc_t and can be used for state
             storage there.
Returns true if successful.
*/
bool ImgTgaStart(uint16_t sizeX, uint16_t sizeY, uint16_t colorMapEntries, uint8_t colorBits,
                 bool compression, outputWriteFunc_t * pWriter, void * param);

bool ImgTgaColormap16(uint8_t redBits, uint8_t greenBits, uint8_t blueBits,
                    outputWriteFunc_t * pWriter, void * param);

bool ImgTgaColormap24(uint8_t redBits, uint8_t greenBits, uint8_t blueBits,
                    outputWriteFunc_t * pWriter, void * param);


bool ImgTgaAppendDirect(const void * data, size_t bytes, outputWriteFunc_t * pWriter, void * param);

bool ImgTgaAppend32To24(const uint32_t * data, size_t elements, outputWriteFunc_t * pWriter, void * param);

bool ImgTgaAppendCompress1Byte(const uint8_t * data, size_t bytes, outputWriteFunc_t * pWriter, void * param);

bool ImgTgaAppendCompress2Byte(const uint16_t * data, size_t elements, outputWriteFunc_t * pWriter, void * param);

bool ImgTgaAppendCompress3Byte(const uint32_t * data, size_t elements, outputWriteFunc_t * pWriter, void * param);

