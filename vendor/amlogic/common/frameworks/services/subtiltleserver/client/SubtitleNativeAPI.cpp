#define LOG_NDEBUG 0
#define LOG_TAG "SubtitleNativeAPI"
#include <utils/Log.h>
#include <utils/CallStack.h>
#include <mutex>
#include <utils/RefBase.h>

#include <fmq/EventFlag.h>
#include <fmq/MessageQueue.h>
#include <vendor/amlogic/hardware/subtitleserver/1.0/ISubtitleServer.h>


#include "SubtitleNativeAPI.h"
#include "SubtitleServerClient.h"

#define DEBUG_CALL 1
#define DEBUG_SESSION 1

using ::android::CallStack;
using ::android::sp;

static AmlSubDataType __mapServerType2ApiType(int type) {
    switch (type) {
    case TYPE_SUBTITLE_DVB:
    case TYPE_SUBTITLE_PGS:
        return SUB_DATA_TYPE_POSITON_BITMAP;

    case TYPE_SUBTITLE_EXTERNAL:
    case TYPE_SUBTITLE_MKV_STR:
    case TYPE_SUBTITLE_SSA:
        return SUB_DATA_TYPE_STRING;

    case TYPE_SUBTITLE_CLOSED_CATPTION:
        return SUB_DATA_TYPE_CC_JSON;

    }

    //default:
    return SUB_DATA_TYPE_BITMAP;
}

static int __mapApiType2SubtitleType(int type) {
    if (DEBUG_CALL) ALOGD("call>> %s [stype:%d]", __func__, type);
    switch (type) {
    case TYPE_SUBTITLE_DVB:
        return DTV_SUB_DTVKIT_DVB;

    case TYPE_SUBTITLE_DVB_TELETEXT:
        return DTV_SUB_DTVKIT_TELETEXT;
    case TYPE_SUBTITLE_SCTE27:
        return DTV_SUB_DTVKIT_SCTE27;
    }
    //default:
    return type;
}



class MyAdaptorListener : public amlogic::SubtitleListener {
public:
    MyAdaptorListener() {
        mDataCb = nullptr;
        mChannelUpdateCb = nullptr;
        mSubtitleAvailCb = nullptr;
        mAfdEventCb = nullptr;
        mDimesionCb  = nullptr;
        mLanguageCb  = nullptr;
        mSubtitleInfoCb  = nullptr;
    }
    virtual ~MyAdaptorListener() {
        mDataCb = nullptr;
        mChannelUpdateCb = nullptr;
        mSubtitleAvailCb = nullptr;
        mAfdEventCb = nullptr;
        mDimesionCb  = nullptr;
        mLanguageCb  = nullptr;
        mSubtitleInfoCb  = nullptr;
    }

    virtual void onSubtitleEvent(const char *data, int size, int parserType,
                int x, int y, int width, int height,
                int videoWidth, int videoHeight, int showing) {
        if (mDataCb != nullptr) {
            mDataCb(data, size,
                __mapServerType2ApiType((AmlSubtitletype)parserType),
                 x, y, width, height, videoWidth, videoHeight, showing);
        }
    }

    virtual void onSubtitleDataEvent(int event, int id) {
        if (mChannelUpdateCb != nullptr) mChannelUpdateCb(event, id);
    }

    virtual void onSubtitleAvail(int avail) {
        if (mSubtitleAvailCb != nullptr) mSubtitleAvailCb(avail);
    }

    virtual void onSubtitleAfdEvent(int event) {
        if (mAfdEventCb != nullptr) mAfdEventCb(event);
    }

    virtual void onSubtitleDimension(int width, int height) {
        if (mDimesionCb != nullptr) mDimesionCb(width, height);
    }

    virtual void onSubtitleLanguage(char *lang) {
        if (mLanguageCb != nullptr) mLanguageCb(lang);
    }

    virtual void onSubtitleInfo(int what, int extra) {
        if (mSubtitleInfoCb != nullptr) mSubtitleInfoCb(what, extra);
    }

    // middleware api do not need this
    virtual void onMixVideoEvent(int val) {}
    virtual void onSubtitleUIEvent(int uiCmd, const std::vector<int> &params) {}


