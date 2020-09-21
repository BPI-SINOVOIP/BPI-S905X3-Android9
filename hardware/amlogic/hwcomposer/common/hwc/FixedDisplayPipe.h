/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef FIXED_DISPLAY_PIPE_H
#define FIXED_DISPLAY_PIPE_H
#include <HwcDisplayPipe.h>

class FixedDisplayPipe : public HwcDisplayPipe {
public:
    FixedDisplayPipe();
    ~FixedDisplayPipe();

    int32_t init(std::map<uint32_t, std::shared_ptr<HwcDisplay>> & hwcDisps);
    void handleEvent(drm_display_event event, int val);

protected:
    int32_t getPipeCfg(uint32_t hwcid, PipeCfg & cfg);
    drm_connector_type_t getConnetorCfg(uint32_t hwcid);

};

#endif
