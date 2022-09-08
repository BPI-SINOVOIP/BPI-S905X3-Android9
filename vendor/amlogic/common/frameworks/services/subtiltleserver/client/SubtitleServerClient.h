#pragma once

#include <type_traits>  // for common_type.
#include <utils/RefBase.h>
#include <utils/Mutex.h>

#include <utils/Atomic.h>
#include <utils/Log.h>
#include <utils/String8.h>

#include <hidlmemory/mapping.h>
#include <android/hidl/memory/1.0/IMemory.h>
#include <vendor/amlogic/hardware/subtitleserver/1.0/ISubtitleServer.h>
#include <vendor/amlogic/hardware/subtitleserver/1.0/ISubtitleCallback.h>
using android::hardware::hidl_vec;
using android::sp;
using android::wp;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_handle;
using ::android::hardware::hidl_memory;

using ::android::hardware::mapMemory;
using ::android::hidl::memory::V1_0::IMemory;

using ::vendor::amlogic::hardware::subtitleserver::V1_0::ISubtitleCallback;
using ::vendor::amlogic::hardware::subtitleserver::V1_0::ISubtitleServer;
using ::vendor::amlogic::hardware::subtitleserver::V1_0::Result;
using ::vendor::amlogic::hardware::subtitleserver::V1_0::ConnectType;
using ::vendor::amlogic::hardware::subtitleserver::V1_0::OpenType;
using ::vendor::amlogic::hardware::subtitleserver::V1_0::FallthroughUiCmd;
using ::vendor::amlogic::hardware::subtitleserver::V1_0::SubtitleHidlParcel;


// must keep sync with SubtitleServer.
typedef enum {
    DTV_SUB_INVALID         = -1,
    DTV_SUB_CC              = 2,
    DTV_SUB_SCTE27          = 3,
    DTV_SUB_DVB             = 4,
    DTV_SUB_DTVKIT_DVB      = 5,
    DTV_SUB_DTVKIT_TELETEXT = 6,
    DTV_SUB_DTVKIT_SCTE27   = 7,
} DtvSubtitleType;


namespace amlogic {

class SubtitleListener : public android::RefBase {
public:
    virtual ~SubtitleListener() {}

    virtual void onSubtitleEvent(const char *data, int size, int parserType,
                int x, int y, int width, int height,
                int videoWidth, int videoHeight, int cmd) = 0;

    virtual void onSubtitleDataEvent(int event, int id) = 0;

    virtual void onSubtitleAvail(int avail) = 0;
    virtual void onSubtitleAfdEvent(int avail) = 0;
    virtual void onSubtitleDimension(int width, int height) = 0;
    virtual void onMixVideoEvent(int val) = 0;
    virtual void onSubtitleLanguage(char *lang) = 0;
    virtual void onSubtitleInfo(int what, int extra) = 0;

    // Middleware API do not need, this transfer UI command to fallback displayer
    virtual void onSubtitleUIEvent(int uiCmd, const std::vector<int> &params) = 0;



    // sometime, server may crash, we need clean up in server side.
    virtual void onServerDied() = 0;
};


class SubtitleServerClient : public android::RefBase {

public:
    SubtitleServerClient() = delete;
    SubtitleServerClient(bool isFallback, sp<SubtitleListener> listener, OpenType openType);
    ~SubtitleServerClient();

    bool open(const char *path, int ioType);
    bool close();

    /* for external subtitle update PTS */
    bool updateVideoPos(int pos);

    int totalTracks();

    bool setSubType(int type);
    int getSubType();
    bool resetForSeek();

    // most, for scte.
    bool setSubPid(int pid);

    // for select CC index
    bool selectCcChannel(int idx);
    bool setClosedCaptionId(int id);
    bool setClosedCaptionVfmt(int fmt);
    bool setCompositionPageId(int pageId);
    bool setAncillaryPageId(int ancPageId);


    bool ttControl(int cmd, int pageNo, int subPageNo, int pageDir, int subPageDir);


    bool userDataOpen();
    bool userDataClose();

    // ui related.
    // Below api only used for control standalone UI.
    // Thes UI is not recomendated, only for some native app/middleware
    // that cannot Android (Java) UI hierarchy.
    bool uiShow();
    bool uiHide();
    bool uiSetTextColor(int color);
    bool uiSetTextSize(int size);
    bool uiSetGravity(int gravity);
    bool uiSetTextStyle(int style);
    bool uiSetYOffset(int yOffset);
    bool uiSetImageRatio(float ratioW, float ratioH, int32_t maxW, int32_t maxH);
    bool uiGetSubDemision(int *pWidth, int *pHeight);
    bool uiSetSurfaceViewRect(int x, int y, int width, int height);

private:
    struct SubtitleDeathRecipient : public android::hardware::hidl_death_recipient {
    public:
        SubtitleDeathRecipient(wp<SubtitleServerClient> sc) : mOwner(sc) {}
        virtual void serviceDied(uint64_t cookie,
                const ::android::wp<::android::hidl::base::V1_0::IBase>& who) override;
    private:
        wp<SubtitleServerClient> mOwner;
    };

    class SubtitleCallback : public ISubtitleCallback {
    public:
        SubtitleCallback(sp<SubtitleListener> sl) : mListener(sl) {}
        ~SubtitleCallback() {mListener = nullptr;}
        virtual Return<void> notifyDataCallback(const SubtitleHidlParcel &parcel) override;
        virtual Return<void> uiCommandCallback(const SubtitleHidlParcel &parcel)  override ;
        virtual Return<void> eventNotify(const SubtitleHidlParcel& parcel) override;

    private:
        sp<SubtitleListener> mListener;
    };


    template<typename T>
    void checkRemoteResultLocked(Return<T> &r);

    void initRemoteLocked();

    sp<ISubtitleServer> mRemote;
    sp<SubtitleListener> mListener;

    sp<SubtitleCallback> mCallback;
    sp<SubtitleDeathRecipient> mDeathRecipient;

    int mSessionId;
    mutable android::Mutex mLock;

    // standalone fallback impl
    bool mIsFallback;

    // As hidl. check if from middleware or APP.
    OpenType mOpenType;
};

}
