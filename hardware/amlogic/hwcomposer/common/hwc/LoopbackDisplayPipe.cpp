/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include "LoopbackDisplayPipe.h"
#include <HwcConfig.h>
#include <HwDisplayManager.h>
#include <MesonLog.h>

LoopbackDisplayPipe::LoopbackDisplayPipe()
    : HwcDisplayPipe() {
    if (HwcConfig::alwaysVdinLoopback())
        mPostProcessor = true;
    else
        mPostProcessor = false;
}

LoopbackDisplayPipe::~LoopbackDisplayPipe() {
    mVdinPostProcessor.reset();
}

int32_t LoopbackDisplayPipe::init(
    std::map<uint32_t, std::shared_ptr<HwcDisplay>> & hwcDisps) {
    HwcDisplayPipe::init(hwcDisps);


    std::shared_ptr<PipeStat> stat = mPipeStats.find(0)->second;

    std::map<uint32_t, drm_mode_info_t> viu2modes;
    stat->modeConnector->getModes(viu2modes);
    stat->modeCrtc->setMode(viu2modes[0]);
    MESON_LOGI("initDisplays viu2: set mode (%s)",viu2modes[0].name);

    if (mPostProcessor) {
        /*set viu1 to dummyplane */
        std::map<uint32_t, drm_mode_info_t> viu1modes;
        stat->hwcConnector->getModes(viu1modes);
        MESON_ASSERT(viu1modes.size() > 0, "no modes got.");
        MESON_LOGI("initDisplays viu1: get modes %s",viu1modes[0].name);
        stat->hwcCrtc->setMode(viu1modes[0]);
    }

    stat->modeMgr->update();

    if (mPostProcessor && stat->hwcPostProcessor)
        stat->hwcPostProcessor->start();

    return 0;
}

int32_t LoopbackDisplayPipe::getPipeCfg(uint32_t hwcid, PipeCfg & cfg) {
    MESON_ASSERT(hwcid == 0, "Only one display for this policy.");
    drm_connector_type_t  connector = getConnetorCfg(hwcid);
    MESON_ASSERT(connector == DRM_MODE_CONNECTOR_PANEL, "unsupported connector config");

    if (mPostProcessor) {
        cfg.hwcCrtcId = CRTC_VOUT1;
        cfg.hwcConnectorType = DRM_MODE_CONNECTOR_DUMMY;
        cfg.modeCrtcId = CRTC_VOUT2;
        cfg.modeConnectorType = connector;
        cfg.hwcPostprocessorType = VDIN_POST_PROCESSOR;
    } else {
        cfg.modeCrtcId = cfg.hwcCrtcId = CRTC_VOUT1;
        cfg.modeConnectorType = cfg.hwcConnectorType = connector;
        cfg.hwcPostprocessorType = INVALID_POST_PROCESSOR;
    }

    return 0;
}

int32_t LoopbackDisplayPipe::getPostProcessor(
    hwc_post_processor_t type,
    std::shared_ptr<HwcPostProcessor> & processor) {
    MESON_ASSERT(type == VDIN_POST_PROCESSOR,
        "only support VDIN_POST_PROCESSOR.");

    if (!mVdinPostProcessor) {
        mVdinPostProcessor = std::make_shared<VdinPostProcessor>();
        /*use the primary fb size for viu2.*/
        uint32_t w, h;
        HwcConfig::getFramebufferSize(0, w, h);

        std::shared_ptr<HwDisplayCrtc> crtc;
        std::vector<std::shared_ptr<HwDisplayPlane>> planes;
        getCrtc(CRTC_VOUT2, crtc);
        getPlanes(CRTC_VOUT2, planes);
        mVdinPostProcessor->setVout(crtc, planes, w, h);
    }

    processor = std::dynamic_pointer_cast<HwcPostProcessor>(mVdinPostProcessor);
    return 0;
}

int32_t LoopbackDisplayPipe::handleRequest(uint32_t flags) {
    MESON_LOGD("LoopbackDisplayPipe::handleRequest %x", flags);
    std::lock_guard<std::mutex> lock(mMutex);

    std::shared_ptr<PipeStat> stat = mPipeStats.find(0)->second;
    if ((flags & rPostProcessorStart) || (flags & rPostProcessorStop)) {
        bool bEnable = flags & rPostProcessorStart ? true : false;
        MESON_LOGV("Postprocessor enable event (%d)", bEnable);
        if (mPostProcessor != bEnable) {
            mPostProcessor = bEnable;
            if (!bEnable) {
                stat->hwcPostProcessor->stop();
                stat->hwcDisplay->setPostProcessor(NULL);
            }

            /*reset vout displaymode, for we need do pipeline switch*/
            stat->hwcCrtc->unbind();
            stat->modeCrtc->unbind();
            /*update display pipe.*/
            updatePipe(stat);

            if (bEnable) {
                /*set viu2 to plane*/
                std::map<uint32_t, drm_mode_info_t> viu2modes;
                stat->modeConnector->getModes(viu2modes);
                stat->modeCrtc->setMode(viu2modes[0]);
                MESON_LOGV("initDisplays viu2: get mode (%s)",viu2modes[0].name);

                /*set viu1 to dummyplane */
                std::map<uint32_t, drm_mode_info_t> viu1modes;
                stat->hwcConnector->getModes(viu1modes);
                MESON_ASSERT(viu1modes.size() > 0, "no modes got.");
                MESON_LOGV("initDisplays viu1: get modes %s",viu1modes[0].name);
                stat->hwcCrtc->setMode(viu1modes[0]);

                stat->modeMgr->update();
            } else {
                std::map<uint32_t, drm_mode_info_t> viu1modes;
                stat->modeConnector->getModes(viu1modes);
                stat->modeCrtc->setMode(viu1modes[0]);
                MESON_LOGV("initDisplays viu1: get mode (%s)",viu1modes[0].name);
            }

            if (bEnable)
                stat->hwcPostProcessor->start();
        }
    }

    if (mPostProcessor) {
        if ((flags & rKeystoneEnable) || (flags & rKeystoneDisable)) {
            bool bSetKeystone = flags & rKeystoneEnable ? true : false;
            MESON_LOGV("Keystone enable event (%d)", bSetKeystone);
            std::shared_ptr<PipeStat> stat = mPipeStats.find(0)->second;
            VdinPostProcessor * vdinProcessor =
                (VdinPostProcessor *)stat->hwcPostProcessor.get();
            MESON_ASSERT(vdinProcessor != NULL, "vdinProcessor should not NULL.");

            static std::shared_ptr<FbProcessor> keystoneprocessor = NULL;
            std::shared_ptr<FbProcessor> fbprocessor = NULL;
            if (bSetKeystone) {
                if (keystoneprocessor == NULL)
                    createFbProcessor(FB_KEYSTONE_PROCESSOR, keystoneprocessor);
                fbprocessor = keystoneprocessor;
            }
            vdinProcessor->setFbProcessor(fbprocessor);
        }
    }

    return 0;
}


