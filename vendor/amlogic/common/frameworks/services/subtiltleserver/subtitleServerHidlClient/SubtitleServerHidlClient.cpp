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
#define LOG_TAG "SubtitleServerHidlClient"

#include "SubtitleServerHidlClient.h"
//#include <binder/Binder.h>
#include <utils/Atomic.h>
#include <utils/Log.h>
//#include <utils/RefBase.h>
#include <utils/String8.h>
#include <utils/String16.h>
#include <unistd.h>

namespace android {
static Mutex amgLock;
int mTotal = -1;
int mRetry = RETRY_MAX;
pthread_t mSubtitleThread;
bool mThreadStop;
int mpos = 0;
static sp<ISubtitleServer> service;

sp<ISubtitleServer> SubtitleServerHidlClient::getSubtitleService()
{
    ALOGE("SubtitleServerHidlClient getSubtitleService ...");
    Mutex::Autolock _l(amgLock);
    if (service == nullptr) {
        service = ISubtitleServer::tryGetService();
        while (service == nullptr) {
           usleep(200*1000);//sleep 200ms
           service = ISubtitleServer::tryGetService();
           ALOGE("tryGet ISubtitleServer daemon Service");
        };
    }
    return service;
}

SubtitleServerHidlClient::SubtitleServerHidlClient()
{
    ALOGE("SubtitleServerHidlClient creat ...");
    getSubtitleService();
}

SubtitleServerHidlClient::~SubtitleServerHidlClient()
{
}

int SubtitleServerHidlClient::subtitleGetTypeDetial()
{
    int ret = 0;
    const sp<ISubtitleServer>& subser = getSubtitleService();
    if (subser != 0) {
        ret = subser->getTypeDetial();
    }
    return ret;
}

void SubtitleServerHidlClient::subtitleShowSub(int pos)
{
    const sp<ISubtitleServer>& subser = getSubtitleService();
    if (subser != 0) {
        ALOGE("subtitleShowSub pos:%d\n", pos);
	 if (pos > 0) {
            subser->showSub(pos);
	 }
    }
    return;
}

void *SubtitleServerHidlClient::thrSubtitleShow(void *pthis)
{
    int pos = 0;
    SubtitleServerHidlClient *ssh = (SubtitleServerHidlClient *) pthis;
    //ALOGE("thrSubtitleShow pos:%d\n",pos);

    do {
        ALOGE("1thrSubtitleShow pos:%d\n", (ssh->gsubtitleCtx.ptscb)());
        ssh->subtitleShowSub((ssh->gsubtitleCtx.ptscb)());
        usleep((300 - (pos % 300)) * 1000);
    }
    while (!mThreadStop);
    return NULL;
}

void SubtitleServerHidlClient::subtitleShow()
{
    pthread_create(&mSubtitleThread, NULL, thrSubtitleShow, (void *)this);
}

void SubtitleServerHidlClient::switchSubtitle(SUB_Para_t *para)
{
    ALOGE("switchSubtitle tvType:%d\n", para->tvType);
    const sp<ISubtitleServer>& subser = getSubtitleService();
    if (subser != 0) {
	    subser->setSubType(para->tvType);
	    if (para->tvType == CODEC_ID_SCTE27) {
	        subser->setSubPid(para->pid);
		 subser->close();
		 subser->open("");
	    } else if (para->tvType == CODEC_ID_DVB) {
	        subser->setSubPid(para->pid);
		 subser->close();
		 subser->open("");
	   } else if (para->vfmt == CODEC_ID_CLOSEDCAPTION_MPEG) {
		 subser->close();
		 subser->open("");
	   } else if (para->vfmt == CODEC_ID_CLOSEDCAPTION_H264) {
		 subser->close();
		 subser->open("");
	   }
     }

}

void SubtitleServerHidlClient::initSubtitle(SUB_Para_t *para)
{
    ALOGE("initSubtitle tvType:%d\n", para->tvType);
    const sp<ISubtitleServer>& subser = getSubtitleService();
    if (subser != 0) {
	    subser->setSubType(para->tvType);
	    if (para->tvType == CODEC_ID_SCTE27) {
	        subser->setSubPid(para->pid);
               subser->setChannelId(15);
	    } else if (para->tvType == CODEC_ID_DVB) {
	        subser->setSubPid(para->pid);
               subser->setPageId(para->pageId);
               subser->setAncPageId(para->ancPageId);
               subser->setChannelId(15);
	   } else if (para->vfmt == CODEC_ID_CLOSEDCAPTION_MPEG) {
              subser->setClosedCaptionVfmt(CODEC_ID_CLOSEDCAPTION_MPEG);
              subser->setChannelId(0);
	   } else if (para->vfmt == CODEC_ID_CLOSEDCAPTION_H264) {
              subser->setClosedCaptionVfmt(CODEC_ID_CLOSEDCAPTION_H264);
              subser->setChannelId(0);
	   }
     }
}

void SubtitleServerHidlClient::subtitleOpen(const std::string& path, getPtsCb cb, SUB_Para_t * para)
{
    //ALOGE("subtitleOpen pos:%d\n", cb());
    //gsubtitleCtx = new subtitlectx();
    const sp<ISubtitleServer>& subser = getSubtitleService();
    if (subser != 0) {
        initSubtitle(para);
        subser->open(path);
        mRetry = RETRY_MAX;
        mThreadStop = false;
    }
   gsubtitleCtx.ptscb = cb;
    //ctc_mp = static_cast<CTsPlayer *>(pthis);
    ALOGE("subtitleOpen pos2:%d\n", gsubtitleCtx.ptscb());
    return;
}

void SubtitleServerHidlClient::subtitleOpenIdx(int idx)
{
    const sp<ISubtitleServer>& subser = getSubtitleService();
    if (subser != 0) {
        subser->openIdx(idx);
    }
    return;
}

void SubtitleServerHidlClient::subtitleClose()
{
    const sp<ISubtitleServer>& subser = getSubtitleService();
    if (subser != 0) {
        subser->close();
    }
    mThreadStop = true;
    pthread_join(mSubtitleThread, NULL);
    return;
}

int SubtitleServerHidlClient::subtitleGetTotal()
{
    int ret = -1;
    const sp<ISubtitleServer>& subser = getSubtitleService();
    if (subser != 0) {
        ret = subser->getTotal();
    }
    return ret;
}

void SubtitleServerHidlClient::subtitleNext()
{
    const sp<ISubtitleServer>& subser = getSubtitleService();
    if (subser != 0) {
        subser->next();
    }
    return;
}

void SubtitleServerHidlClient::subtitlePrevious()
{
    const sp<ISubtitleServer>& subser = getSubtitleService();
    if (subser != 0) {
        subser->previous();
    }
    return;
}

void SubtitleServerHidlClient::subtitleOption()
{
    const sp<ISubtitleServer>& subser = getSubtitleService();
    if (subser != 0) {
        subser->option();
    }
    return;
}

int SubtitleServerHidlClient::subtitleGetType()
{
    int ret = 0;
    const sp<ISubtitleServer>& subser = getSubtitleService();
    if (subser != 0) {
        ret = subser->getType();
    }
    return ret;
}

std::string SubtitleServerHidlClient::subtitleGetTypeStr()
{
    char* ret;
    const sp<ISubtitleServer>& subser = getSubtitleService();
    if (subser != 0) {
        //@@ret = String8(subser->getTypeStr()).string();
    }
    return ret;
}

void SubtitleServerHidlClient::subtitleSetTextColor(int color)
{
    const sp<ISubtitleServer>& subser = getSubtitleService();
    if (subser != 0) {
        subser->setTextColor(color);
    }
    return;
}

void SubtitleServerHidlClient::subtitleSetTextSize(int size)
{
    const sp<ISubtitleServer>& subser = getSubtitleService();
    if (subser != 0) {
        subser->setTextSize(size);
    }
    return;
}

void SubtitleServerHidlClient::subtitleSetGravity(int gravity)
{
    const sp<ISubtitleServer>& subser = getSubtitleService();
    if (subser != 0) {
        subser->setGravity(gravity);
    }
    return;
}

void SubtitleServerHidlClient::subtitleSetTextStyle(int style)
{
    const sp<ISubtitleServer>& subser = getSubtitleService();
    if (subser != 0) {
        subser->setTextStyle(style);
    }
    return;
}

void SubtitleServerHidlClient::subtitleSetPosHeight(int height)
{
    const sp<ISubtitleServer>& subser = getSubtitleService();
    if (subser != 0) {
        subser->setPosHeight(height);
    }
    return;
}

void SubtitleServerHidlClient::subtitleSetImgRatio(float ratioW, float ratioH, int maxW, int maxH)
{
    const sp<ISubtitleServer>& subser = getSubtitleService();
    if (subser != 0) {
        subser->setImgRatio(ratioW, ratioH, maxW, maxH);
    }
    return;
}

void SubtitleServerHidlClient::subtitleSetSurfaceViewParam(int x, int y, int w, int h)
{
    const sp<ISubtitleServer>& subser = getSubtitleService();
    ALOGE("subtitleSetSurfaceViewParam 00\n");
    if (subser != 0) {
        ALOGE("subtitleSetSurfaceViewParam 01\n");
        subser->setSurfaceViewParam(x, y, w, h);
    }
    return;
}

void SubtitleServerHidlClient::subtitleClear()
{
    const sp<ISubtitleServer>& subser = getSubtitleService();
    if (subser != 0) {
        subser->clear();
    }
    return;
}

void SubtitleServerHidlClient::subtitleResetForSeek()
{
    const sp<ISubtitleServer>& subser = getSubtitleService();
    if (subser != 0) {
        subser->resetForSeek();
    }
    return;
}

void SubtitleServerHidlClient::subtitleHide()
{
    const sp<ISubtitleServer>& subser = getSubtitleService();
    if (subser != 0) {
        subser->hide();
    }
    return;
}

void SubtitleServerHidlClient::subtitleDisplay()
{
    const sp<ISubtitleServer>& subser = getSubtitleService();
    if (subser != 0) {
        subser->display();
    }
    return;
}

std::string SubtitleServerHidlClient::subtitleGetCurName()
{
    const sp<ISubtitleServer>& subser = getSubtitleService();
    std::string curName;
    if (subser != 0) {
        subser->getCurName([&](const ::android::hardware::hidl_string& currentName){
            curName = currentName;
        });
    }
    return curName;
}

std::string SubtitleServerHidlClient::subtitleGetName(int idx)
{
    const sp<ISubtitleServer>& subser = getSubtitleService();
    std::string name;
    if (subser != 0) {
        subser->getName([&](const ::android::hardware::hidl_string& subName){
            name = subName;
        });
    }
    return name;
}

std::string SubtitleServerHidlClient::subtitleGetLanguage(int idx)
{
    ALOGE("[subtitleGetLanguage]");
    const sp<ISubtitleServer>& subser = getSubtitleService();
    std::string langugge;
    if (subser != 0) {
        subser->getLanguage([&](const ::android::hardware::hidl_string& subLanguage){
            langugge = subLanguage;
        });
    }
    //ALOGE("[subtitleGetLanguage]langugge:%s", langugge.c_str());
    return langugge;
}

void SubtitleServerHidlClient::subtitleLoad(const std::string& path)
{
    const sp<ISubtitleServer>& subser = getSubtitleService();
    if (subser != 0) {
        subser->load(path);
    }
    return;
}

void SubtitleServerHidlClient::subtitleCreat()
{
    ALOGE("subtitleCreat");
    const sp<ISubtitleServer>& subser = getSubtitleService();
    if (subser != 0) {
        subser->creat();
    }
    usleep(500*1000);
    return;
}

void SubtitleServerHidlClient::subtitleSetSubPid(int pid)
{
    const sp<ISubtitleServer>& subser = getSubtitleService();
    if (subser != 0) {
        subser->setSubPid(pid);
    }
    return;
}

int SubtitleServerHidlClient::subtitleGetSubHeight()
{
    int ret = 0;
    const sp<ISubtitleServer>& subser = getSubtitleService();
    if (subser != 0) {
        ret = subser->getSubHeight();
    }
    return ret;
}

int SubtitleServerHidlClient::subtitleGetSubWidth()
{
    int ret = 0;
    const sp<ISubtitleServer>& subser = getSubtitleService();
    if (subser != 0) {
        ret = subser->getSubWidth();
    }
    return ret;
}

void SubtitleServerHidlClient::subtitleSetSubType(int type)
{
    const sp<ISubtitleServer>& subser = getSubtitleService();
    if (subser != 0) {
        subser->setSubType(type);
    }
    return;
}

void SubtitleServerHidlClient::subtitleDestory()
{
    ALOGE("subtitleDestory");
    const sp<ISubtitleServer>& subser = getSubtitleService();
    if (subser != 0) {
        subser->destory();
    }
    return;
}


void SubtitleServerHidlClient::setListener(const sp<SubtitleMiddleListener> &listener)
{
    ALOGI("SubtitleServerHidlClient setListener");
    mListener = listener;
    const sp<ISubtitleServer>& subser = getSubtitleService();
    if (listener != NULL) {
        if (mSubtitleServerHidlCallback == NULL) {
            mSubtitleServerHidlCallback = new SubtitleServerHidlCallback(this);
            ALOGI("SubtitleServerHidlClient setCallback");
            subser->setCallback(mSubtitleServerHidlCallback, static_cast<ConnectType>(1)/*ConnectType:TYPE_EXTEND*/);
        }
    }
}

// callback from subtitle service
Return<void> SubtitleServerHidlClient::SubtitleServerHidlCallback::notifyCallback(const SubtitleHidlParcel& hidlParcel)
{
    //ALOGI("notifyCallback event type:%d", hidlParcel.msgType);

    if (hidlParcel.msgType == EVENT_ONSUBTITLEDATA_CALLBACK) {
         ALOGD("notifyCallback notify parcel.msgType = %d, event:%d, id:%d", hidlParcel.msgType,hidlParcel.bodyInt[0], hidlParcel.bodyInt[1]);
    } else if (hidlParcel.msgType == EVENT_ONSUBTITLEAVAILABLE_CALLBACK) {
         ALOGD("notifyCallback notify parcel.msgType = %d, available:%d", hidlParcel.msgType, hidlParcel.bodyInt[0]);
    }	else {
         return Void();
    }

    if (subtitleserverClient->mListener == NULL) {
        ALOGI("listener is  null");
        return Void();
    } else {
        ALOGI("listener not  null");
    }

    sp<SubtitleMiddleListener> listener;
    {
        Mutex::Autolock _l(amgLock);
        listener = subtitleserverClient->mListener;
    }

    subtitle_parcel_t parcel;
    parcel.msgType = hidlParcel.msgType;
    for (int i = 0; i < hidlParcel.bodyInt.size(); i++) {
        parcel.bodyInt.push_back(hidlParcel.bodyInt[i]);
    }

    for (int j = 0; j < hidlParcel.bodyString.size(); j++) {
        parcel.bodyString.push_back(hidlParcel.bodyString[j]);
    }

    if (listener != NULL) {
        listener->notify(parcel);
    }
    return Void();
}
/*void SubtitleServerHidlClient::SubtitleServerDaemonDeathRecipient::serviceDied(uint64_t cookie __unused,
        const ::android::wp<::android::hidl::base::V1_0::IBase>& who __unused)
{
    ALOGE("tvserver daemon died.");
    Mutex::Autolock _l(amgLock);

    //usleep(200*1000);//sleep 200ms
    //subtitleserverClient->reconnect();
}*/
}

