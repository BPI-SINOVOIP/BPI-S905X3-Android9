/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#include "CopyProcessor.h"
#include <MesonLog.h>

CopyProcessor::CopyProcessor() {
}

CopyProcessor::~CopyProcessor() {
}

int32_t CopyProcessor::setup() {
    return 0;
}

int32_t CopyProcessor::process(
    std::shared_ptr<DrmFramebuffer> & inputfb,
    std::shared_ptr<DrmFramebuffer> & outfb) {

    void * inmem = NULL, * outmem = NULL;
    int infmt = am_gralloc_get_format (inputfb->mBufferHandle);
    int outfmt = am_gralloc_get_format (outfb->mBufferHandle);
    int w = am_gralloc_get_width(inputfb->mBufferHandle);
    int h = am_gralloc_get_height(inputfb->mBufferHandle);
    int instride = am_gralloc_get_stride_in_pixel(inputfb->mBufferHandle);
    int outstride = am_gralloc_get_stride_in_pixel(outfb->mBufferHandle);

    //MESON_LOGD("CopyProcessor %dx%d(%d,%d), fmt %d, %d",
    //    w, h, instride, outstride, infmt, outfmt);

    if (inputfb->lock(&inmem) == 0 && outfb->lock(&outmem) == 0) {
        char * src =  (char *)inmem;
        char * dst =  (char *)outmem;

        if (infmt == HAL_PIXEL_FORMAT_RGB_888 &&
            outfmt == HAL_PIXEL_FORMAT_RGB_888) {
            for (int ir = 0; ir < h; ir++) {
                memcpy(dst, src, w * 3);
                src += instride * 3;
                dst += outstride * 3;
            }
        }

        inputfb->unlock();
        outfb->unlock();
    }

    return 0;
}

int32_t CopyProcessor::teardown() {
    return 0;
}

