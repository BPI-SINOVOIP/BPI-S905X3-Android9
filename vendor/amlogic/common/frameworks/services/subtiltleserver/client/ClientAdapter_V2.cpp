#define LOG_TAG "subtitleMiddleClient"

#include "subtitleServerHidlClient/subtitleMiddleCallback.h"
#include "ClientAdapter.h"
#include "Amsysfsutils.h"
#include "string.h"

#include <binder/Binder.h>

#include <utils/Atomic.h>
#include <utils/Log.h>
#include <utils/RefBase.h>
#include <utils/String8.h>
#include <utils/String16.h>
#include <utils/threads.h>
#include <unistd.h>


using namespace android;

#define STR_LEN 256

static sp<SubtitleServerHidlClient> service = NULL;
static Mutex amgLock;
sp<EventCallback> spEventCB;
AM_SUBTITLE_Para_t sub_p;

static const sp<SubtitleServerHidlClient>& getSubtitleService()
{
    Mutex::Autolock _l(amgLock);
    if (service == nullptr) {
        service = new SubtitleServerHidlClient();
    }
    ALOGE_IF(service == 0, "no subtitle_service!!");
    //ALOGE("find ISubtitle service...");
    return service;
}

void subtitleShow()
{
    ALOGE("subtitleShow");
    const sp<SubtitleServerHidlClient>& subser = getSubtitleService();
    if (subser != 0) {
        subser->subtitleShow();
    }
    return;
}


int getSubtitleCurrentPos()
{
    ALOGE("getSubtitleCurrentPos");
    int pos = 0;
    int sub_type = -1;
    int firstvpts = amsysfs_get_sysfs_ulong("/sys/class/tsync/firstvpts");
    //ALOGE("firstvpts :%d, subtitleGetTypeDetial:%d,subtitleGetType():%d\n",
    //	amsysfs_get_sysfs_ulong("/sys/class/tsync/firstvpts"),subtitleGetTypeDetial(),subtitleGetType());
    //if (firstvpts == 0) {
    //pos = 0;
    //} else {
    sub_type = 5;//amsysfs_get_sysfs_int("/sys/class/subtitle/subtype");
    if (sub_type == 5){//subtitleGetTypeDetial() == 6) {//dvb sub  return pts_video
        //pos = (ctc_mp->GetCurrentPlayTime()/90);
    } else {
        //pos = ((ctc_mp->GetCurrentPlayTime() - firstvpts)/90);
    }
    //}
    return pos;
}

void subtitleOpenIdx(int idx) {
    const sp<SubtitleServerHidlClient>& subser = getSubtitleService();
    if (subser != 0) {
        subser->subtitleOpenIdx(idx);
    }
    return;
}

void subtitleClose()
{
    const sp<SubtitleServerHidlClient>& subser = getSubtitleService();
    if (subser != 0) {
        subser->subtitleClose();
    }
    return;
}

void subtitleDestory()
{
    const sp<SubtitleServerHidlClient>& subser = getSubtitleService();
    if (subser != 0) {
        subser->subtitleDestory();
    }
    return;
}

int subtitleGetTotal()
{
    int ret = -1;
    const sp<SubtitleServerHidlClient>& subser = getSubtitleService();
    if (subser != 0) {
        ret = subser->subtitleGetTotal();
    }
    return ret;
}

void subtitleNext()
{
    const sp<SubtitleServerHidlClient>& subser = getSubtitleService();
    if (subser != 0) {
        subser->subtitleNext();
    }
    return;
}

void subtitlePrevious()
{
    const sp<SubtitleServerHidlClient>& subser = getSubtitleService();
    if (subser != 0) {
        subser->subtitlePrevious();
    }
    return;
}

void subtitleShowSub(int pos)
{
    const sp<SubtitleServerHidlClient>& subser = getSubtitleService();
    if (subser != 0) {
        //ALOGE("subtitleShowSub pos:%d\n", pos);
        subser->subtitleShowSub(pos);
    }
    return;
}

void subtitleOption()
{
    const sp<SubtitleServerHidlClient>& subser = getSubtitleService();
    if (subser != 0) {
        subser->subtitleOption();
    }
    return;
}

int subtitleGetType()
{
    int ret = 0;
    const sp<SubtitleServerHidlClient>& subser = getSubtitleService();
    if (subser != 0) {
        ret = subser->subtitleGetType();
    }
    return ret;
}

