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
 *  @date     2019/08/15
 *  @par function description:
 *  - 1 subtitle server hwbinder client for CTC
 */

#ifndef _ANDROID_SUBTITLE_SERVER_HIDL_CLIENT_H_
#define _ANDROID_SUBTITLE_SERVER_HIDL_CLIENT_H_
#define RETRY_MAX 3
#define EVENT_ONSUBTITLEDATA_CALLBACK       0xA00000
#define EVENT_ONSUBTITLEAVAILABLE_CALLBACK  0xA00001
#define EVENT_ONVIDEOAFDCHANGE_CALLBACK     0xA00002
#define EVENT_ONMIXVIDEOEVENT_CALLBACK      0xA00003


#include <utils/Timers.h>
//#include <utils/threads.h>
#include <utils/RefBase.h>
#include <utils/Mutex.h>

#include <vendor/amlogic/hardware/subtitleserver/1.0/ISubtitleServer.h>

namespace android {

using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::sp;
using ::android::hardware::Return;
using ::android::hardware::Void;

using ::vendor::amlogic::hardware::subtitleserver::V1_0::ISubtitleServer;
using ::vendor::amlogic::hardware::subtitleserver::V1_0::ISubtitleCallback;
using ::vendor::amlogic::hardware::subtitleserver::V1_0::Result;
using ::vendor::amlogic::hardware::subtitleserver::V1_0::ConnectType;
using ::vendor::amlogic::hardware::subtitleserver::V1_0::OpenType;
using ::vendor::amlogic::hardware::subtitleserver::V1_0::SubtitleHidlParcel;

typedef struct subtitle_parcel_s {
    int msgType;
    std::vector<int> bodyInt;
    std::vector<std::string> bodyString;
} subtitle_parcel_t;

class SubtitleMiddleListener : virtual public RefBase {
public:
    virtual void notify(const subtitle_parcel_t &parcel) = 0;
};

class SubtitleServerHidlClient  : public RefBase{
public:

typedef int (*getPtsCb)();

typedef struct{
    getPtsCb ptscb;
    int tvType;
    int pid;
    int vfmt;                         //cc
    int channelId;                 //cc
    int ancPageId;               //dvb
    int pageId;                    //dvb
} SUB_Para_t;
SUB_Para_t gsubtitleCtx;

typedef enum
{
    CODEC_ID_CLOSEDCAPTION_MPEG= 0,
    CODEC_ID_CLOSEDCAPTION_H264=2,
    CODEC_ID_SCTE27=3,
    CODEC_ID_DVB=4,
    CODEC_ID_DEMUX_DVB=5,
    CODEC_ID_DEMUX_TELETEXT=6,
    CODEC_ID_DEMUX_SCTE27=7,
}SUB_TYPE;


    SubtitleServerHidlClient();
    ~SubtitleServerHidlClient();

    void subtitleCreat();
    void subtitleDestory();
    void subtitleShow();
    void subtitleOpen(const std::string& path, getPtsCb cb, SUB_Para_t *para);
    void subtitleOpenIdx(int32_t idx);
    void subtitleClose();
    int subtitleGetTotal();
    void subtitleNext();
    void subtitlePrevious();
    void subtitleShowSub(int32_t pos);
    void subtitleOption();
    int subtitleGetType();
    std::string subtitleGetTypeStr();
    int subtitleGetTypeDetial();
    void subtitleSetTextColor(int32_t color);
    void subtitleSetTextSize(int32_t size);
    void subtitleSetGravity(int32_t gravity);
    void subtitleSetTextStyle(int32_t style);
    void subtitleSetPosHeight(int32_t height);
    void subtitleSetImgRatio(float ratioW, float ratioH, int32_t maxW, int32_t maxH);
    void subtitleClear();
    void subtitleResetForSeek();
    void subtitleHide();
    void subtitleDisplay();
    std::string subtitleGetCurName();
    std::string subtitleGetName(int32_t idx);
    std::string subtitleGetLanguage(int32_t idx);
    void subtitleLoad(const std::string& path);
    void subtitleSetSurfaceViewParam(int x, int y, int w, int h);
    void subtitleSetSubPid(int32_t pid);
    int subtitleGetSubHeight();
    int subtitleGetSubWidth();
    void subtitleSetSubType(int32_t type);
    void setListener(const sp<SubtitleMiddleListener> &listener);
    void switchSubtitle(SUB_Para_t *para);

    int ttGoHome();
    int ttNextPage(int32_t dir);
    int ttNextSubPage(int32_t dir);
    int ttGotoPage(int32_t pageNo, int32_t subPageNo);
    //void unregisterListener(const sp<SubtitleMiddleListener> &listener);
private:
    class SubtitleServerHidlCallback : public ISubtitleCallback {
    public:
        SubtitleServerHidlCallback(SubtitleServerHidlClient *client): subtitleserverClient(client) {};
        virtual Return<void> notifyDataCallback(const SubtitleHidlParcel &parcel) override {return Void();};
        virtual Return<void> uiCommandCallback(const SubtitleHidlParcel &parcel)  override {return Void();};
        virtual Return<void> eventNotify(const SubtitleHidlParcel& parcel) override;

    private:
        SubtitleServerHidlClient *subtitleserverClient;
    };
   /* struct SubtitleServerDaemonDeathRecipient : public android::hardware::hidl_death_recipient  {
        SubtitleServerDaemonDeathRecipient(SubtitleServerHidlClient *client): subtitleserverClient(client) {};

        // hidl_death_recipient interface
        virtual void serviceDied(uint64_t cookie,
            const ::android::wp<::android::hidl::base::V1_0::IBase>& who) override;
    private:
        SubtitleServerHidlClient *subtitleserverClient;
    };*/
    //sp<SubtitleServerDaemonDeathRecipient> mDeathRecipient = nullptr;
    sp<SubtitleServerHidlCallback> mSubtitleServerHidlCallback = nullptr;
    sp<SubtitleMiddleListener> mListener;
    sp<ISubtitleServer> getSubtitleService();
    static void *thrSubtitleShow(void *pthis);
    void initSubtitle(SUB_Para_t *para);

    int mSessionId;

    bool mDisabled;
};

}//namespace android

#endif/*_ANDROID_TV_SERVER_HIDL_CLIENT_H_*/
