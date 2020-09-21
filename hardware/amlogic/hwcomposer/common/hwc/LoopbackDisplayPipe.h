/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef LOOPBACK_DISPLAY_PIPE_H
#define LOOPBACK_DISPLAY_PIPE_H
#include <HwcDisplayPipe.h>
#include <VdinPostProcessor.h>


class LoopbackDisplayPipe : public HwcDisplayPipe {
public:
    LoopbackDisplayPipe();
    ~LoopbackDisplayPipe();

    int32_t init(std::map<uint32_t, std::shared_ptr<HwcDisplay>> & hwcDisps);
    int32_t getPipeCfg(uint32_t hwcdisp, PipeCfg & cfg);
    int32_t handleRequest(uint32_t flags);

protected:
    int32_t getPostProcessor(
        hwc_post_processor_t type, std::shared_ptr<HwcPostProcessor> & processor);

protected:
    bool mPostProcessor;
    std::shared_ptr<VdinPostProcessor> mVdinPostProcessor;
};

#endif