char* subtitleGetTypeStr()
{
    char* ret;
    const sp<SubtitleServerHidlClient>& subser = getSubtitleService();
    if (subser != 0) {
        //@@ret = String8(subser->getTypeStr()).string();
    }
    return ret;
}

int subtitleGetTypeDetial()
{
    int ret = 0;
    const sp<SubtitleServerHidlClient>& subser = getSubtitleService();
    if (subser != 0) {
        ret = subser->subtitleGetTypeDetial();
    }
    return ret;
}

void subtitleSetTextColor(int color)
{
    const sp<SubtitleServerHidlClient>& subser = getSubtitleService();
    if (subser != 0) {
        subser->subtitleSetTextColor(color);
    }
    return;
}

void subtitleSetTextSize(int size)
{
    const sp<SubtitleServerHidlClient>& subser = getSubtitleService();
    if (subser != 0) {
        subser->subtitleSetTextSize(size);
    }
    return;
}

void subtitleSetGravity(int gravity)
{
    const sp<SubtitleServerHidlClient>& subser = getSubtitleService();
    if (subser != 0) {
        subser->subtitleSetGravity(gravity);
    }
    return;
}

void subtitleSetTextStyle(int style)
{
    const sp<SubtitleServerHidlClient>& subser = getSubtitleService();
    if (subser != 0) {
        subser->subtitleSetTextStyle(style);
    }
    return;
}

void subtitleSetPosHeight(int height)
{
    const sp<SubtitleServerHidlClient>& subser = getSubtitleService();
    if (subser != 0) {
        subser->subtitleSetPosHeight(height);
    }
    return;
}

void subtitleSetImgRatio(float ratioW, float ratioH, int maxW, int maxH)
{
    const sp<SubtitleServerHidlClient>& subser = getSubtitleService();
    if (subser != 0) {
        subser->subtitleSetImgRatio(ratioW, ratioH, maxW, maxH);
    }
    return;
}

void subtitleSetSurfaceViewParam(int x, int y, int w, int h)
{
    const sp<SubtitleServerHidlClient>& subser = getSubtitleService();
    ALOGE("subtitleSetSurfaceViewParam 00 x:%d y:%d w:%d h:%d\n",x,y,w,h);
    if (subser != 0) {
        //ALOGE("subtitleSetSurfaceViewParam 01\n");
        subser->subtitleSetSurfaceViewParam(x, y, w, h);
    }
    return;
}

void subtitleClear()
{
    const sp<SubtitleServerHidlClient>& subser = getSubtitleService();
    if (subser != 0) {
        subser->subtitleClear();
    }
    return;
}

void subtitleResetForSeek()
{
    ALOGE("subtitleResetForSeek");
    const sp<SubtitleServerHidlClient>& subser = getSubtitleService();
    if (subser != 0) {
        subser->subtitleResetForSeek();
    }
    return;
}

void subtitleHide()
{
    const sp<SubtitleServerHidlClient>& subser = getSubtitleService();
    if (subser != 0) {
        subser->subtitleHide();
    }
    return;
}

void subtitleDisplay()
{
    const sp<SubtitleServerHidlClient>& subser = getSubtitleService();
    if (subser != 0) {
        subser->subtitleDisplay();
    }
    return;
}

char* subtitleGetCurName()
{
    char ret[STR_LEN] = {0,};
    const sp<SubtitleServerHidlClient>& subser = getSubtitleService();
    if (subser != 0) {
        /*String16 value;
        value = subser->getCurName();
        memset(ret, 0, STR_LEN);
        strcpy(ret, String8(value).string());*/
    }
    return ret;
}

char* subtitleGetName(int idx)
{
    char ret[STR_LEN] = {0,};
    const sp<SubtitleServerHidlClient>& subser = getSubtitleService();
    if (subser != 0) {
        /*String16 value;
        value = subser->getName(idx);
        memset(ret, 0, STR_LEN);
        strcpy(ret, String8(value).string());*/
    }
    return ret;
}

const char* subtitleGetLanguage()
{
    ALOGE("subtitleGetLanguage");
    char ret[STR_LEN] = {0,};
    const sp<SubtitleServerHidlClient>& subser = getSubtitleService();
    if (subser != 0) {
        std::string value;
        value = subser->subtitleGetLanguage(0);
        //memset(ret, 0, STR_LEN);
        //strcpy(ret, subser->subtitleGetLanguage(0));
        if (value != "") {
            ALOGE("subtitleGetLanguage -languege:%s", value.c_str());
            strcpy(ret, value.c_str());
            return ret;
        }
    }
    return NULL;
}

