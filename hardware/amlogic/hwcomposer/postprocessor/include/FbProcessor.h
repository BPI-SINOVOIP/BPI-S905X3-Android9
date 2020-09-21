/*
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef FB_PROCESSOR_H
#define FB_PROCESSOR_H

#include <BasicTypes.h>
#include <DrmFramebuffer.h>

typedef enum {
    FB_DUMMY_PROCESSOR = 0,
    FB_COPY_PROCESSOR,
    FB_KEYSTONE_PROCESSOR,
} meson_fb_processor_t;

class FbProcessor {
public:
    virtual ~FbProcessor() {}

    virtual int32_t setup() = 0;
    /*block operation, no fence.*/
    virtual int32_t process(
        std::shared_ptr<DrmFramebuffer> & inputfb,
        std::shared_ptr<DrmFramebuffer> & outfb) = 0;
    virtual int32_t teardown() = 0;
};

int32_t createFbProcessor(meson_fb_processor_t type, std::shared_ptr<FbProcessor> & processor);

#endif
