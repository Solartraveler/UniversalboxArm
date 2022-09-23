/*
(c) 2022 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause

*/

#include "imageDrawer.h"

#include "imageDrawerLowres.h"

#define ImgOrderPixelsGeneric ImgOrderPixelsLowres
#define ImgCompressed1ByteGeneric ImgCompressed1ByteLowres
#define ImgInterpolateToPixelsGeneric ImgInterpolateToPixelsLowres
#define ImgCreateLineGfx1BitGeneric ImgCreateLineGfx1BitLowres
#define ImgCreateLinesGfxGeneric ImgCreateLinesGfxLowres

typedef Img1BytePixelLowres_t Img1BytePixelGeneric_t;

typedef uint8_t ImgResGeneric_t;

typedef uint16_t ImgPixelsMax_t;

#define IMGTYPESDEFINED

#include "imageDrawerGeneric.c"
