// FIXME: your file license if you have one
#define LOG_NDEBUG  0
#define LOG_TAG "SubtitleServer"
#include <fmq/EventFlag.h>
#include "SubtitleServer.h"
#include "MemoryLeakTrackUtil.h"
#include <utils/CallStack.h>

using android::CallStack;
namespace vendor {
namespace amlogic {
namespace hardware {
namespace subtitleserver {
namespace V1_0 {
namespace implementation {

// FMQ polling data
class FmqReaderImpl : public FmqReader {
public:
    FmqReaderImpl(DataMQ* dataMQ) : mDataMQ(dataMQ) {
        ALOGD("%s", __func__);
    }
    virtual ~FmqReaderImpl() {
        ALOGD("%s", __func__);
    }

    virtual bool pollEvent() {return true;}
    virtual size_t availableSize() {
        if (mDataMQ == nullptr) return 0;
        return mDataMQ->availableToRead();
    }

    virtual size_t read(uint8_t *buffer, size_t size) {
            size_t availToRead = mDataMQ->availableToRead();
            if (availToRead > size) {
                availToRead = size;
            }

            if (availToRead > 0) {
                if (mDataMQ->readBlocking(buffer, availToRead, 50*1000*1000LL)) {
                    return availToRead;
                } else {
                    ALOGE("Error! cannot read data! require:%d avail:%d", size, availToRead);
                    return -1;
                }
            }
        return 0;
    }

private:
    DataMQ* mDataMQ;
};


SubtitleServer *SubtitleServer::sInstance = nullptr;
SubtitleServer* SubtitleServer::Instance() {
    if (!sInstance) sInstance = new SubtitleServer();
    return sInstance;
}

const static int FIRST_SESSION_ID = 0x1F230000;
const static int LAST_SESSION_ID  = 0x7FFFFFFF;

#define SUPPORT_MULTI 0

SubtitleServer::SubtitleServer() {
    mDeathRecipient =new DeathRecipient(this);
    mFallbackPlayStarted = false;
    mFallbackCallback = nullptr;

    // Here, we create this messageQueue, it automatically start a thread to pump
    // decoded data/event from parser, and call CallbackHandler to send to remote side.
    mMessagQueue = new AndroidCallbackMessageQueue(new ClientMessageHandlerImpl(this));

    mCurSessionId = FIRST_SESSION_ID;
    mOpenCalled = false;
    mLastOpenType = OpenType::TYPE_APPSDK;
    mLastOpenTime = -1;

}


std::shared_ptr<SubtitleService> SubtitleServer::getSubtitleServiceLocked(int sId) {
#if SUPPORT_MULTI
    auto search = mServiceClients.find((sId));
    if (search != mServiceClients.end()) {
        return search->second;
    } else {
        ALOGE("Error! cannot found service by id:%x", sId);
    }
#else
    (void) sId;

    if (mServiceClients.size() > 0) {
        return mServiceClients[0];
    }
#endif
    return nullptr;
}
std::shared_ptr<SubtitleService> SubtitleServer::getSubtitleService(int sId) {
    android::AutoMutex _l(mLock);
    return getSubtitleServiceLocked(sId);
}

// Methods from ISubtitleServer follow.
Return<void> SubtitleServer::openConnection(openConnection_cb _hidl_cb) {
    ALOGV("%s ", __func__);
    int sessionId = -1;
    {
        android::AutoMutex _l(mLock);
        sessionId = mCurSessionId++;
        if (sessionId == LAST_SESSION_ID) {
            sessionId = mCurSessionId = LAST_SESSION_ID;
        }
        std::shared_ptr<SubtitleService> ss = std::shared_ptr<SubtitleService>(new SubtitleService());
        //auto p = std::make_pair(sessionId, ss);
        //mClients.insert(p);
        mServiceClients[0] = ss;
        ALOGD("openConnection: size:%d", mServiceClients.size());
        for (int i=0; i<mServiceClients.size(); i++) {
            ALOGD("client %d-%d: %p", i, mServiceClients.size(), mServiceClients[i].get());
        }

    }
    _hidl_cb(Result::OK, sessionId);
    return Void();
}

Return<Result> SubtitleServer::closeConnection(int32_t sId) {
    ALOGV("%s sId=%d", __func__, sId);

    //TODO: too simple here! need more condition.
    hide(sId); // for standalone play, need hide!
    ALOGD("need hide here");


    android::AutoMutex _l(mLock);

    int clientSize = mServiceClients.size();
    ALOGD("clientSize=%d", clientSize);
#if SUPPORT_MULTI
    for (auto it = mServiceClients.begin(); it != mServiceClients.end();) {
        if (it->first == sId) {
            mServiceClients.erase(it);
            return Result::OK;
        }
        it++;
    }
#else
    mServiceClients[0] = nullptr;
#endif
    return Result::FAIL;
}

Return<Result> SubtitleServer::open(int32_t sId, const hidl_handle& handle, int32_t ioType, OpenType openType) {
    android::AutoMutex _l(mLock);
    std::shared_ptr<SubtitleService>  ss = getSubtitleServiceLocked(sId);
    ALOGV("%s ss=%p ioType=%d openType:%d", __func__, ss.get(), ioType, openType);

    int fd = -1;
    int dupFd = -1; // fd will auto closed when destruct hidl_handle, dump one.
    int demuxId = -1;
    if (handle != nullptr && handle->numFds >= 1) {
        fd = handle->data[0];
        dupFd = ::dup(fd);
    }

    auto now = systemTime(SYSTEM_TIME_MONOTONIC);
    const int64_t diff200ms = 200000000LL;
    ALOGD("mOpenCalled: %d mLastOpenType=%d, mLastOpenTime=%lld, now=%lld",
            mOpenCalled, mLastOpenType, mLastOpenTime, now);

    if (ss != nullptr) {
        // because current we test middlware player(e.g CTC) with VideoPlayer, it know nothing about
        // middleware calling, if ctc called, close VideoPlayer's request, use CTC's request.
        if (mOpenCalled && openType == OpenType::TYPE_MIDDLEWARE) {
             if ((openType != mLastOpenType && (now-mLastOpenTime) < diff200ms) || mLastOpenTime == -1) {
                bool r = ss->stopSubtitle();
                ss->stopFmqReceiver();
                if (mDataMQ) {
                    std::unique_ptr<DataMQ> removeDataMQ(std::move(mDataMQ));
                    mDataMQ.reset(nullptr);
                }
            }
        }
        // TODO: need revise, fix the value.
        if ((ioType&0xff) == E_SUBTITLE_DEMUX) {
            demuxId = ioType>>16;
            ioType = ioType&0xff;
            ALOGD("mOpenCalled : demux id= %d, ioType =%d\n", demuxId, ioType);
            ss->setDemuxId(demuxId);
        }
        bool r = ss->startSubtitle(dupFd, (SubtitleIOType)ioType, mMessagQueue.get());

        mOpenCalled = true;
        mLastOpenType = openType;
        mLastOpenTime = now;

        return (r ? Result::OK : Result::FAIL);
    }

    if (dupFd != -1) close(dupFd);
    ALOGD("no valid ss, Should not enter here!");
    return Result::FAIL;
}

Return<Result> SubtitleServer::close(int32_t sId) {
    std::shared_ptr<SubtitleService>  ss = getSubtitleService(sId);
    ALOGV("%s ss=%p", __func__, ss.get());
    if (ss != nullptr) {
        bool r = ss->stopSubtitle();
        ss->stopFmqReceiver();
        {
            android::AutoMutex _l(mLock);
            if (mDataMQ) {
                std::unique_ptr<DataMQ> removeDataMQ(std::move(mDataMQ));
                mDataMQ.reset(nullptr);
            }
            mOpenCalled = false;
        }
        return (r ? Result::OK : Result::FAIL);
    }

    return Result::FAIL;
}

Return<Result> SubtitleServer::resetForSeek(int32_t sId) {
    std::shared_ptr<SubtitleService>  ss = getSubtitleService(sId);
    if (ss != nullptr) {
        bool r = ss->resetForSeek();
        return (r ? Result::OK : Result::FAIL);
    }
    return Result::FAIL;
}

Return<Result> SubtitleServer::updateVideoPos(int32_t sId, int32_t pos) {
    std::shared_ptr<SubtitleService>  ss = getSubtitleService(sId);
    if (ss != nullptr) {
        bool r = ss->updateVideoPosAt(pos);
        return (r ? Result::OK : Result::FAIL);
    }
    return Result::FAIL;
}

Return<void> SubtitleServer::getTotalTracks(int32_t sId, getTotalTracks_cb _hidl_cb) {
    std::shared_ptr<SubtitleService>  ss = getSubtitleService(sId);
    ALOGV("%s ss=%p", __func__, ss.get());
    if (ss != nullptr) {
        ALOGV("%s total=%d", __func__, ss->totalSubtitles());
        _hidl_cb(Result::OK, ss->totalSubtitles());
    } else {
        _hidl_cb(Result::FAIL, 1);//-1);
    }

    return Void();
}

Return<void> SubtitleServer::getType(int32_t sId, getType_cb _hidl_cb) {
    std::shared_ptr<SubtitleService>  ss = getSubtitleService(sId);
    ALOGV("%s ss=%p", __func__, ss.get());
    if (ss != nullptr) {
        ALOGV("%s subType=%d", __func__, ss->subtitleType());
        _hidl_cb(Result::OK, ss->subtitleType());
    } else {
        _hidl_cb(Result::FAIL, 1);
    }
    return Void();
}

Return<void> SubtitleServer::getLanguage(int32_t sId, getLanguage_cb _hidl_cb) {
    std::shared_ptr<SubtitleService>  ss = getSubtitleService(sId);
    ALOGV("%s ss=%p", __func__, ss.get());
    if (ss != nullptr) {
        _hidl_cb(Result::OK, ss->currentLanguage());
    }
    _hidl_cb(Result::FAIL, std::string("No lang"));
    return Void();
}


Return<Result> SubtitleServer::setSubType(int32_t sId, int32_t type) {
    std::shared_ptr<SubtitleService>  ss = getSubtitleService(sId);
    ALOGV("%s ss=%p subType=%d", __func__, ss.get(), type);

    if (ss != nullptr) {
        ss->setSubType(type);
    }
    return Result {};
}

Return<Result> SubtitleServer::setSubPid(int32_t sId, int32_t pid) {
    std::shared_ptr<SubtitleService>  ss = getSubtitleService(sId);
    ALOGV("%s ss=%p pid=%d", __func__, ss.get(), pid);
    if (ss != nullptr) {
        ss->setSubPid(pid);
    }
    return Result {};
}

Return<Result> SubtitleServer::setPageId(int32_t sId, int32_t pageId) {
    std::shared_ptr<SubtitleService>  ss = getSubtitleService(sId);
    ALOGV("%s ss=%p", __func__, ss.get());
    if (ss != nullptr) {
        ss->setSubPageId(pageId);
    }
    return Result {};
}

Return<Result> SubtitleServer::setAncPageId(int32_t sId, int32_t ancPageId) {
    std::shared_ptr<SubtitleService>  ss = getSubtitleService(sId);
    ALOGV("%s ss=%p setAncPageId=%d", __func__, ss.get(), ancPageId);
    if (ss != nullptr) {
        ss->setSubAncPageId(ancPageId);
    }
    return Result {};
}

Return<Result> SubtitleServer::setChannelId(int32_t sId, int32_t channelId) {
    std::shared_ptr<SubtitleService>  ss = getSubtitleService(sId);
    ALOGV("%s ss=%p", __func__, ss.get());
    if (ss != nullptr) {
        ss->setChannelId(channelId);
    }
    return Result {};
}

Return<Result> SubtitleServer::setClosedCaptionVfmt(int32_t sId, int32_t vfmt) {
    std::shared_ptr<SubtitleService>  ss = getSubtitleService(sId);
    ALOGV("%s ss=%p", __func__, ss.get());
    if (ss != nullptr) {
        ss->setClosedCaptionVfmt(vfmt);
    }
    return Result {};
}

Return<Result> SubtitleServer::ttControl(int32_t sId, int cmd, int pageNo, int subPageNo, int pageDir, int subPageDir) {
    std::shared_ptr<SubtitleService>  ss = getSubtitleService(sId);
    if (ss == nullptr) {
        return Result::FAIL;
    }
    ALOGV("%s ss=%p cmd=%d", __func__, ss.get(), cmd);

    bool r = ss->ttControl(cmd, pageNo, subPageNo, pageDir, subPageDir);
    return r ? Result::OK : Result::FAIL;
}

Return<Result> SubtitleServer::ttGoHome(int32_t sId) {
    std::shared_ptr<SubtitleService>  ss = getSubtitleService(sId);
    if (ss == nullptr) {
        return Result::FAIL;
    }
    ss->ttGoHome();
    return Result::OK;
}

Return<Result> SubtitleServer::ttNextPage(int32_t sId, int32_t dir) {
    std::shared_ptr<SubtitleService>  ss = getSubtitleService(sId);
    if (ss == nullptr) {
        return Result::FAIL;
    }
    ss->ttNextPage(dir);
    return Result::OK;
}

Return<Result> SubtitleServer::ttNextSubPage(int32_t sId, int32_t dir) {
    std::shared_ptr<SubtitleService>  ss = getSubtitleService(sId);
    if (ss == nullptr) {
        return Result::FAIL;
    }
    ss->ttNextSubPage(dir);
    return Result::OK;
}

Return<Result> SubtitleServer::ttGotoPage(int32_t sId, int32_t pageNo, int32_t subPageNo) {
    std::shared_ptr<SubtitleService>  ss = getSubtitleService(sId);
    if (ss == nullptr) {
        return Result::FAIL;
    }
    ALOGE(" ttGotoPage pageNo=%d, subPageNo=%d",pageNo, subPageNo);
    ss->ttGotoPage(pageNo, subPageNo);
    return Result::OK;
}

Return<Result> SubtitleServer::userDataOpen(int32_t sId) {
    ALOGV("%s", __func__);
    std::shared_ptr<SubtitleService>  ss = getSubtitleService(sId);
    if (ss == nullptr) {
        return Result::FAIL;
    }
    ss->userDataOpen(mMessagQueue.get());
    return Result::OK;
}

Return<Result> SubtitleServer::userDataClose(int32_t sId) {
    ALOGV("%s", __func__);
    std::shared_ptr<SubtitleService>  ss = getSubtitleService(sId);
    if (ss == nullptr) {
        return Result::FAIL;
    }
    ss->userDataClose();
    return Result::OK;
}



Return<void> SubtitleServer::prepareWritingQueue(int32_t sId, int32_t size, prepareWritingQueue_cb _hidl_cb) {
    auto sendError = [&_hidl_cb](Result result) {
        _hidl_cb(result, DataMQ::Descriptor());
    };

    android::AutoMutex _l(mLock);

    // Create message queues.
    if (mDataMQ) {
        ALOGW("the client attempts to call prepareForWriting twice");
    } else {
        std::unique_ptr<DataMQ> tempDataMQ(new DataMQ(size, true /* EventFlag */));
        if (!tempDataMQ->isValid()) {
            ALOGE_IF(!tempDataMQ->isValid(), "data MQ is invalid");
            sendError(Result::FAIL);
            return Void();
        }
        mDataMQ = std::move(tempDataMQ);
    }
    //TODO: create new thread for fetch the client writed data.
    std::unique_ptr<FmqReader> tempReader(new FmqReaderImpl(mDataMQ.get()));
    std::shared_ptr<SubtitleService>  ss = getSubtitleServiceLocked(sId);
    ALOGV("%s ss=%p", __func__, ss.get());
    if (ss != nullptr) {
        ss->startFmqReceiver(std::move(tempReader));
    } else {
        ALOGE("Error! cannot get subtitle service through ID:%d(%x)", sId, sId);
    }
    _hidl_cb(Result::OK, *mDataMQ->getDesc());

    return Void();
}



Return<void> SubtitleServer::setCallback(const sp<ISubtitleCallback>& callback, ConnectType type) {
    android::AutoMutex _l(mLock);
    if (callback != nullptr) {
        int cookie = -1;
        int clientSize = mCallbackClients.size();
        for (int i = 0; i < clientSize; i++) {
            if (mCallbackClients[i] == nullptr) {
                ALOGI("%s, client index:%d had died, this id give the new client", __FUNCTION__, i);
                cookie = i;
                mCallbackClients[i] = callback;
                break;
            }
        }

        if (cookie < 0) {
            cookie = clientSize;
            mCallbackClients[clientSize] = callback;
        }

        Return<bool> linkResult = callback->linkToDeath(mDeathRecipient, cookie);
        bool linkSuccess = linkResult.isOk() ? static_cast<bool>(linkResult) : false;
        if (!linkSuccess) {
            ALOGW("Couldn't link death recipient for cookie: %d", cookie);
        }

        ALOGI("%s cookie:%d, client size:%d", __FUNCTION__, cookie, (int)mCallbackClients.size());
    }

    return Void();
}

Return<void> SubtitleServer::setFallbackCallback(const sp<ISubtitleCallback>& callback, ConnectType type) {
    android::AutoMutex _l(mLock);
    mFallbackCallback = callback;
    return Void();
}

Return<void> SubtitleServer::removeCallback(const sp<ISubtitleCallback>& callback) {
    android::AutoMutex _l(mLock);
    if (callback != nullptr) {
        // Remove, if fallback callback.
        if (mFallbackCallback != nullptr && mFallbackCallback.get() == callback.get()) {
            mFallbackCallback->unlinkToDeath(mDeathRecipient);
            mFallbackCallback = nullptr;
        }

        int clientSize = mCallbackClients.size();
        ALOGI("[removeCallback] remove:%p clientSize=%d", callback.get(), clientSize);
        for (auto it = mCallbackClients.begin(); it != mCallbackClients.end();) {
            if (it->second != nullptr) {
                ALOGI("[removeCallback] %p", (it->second).get());
                it->second->unlinkToDeath(mDeathRecipient);
            }
            it = mCallbackClients.erase(it);
        }
    }
    return Void();
}

// This only valid for global fallback display.
Return<Result> SubtitleServer::show(int32_t sId) {
    SubtitleHidlParcel parcel;
    android::AutoMutex _l(mLock);
    mFallbackPlayStarted = true;
    parcel.msgType = (int)FallthroughUiCmd::CMD_UI_SHOW;
    parcel.bodyInt.resize(0);
    sendUiEvent(parcel);

    // Can send display Data to subtitle service now.
    return Result {};
}

Return<Result> SubtitleServer::hide(int32_t sId) {
    SubtitleHidlParcel parcel;
    android::AutoMutex _l(mLock);
    mFallbackPlayStarted = false;
    parcel.msgType = (int)FallthroughUiCmd::CMD_UI_HIDE;
    parcel.bodyInt.resize(0);
    sendUiEvent(parcel);
    return Result {};
}

/*CMD_UI_SHOW = 0,
CMD_UI_SET_IMGRATIO,
CMD_UI_SET_SUBDEMISION,
CMD_UI_SET_SURFACERECT*/

Return<Result> SubtitleServer::setTextColor(int32_t sId, int32_t color) {
    SubtitleHidlParcel parcel;
    android::AutoMutex _l(mLock);
    parcel.msgType = (int)FallthroughUiCmd::CMD_UI_SET_TEXTCOLOR;
    parcel.bodyInt.resize(1);
    parcel.bodyInt[0] = color;
    sendUiEvent(parcel);
    return Result {};
}

Return<Result> SubtitleServer::setTextSize(int32_t sId, int32_t size) {
    SubtitleHidlParcel parcel;
    android::AutoMutex _l(mLock);
    parcel.msgType = (int)FallthroughUiCmd::CMD_UI_SET_TEXTSIZE;
    parcel.bodyInt.resize(1);
    parcel.bodyInt[0] = size;
    sendUiEvent(parcel);
    return Result {};
}

Return<Result> SubtitleServer::setGravity(int32_t sId, int32_t gravity) {
    SubtitleHidlParcel parcel;
    android::AutoMutex _l(mLock);
    parcel.msgType = (int)FallthroughUiCmd::CMD_UI_SET_GRAVITY;
    parcel.bodyInt.resize(1);
    parcel.bodyInt[0] = gravity;
    sendUiEvent(parcel);
    return Result {};
}

Return<Result> SubtitleServer::setTextStyle(int32_t sId, int32_t style) {
    SubtitleHidlParcel parcel;
    android::AutoMutex _l(mLock);
    parcel.msgType = (int)FallthroughUiCmd::CMD_UI_SET_TEXTSTYLE;
    parcel.bodyInt.resize(1);
    parcel.bodyInt[0] = style;
    sendUiEvent(parcel);
    return Result {};
}

Return<Result> SubtitleServer::setPosHeight(int32_t sId, int32_t yOffset) {
    SubtitleHidlParcel parcel;
    android::AutoMutex _l(mLock);
    parcel.msgType = (int)FallthroughUiCmd::CMD_UI_SET_POSHEIGHT;
    parcel.bodyInt.resize(1);
    parcel.bodyInt[0] = yOffset;
    sendUiEvent(parcel);
    return Result {};
}

Return<Result> SubtitleServer::setImgRatio(int32_t sId, float ratioW, float ratioH, int32_t maxW, int32_t maxH) {
    SubtitleHidlParcel parcel;
    android::AutoMutex _l(mLock);
    parcel.msgType = (int)FallthroughUiCmd::CMD_UI_SET_IMGRATIO;
    parcel.bodyInt.resize(4);
    parcel.bodyInt[0] = (int)ratioW;
    parcel.bodyInt[1] = (int)ratioH;
    parcel.bodyInt[2] = maxW;
    parcel.bodyInt[3] = maxH;
    sendUiEvent(parcel);
    return Result {};
}

Return<void> SubtitleServer::getSubDemision(int32_t sId, getSubDemision_cb _hidl_cb) {
    // TODO implement
    return Void();
}

Return<Result> SubtitleServer::setSubDemision(int32_t sId, int32_t width, int32_t height) {
    // TODO implement
    return Result {};
}

Return<Result> SubtitleServer::setSurfaceViewRect(int32_t sId, int32_t x, int32_t y, int32_t w, int32_t h) {
    SubtitleHidlParcel parcel;
    android::AutoMutex _l(mLock);
    parcel.msgType = (int)FallthroughUiCmd::CMD_UI_SET_SURFACERECT;
    parcel.bodyInt.resize(4);
    parcel.bodyInt[0] = x;
    parcel.bodyInt[1] = y;
    parcel.bodyInt[2] = w;
    parcel.bodyInt[3] = h;
    sendUiEvent(parcel);
    return Result {};
}


void SubtitleServer::sendSubtitleDisplayNotify(SubtitleHidlParcel &event) {
    android::AutoMutex _l(mLock);

    ALOGI("onEvent event:%d, client size:%d", event.msgType, mCallbackClients.size());

    if (mFallbackPlayStarted && mFallbackCallback != nullptr) {
        // enabled fallback display and fallback displayer started, then
        auto r = mFallbackCallback->notifyDataCallback(event);
        ALOGE_IF(!r.isOk(), "Error, call notifyDataCallback failed! client died?");
        return;
    }

    for (int i = 0; i<mCallbackClients.size(); i++) {
        if (mCallbackClients[i] != nullptr) {
            ALOGI("%s, client cookie:%d notifyCallback", __FUNCTION__, i);
            auto r = mCallbackClients[i]->notifyDataCallback(event);
            ALOGE_IF(!r.isOk(), "Error, call notifyDataCallback failed! client died?");
        }
    }
}

void SubtitleServer::sendSubtitleEventNotify(SubtitleHidlParcel &event) {
    android::AutoMutex _l(mLock);
    if (mFallbackPlayStarted && mFallbackCallback != nullptr) {
        auto r = mFallbackCallback->eventNotify(event);
        ALOGE_IF(!r.isOk(), "Error, call eventNotify failed! client died?");
    }
    int clientSize = mCallbackClients.size();

    ALOGI("onEvent event:%d, client size:%d", event.msgType, clientSize);

    // check has valid client or not.
    bool hasEventClient = false;
    if (clientSize > 0) {
        for (int i = 0; i < clientSize; i++) {
            if (mCallbackClients[i] != nullptr) {
                hasEventClient = true;
                break;
            }
        }
    }

    // send DisplayEvent to clients
    if (hasEventClient > 0) {
        for (int i = 0; i < clientSize; i++) {
            if (mCallbackClients[i] != nullptr) {
                ALOGI("%s, client cookie:%d notifyCallback", __FUNCTION__, i);
                auto r = mCallbackClients[i]->eventNotify(event);
                ALOGE_IF(!r.isOk(), "Error, call notifyDataCallback failed! client died?");
            }
        }
    } else {
        // No client connected, try fallback display.
        //if (mFallbackDisplayClient != nullptr) {
        //    if (ENABLE_LOG_PRINT) ALOGI("fallback display event:%d, client size:%d", parcel.msgType, clientSize);
        //    mFallbackDisplayClient->notifyDisplayCallback(parcel);
        //}
    }
}


void SubtitleServer::sendUiEvent(SubtitleHidlParcel &event) {
    //if (!mFallbackPlayStarted) {
    //    ALOGE("UI event request not procceed, do you called uiShow()?");
    //    return;
    //}

    if (mFallbackCallback == nullptr) {
        ALOGE("Error, no default fallback display registed!");
        return;
    }

    auto r = mFallbackCallback->uiCommandCallback(event);
}




Return<void> SubtitleServer::debug(const hidl_handle& handle, const hidl_vec<hidl_string>& options) {
    ALOGV("%s", __func__);
    if (handle != nullptr && handle->numFds >= 1) {
        int fd = handle->data[0];

        std::vector<std::string> args;
        for (size_t i = 0; i < options.size(); i++) {
            args.push_back(options[i]);
        }
        dump(fd, args);
        fsync(fd);
    }
    return Void();
}

void SubtitleServer::dump(int fd, const std::vector<std::string>& args) {
    android::Mutex::Autolock lock(mLock);
    ALOGV("%s", __func__);

    int len = args.size();
    for (int i = 0; i < len; i ++) {
        std::string debugInfoAll("-a");
        std::string debugLevel("-l");
        std::string dumpraw("--dumpraw");
        std::string dumpmemory("-m");
        std::string help("-h");
        if (args[i] == debugInfoAll) {
            //print all subtitle server runtime status(include subtitle ui)
            //we need write an api to get information from subtitle ui process:
            dprintf(fd, "\n");
        }
        else if (args[i] == debugLevel) {
            if (i + 1 < len) {
                std::string levelStr(args[i+1]);
                int level = atoi(levelStr.c_str());
                //setLogLevel(level);

                dprintf(fd, "Setting log level to %d.\n", level);
                break;
            }
        } else if (args[i] == dumpmemory) {
            if (!android::dumpMemoryAddresses(fd)) {
                dprintf(fd, "Cannot get malloc information!\n");
                dprintf(fd, "Please executed this first[need su]:\n\n");
                dprintf(fd, "    setprop libc.debug.malloc.options backtrace\n");
                dprintf(fd, "    stop subtitleserver\n");
                dprintf(fd, "    start subtitleserver\n");
                dprintf(fd, "    setprop libc.debug.malloc.options \"\"\n\n\n");
            }
            return;
        } else if (args[i] ==dumpraw) {
                mDumpMaps |= (1 << SUBTITLE_DUMP_SOURCE);
                dprintf(fd, "dump raw data source enabled.\n");
                return;
        } else if (args[i] == help) {
            dprintf(fd,
                "subtitle server hwbinder service use to control the ctc/apk channel to subtitle ui \n"
                "usage: \n"
                "lshal debug vendor.amlogic.hardware.subtitleserver@1.0::ISubtitleServer/default -l value \n"
                "-a: dump all subtitle server and subtitle ui related information \n"
                "-l: set subtitle server debug level \n"
                "-m: dump memory alloc info [need restart subtitle with libc.debug.malloc.options=backtrace]"
                "-h: help \n");
            return;
        }
    }

    //dump client:
    dprintf(fd, "\n\n HIDL Service: \n");
    dprintf(fd, "--------------------------------------------------------------------------------------\n\n");
    dprintf(fd, "Subtitile Clients:\n");
    if (mFallbackCallback != nullptr) {
        dprintf(fd, "    FallbackDisplayCallback: (%p)%s\n", mFallbackCallback.get(), toString(mFallbackCallback).c_str());
    }
    int clientSize = mCallbackClients.size();
    dprintf(fd, "    EventCallback: count=%d\n", clientSize);
    for (int i = 0; i < clientSize; i++) {
        dprintf(fd, "        %d: (%p)%s\n", i, mCallbackClients[i].get(),
            mCallbackClients[i]==nullptr ? "null" : toString(mCallbackClients[i]).c_str());
    }

    // TODO: travsel each service.
    std::shared_ptr<SubtitleService>  ss = getSubtitleServiceLocked(0);
    if (ss != nullptr) {
        ss->dump(fd);
    }
}

bool SubtitleServer::ClientMessageHandlerImpl::onSubtitleDisplayNotify(SubtitleHidlParcel &event) {
    ALOGE("CallbackHandlerImpl onSubtitleDataEvent");
    mSubtitleServer->sendSubtitleDisplayNotify(event);
    return false;
}
bool SubtitleServer::ClientMessageHandlerImpl::onSubtitleEventNotify(SubtitleHidlParcel &event) {
        ALOGE("CallbackHandlerImpl onSubtitleDataEvent");
        mSubtitleServer->sendSubtitleEventNotify(event);
        return false;
    }

void SubtitleServer::handleServiceDeath(uint32_t cookie) {
    ALOGV("%s", __func__);
    {
        android::AutoMutex _l(mLock);
        /*if (cookie == FALLBACK_DISPLAY_COOKIE) {
            mFallbackDisplayClient->unlinkToDeath(mDeathRecipient);
            mFallbackDisplayClient = nullptr;
        } else */{
            mCallbackClients[cookie]->unlinkToDeath(mDeathRecipient);
            mCallbackClients[cookie] = nullptr;
        }
    }
    // TODO: handle which client exited
    // currently, only support 1 subs
    //mSubtitleService->stopSubtitle();
    //close();
}

void SubtitleServer::DeathRecipient::serviceDied(
        uint64_t cookie,
        const ::android::wp<::android::hidl::base::V1_0::IBase>& who) {
    ALOGE("subtitleserver daemon client died cookie:%d", (int)cookie);

    ::android::sp<::android::hidl::base::V1_0::IBase> s = who.promote();
    ALOGE("subtitleserver daemon client died who:%p", s.get());

    if (s != nullptr) {
        auto r = s->interfaceDescriptor([&](const hidl_string &types) {
                ALOGE("subtitleserver daemon client, who=%s", types.c_str());
            });
        if (!r.isOk()) {
            ALOGE("why?");
        }
    }
    uint32_t type = static_cast<uint32_t>(cookie);
    mSubtitleServer->handleServiceDeath(type);
}


// Methods from ::android::hidl::base::V1_0::IBase follow.

//ISubtitleServer* HIDL_FETCH_ISubtitleServer(const char* /* name */) {
    //return new SubtitleServer();
//}
//
}  // namespace implementation
}  // namespace V1_0
}  // namespace subtitleserver
}  // namespace hardware
}  // namespace amlogic
}  // namespace vendor
