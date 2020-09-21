/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include <MesonLog.h>
#include "DummyProcessor.h"

DummyProcessor::DummyProcessor() {

}

DummyProcessor::~DummyProcessor() {
}

int32_t DummyProcessor::setup() {
    return 0;
}

int32_t DummyProcessor::process(
    std::shared_ptr<DrmFramebuffer> & inputfb,
    std::shared_ptr<DrmFramebuffer> & outfb) {
    UNUSED(inputfb);

    static int colorc = 0;

    void * fbmem = NULL;
    if (outfb->lock(&fbmem) == 0) {
        native_handle_t * handle = outfb->mBufferHandle;
        int w = am_gralloc_get_width(handle);
        int h = am_gralloc_get_height(handle);
        int format = am_gralloc_get_format(handle);

        char r = 0, g = 0, b = 0;

        int type = colorc % 300;
        if (type == 0) {
            MESON_LOGD("DummyProcessor::process color %d x %d, %d, %x, %p",
                w, h, format, am_gralloc_get_vpu_afbc_mask(handle), fbmem);
        }

        if (type <=  100) {
            r = 255;
        } else if (type <= 200) {
            g = 255;
        } else {
            b = 255;
        }

        char * colorbuf = (char *) fbmem;
        for (int ir = 0; ir < h; ir++) {
            for (int ic = 0; ic < w; ic++) {
                switch (format) {
                    case HAL_PIXEL_FORMAT_BGRA_8888:
                    case HAL_PIXEL_FORMAT_RGBA_8888:
                    case HAL_PIXEL_FORMAT_RGBX_8888:
                        colorbuf[0] = r;
                        colorbuf[1] = g;
                        colorbuf[2] = b;
                        colorbuf[3] = 0xff;
                        colorbuf += 4;
                        break;
                    case HAL_PIXEL_FORMAT_RGB_888:
                        colorbuf[0] = r;
                        colorbuf[1] = g;
                        colorbuf[2] = b;
                        colorbuf += 3;
                        break;
                    default:
                        MESON_LOGE("unsupport format, nothing todo.");
                        break;
                }
            }
        }

        outfb->unlock();
    }

    colorc ++;
    return 0;
}

int32_t DummyProcessor::teardown() {
    return 0;
}

