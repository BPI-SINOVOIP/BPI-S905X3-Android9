/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef HWC_DISPLAY_PIPE_H
#define HWC_DISPLAY_PIPE_H

#include <BasicTypes.h>
#include <HwDisplayConnector.h>
#include <HwDisplayCrtc.h>
#include <HwDisplayEventListener.h>
#include <HwcDisplay.h>
#include <HwcVsync.h>
#include <HwcPostProcessor.h>
#include <HwcModeMgr.h>

#include <MesonLog.h>

typedef enum {
    /*primary:
    viu1 + connector from config*/
    HWC_PIPE_DEFAULT = 0,

    /* two display:
    viu1 + connector from config
    extend:
    viu2 + connector from config*/
    HWC_PIPE_DUAL,

    /*primary:
        when postprocessor disable: viu1 -> conntector
        when postprocessor enable: viu1->vdin->viu2->conntector
    extend:
        NONE*/
    HWC_PIPE_LOOPBACK,

} hwc_pipe_policy_t;

/*requests*/
enum {
    rPostProcessorStart = 1 << 0,
    rPostProcessorStop = 1 << 1,
    rPostProcessorStartExt = 1 << 2,
    rPostProcessorStopExt  = 1 << 3,

    rDisplayModeSet = 1 << 4,
    rDisplayModeSetExt = 1 << 5,

    rCalibrationSet = 1 << 6,
    rCalibrationSetExt = 1 << 7,

    rKeystoneEnable = 1 << 8,
    rKeystoneDisable = 1 << 9,
};

/*events*/
enum {
    eHdmiPlugIn = 1 << 0,
    eHdmiPlugOut = 1<< 1,

    eDisplayModeChange = 1 << 2,
    eDisplayModeExtChange = 1 << 3,
};

class HwcDisplayPipe :  public HwDisplayEventHandler {
public:
    HwcDisplayPipe();
    virtual ~HwcDisplayPipe();

    virtual int32_t init(std::map<uint32_t, std::shared_ptr<HwcDisplay>> & hwcDisps);
    virtual int32_t handleRequest(uint32_t flags);
    virtual void handleEvent(drm_display_event event, int val);

protected:
    class PipeCfg {
    public:
        int32_t hwcCrtcId;
        drm_connector_type_t hwcConnectorType;
        hwc_post_processor_t hwcPostprocessorType;

        int32_t modeCrtcId;
        drm_connector_type_t modeConnectorType;
    };

    class PipeStat {
    public:
        PipeStat(uint32_t id);
        ~PipeStat();

        uint32_t hwcId;
        PipeCfg cfg;

        std::shared_ptr<HwcDisplay> hwcDisplay;
        std::shared_ptr<HwDisplayCrtc> hwcCrtc;
        std::vector<std::shared_ptr<HwDisplayPlane>> hwcPlanes;
        std::shared_ptr<HwDisplayConnector> hwcConnector;
        std::shared_ptr<HwcVsync> hwcVsync;
        std::shared_ptr<HwcPostProcessor> hwcPostProcessor;

        std::shared_ptr<HwcModeMgr> modeMgr;
        std::shared_ptr<HwDisplayCrtc> modeCrtc;
        std::shared_ptr<HwDisplayConnector> modeConnector;
    };

protected:
    /*load display pipe config*/
    virtual int32_t updatePipe(std::shared_ptr<PipeStat> & stat);
    virtual int32_t getPipeCfg(uint32_t hwcid, PipeCfg & cfg) = 0;
    virtual drm_connector_type_t getConnetorCfg(uint32_t hwcid);

    virtual int32_t initDisplayMode(std::shared_ptr<PipeStat> & stat);

    /*load display resource*/
    int32_t getCrtc(
        int32_t crtcid, std::shared_ptr<HwDisplayCrtc> & crtc);
    int32_t getPlanes(
        int32_t crtcid, std::vector<std::shared_ptr<HwDisplayPlane>> & planes);
    int32_t getConnector(
        drm_connector_type_t type, std::shared_ptr<HwDisplayConnector> & connector);
    virtual int32_t getPostProcessor(
        hwc_post_processor_t type, std::shared_ptr<HwcPostProcessor> & processor);

protected:
    std::vector<std::shared_ptr<HwDisplayPlane>> mPlanes;
    std::vector<std::shared_ptr<HwDisplayCrtc>> mCrtcs;
    std::map<drm_connector_type_t, std::shared_ptr<HwDisplayConnector>> mConnectors;
    std::map<uint32_t, std::shared_ptr<PipeStat>> mPipeStats;
    std::mutex mMutex;
};

std::shared_ptr<HwcDisplayPipe> createDisplayPipe(hwc_pipe_policy_t pipet);

#endif