    // sometime, server may crash, we need clean up in server side.
    virtual void onServerDied() {}

    void setupDataCb(AmlSubtitleDataCb cb) { mDataCb = cb; }
    void setupChannelUpdateCb(AmlChannelUpdateCb cb) { mChannelUpdateCb = cb; }
    void setupSubtitleAvailCb(AmlSubtitleAvailCb cb) { mSubtitleAvailCb = cb; }
    void setupAfdEventCb(AmlAfdEventCb cb) { mAfdEventCb = cb; }
    void setupDimesionCb(AmlSubtitleDimensionCb cb) { mDimesionCb = cb; }
    void setupLanguageCb(AmlSubtitleLanguageCb cb) { mLanguageCb = cb; }
    void setupSubtitleInfoCb(AmlSubtitleInfoCb cb) { mSubtitleInfoCb = cb; }


private:
    AmlSubtitleDataCb mDataCb;
    AmlChannelUpdateCb mChannelUpdateCb;
    AmlSubtitleAvailCb mSubtitleAvailCb;
    AmlAfdEventCb mAfdEventCb;
    AmlSubtitleDimensionCb mDimesionCb;
    AmlSubtitleLanguageCb mLanguageCb;
    AmlSubtitleInfoCb mSubtitleInfoCb;
};

class SubtitleContext : public android::RefBase {
public:
    SubtitleContext() {
        if (DEBUG_CALL) ALOGD("call>> %s [%p]", __func__, this);
    }

    ~SubtitleContext() {
        if (DEBUG_CALL) ALOGD("call>> %s [%p]", __func__, this);
    }

    amlogic::SubtitleServerClient *mClient;
    sp<MyAdaptorListener> mAdaptorListener;
};


std::vector<sp<SubtitleContext>> gAvailContexMap;
android::Mutex gContextMapLock;

static bool __pushContext(sp<SubtitleContext> ctx) {
    android::Mutex::Autolock autoLock(gContextMapLock);
    auto it = gAvailContexMap.begin();
    it = gAvailContexMap.insert (it , ctx);

    if (DEBUG_SESSION) ALOGD("%s>> ctx:%p MapSize=%d", __func__, ctx.get(), gAvailContexMap.size());

    return *it != nullptr;
}

static bool __checkInContextMap(SubtitleContext *ctx) {
    android::Mutex::Autolock autoLock(gContextMapLock);
    for (auto it=gAvailContexMap.begin(); it<gAvailContexMap.end(); it++) {
        if (DEBUG_SESSION) ALOGD("%s>> search ctx:%p current:%p MapSize=%d",
                __func__, ctx, (*it).get(), gAvailContexMap.size());

        if (ctx == (*it).get()) {
            return true;
        }
    }
    ALOGE("Error Invalid context handle!");
    return false;
}

static bool __eraseFromContext(SubtitleContext *ctx) {
    android::Mutex::Autolock autoLock(gContextMapLock);
    for (auto it=gAvailContexMap.begin(); it<gAvailContexMap.end(); it++) {

        if (DEBUG_SESSION) ALOGD("%s>> search ctx:%p current:%p MapSize=%d",
                __func__, ctx, (*it).get(), gAvailContexMap.size());

        if (ctx == (*it).get()) {
            gAvailContexMap.erase(it);
            return true;
        }
    }
    ALOGE("Error Invalid context handle!");
    return false;
}




AmlSubtitleHnd amlsub_Create() {
    if (DEBUG_CALL) ALOGD("call>> %s", __func__);
    sp<SubtitleContext> ctx = new SubtitleContext();
    sp<MyAdaptorListener> listner = new MyAdaptorListener();

    ctx->mClient = new amlogic::SubtitleServerClient(false, listner, OpenType::TYPE_MIDDLEWARE);
    ctx->mAdaptorListener = listner;

    __pushContext(ctx);

    return ctx.get();
}

