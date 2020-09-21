/*
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef DEBUG_HELPER_H
#define DEBUG_HELPER_H

#include <BasicTypes.h>

class DebugHelper : public Singleton<DebugHelper> {
public:
    DebugHelper();
    ~DebugHelper();

    void dump(String8 & dumpstr);

    void resolveCmd();

    /*dump internal info in hal dumpsys.*/
    inline bool dumpDetailInfo() {return mDumpDetail;}

    /*log informations.*/
    inline bool logCompositionDetail() {return mLogCompositionDetail;}
    inline bool monitorDeviceComposition() {return mMonitorDeviceComposition;}
    inline uint32_t deviceCompositionThreshold() {return mDeviceCompositionThreshold;}
    inline bool logFps() {return mLogFps;}

    /*check if UI/osd hwcomposer disabled.*/
    bool disableUiHwc() {return mDisableUiHwc;}

    /*for fence debug*/
    inline bool discardInFence() {return mDiscardInFence;}
    inline bool discardOutFence() {return mDiscardOutFence;}

    /*for layer compostion debug*/
    inline void getHideLayers(std::vector<int> & layers) {layers = mHideLayers;}
    inline bool debugHideLayers() {return mDebugHideLayer;}

    inline void getHidePlanes(std::vector<int> & planes) {planes = mHidePlanes;}
    inline bool debugHidePlanes() {return mDebugHidePlane;}

    /*save layer's raw data by layerid.*/
    inline void getSavedLayers(std::vector<int> & layers) {layers = mSaveLayers;}

    /*remove debug layer*/
    void removeDebugLayer(int id);

protected:
    bool isEnabled();
    void clearOnePassCmd();
    void clearPersistCmd();

    void addHideLayer(int id);
    void removeHideLayer(int id);

    void addHidePlane(int id);
    void removeHidePlane(int id);

protected:
    bool mEnabled;
    bool mDumpUsage;
    bool mDisableUiHwc;
    bool mDumpDetail;

    bool mLogCompositionDetail;
    bool mLogFps;

    bool mMonitorDeviceComposition;
    uint32_t mDeviceCompositionThreshold;

    std::vector<int> mSaveLayers;
    std::vector<int> mHideLayers;
    std::vector<int> mHidePlanes;

    bool mDebugHideLayer;
    bool mDebugHidePlane;

    /*handle osd in/out fence in hwc.*/
    bool mDiscardInFence;
    bool mDiscardOutFence;
};
#endif
