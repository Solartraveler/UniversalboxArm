/* Writes a screenshot to the filesystem in the compressed tga format
(c) 2023 by Malte Marwedel

SPDX-License-Identifier: BSD-3-Clause

*/

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "screenshot.h"

#include "imageTgaWrite.h"
#include "filesystem.h"
#include "framebuffer.h"
#include "ff.h"


bool Screenshot(void) {
	printf("Saving screnshot...\r\n");
	bool success = true;
	fileBuffer_t fb;
	char filename[64];
	FB_SCREENPOS_TYPE pixelX, pixelY;
	menu_screen_size_get(&pixelX, &pixelY);
	//allocate output buffer and open file
	f_mkdir("/screenshots");
	uint32_t unusedId = FilesystemGetUnusedFilename("/screenshots", "shot");
	snprintf(filename, sizeof(filename), "/screenshots/shot%04u.tga", (unsigned int)unusedId);
	if ((unusedId == 0) || (!FilesystemBufferwriterStart(&fb, filename))) {
		printf("Error, could not write file %s\r\n", filename);
		return false;
	}
	//write header
	uint32_t colors = 1 << (FB_RED_IN_BITS + FB_GREEN_IN_BITS + FB_BLUE_IN_BITS);
	success &= ImgTgaStart(pixelX, pixelY, colors, 24, true, &FilesystemBufferwriterAppend, &fb);
	success &= ImgTgaColormap24(FB_RED_IN_BITS, FB_GREEN_IN_BITS, FB_BLUE_IN_BITS, &FilesystemBufferwriterAppend, &fb);
	//write image data
	for (uint32_t y = 0; y < pixelY; y++) {
		uint8_t data[pixelX];
		for (uint32_t x = 0; x < pixelX; x++) {
			data[x] = menu_screen_get(x, y);
		}
		success &= ImgTgaAppendCompress1Byte(data, pixelX, &FilesystemBufferwriterAppend, &fb);
		//success &= ImgTgaAppendDirect(data, pixelX, &FilesystemBufferwriterAppend, &fb);
	}
	//close file
	success &= FilesystemBufferwriterClose(&fb);
	printf("Saved %s with %s\r\n", filename, success ? "success" : "failure");
	return success;
}

bool ScreenshotPalette24(uint8_t scale, const uint32_t * colormap, const uint8_t colors) {
	printf("Saving screnshot...\r\n");
	bool success = true;
	fileBuffer_t fb;
	char filename[64];
	FB_SCREENPOS_TYPE pixelX, pixelY;
	menu_screen_size_get(&pixelX, &pixelY);
	//allocate output buffer and open file
	f_mkdir("/screenshots");
	uint32_t unusedId = FilesystemGetUnusedFilename("/screenshots", "shot");
	snprintf(filename, sizeof(filename), "/screenshots/shot%04u.tga", (unsigned int)unusedId);
	if ((unusedId == 0) || (!FilesystemBufferwriterStart(&fb, filename))) {
		printf("Error, could not write file %s\r\n", filename);
		return false;
	}
	//write header
	success &= ImgTgaStart(pixelX * scale, pixelY * scale, colors, 24, true, &FilesystemBufferwriterAppend, &fb);
	success &= ImgTgaAppend32To24(colormap, colors, &FilesystemBufferwriterAppend, &fb);
	//write image data
	for (uint32_t y = 0; y < pixelY; y++) {
		uint8_t data[pixelX * scale];
		for (uint32_t x = 0; x < pixelX; x++) {
			data[x * scale] = menu_screen_get(x, y);
			for (uint32_t s = 1; s < scale; s++) {
				data[x * scale + s] = data[x * scale];
			}
		}
		for (uint32_t s = 0; s < scale; s++) {
			success &= ImgTgaAppendCompress1Byte(data, pixelX * scale, &FilesystemBufferwriterAppend, &fb);
		}
		//success &= ImgTgaAppendDirect(data, pixelX, &FilesystemBufferwriterAppend, &fb);
	}
	//close file
	success &= FilesystemBufferwriterClose(&fb);
	printf("Saved %s with %s\r\n", filename, success ? "success" : "failure");
	return success;
}
