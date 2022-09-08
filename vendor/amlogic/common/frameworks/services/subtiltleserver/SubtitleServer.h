// FIXME: your file license if you have one

#pragma once

#include <map>
#include <vendor/amlogic/hardware/subtitleserver/1.0/ISubtitleServer.h>

#include <fmq/EventFlag.h>
#include <fmq/MessageQueue.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>
#include <utils/Mutex.h>
#include "SubtitleService.h"

#include "AndroidCallbackMessageQueue.h"
// register a callback, for send to remote process and showing.
using ::amlogic::AndroidCallbackHandler;
using ::amlogic::AndroidCallbackMessageQueue;

namespace vendor {
namespace amlogic {
namespace hardware {
namespace subtitleserver {
namespace V1_0 {
namespace implementation {

using ::android::hardware::hidl_array;
using ::android::hardware::hidl_handle;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

using ::android::hardware::MessageQueue;
using ::android::hardware::kSynchronizedReadWrite;
using ::android::hardware::EventFlag;

typedef MessageQueue<uint8_t, kSynchronizedReadWrite> DataMQ;


class SubtitleServer : public ISubtitleServer {
public:
    static SubtitleServer* Instance();

    SubtitleServer();
    // Methods from ISubtitleServer follow.
    Return<void> openConnection(openConnection_cb _hidl_cb) override;
    Return<Result> closeConnection(int32_t sId) override;
    Return<Result> open(int32_t sId, const hidl_handle& handle, int32_t ioType, OpenType openType) override;
    Return<Result> close(int32_t sId) override;
    Return<Result> resetForSeek(int32_t sId) override;
    Return<Result> updateVideoPos(int32_t sId, int32_t pos) override;


    Return<void> getTotalTracks(int32_t sId, getTotalTracks_cb _hidl_cb) override;
    Return<void> getType(int32_t sId, getType_cb _hidl_cb) override;
    Return<void> getLanguage(int32_t sId, getLanguage_cb _hidl_cb) override;

    Return<Result> setSubType(int32_t sId, int32_t type) override;
    Return<Result> setSubPid(int32_t sId, int32_t pid) override;
    Return<Result> setPageId(int32_t sId, int32_t pageId) override;
    Return<Result> setAncPageId(int32_t sId, int32_t ancPageId) override;
    Return<Result> setChannelId(int32_t sId, int32_t channelId) override;
    Return<Result> setClosedCaptionVfmt(int32_t sId, int32_t vfmt) override;

    Return<Result> ttControl(int32_t sId, int cmd, int pageNo, int subPageNo, int pageDir, int subPageDir) override;
    Return<Result> ttGoHome(int32_t sId) override;
    Return<Result> ttNextPage(int32_t sId, int32_t dir) override;
    Return<Result> ttNextSubPage(int32_t sId, int32_t dir) override;
    Return<Result> ttGotoPage(int32_t sId, int32_t pageNo, int32_t subPageNo) override;

    Return<Result> userDataOpen(int32_t sId) override;
    Return<Result> userDataClose(int32_t sId) override;


    Return<void> prepareWritingQueue(int32_t sId, int32_t size, prepareWritingQueue_cb _hidl_cb) override;

    Return<void> setCallback(const sp<ISubtitleCallback>& callback, ConnectType type) override;
    Return<void> setFallbackCallback(const sp<ISubtitleCallback>& callback, ConnectType type) override;
    Return<void> removeCallback(const sp<ISubtitleCallback>& callback);

    //============ for stand alone UI, proxy API =============
    Return<Result> show(int32_t sId) override;
    Return<Result> hide(int32_t sId) override;
    Return<Result> setTextColor(int32_t sId, int32_t color) override;
    Return<Result> setTextSize(int32_t sId, int32_t size) override;
    Return<Result> setGravity(int32_t sId, int32_t gravity) override;
    Return<Result> setTextStyle(int32_t sId, int32_t style) override;
    Return<Result> setPosHeight(int32_t sId, int32_t height) override;
    Return<Result> setImgRatio(int32_t sId, float ratioW, float ratioH, int32_t maxW, int32_t maxH) override;
    Return<void> getSubDemision(int32_t sId, getSubDemision_cb _hidl_cb) override;
    Return<Result> setSubDemision(int32_t sId, int32_t width, int32_t height) override;
    Return<Result> setSurfaceViewRect(int32_t sId, int32_t x, int32_t y, int32_t w, int32_t h) override;



    // Methods from ::android::hidl::base::V1_0::IBase follow.
    Return<void> debug(const hidl_handle& fd, const hidl_vec<hidl_string>& args) override;

private:
    void dump(int fd, const std::vector<std::string>& args);
    int mDumpMaps;

    std::shared_ptr<SubtitleService> getSubtitleServiceLocked(int sId);
    std::shared_ptr<SubtitleService> getSubtitleService(int sId);
    void handleServiceDeath(uint32_t cookie);

    // TODO: merge the callbacks?
    void sendSubtitleDisplayNotify(SubtitleHidlParcel &event);
    void sendSubtitleEventNotify(SubtitleHidlParcel &event);

    void sendUiEvent(SubtitleHidlParcel &event);

    mutable android::Mutex mLock;

    int mCurSessionId;
    std::map<uint32_t, std::shared_ptr<SubtitleService>> mServiceClients;

    /* FMQ for media send softdemuxed subtitle data */
    std::unique_ptr<DataMQ> mDataMQ;


    std::map<uint32_t,  sp<ISubtitleCallback>> mCallbackClients;
    sp<ISubtitleCallback> mFallbackCallback;


    class ClientMessageHandlerImpl : public AndroidCallbackHandler {
    public:
        ClientMessageHandlerImpl(sp<SubtitleServer> ssh) : mSubtitleServer(ssh) {}
        bool onSubtitleDisplayNotify(SubtitleHidlParcel &event);
        bool onSubtitleEventNotify(SubtitleHidlParcel &event);
    private:
        sp<SubtitleServer> mSubtitleServer;
    };
    sp<AndroidCallbackMessageQueue> mMessagQueue;

    class DeathRecipient : public android::hardware::hidl_death_recipient  {
    public:
        DeathRecipient(sp<SubtitleServer> ssh) : mSubtitleServer(ssh) {}

        // hidl_death_recipient interface
        virtual void serviceDied(uint64_t cookie,
            const ::android::wp<::android::hidl::base::V1_0::IBase>& who) override;

    private:
        sp<SubtitleServer> mSubtitleServer;
    };

    sp<DeathRecipient> mDeathRecipient;

    static SubtitleServer* sInstance;

    bool mFallbackPlayStarted;

    // avoid CTC or other native impl called in middleware which is used by VideoPlayer
    // then VideoPlayer and middleware both call subtitle, but open related api we want
    // middleware's, other configureation related can from VideoPlayer.
    bool mOpenCalled;
    OpenType mLastOpenType;
    int64_t mLastOpenTime;
};

// FIXME: most likely delete, this is only for passthrough implementations
// extern "C" ISubtitleServer* HIDL_FETCH_ISubtitleServer(const char* name);

}  // namespace implementation
}  // namespace V1_0
}  // namespace subtitleserver
}  // namespace hardware
}  // namespace amlogic
}  // namespace vendor
