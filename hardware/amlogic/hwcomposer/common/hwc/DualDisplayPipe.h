/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef DUAL_DISPLAY_PIPE_H
#define DUAL_DISPLAY_PIPE_H
#include <HwcDisplayPipe.h>

class DualDisplayPipe : public HwcDisplayPipe {
public:
    DualDisplayPipe();
    ~DualDisplayPipe();

    bool mHdmi_connected;
    drm_connector_type_t mPrimaryConnectorType;
    drm_connector_type_t mExtendConnectorType;
    int32_t init(std::map<uint32_t, std::shared_ptr<HwcDisplay>> & hwcDisps);
    int32_t getPipeCfg(uint32_t hwcid, PipeCfg & cfg);
    void handleEvent(drm_display_event event, int val);
};

#endif