AmlSubtitleStatus amlsub_Destroy(AmlSubtitleHnd handle) {
    SubtitleContext *ctx = (SubtitleContext *)handle;

    if (DEBUG_CALL) ALOGD("call>> %s handle[%p]", __func__, handle);

    if (ctx == nullptr) {
        return SUB_STAT_INV;
    }

    if (!__checkInContextMap(ctx)) {
        ALOGE("Error! Bad handle! mis-using API?");
        return SUB_STAT_INV;
    }

    //ctx->mClient = nullptr;
    ctx->mAdaptorListener = nullptr;

    __eraseFromContext(ctx);
    delete ctx->mClient;

    return SUB_STAT_OK;
}


////////////////////////////////////////////////////////////
///////////////////// generic control //////////////////////
////////////////////////////////////////////////////////////
/**
 * open, start to play subtitle.
 * Param:
 *   handle: current subtitle handle.
 *   path: the subtitle external file path.
 */
AmlSubtitleStatus amlsub_Open(AmlSubtitleHnd handle, AmlSubtitleParam *param) {
    if (DEBUG_CALL) ALOGD("call>> %s handle[%p]", __func__, handle);
    SubtitleContext *ctx = (SubtitleContext *)handle;
    if (ctx == nullptr || ctx->mClient == nullptr) {
        ALOGE("Error! invalid handle, uninitialized?");
        return SUB_STAT_INV;
    }

    if (!__checkInContextMap(ctx)) {
        ALOGE("Error! Bad handle! mis-using API?");
        return SUB_STAT_INV;
    }

    if (param == nullptr) {
        ALOGE("Error! invalid input param, param=%p", param);
        return SUB_STAT_INV;
    }
    ctx->mClient->userDataOpen();

    // Here, we support all the parametes from client to server
    // server will filter the parameter, select to use according from the type.
    ctx->mClient->setSubType(__mapApiType2SubtitleType(param->subtitleType));
    ctx->mClient->setSubPid(param->pid);
    ctx->mClient->setClosedCaptionId(param->channelId);
    ctx->mClient->setClosedCaptionVfmt(param->videoFormat);
    ctx->mClient->setCompositionPageId(param->ancillaryPageId);
    ctx->mClient->setAncillaryPageId(param->compositionPageId);

    // TODO: CTC always use amstream. later we may all change to hwdemux
    // From middleware, the extSubPath should alway null.
    bool r = ctx->mClient->open(param->extSubPath, param->ioSource);

    return r ? SUB_STAT_OK : SUB_STAT_FAIL;
}
AmlSubtitleStatus amlsub_Close(AmlSubtitleHnd handle) {
    if (DEBUG_CALL) ALOGD("call>> %s handle[%p]", __func__, handle);
    SubtitleContext *ctx = (SubtitleContext *)handle;
    if (ctx == nullptr || ctx->mClient == nullptr) {
        return SUB_STAT_INV;
    }
    if (!__checkInContextMap(ctx)) {
        ALOGE("Error! Bad handle! mis-using API?");
        return SUB_STAT_INV;
    }
    ctx->mClient->userDataClose();
    return ctx->mClient->close() ? SUB_STAT_OK : SUB_STAT_FAIL;
}

/**
 * reset current play, most used for seek.
 */
AmlSubtitleStatus amlsub_Reset(AmlSubtitleHnd handle) {
    if (DEBUG_CALL) ALOGD("call>> %s handle[%p]", __func__, handle);
    SubtitleContext *ctx = (SubtitleContext *)handle;
    if (ctx == nullptr || ctx->mClient == nullptr) {
        return SUB_STAT_INV;
    }
    if (!__checkInContextMap(ctx)) {
        ALOGE("Error! Bad handle! mis-using API?");
        return SUB_STAT_INV;
    }
    return ctx->mClient->resetForSeek() ? SUB_STAT_OK : SUB_STAT_FAIL;
}

////////////////////////////////////////////////////////////
//////////////// DTV operation/param related ///////////////
////////////////////////////////////////////////////////////

AmlSubtitleStatus amlsub_SetParameter(AmlSubtitleHnd handle, AmlSubtitleParamCmd cmd, void *value, int paramSize) {
    if (DEBUG_CALL) ALOGD("call>> %s handle[%p]", __func__, handle);
    SubtitleContext *ctx = (SubtitleContext *)handle;
    if (ctx == nullptr) {
        return SUB_STAT_INV;
    }
    if (!__checkInContextMap(ctx)) {
        ALOGE("Error! Bad handle! mis-using API?");
        return SUB_STAT_INV;
    }
    return SUB_STAT_OK;
}

