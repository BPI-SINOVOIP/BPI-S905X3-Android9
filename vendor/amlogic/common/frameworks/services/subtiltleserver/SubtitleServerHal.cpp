/*
 * Copyright (C) 2016 The Android Open Source Project
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
 *  @author   wanglu
 *  @version  1.0
 *  @date     2019/8/07
 *  @par function description:
 *  - 1 subtitle server hwbinder service
 */

#define LOG_TAG "SubtitleServerHal"

#include <log/log.h>
#include <utils/Mutex.h>
#include <atomic>
#include <chrono>
#include <cmath>
#include <inttypes.h>
#include <string>
#include <vector>

#include <utils/KeyedVector.h>
#include <utils/String8.h>

#include "SubtitleServerHal.h"

namespace vendor {
namespace amlogic {
namespace hardware {
namespace subtitleserver {
namespace V1_0 {
namespace implementation {

#define ENABLE_LOG_PRINT 1
int mTotal;
int mType;
int mHeight;
int mWidth;
std::string mLanguage;

SubtitleServerHal::SubtitleServerHal() : mDeathRecipient(new DeathRecipient(this)) {
    ALOGI("SubtitleServerHal .....");
    //control->setListener(this);
}

SubtitleServerHal::~SubtitleServerHal() {
}

void SubtitleServerHal::sendEvent(SubtitleHidlParcel parcel) {
    android::AutoMutex _l( mLock );
    int clientSize = mClients.size();

    if (ENABLE_LOG_PRINT)
        ALOGI("onEvent event:%d, client size:%d", parcel.msgType, clientSize);

    for (int i = 0; i < clientSize; i++) {
        if (mClients[i] != nullptr) {
            if (ENABLE_LOG_PRINT)
                ALOGI("%s, client cookie:%d notifyCallback", __FUNCTION__, i);
            mClients[i]->notifyCallback(parcel);
        }
    }
}

Return<int32_t> SubtitleServerHal::showSub(int32_t pos) {
    ALOGI("[showSub]pos:%d", pos);
    SubtitleHidlParcel parcel;
    parcel.msgType = EVENT_SHOW_SUB;
    parcel.bodyInt.resize(1);
    parcel.bodyInt[0] = pos;
    ALOGI("[showSub]-1-");
    sendEvent(parcel);
    ALOGI("[showSub]-2-");
    return 1;
}

Return<int32_t> SubtitleServerHal::open(const hidl_string &path) {
    ALOGI("[open]");
    SubtitleHidlParcel parcel;
    parcel.msgType = EVENT_SETSOURCE;
    parcel.bodyString.resize(1);
    parcel.bodyString[0] = path;
    sendEvent(parcel);
    return 1;
}


Return<int32_t> SubtitleServerHal::openIdx(int32_t idx) {
    ALOGI("[openIdx]idx:%d", idx);
    SubtitleHidlParcel parcel;
    parcel.msgType = EVENT_OPENIDX;
    parcel.bodyInt.resize(1);
    parcel.bodyInt[0] = idx;
    sendEvent(parcel);
    return 1;
}

Return<int32_t> SubtitleServerHal::close() {
    ALOGI("[close]");
    SubtitleHidlParcel parcel;
    parcel.msgType = EVENT_CLOSE;
    sendEvent(parcel);
    return 1;
}

Return<int32_t> SubtitleServerHal::getTotal() {
    ALOGI("[getTotal]");
    SubtitleHidlParcel parcel;
    parcel.msgType = EVENT_GETOTAL;
    sendEvent(parcel);
    usleep(10*1000);
    ALOGI("[getTotal]-1-total:%d", mTotal);
    return mTotal;
}

Return<int32_t> SubtitleServerHal::setTotal(int32_t total) {
    ALOGI("[setTotal]total:%d", total);
    mTotal = total;
    return 1;
}

Return<int32_t> SubtitleServerHal::next() {
    ALOGI("[next]");
    SubtitleHidlParcel parcel;
    parcel.msgType = EVENT_NEXT;
    sendEvent(parcel);
    return 1;
}

Return<int32_t> SubtitleServerHal::previous() {
    ALOGI("[previous]");
    SubtitleHidlParcel parcel;
    parcel.msgType = EVENT_PREVIOUS;
    sendEvent(parcel);
    return 1;
}

Return<int32_t> SubtitleServerHal::option() {
    ALOGI("[option]");
    SubtitleHidlParcel parcel;
    parcel.msgType = EVENT_OPTION;
    sendEvent(parcel);
    return 1;
}

Return<int32_t> SubtitleServerHal::getType() {
    ALOGI("[getType]");
    SubtitleHidlParcel parcel;
    parcel.msgType = EVENT_GETTYPE;
    sendEvent(parcel);
    usleep(20*1000);   //for java callback 0.02s
    ALOGI("[getType]-1-type:%d", mType);
    return mType;
}

Return<int32_t> SubtitleServerHal::setType(int32_t type) {
    ALOGI("[setType]type:%d", type);
    mType = type;
    return 1;
}

Return<int32_t> SubtitleServerHal::getTypeDetial() {
    ALOGI("[getTypeDetial]");
    SubtitleHidlParcel parcel;
    parcel.msgType = EVENT_GETTYPEDETIAL;
    sendEvent(parcel);
    usleep(10*1000);   //for java callback 0.1s
    ALOGI("[getTypeDetial]-1-type:%d", mType);
    return mType;
}

Return<int32_t> SubtitleServerHal::setTypeDetial(int32_t type) {
    ALOGI("[setTypeDetial]type:%d", type);
    return 1;
}

Return<int32_t> SubtitleServerHal::setTextColor(int32_t color) {
    ALOGI("[setTextColor]");
    SubtitleHidlParcel parcel;
    parcel.msgType = EVENT_SETTEXTCOLOR;
    parcel.bodyInt.resize(1);
    parcel.bodyInt[0] = color;
    sendEvent(parcel);
    return 1;
}

Return<int32_t> SubtitleServerHal::setTextSize(int32_t size) {
    ALOGI("[setTextSize]");
    SubtitleHidlParcel parcel;
    parcel.msgType = EVENT_SETTEXTSIZE;
    parcel.bodyInt.resize(1);
    parcel.bodyInt[0] = size;
    sendEvent(parcel);
    return 1;
}

Return<int32_t> SubtitleServerHal::setGravity(int32_t gravity) {
    ALOGI("[setGravity]");
    SubtitleHidlParcel parcel;
    parcel.msgType = EVENT_SETGRAVITY;
    parcel.bodyInt.resize(1);
    parcel.bodyInt[0] = gravity;
    sendEvent(parcel);
    return 1;
}

Return<int32_t> SubtitleServerHal::setTextStyle(int32_t style) {
    ALOGI("[setTextStyle]");
    SubtitleHidlParcel parcel;
    parcel.msgType = EVENT_SETTEXTSTYLE;
    parcel.bodyInt.resize(1);
    parcel.bodyInt[0] = style;
    sendEvent(parcel);
    return 1;
}

Return<int32_t> SubtitleServerHal::setPosHeight(int32_t height) {
    ALOGI("[setPosHeight]");
    SubtitleHidlParcel parcel;
    parcel.msgType = EVENT_SETPOSHEIGHT;
    parcel.bodyInt.resize(1);
    parcel.bodyInt[0] = height;
    sendEvent(parcel);
    return 1;
}

Return<int32_t> SubtitleServerHal::setImgRatio(float ratioW, float ratioH, int32_t maxW, int32_t maxH) {
    ALOGI("[setImgRatio]");
    SubtitleHidlParcel parcel;
    parcel.msgType = EVENT_SETIMGRATIO;
    parcel.bodyFloat.resize(2);
    parcel.bodyInt.resize(2);
    parcel.bodyFloat[0] = ratioW;
    parcel.bodyFloat[1] = ratioH;
    parcel.bodyInt[2] = maxW;
    parcel.bodyInt[3] = maxH;
    sendEvent(parcel);
    return 1;
}

Return<int32_t> SubtitleServerHal::clear() {
    ALOGI("[clear]");
    SubtitleHidlParcel parcel;
    parcel.msgType = EVENT_CLEAR;
    sendEvent(parcel);
    return 1;
}

Return<int32_t> SubtitleServerHal::resetForSeek() {
    ALOGI("[resetForSeek]");
    SubtitleHidlParcel parcel;
    parcel.msgType = EVENT_RESETFORSEEK;
    sendEvent(parcel);
    return 1;
}

Return<int32_t> SubtitleServerHal::hide() {
    ALOGI("[hide]");
    SubtitleHidlParcel parcel;
    parcel.msgType = EVENT_HIDE;
    sendEvent(parcel);
    return 1;
}

Return<int32_t> SubtitleServerHal::display() {
    ALOGI("[display]");
    SubtitleHidlParcel parcel;
    parcel.msgType = EVENT_DISPLAY;
    sendEvent(parcel);
    return 1;
}

Return<void> SubtitleServerHal::getCurName(getCurName_cb _hidl_cb) {
    ALOGI("[getCurName]");
    SubtitleHidlParcel parcel;
    parcel.msgType = EVENT_GETCURNAME;
    sendEvent(parcel);
    std::string curName = "cn";
    _hidl_cb(curName);
    return Void();
}

Return<void> SubtitleServerHal::getName(getName_cb _hidl_cb) {
    ALOGI("[getName]");
    SubtitleHidlParcel parcel;
    parcel.msgType = EVENT_GETNAME;
    sendEvent(parcel);
    std::string name = "cn";
    _hidl_cb(name);
    return Void();
}

Return<void> SubtitleServerHal::getLanguage(getLanguage_cb _hidl_cb) {
    ALOGI("[getLanguage]");
    SubtitleHidlParcel parcel;
    parcel.msgType = EVENT_GETLANAGUAGE;
    sendEvent(parcel);
    usleep(20*1000);   //for java callback 0.02s
    //std::string language = "cn";
    _hidl_cb(mLanguage);
    return Void();
}

Return<int32_t> SubtitleServerHal::setName(const hidl_string &name) {
    //ALOGI("[setName]name:%s", name);
    return 1;
}

Return<int32_t> SubtitleServerHal::setCurName(const hidl_string &curName) {
    //ALOGI("[setCurName]curName:%s", curName);
    return 1;
}

Return<int32_t> SubtitleServerHal::setLanguage(const hidl_string &language) {
    ALOGI("[setLanguage]language:%s", language.c_str());
    mLanguage = language;
    return 1;
}

Return<int32_t> SubtitleServerHal::load(const hidl_string &path) {
    ALOGI("[load]");
    SubtitleHidlParcel parcel;
    parcel.msgType = EVENT_LOAD;
    parcel.bodyString.resize(1);
    parcel.bodyString[0] = path;
    sendEvent(parcel);
    return 1;
}

Return<int32_t> SubtitleServerHal::setSurfaceViewParam(int32_t x, int32_t y, int32_t w, int32_t h) {
    ALOGI("[setSurfaceViewParam]");
    SubtitleHidlParcel parcel;
    parcel.msgType = EVENT_SETSURFACEVIEWPARAM;
    parcel.bodyInt.resize(4);
    parcel.bodyInt[0] = x;
    parcel.bodyInt[1] = y;
    parcel.bodyInt[2] = w;
    parcel.bodyInt[3] = h;
    sendEvent(parcel);
    return 1;
}

Return<int32_t> SubtitleServerHal::creat() {
    ALOGI("[creat]");
    SubtitleHidlParcel parcel;
    parcel.msgType = EVENT_CREAT;
    sendEvent(parcel);
    return 1;
}

Return<int32_t> SubtitleServerHal::setSubPid(int32_t pid) {
    ALOGI("[setSubPid]");
    SubtitleHidlParcel parcel;
    parcel.msgType = EVENT_SETSUBPID;
    parcel.bodyInt.resize(1);
    parcel.bodyInt[0] = pid;
    sendEvent(parcel);
    return 1;
}

Return<int32_t> SubtitleServerHal::getSubHeight() {
    ALOGI("[getSubHeight]");
    SubtitleHidlParcel parcel;
    parcel.msgType = EVENT_GETSUBHEIGHT;
    sendEvent(parcel);
    usleep(20*1000);   //for java callback 0.1s
    ALOGI("[getSubHeight]-1-type:%d", mHeight);
    return mHeight;
}

Return<int32_t> SubtitleServerHal::setSubHeight(int32_t height) {
    ALOGI("[setSubHeight]height:%d", height);
    mHeight = height;
    return 1;
}

Return<int32_t> SubtitleServerHal::getSubWidth() {
    ALOGI("[getSubWidth]");
    SubtitleHidlParcel parcel;
    parcel.msgType = EVENT_GETSUBWIDTH;
    sendEvent(parcel);
    usleep(20*1000);   //for java callback 0.1s
    ALOGI("[getTypeDetial]-1-type:%d", mWidth);
    return mWidth;
}

Return<int32_t> SubtitleServerHal::setSubWidth(int32_t width) {
    ALOGI("[setSubWidth]width:%d", width);
    mWidth = width;
    return 1;
}

Return<int32_t> SubtitleServerHal::setSubType(int32_t type) {
    ALOGI("[setSubType]");
    SubtitleHidlParcel parcel;
    parcel.msgType = EVENT_SETSUBTYPE;
    parcel.bodyInt.resize(1);
    parcel.bodyInt[0] = type;
    sendEvent(parcel);
    return 1;
}

Return<int32_t> SubtitleServerHal::subTitleIdCallback(int32_t event, int32_t id) {
    ALOGI("[subTitleIdCallback]event:%d, id:%d", event, id);
    SubtitleHidlParcel parcel;
    parcel.msgType = EVENT_ONSUBTITLEDATA_CALLBACK;
    parcel.bodyInt.resize(2);
    parcel.bodyInt[0] = event;
    parcel.bodyInt[1] = id;
    sendEvent(parcel);
    return 1;
}

Return<int32_t> SubtitleServerHal::subTitleAvailableCallback(int32_t available) {
    ALOGI("[subTitleAvailableCallback]available:%d", available);
    SubtitleHidlParcel parcel;
    parcel.msgType = EVENT_ONSUBTITLEAVAILABLE_CALLBACK;
    parcel.bodyInt.resize(1);
    parcel.bodyInt[0] = available;
    sendEvent(parcel);
    return 1;
}

Return<int32_t> SubtitleServerHal::destory() {
    ALOGI("[destory]");
    SubtitleHidlParcel parcel;
    parcel.msgType = EVENT_DESTORY;
    sendEvent(parcel);
    return 1;
}

Return<int32_t> SubtitleServerHal::setPageId(int32_t pageId) {
    ALOGI("[setPageId]");
    SubtitleHidlParcel parcel;
    parcel.msgType = EVENT_SETPAGEID;
    parcel.bodyInt.resize(1);
    parcel.bodyInt[0] = pageId;
    sendEvent(parcel);
    return 1;
}

Return<int32_t> SubtitleServerHal::setAncPageId(int32_t ancPageId) {
    ALOGI("[setAncPageId]");
    SubtitleHidlParcel parcel;
    parcel.msgType = EVENT_SETANCPAGEID;
    parcel.bodyInt.resize(1);
    parcel.bodyInt[0] = ancPageId;
    sendEvent(parcel);
    return 1;
}

Return<int32_t> SubtitleServerHal::setChannelId(int32_t channelId) {
    ALOGI("[setChannelId]");
    SubtitleHidlParcel parcel;
    parcel.msgType = EVENT_SETCHANNELID;
    parcel.bodyInt.resize(1);
    parcel.bodyInt[0] = channelId;
    sendEvent(parcel);
    return 1;
}

Return<int32_t> SubtitleServerHal::setClosedCaptionVfmt(int32_t vfmt) {
    ALOGI("[setClosedCaptionVfmt]");
    SubtitleHidlParcel parcel;
    parcel.msgType = EVENT_SETCLOSEDCAPTIONVFMT;
    parcel.bodyInt.resize(1);
    parcel.bodyInt[0] = vfmt;
    sendEvent(parcel);
    return 1;
}



Return<void> SubtitleServerHal::setCallback(const sp<ISubtitleServerCallback>& callback, ConnectType type) {
    ALOGI("[setCallback]");
    android::AutoMutex _l(mLock);
    if (callback != nullptr) {
        int cookie = -1;
        int clientSize = mClients.size();
        for (int i = 0; i < clientSize; i++) {
            if (mClients[i] == nullptr) {
                ALOGI("%s, client index:%d had died, this id give the new client", __FUNCTION__, i);
                cookie = i;
                mClients[i] = callback;
                break;
            }
        }

        if (cookie < 0) {
            cookie = clientSize;
            mClients[clientSize] = callback;
        }

        Return<bool> linkResult = callback->linkToDeath(mDeathRecipient, cookie);
        bool linkSuccess = linkResult.isOk() ? static_cast<bool>(linkResult) : false;
        if (!linkSuccess) {
            ALOGW("Couldn't link death recipient for cookie: %d", cookie);
        }

        ALOGI("%s cookie:%d, client size:%d", __FUNCTION__, cookie, (int)mClients.size());
    }

    return Void();
}

void SubtitleServerHal::handleServiceDeath(uint32_t cookie) {
    android::AutoMutex _l( mLock );
    mClients[cookie]->unlinkToDeath(mDeathRecipient);
    mClients[cookie].clear();
}

SubtitleServerHal::DeathRecipient::DeathRecipient(sp<SubtitleServerHal> ssh)
        : mSubtitleServerHal(ssh) {}

void SubtitleServerHal::DeathRecipient::serviceDied(
        uint64_t cookie,
        const ::android::wp<::android::hidl::base::V1_0::IBase>& /*who*/) {
    ALOGE("subtitleserver daemon client died cookie:%d", (int)cookie);

    uint32_t type = static_cast<uint32_t>(cookie);
    mSubtitleServerHal->handleServiceDeath(type);
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace subtitleserver
}  // namespace hardware
}  // namespace android
}  // namespace vendor
