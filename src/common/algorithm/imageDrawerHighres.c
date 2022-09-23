/*
(c) 2022 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause

*/

#include "imageDrawer.h"

#include "imageDrawerHighres.h"

#define ImgOrderPixelsGeneric ImgOrderPixelsHighres
#define ImgCompressed1ByteGeneric ImgCompressed1ByteHighres
#define ImgInterpolateToPixelsGeneric ImgInterpolateToPixelsHighres
#define ImgCreateLineGfx1BitGeneric ImgCreateLineGfx1BitHighres
#define ImgCreateLinesGfxGeneric ImgCreateLinesGfxHighres

typedef Img1BytePixelHighres_t Img1BytePixelGeneric_t;

typedef uint16_t ImgResGeneric_t;

typedef uint32_t ImgPixelsMax_t;

#define IMGTYPESDEFINED

#include "imageDrawerGeneric.c"