int amlsub_GetParameter(AmlSubtitleHnd handle, AmlSubtitleParamCmd cmd, void *value) {
    if (DEBUG_CALL) ALOGD("call>> %s handle[%p]", __func__, handle);
    return 0;
}

AmlSubtitleStatus amlsub_TeletextControl(AmlSubtitleHnd handle, AmlTeletextCtrlParam *param) {
    if (DEBUG_CALL) ALOGD("call>> %s handle[%p]", __func__, handle);
    SubtitleContext *ctx = (SubtitleContext *)handle;
    if (ctx == nullptr || ctx->mClient == nullptr || param == nullptr) {
        return SUB_STAT_INV;
    }
    if (!__checkInContextMap(ctx)) {
        ALOGE("Error! Bad handle! mis-using API?");
        return SUB_STAT_INV;
    }

    ctx->mClient->ttControl(param->event, param->magazine, param->page, 1, 1);
    return SUB_STAT_OK;
}

AmlSubtitleStatus amlsub_SelectCcChannel(AmlSubtitleHnd handle, int ch) {
    if (DEBUG_CALL) ALOGD("call>> %s handle[%p] ch:%d", __func__, handle, ch);
    SubtitleContext *ctx = (SubtitleContext *)handle;
    if (ctx == nullptr || ctx->mClient == nullptr || ch < 0) {
        return SUB_STAT_INV;
    }
    if (!__checkInContextMap(ctx)) {
        ALOGE("Error! Bad handle! mis-using API?");
        return SUB_STAT_INV;
    }

    ctx->mClient->selectCcChannel(ch);
    return SUB_STAT_OK;
}



////////////////////////////////////////////////////////////
////// Regist callbacks for subtitle Event and data ////////
////////////////////////////////////////////////////////////
AmlSubtitleStatus amlsub_RegistOnDataCB(AmlSubtitleHnd handle, AmlSubtitleDataCb listener) {
    if (DEBUG_CALL) ALOGD("call>> %s handle[%p]", __func__, handle);
    SubtitleContext *ctx = (SubtitleContext *)handle;
    if (ctx == nullptr) {
        return SUB_STAT_INV;
    }
    if (!__checkInContextMap(ctx)) {
        ALOGE("Error! Bad handle! mis-using API?");
        return SUB_STAT_INV;
    }

    ctx->mAdaptorListener->setupDataCb(listener);

    return SUB_STAT_OK;
}

AmlSubtitleStatus amlsub_RegistOnChannelUpdateCb(AmlSubtitleHnd handle, AmlChannelUpdateCb listener) {
    if (DEBUG_CALL) ALOGD("call>> %s handle[%p]", __func__, handle);
    SubtitleContext *ctx = (SubtitleContext *)handle;
    if (ctx == nullptr) {
        return SUB_STAT_INV;
    }
    if (!__checkInContextMap(ctx)) {
        ALOGE("Error! Bad handle! mis-using API?");
        return SUB_STAT_INV;
    }
    ctx->mAdaptorListener->setupChannelUpdateCb(listener);
    return SUB_STAT_OK;
}

AmlSubtitleStatus amlsub_RegistOnSubtitleAvailCb(AmlSubtitleHnd handle, AmlSubtitleAvailCb listener) {
    if (DEBUG_CALL) ALOGD("call>> %s handle[%p]", __func__, handle);
    SubtitleContext *ctx = (SubtitleContext *)handle;
    if (ctx == nullptr) {
        return SUB_STAT_INV;
    }
    if (!__checkInContextMap(ctx)) {
        ALOGE("Error! Bad handle! mis-using API?");
        return SUB_STAT_INV;
    }
    ctx->mAdaptorListener->setupSubtitleAvailCb(listener);
    return SUB_STAT_OK;
}

