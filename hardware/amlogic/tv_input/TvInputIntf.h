/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *  @author   Tellen Yu
 *  @version  1.0
 *  @date     2018/1/12
 *  @par function description:
 *  - 1 tv_input hal to tvserver interface
 */

#ifndef _ANDROID_TV_INPUT_INTERFACE_H_
#define _ANDROID_TV_INPUT_INTERFACE_H_

#include "TvServerHidlClient.h"

using namespace android;

#define HDMI_MAX_SUPPORT_NUM 4

typedef enum tv_source_input_e {
    SOURCE_INVALID = -1,
    SOURCE_TV = 0,
    SOURCE_AV1,
    SOURCE_AV2,
    SOURCE_YPBPR1,
    SOURCE_YPBPR2,
    SOURCE_HDMI1,
    SOURCE_HDMI2,
    SOURCE_HDMI3,
    SOURCE_HDMI4,
    SOURCE_VGA,
    SOURCE_MPEG,
    SOURCE_DTV,
    SOURCE_SVIDEO,
    SOURCE_IPTV,
    SOURCE_DUMMY,
    SOURCE_SPDIF,
    SOURCE_ADTV,
    SOURCE_AUX,
    SOURCE_ARC,
    SOURCE_DTVKIT,
    SOURCE_MAX,
} tv_source_input_t;

typedef struct source_connect_s {
    int msgType;
    int source;
    int state;
} source_connect_t;

class TvPlayObserver {
public:
    TvPlayObserver() {};
    virtual ~TvPlayObserver() {};
    virtual void onTvEvent (const source_connect_t &scrConnect) = 0;
};

class TvInputIntf : public TvListener {
public:
    TvInputIntf();
    ~TvInputIntf();
    int startTv();
    int stopTv();
    int switchSourceInput(tv_source_input_t source_input);
    int getSourceConnectStatus(tv_source_input_t source_input);
    int getCurrentSourceInput();
    int getHdmiAvHotplugDetectOnoff();
    int setTvObserver (TvPlayObserver *ob);
    int getSupportInputDevices(int *devices, int *count);
    int getHdmiPort(tv_source_input_t source_input);
    virtual void notify(const tv_parcel_t &parcel);

private:
    sp<TvServerHidlClient> mTvSession;
    TvPlayObserver *mpObserver;
};

#endif/*_ANDROID_TV_INPUT_INTERFACE_H_*/