void subtitleLoad(char* path)
{
    const sp<SubtitleServerHidlClient>& subser = getSubtitleService();
    if (subser != 0) {
        //subser->load(String16(path));
    }
    return;
}

void subtitleCreat()
{
    ALOGE("subtitleCreat");
    const sp<SubtitleServerHidlClient>& subser = getSubtitleService();
    if (subser != 0) {
        subser->subtitleCreat();
    }
    ALOGE("subtitleCreated");
    return;
}

void subtitleOpen(char* path, void *pthis, android::SubtitleServerHidlClient::SUB_Para_t * para)
{
    ALOGE("subtitleOpen");
    const sp<SubtitleServerHidlClient>& subser = getSubtitleService();
    if (subser != 0) {
        subser->subtitleOpen(path, getSubtitleCurrentPos, para);
    }
    return;
}

void subtitleSetSubPid(int pid)
{
    const sp<SubtitleServerHidlClient>& subser = getSubtitleService();
    if (subser != 0) {
        subser->subtitleSetSubPid(pid);
    }
    return;
}

void switchSubtitle(android::SubtitleServerHidlClient::SUB_Para_t * para)
{
    const sp<SubtitleServerHidlClient>& subser = getSubtitleService();
    if (subser != 0) {
        subser->switchSubtitle(para);
    }
    return;
}

int subtitleGetSubHeight()
{
    int ret = 0;
    const sp<SubtitleServerHidlClient>& subser = getSubtitleService();
    if (subser != 0) {
        ret = subser->subtitleGetSubHeight();
    }
    return ret;
}

int subtitleGetSubWidth()
{
    int ret = 0;
    const sp<SubtitleServerHidlClient>& subser = getSubtitleService();
    if (subser != 0) {
        ret = subser->subtitleGetSubWidth();
    }
    return ret;
}

void subtitleSetSubType(int type)
{
    const sp<SubtitleServerHidlClient>& subser = getSubtitleService();
    if (subser != 0) {
        subser->subtitleSetSubType(type);
    }
    return;
}

void registerSubtitleMiddleListener()
{
    ALOGD("[registerSubtitleMiddleListener]");
    const sp<SubtitleServerHidlClient>& subser = getSubtitleService();
    if (subser != 0) {
        spEventCB = new EventCallback();
        subser->setListener(spEventCB);
    }
    return;
}

void EventCallback::notify (const subtitle_parcel_t &parcel) {
    //ALOGD("eventcallback notify parcel.msgType = %d", parcel.msgType);
    if (parcel.msgType == EVENT_ONSUBTITLEDATA_CALLBACK) {
         ALOGD("subtitleMiddleClient notify parcel.msgType = %d, event:%d, id:%d", parcel.msgType, parcel.bodyInt[0], parcel.bodyInt[1]);
         if ((sub_p. sub_evt != NULL) && (parcel.bodyInt[0] == 0))
            sub_p. sub_evt(SUBTITLE_EVENT_NONE, parcel.bodyInt[0]);
         else if ((sub_p.sub_evt != NULL) && (parcel.bodyInt[0] == 1))
            sub_p. sub_evt(SUBTITLE_EVENT_DATA, parcel.bodyInt[1]);
    } else if (parcel.msgType == EVENT_ONSUBTITLEAVAILABLE_CALLBACK) {
         ALOGD("subtitleMiddleClient notify parcel.msgType = %d, available:%d", parcel.msgType, parcel.bodyInt[0]);
         if ((sub_p. available != NULL) && (parcel.bodyInt[0] == 0))
            sub_p. available(SUBTITLE_UNAVAIABLE, 0);
         else if ((sub_p.sub_evt != NULL) && (parcel.bodyInt[0] == 1))
            sub_p. available(SUBTITLE_AVAIABLE, 1);
    }
}

void subtitle_register_available(AM_SUBTITLELIS sub)
{
    ALOGD("subtitle_register start");
    sub_p. available= sub;
    ALOGD("subtitle_register end");
}

void subtitle_register_event(AM_SUBTITLEEVT evt)
{
    ALOGD("subtitle_register_event start");
    sub_p.sub_evt = evt;
    ALOGD("subtitle_register end");
}


