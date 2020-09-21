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
 *  @date     2019/08/07
 *  @par function description:
 *  - 1 subtitle server hwbinder service
 */

#ifndef ANDROID_DROIDLOGIC_SUBTITLE_SERVER_V1_0_H
#define ANDROID_DROIDLOGIC_SUBTITLE_SERVER_V1_0_H

#include <utils/Mutex.h>

#include <vendor/amlogic/hardware/subtitleserver/1.0/ISubtitleServer.h>
#include <vendor/amlogic/hardware/subtitleserver/1.0/types.h>

//#include <SubtitleServerNotify.h>
//#include <SystemControlService.h>

namespace vendor {
namespace amlogic {
namespace hardware {
namespace subtitleserver {
namespace V1_0 {
namespace implementation {

using ::vendor::amlogic::hardware::subtitleserver::V1_0::ISubtitleServer;
using ::vendor::amlogic::hardware::subtitleserver::V1_0::ISubtitleServerCallback;
using ::vendor::amlogic::hardware::subtitleserver::V1_0::Result;
using ::vendor::amlogic::hardware::subtitleserver::V1_0::ConnectType;
using ::vendor::amlogic::hardware::subtitleserver::V1_0::SubtitleHidlParcel;

using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_array;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

class SubtitleServerHal : public ISubtitleServer/*, public SubtitleServerNotify*/ {
  public:
    SubtitleServerHal();/*SystemControlService * control*/
    ~SubtitleServerHal();

    enum EventType
    {
        EVENT_SHOW_SUB = 0,
        EVENT_SETSOURCE,
        EVENT_OPENIDX,
        EVENT_CLOSE,
        EVENT_GETOTAL,
        EVENT_NEXT,
        EVENT_PREVIOUS,
        EVENT_OPTION,
        EVENT_GETTYPE,
        EVENT_GETTYPEDETIAL,
        EVENT_SETTEXTCOLOR,
        EVENT_SETTEXTSIZE,
        EVENT_SETGRAVITY,
        EVENT_SETTEXTSTYLE,
        EVENT_SETPOSHEIGHT,
        EVENT_SETIMGRATIO,
        EVENT_CLEAR,
        EVENT_RESETFORSEEK,
        EVENT_HIDE,
        EVENT_DISPLAY,
        EVENT_GETCURNAME,
        EVENT_GETNAME,
        EVENT_GETLANAGUAGE,
        EVENT_LOAD,
        EVENT_SETSURFACEVIEWPARAM,
        EVENT_CREAT,
        EVENT_SETSUBPID,
        EVENT_GETSUBHEIGHT,
        EVENT_GETSUBWIDTH,
        EVENT_SETSUBTYPE,
        EVENT_ONSUBTITLEDATA_CALLBACK,
        EVENT_ONSUBTITLEAVAILABLE_CALLBACK,
        EVENT_DESTORY,
        EVENT_SETPAGEID,
        EVENT_SETANCPAGEID,
        EVENT_SETCHANNELID,
        EVENT_SETCLOSEDCAPTIONVFMT,
    };

  Return<int32_t> showSub(int32_t pos) override;

  Return<int32_t> open(const hidl_string &path) override;

  Return<int32_t> openIdx(int32_t idx) override;

  Return<int32_t> close() override;

  Return<int32_t> getTotal() override;

  Return<int32_t> setTotal(int32_t total) override;

  Return<int32_t> next() override;

  Return<int32_t> previous() override;

  Return<int32_t> option() override;

  Return<int32_t> getType() override;

  Return<int32_t> setType(int32_t type) override;

  Return<int32_t> getTypeDetial() override;

  Return<int32_t> setTypeDetial(int32_t type) override;

  Return<int32_t> setTextColor(int32_t color) override;

  Return<int32_t> setTextSize(int32_t size) override;

  Return<int32_t> setGravity(int32_t gravity) override;

  Return<int32_t> setTextStyle(int32_t style) override;

  Return<int32_t> setPosHeight(int32_t height) override;

  Return<int32_t> setImgRatio(float ratioW, float ratioH, int32_t maxW, int32_t maxH) override;

  Return<int32_t> clear() override;

  Return<int32_t> resetForSeek() override;

  Return<int32_t> hide() override;

  Return<int32_t> display() override;

  Return<void> getCurName(getCurName_cb _hidl_cb) override;

  Return<void> getName(getName_cb _hidl_cb) override;

  Return<void> getLanguage(getLanguage_cb _hidl_cb) override;

  Return<int32_t> setCurName(const hidl_string &curName) override;

  Return<int32_t> setName(const hidl_string &name) override;

  Return<int32_t> setLanguage(const hidl_string &language) override;

  Return<int32_t> load(const hidl_string &path) override;

  Return<int32_t> setSurfaceViewParam(int32_t x, int32_t y, int32_t w, int32_t h) override;

  Return<int32_t> creat() override;

  Return<int32_t> setSubPid(int32_t pid) override;

  Return<int32_t> getSubHeight() override;

  Return<int32_t> setSubHeight(int32_t height) override;

  Return<int32_t> getSubWidth() override;

  Return<int32_t> setSubWidth(int32_t width) override;

  Return<int32_t> setSubType(int32_t type) override;

  Return<int32_t> subTitleIdCallback(int32_t event, int id) override;

  Return<int32_t> subTitleAvailableCallback(int32_t available) override;

  Return<int32_t> destory() override;

  Return<void> setCallback(const sp<ISubtitleServerCallback>& callback, ConnectType type) override;

   // virtual void onEvent(int event);
  Return<int32_t> setPageId(int32_t pageId) override;

  Return<int32_t> setAncPageId(int32_t andPageId) override;

  Return<int32_t> setChannelId(int32_t channelId) override;

  Return<int32_t> setClosedCaptionVfmt(int32_t vfmt) override;


  private:
    void handleServiceDeath(uint32_t cookie);
    void sendEvent(SubtitleHidlParcel event);

    //SystemControlService *mSysControl;
    mutable android::Mutex mLock;
    std::map<uint32_t, sp<ISubtitleServerCallback>> mClients;

    class DeathRecipient : public android::hardware::hidl_death_recipient  {
    public:
        DeathRecipient(sp<SubtitleServerHal> ssh);

        // hidl_death_recipient interface
        virtual void serviceDied(uint64_t cookie,
            const ::android::wp<::android::hidl::base::V1_0::IBase>& who) override;

    private:
        sp<SubtitleServerHal> mSubtitleServerHal;
    };

    sp<DeathRecipient> mDeathRecipient;

};

}  // namespace implementation
}  // namespace V1_1
}  // namespace systemcontrol
}  // namespace hardware
}  // namespace android
}  // namespace vendor

#endif  // ANDROID_DROIDLOGIC_SYSTEM_CONTROL_V1_0_SYSTEM_CONTROL_HAL_H