AmlSubtitleStatus amlsub_RegistAfdEventCB(AmlSubtitleHnd handle, AmlAfdEventCb listener) {
    if (DEBUG_CALL) ALOGD("call>> %s handle[%p]", __func__, handle);
    SubtitleContext *ctx = (SubtitleContext *)handle;
    if (ctx == nullptr) {
        return SUB_STAT_INV;
    }
    if (!__checkInContextMap(ctx)) {
        ALOGE("Error! Bad handle! mis-using API?");
        return SUB_STAT_INV;
    }
    ctx->mAdaptorListener->setupAfdEventCb(listener);
    return SUB_STAT_OK;
}

AmlSubtitleStatus amlsub_RegistGetDimesionCb(AmlSubtitleHnd handle, AmlSubtitleDimensionCb listener) {
    if (DEBUG_CALL) ALOGD("call>> %s handle[%p]", __func__, handle);
    SubtitleContext *ctx = (SubtitleContext *)handle;
    if (ctx == nullptr) {
        return SUB_STAT_INV;
    }
    if (!__checkInContextMap(ctx)) {
        ALOGE("Error! Bad handle! mis-using API?");
        return SUB_STAT_INV;
    }
    ctx->mAdaptorListener->setupDimesionCb(listener);
    return SUB_STAT_OK;
}

AmlSubtitleStatus amlsub_RegistOnSubtitleLanguageCb(AmlSubtitleHnd handle, AmlSubtitleLanguageCb listener) {
    if (DEBUG_CALL) ALOGD("call>> %s handle[%p]", __func__, handle);
    SubtitleContext *ctx = (SubtitleContext *)handle;
    if (ctx == nullptr) {
        return SUB_STAT_INV;
    }
    if (!__checkInContextMap(ctx)) {
        ALOGE("Error! Bad handle! mis-using API?");
        return SUB_STAT_INV;
    }
    ctx->mAdaptorListener->setupLanguageCb(listener);
    return SUB_STAT_OK;
}

AmlSubtitleStatus amlsub_RegistOnSubtitleInfoCB(AmlSubtitleHnd handle, AmlSubtitleInfoCb listener) {
    if (DEBUG_CALL) ALOGD("call>> %s handle[%p]", __func__, handle);
    SubtitleContext *ctx = (SubtitleContext *)handle;
    if (ctx == nullptr) {
        return SUB_STAT_INV;
    }
    if (!__checkInContextMap(ctx)) {
        ALOGE("Error! Bad handle! mis-using API?");
        return SUB_STAT_INV;
    }

    ctx->mAdaptorListener->setupSubtitleInfoCb(listener);

    return SUB_STAT_OK;
}


AmlSubtitleStatus amlsub_UiShow(AmlSubtitleHnd handle) {
    if (DEBUG_CALL) ALOGD("call>> %s handle[%p]", __func__, handle);
    SubtitleContext *ctx = (SubtitleContext *)handle;
    if (ctx == nullptr || ctx->mClient == nullptr) {
        return SUB_STAT_INV;
    }
    if (!__checkInContextMap(ctx)) {
        ALOGE("Error! Bad handle! mis-using API?");
        return SUB_STAT_INV;
    }

    bool r  = ctx->mClient->uiShow();
    return r ? SUB_STAT_OK : SUB_STAT_FAIL;
}

AmlSubtitleStatus amlsub_UiHide(AmlSubtitleHnd handle) {
    if (DEBUG_CALL) ALOGD("call>> %s handle[%p]", __func__, handle);
    SubtitleContext *ctx = (SubtitleContext *)handle;
    if (ctx == nullptr || ctx->mClient == nullptr) {
        return SUB_STAT_INV;
    }
    if (!__checkInContextMap(ctx)) {
        ALOGE("Error! Bad handle! mis-using API?");
        return SUB_STAT_INV;
    }

    bool r  = ctx->mClient->uiHide();
    return r ? SUB_STAT_OK : SUB_STAT_FAIL;
}

