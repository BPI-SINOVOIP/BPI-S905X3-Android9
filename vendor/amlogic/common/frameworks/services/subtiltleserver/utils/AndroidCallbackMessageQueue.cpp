#define LOG_NDEBUG  0
#define LOG_TAG "SubtitleServer"
#include <thread>
#include "AndroidCallbackMessageQueue.h"

#define EVENT_ONSUBTITLEDATA_CALLBACK       0xA00000
#define EVENT_ONSUBTITLEAVAILABLE_CALLBACK  0xA00001
#define EVENT_ONVIDEOAFDCHANGE_CALLBACK     0xA00002
#define EVENT_ONMIXVIDEOEVENT_CALLBACK      0xA00003
#define EVENT_ONSUBTITLE_DIMESION_CALLBACK  0xA00004
#define EVENT_ONSUBTITLE_LANGUAGE_CALLBACK  0xA00005
#define EVENT_ONSUBTITLE_INFO_CALLBACK      0xA00006

namespace amlogic {

static const int MSG_CHECK_SUBDATA = 0;

// This hal never recreate
sp<AndroidCallbackMessageQueue> AndroidCallbackMessageQueue::sInstance = nullptr;
sp<AndroidCallbackMessageQueue> AndroidCallbackMessageQueue::Instance() {
    return sInstance;
}

AndroidCallbackMessageQueue::AndroidCallbackMessageQueue(sp<AndroidCallbackHandler> proxy) {
    android::AutoMutex _l(mLock);
    mStopped = false;
    mProxyHandler = proxy;
    sInstance = this;
    mCallbackLooperThread = std::thread(&AndroidCallbackMessageQueue::looperLoop, this);
}

AndroidCallbackMessageQueue::~AndroidCallbackMessageQueue() {
    mStopped = true;
    if (mLooper) {
        mLooper->wake();
    }
    // post a message to wakeup and exit.
    if (mLooper != nullptr) {
        mLooper->sendMessage(this, Message(MSG_CHECK_SUBDATA));
    }
    mCallbackLooperThread.join();
    sInstance = nullptr;
}

void AndroidCallbackMessageQueue::looperLoop() {
    mLooper = new Looper(false);
    mLooper->sendMessageDelayed(100LL, this, Message(MSG_CHECK_SUBDATA));
    // never exited
    while (!mStopped) {
        mLooper->pollAll(-1);
    }
    mLooper = nullptr;
}

void AndroidCallbackMessageQueue::handleMessage(const Message& message) {
    std::unique_ptr<SubtitleData> data(nullptr);

    mLock.lock();
    if (mSubtitleData.size() > 0) {
        auto iter = mSubtitleData.begin();
        std::unique_ptr<SubtitleData> items = std::move(*iter);
        data.swap(items);
        mSubtitleData.erase(iter);
    }
    if (mSubtitleData.size() > 0) {
        mLooper->removeMessages(this, MSG_CHECK_SUBDATA);
        mLooper->sendMessage(this, Message(MSG_CHECK_SUBDATA));
    } else {
        mLooper->removeMessages(this, MSG_CHECK_SUBDATA);
        mLooper->sendMessageDelayed(s2ns(2), this, Message(MSG_CHECK_SUBDATA));
    }
    mLock.unlock();

    // processing, outside lock.
    if (mProxyHandler != nullptr && data != nullptr) {
        SubtitleHidlParcel parcel;
        parcel.msgType = data->type;

        switch (parcel.msgType) {
            case EVENT_ONSUBTITLEDATA_CALLBACK:
            {
                parcel.bodyInt.resize(2);
                parcel.bodyInt[0] = data->x;
                parcel.bodyInt[1] = data->y;
                ALOGD("onSubtitleDataEvent EVENT_ONSUBTITLEDATA_CALLBACK: event:%d, id:%d", data->x, data->y);
                mProxyHandler->onSubtitleEventNotify(parcel) ;
            break;
            }
            case EVENT_ONSUBTITLEAVAILABLE_CALLBACK:
            {
                parcel.bodyInt.resize(1);
                parcel.bodyInt[0] = data->x;
                ALOGD("onSubtitleDataEvent EVENT_ONSUBTITLEAVAILABLE_CALLBACK: avaiable:%d", data->x);
                mProxyHandler->onSubtitleEventNotify(parcel) ;
            break;
            }
            case EVENT_ONVIDEOAFDCHANGE_CALLBACK:
            {
                parcel.bodyInt.resize(1);
                parcel.bodyInt[0] = data->x;
                ALOGD("onSubtitleDataEvent EVENT_ONVIDEOAFDCHANGE_CALLBACK:afd:%d", data->x);
                mProxyHandler->onSubtitleEventNotify(parcel) ;
            break;
            }
            case EVENT_ONSUBTITLE_DIMESION_CALLBACK:
            {
                parcel.bodyInt.resize(2);
                parcel.bodyInt[0] = data->x;
                parcel.bodyInt[1] = data->y;
                ALOGD("onSubtitleDataEvent EVENT_ONSUBTITLE_DIMESION_CALLBACK: width:%d, height:%d", data->x, data->y);
                mProxyHandler->onSubtitleEventNotify(parcel) ;
                break;
            }
            case EVENT_ONMIXVIDEOEVENT_CALLBACK:
            {
                parcel.bodyInt.resize(1);
                parcel.bodyInt[0] = data->x;
                ALOGD("onSubtitleDataEvent EVENT_ONMIXVIDEOEVENT_CALLBACK:%d", data->x);
                mProxyHandler->onSubtitleEventNotify(parcel) ;
            break;
            }
            case EVENT_ONSUBTITLE_LANGUAGE_CALLBACK:
            {
                std::string lang;
                lang = data->lang;
                parcel.bodyString.resize(1);
                parcel.bodyString[0] = lang.c_str();
                ALOGD("onSubtitleDataEvent EVENT_ONSUBTITLE_LANGUAGE_CALLBACK:lang:%s", lang.c_str());
                mProxyHandler->onSubtitleEventNotify(parcel) ;
                break;
            }
            case EVENT_ONSUBTITLE_INFO_CALLBACK:
            {
                parcel.bodyInt.resize(2);
                parcel.bodyInt[0] = data->x;
                parcel.bodyInt[1] = data->y;
                ALOGD("onSubtitleDataEvent EVENT_ONSUBTITLE_INFO_CALLBACK:what:%d, extra:%d", data->x, data->y);
                mProxyHandler->onSubtitleEventNotify(parcel) ;
                break;

            }
            default:
            {
                parcel.bodyInt.resize(8);
                parcel.bodyInt[0] = data->width;
                parcel.bodyInt[1] = data->height;
                parcel.bodyInt[2] = data->size;
                parcel.bodyInt[3] = data->isShow;
                parcel.bodyInt[4] = data->x;
                parcel.bodyInt[5] = data->y;
                parcel.bodyInt[6] = data->videoWidth;
                parcel.bodyInt[7] = data->videoHeight;
                parcel.mem = *(data->mem);
                ALOGI("onSubtitleDataEvent, type:%d width:%d, height:%d, size:%d", data->type, data->width, data->height, data->size);
                mProxyHandler->onSubtitleDisplayNotify(parcel);

                // some customer do not want the whole subtitle data
                // but want to know the subtitle dimesion. here report it to make they happy!
                /*{
                    SubtitleHidlParcel parcel;
                    parcel.msgType = EVENT_ONSUBTITLE_DIMESION_CALLBACK;
                    parcel.bodyInt.resize(2);
                    parcel.bodyInt[0] = data->width;
                    parcel.bodyInt[1] = data->height;
                    ALOGD("onSubtitleDataEvent EVENT_ONSUBTITLE_DIMESION_CALLBACK:");
                    mProxyHandler->onSubtitleEventNotify(parcel) ;
                }*/

                break;
             }
        }
    }
}

void AndroidCallbackMessageQueue::onSubtitleDataEvent(int event, int chnnlId) {
    android::AutoMutex _l(mLock);
    std::unique_ptr<SubtitleData> subtitleData = std::make_unique<SubtitleData>();
    subtitleData->type = EVENT_ONSUBTITLEDATA_CALLBACK;
    subtitleData->x = event;
    subtitleData->y = chnnlId;
    mSubtitleData.push_back(std::move(subtitleData));
    mLooper->sendMessage(this, Message(MSG_CHECK_SUBDATA));
}
void AndroidCallbackMessageQueue::onSubtitleDimension(int width, int height) {
    android::AutoMutex _l(mLock);
    std::unique_ptr<SubtitleData> subtitleData = std::make_unique<SubtitleData>();
    subtitleData->type = EVENT_ONSUBTITLE_DIMESION_CALLBACK;
    subtitleData->x = width;
    subtitleData->y = height;
    mSubtitleData.push_back(std::move(subtitleData));
    mLooper->sendMessage(this, Message(MSG_CHECK_SUBDATA));
}

void AndroidCallbackMessageQueue::onSubtitleAvailable(int available) {
    android::AutoMutex _l(mLock);
    std::unique_ptr<SubtitleData> subtitleData = std::make_unique<SubtitleData>();
    subtitleData->type = EVENT_ONSUBTITLEAVAILABLE_CALLBACK;
    subtitleData->x = available;
    mSubtitleData.push_back(std::move(subtitleData));
    mLooper->sendMessage(this, Message(MSG_CHECK_SUBDATA));
}

void AndroidCallbackMessageQueue::onVideoAfdChange(int afd) {
    android::AutoMutex _l(mLock);
    std::unique_ptr<SubtitleData> subtitleData = std::make_unique<SubtitleData>();
    subtitleData->type = EVENT_ONVIDEOAFDCHANGE_CALLBACK;
    subtitleData->x = afd;
    mSubtitleData.push_back(std::move(subtitleData));
    mLooper->sendMessage(this, Message(MSG_CHECK_SUBDATA));
}

void AndroidCallbackMessageQueue::onMixVideoEvent(int val) {
    android::AutoMutex _l(mLock);
    std::unique_ptr<SubtitleData> subtitleData = std::make_unique<SubtitleData>();
    subtitleData->type = EVENT_ONMIXVIDEOEVENT_CALLBACK;
    subtitleData->x = val;
    mSubtitleData.push_back(std::move(subtitleData));
    mLooper->sendMessage(this, Message(MSG_CHECK_SUBDATA));
}

void AndroidCallbackMessageQueue::onSubtitleLanguage(char *lang) {
    android::AutoMutex _l(mLock);
    std::unique_ptr<SubtitleData> subtitleData = std::make_unique<SubtitleData>();
    subtitleData->type = EVENT_ONSUBTITLE_LANGUAGE_CALLBACK;
    subtitleData->lang = lang;
    mSubtitleData.push_back(std::move(subtitleData));
    mLooper->sendMessage(this, Message(MSG_CHECK_SUBDATA));
}

void AndroidCallbackMessageQueue::onSubtitleInfo(int what, int extra) {
    android::AutoMutex _l(mLock);
    std::unique_ptr<SubtitleData> subtitleData = std::make_unique<SubtitleData>();
    subtitleData->type = EVENT_ONSUBTITLE_INFO_CALLBACK;
    subtitleData->x = what;
    subtitleData->y = extra;
    mSubtitleData.push_back(std::move(subtitleData));
    mLooper->sendMessage(this, Message(MSG_CHECK_SUBDATA));
}


bool AndroidCallbackMessageQueue::postDisplayData(const char *data,  int type,
        int x, int y, int width, int height,
        int videoWidth, int videoHeight, int size, int cmd) {

    sp<HidlMemory> mem;
    sp<IAllocator> ashmemAllocator = IAllocator::getService("ashmem");

    bool allocRes = false;

    Return<void> r = ashmemAllocator->allocate(size+1024, [&](bool success, const hidl_memory& _mem) {
            if (success) {
                mem  = HidlMemory::getInstance(_mem);
                sp<IMemory> memory = mapMemory(_mem);
                if (memory == NULL) {
                    ALOGE("map Memory Failed!!");
                    return; // we may need  status!
                }

                memory->update();
                if (data != nullptr) memcpy(memory->getPointer(), data, size);
                memory->commit();
                allocRes = true;
            } else {
                ALOGE("alloc memory Fail");
            }
        });

    if (r.isOk() && allocRes) {
        android::AutoMutex _l(mLock);
        std::unique_ptr<SubtitleData> subtitleData = std::make_unique<SubtitleData>();
        subtitleData->type = type;
        subtitleData->x = x;
        subtitleData->y = y;
        subtitleData->width = width;
        subtitleData->height = height;
        subtitleData->videoWidth = videoWidth;
        subtitleData->videoHeight = videoHeight;
        subtitleData->size = size;
        subtitleData->isShow = cmd;
        subtitleData->mem = mem;
        mSubtitleData.push_back(std::move(subtitleData));
        mLooper->sendMessage(this, Message(MSG_CHECK_SUBDATA));

    } else {
        ALOGE("Fail to process hidl memory!!");
    }


    return true;
}

}
