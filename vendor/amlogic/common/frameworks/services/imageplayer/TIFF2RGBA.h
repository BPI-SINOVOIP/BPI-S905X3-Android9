/** @file TIFF2RGBA.h
 *  @par Copyright:
 *  - Copyright 2011 Amlogic Inc as unpublished work
 *  All Rights Reserved
 *  - The information contained herein is the confidential property
 *  of Amlogic.  The use, copying, transfer or disclosure of such information
 *  is prohibited except by express written agreement with Amlogic Inc.
 *  @author   Tellen Yu
 *  @version  1.0
 *  @date     2015/06/08
 *  @par function description:
 *  - 1 save rgb data to picture
 *  @warning This class may explode in your face.
 *  @note If you inherit anything from this class, you're doomed.
 */

#ifndef TIFF_2_RGBA_H
#define TIFF_2_RGBA_H

#include <SkBitmap.h>

#include "tiffiop.h"
#include "tiffio.h"

#define MAX_PIC_SIZE                8000

namespace android {

    class TIFF2RGBA {
      public:
        TIFF2RGBA();
        ~TIFF2RGBA();

        int tiffDecodeBound(const char *filePath, int *width, int *height);
        int tiffDecoder(const char *filePath, SkBitmap *pBitmap);
        void close();
        TIFF* mTif;
        int* mRaster;
    };

}  // namespace android

#endif // TIFF_2_RGBA_H