/* Only available for text subtitle */
AmlSubtitleStatus amlsub_UiSetTextColor(AmlSubtitleHnd handle, int color) {
    if (DEBUG_CALL) ALOGD("call>> %s handle[%p]", __func__, handle);
    SubtitleContext *ctx = (SubtitleContext *)handle;
    if (ctx == nullptr || ctx->mClient == nullptr) {
        return SUB_STAT_INV;
    }
    if (!__checkInContextMap(ctx)) {
        ALOGE("Error! Bad handle! mis-using API?");
        return SUB_STAT_INV;
    }

    bool r  = ctx->mClient->uiSetTextColor(color);
    return r ? SUB_STAT_OK : SUB_STAT_FAIL;
}
AmlSubtitleStatus amlsub_UiSetTextSize(AmlSubtitleHnd handle, int sp) {
    if (DEBUG_CALL) ALOGD("call>> %s handle[%p]", __func__, handle);
    SubtitleContext *ctx = (SubtitleContext *)handle;
    if (ctx == nullptr || ctx->mClient == nullptr) {
        return SUB_STAT_INV;
    }
    if (!__checkInContextMap(ctx)) {
        ALOGE("Error! Bad handle! mis-using API?");
        return SUB_STAT_INV;
    }

    bool r  = ctx->mClient->uiSetTextSize(sp);
    return r ? SUB_STAT_OK : SUB_STAT_FAIL;
}
AmlSubtitleStatus amlsub_UiSetGravity(AmlSubtitleHnd handle, int gravity) {
    if (DEBUG_CALL) ALOGD("call>> %s handle[%p]", __func__, handle);
    SubtitleContext *ctx = (SubtitleContext *)handle;
    if (ctx == nullptr || ctx->mClient == nullptr) {
        return SUB_STAT_INV;
    }
    if (!__checkInContextMap(ctx)) {
        ALOGE("Error! Bad handle! mis-using API?");
        return SUB_STAT_INV;
    }
    bool r  = ctx->mClient->uiSetGravity(gravity);
    return r ? SUB_STAT_OK : SUB_STAT_FAIL;
}

AmlSubtitleStatus amlsub_UiSetPosHeight(AmlSubtitleHnd handle, int yOffset) {
    if (DEBUG_CALL) ALOGD("call>> %s handle[%p]", __func__, handle);
    SubtitleContext *ctx = (SubtitleContext *)handle;
    if (ctx == nullptr || ctx->mClient == nullptr) {
        return SUB_STAT_INV;
    }
    if (!__checkInContextMap(ctx)) {
        ALOGE("Error! Bad handle! mis-using API?");
        return SUB_STAT_INV;
    }

    bool r  = ctx->mClient->uiSetYOffset(yOffset);
    return r ? SUB_STAT_OK : SUB_STAT_FAIL;
}

AmlSubtitleStatus amlsub_UiSetImgRatio(AmlSubtitleHnd handle, float ratioW, float ratioH, int maxW, int maxH) {
    if (DEBUG_CALL) ALOGD("call>> %s handle[%p]", __func__, handle);
    SubtitleContext *ctx = (SubtitleContext *)handle;
    if (ctx == nullptr || ctx->mClient == nullptr) {
        return SUB_STAT_INV;
    }
    if (!__checkInContextMap(ctx)) {
        ALOGE("Error! Bad handle! mis-using API?");
        return SUB_STAT_INV;
    }

    bool r  = ctx->mClient->uiSetImageRatio(ratioW, ratioH, maxW, maxH);
    return r ? SUB_STAT_OK : SUB_STAT_FAIL;
}

AmlSubtitleStatus amlsub_UiSetSurfaceViewRect(AmlSubtitleHnd handle, int x, int y, int width, int height) {
    if (DEBUG_CALL) ALOGD("call>> %s handle[%p]", __func__, handle);
    SubtitleContext *ctx = (SubtitleContext *)handle;
    if (ctx == nullptr || ctx->mClient == nullptr) {
        return SUB_STAT_INV;
    }
    if (!__checkInContextMap(ctx)) {
        ALOGE("Error! Bad handle! mis-using API?");
        return SUB_STAT_INV;
    }

    bool r  = ctx->mClient->uiSetSurfaceViewRect(x, y, width, height);
    return r ? SUB_STAT_OK : SUB_STAT_FAIL;
}


