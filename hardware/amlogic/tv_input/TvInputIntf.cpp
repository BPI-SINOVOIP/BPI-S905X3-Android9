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

#define LOG_TAG "TvInputIntf"

#include <utils/Log.h>
#include <string.h>
#include "TvInputIntf.h"
#include "tvcmd.h"
#include <math.h>

using namespace android;

TvInputIntf::TvInputIntf() : mpObserver(nullptr) {
    mTvSession = TvServerHidlClient::connect(CONNECT_TYPE_HAL);
    mTvSession->setListener(this);
}

TvInputIntf::~TvInputIntf()
{
    mTvSession.clear();
}

int TvInputIntf::setTvObserver ( TvPlayObserver *ob )
{
    //ALOGI("setTvObserver:%p", ob);
    mpObserver = ob;
    return 0;
}

void TvInputIntf::notify(const tv_parcel_t &parcel)
{
    source_connect_t srcConnect;
    srcConnect.msgType = parcel.msgType;
    srcConnect.source = parcel.bodyInt[0];
    srcConnect.state = parcel.bodyInt[1];

    //ALOGI("notify type:%d, %p", srcConnect.msgType, mpObserver);
    if (mpObserver != NULL)
        mpObserver->onTvEvent(srcConnect);
}

int TvInputIntf::startTv()
{
    return mTvSession->startTv();
}

int TvInputIntf::stopTv()
{
    return mTvSession->stopTv();
}

int TvInputIntf::switchSourceInput(tv_source_input_t source_input)
{
    return mTvSession->switchInputSrc(source_input);
}

int TvInputIntf::getSourceConnectStatus(tv_source_input_t source_input)
{
    return mTvSession->getInputSrcConnectStatus(source_input);
}

int TvInputIntf::getCurrentSourceInput()
{
    return mTvSession->getCurrentInputSrc();
}

int TvInputIntf::getHdmiAvHotplugDetectOnoff()
{
    return mTvSession->getHdmiAvHotplugStatus();
}

int TvInputIntf::getSupportInputDevices(int *devices, int *count)
{
    std::string serverDevices = mTvSession->getSupportInputDevices();
    const char *input_list = serverDevices.c_str();
    ALOGD("getAllTvDevices input list = %s", input_list);

    int len = 0;
    const char *seg = ",";
    char *pT = strtok((char*)input_list, seg);
    while (pT) {
        len ++;
        *devices = atoi(pT);
        ALOGD("devices: %d: %d", len , *devices);
        devices ++;
        pT = strtok(NULL, seg);
    }
    *count = len;
    return 0;

}

int TvInputIntf::getHdmiPort(tv_source_input_t source_input) {
    return mTvSession->getHdmiPorts(source_input);
}

