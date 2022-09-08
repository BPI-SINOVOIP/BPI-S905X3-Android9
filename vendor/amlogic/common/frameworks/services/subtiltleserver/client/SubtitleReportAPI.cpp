#define LOG_NDEBUG 0
#define LOG_TAG "SocketAPI"
#include <utils/Log.h>
#include <utils/CallStack.h>
#include <mutex>
#include <utils/RefBase.h>

#include <fmq/EventFlag.h>
#include <fmq/MessageQueue.h>
#include <vendor/amlogic/hardware/subtitleserver/1.0/ISubtitleServer.h>

#include "SubtitleReportAPI.h"
//namespace android {
using ::android::CallStack;
using ::android::sp;

using ::android::hardware::MessageQueue;
using ::android::hardware::kSynchronizedReadWrite;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::vendor::amlogic::hardware::subtitleserver::V1_0::ISubtitleServer;
using ::vendor::amlogic::hardware::subtitleserver::V1_0::Result;

typedef MessageQueue<uint8_t, kSynchronizedReadWrite> DataMQ;

typedef struct {
    sp<ISubtitleServer> mRemote;
    std::unique_ptr<DataMQ> mDataMQ;
    int sId;
    std::mutex mLock;   // TODO: maybe we need global lock
} SubtitleContext;

typedef enum {
    SUBTITLE_TOTAL_TRACK    = 'STOT',
    SUBTITLE_START_PTS      = 'SPTS',
    SUBTITLE_RENDER_TIME    = 'SRDT',
    SUBTITLE_SUB_TYPE       = 'STYP',
    SUBTITLE_TYPE_STRING    = 'TPSR',
    SUBTITLE_LANG_STRING    = 'LGSR',
    SUBTITLE_SUB_DATA       = 'PLDT',

    SUBTITLE_RESET_SERVER   = 'CDRT',
    SUBTITLE_EXIT_SERVER    = 'CDEX',

} PayloadType;

/**
    payload is:
        startFlag   : 4bytes
        sessionID   : 4Bytes (TBD)
        magic       : 4bytes (for double confirm)
        payload size: 4Bytes (indicate the size of payload, size is total_send_bytes - 4*5)
                      exclude this and above 4 items.
        payload Type: defined in PayloadType_t
        payload data: TO BE PARSED data
 */

#define LOGIT 1
const static unsigned int START_FLAG = 0xF0D0C0B1;
const static unsigned int MAGIC_FLAG = 0xCFFFFFFB;

const static unsigned int HEADER_SIZE = 4*5;

static void inline makeHeader(char *buf, int sessionId, PayloadType t, int payloadSize) {
    unsigned int val = START_FLAG;
    memcpy(buf, &val, sizeof(val));

    memcpy(buf+sizeof(int), &sessionId, sizeof(int));

    val = MAGIC_FLAG; // magic
    memcpy(buf+2*sizeof(int), &val, sizeof(int));

    memcpy(buf+3*sizeof(int), &payloadSize, sizeof(int));

    val = t;
    memcpy(buf+4*sizeof(val), &val, sizeof(val));
}

static bool prepareWritingQueueLocked(SubtitleContext *ctx) {
    std::unique_ptr<DataMQ> tempDataMQ;
    Result retval;
    Return<void> ret = ctx->mRemote->prepareWritingQueue(
            ctx->sId,
            2*1024*1024, // 2M buffer.
            [&](Result r, const DataMQ::Descriptor& dataMQ) {
                retval = r;
                if (retval == Result::OK) {
                    tempDataMQ.reset(new DataMQ(dataMQ));
                }
            });
    if (LOGIT) ALOGD("prepareWritingQueueLocked");

    if (!ret.isOk() || retval != Result::OK) {
        ALOGE("Error! prepare message Queue failed!");
        return false;
    }

    if (!tempDataMQ || !tempDataMQ->isValid()) {
        ALOGE("Error! cannot get valid message Queue from service");
        return false;
    }
    ctx->mDataMQ = std::move(tempDataMQ);
    return true;
}

static bool fmqSendDataLocked(SubtitleContext *ctx, const char *data, size_t size) {
    size_t writed = 0;
    size_t remainSize = size;
    //if (LOGIT) ALOGD("fmqSendDataLocked %p %d", ctx->mDataMQ.get(), size);
    if (data != nullptr) {
        while (writed < size) {
            size_t availableToWrite = ctx->mDataMQ->availableToWrite();
            size_t needWrite = (remainSize > availableToWrite) ? availableToWrite : remainSize;

            if (!ctx->mDataMQ->writeBlocking((uint8_t *)data, needWrite, 100*1000*1000LL)) {
                ALOGE("data message queue write failed!");
                return false;
            } else {
                remainSize -= needWrite;
                writed += needWrite;
                //ALOGD("availableToWrite:%d needWrite:%d", availableToWrite, needWrite);
                // TODO: notify!!!
            }
        }
        return true;
    }

    return false;
}

// TODO: move all the report API to subtitlebinder
SubSourceHandle SubSource_Create(int sId) {
    SubtitleContext *ctx = new SubtitleContext();
    if (ctx == nullptr) return nullptr;
    if (LOGIT) ALOGD("SubSource Create %d", sId);
    ctx->mLock.lock();
    sp<ISubtitleServer> service =  ISubtitleServer::tryGetService();
    int retry = 0;
    while ((service == nullptr) && (retry++ < 100)) {
        ctx->mLock.unlock();
        usleep(50*1000);//sleep 50ms
        ctx->mLock.lock();
        service = ISubtitleServer::tryGetService();
    }
    if (service == nullptr) {
        ALOGE("Error, Cannot connect to remote Subtitle Service");
        ctx->mLock.unlock();
        delete ctx;
        return nullptr;
    }

    ctx->mRemote = service;
    ctx->sId = sId;

    if (!prepareWritingQueueLocked(ctx)) {
        ALOGE("Error, Cannot get MessageQueue from remote Subtitle Service");
        ctx->mLock.unlock();
        delete ctx;
        return nullptr;
    }

    ctx->mLock.unlock();
    return ctx;
}

SubSourceStatus SubSource_Destroy(SubSourceHandle handle) {
    SubtitleContext *ctx = (SubtitleContext *)handle;
    if (ctx == nullptr) return SUB_STAT_INV;

    if (LOGIT) ALOGD("SubSource destroy %d", ctx->sId);

    ctx->mLock.lock();
    ctx->mRemote = nullptr;
    ctx->mDataMQ = nullptr;
    ctx->mLock.unlock();

    delete ctx;
    return SUB_STAT_OK;
}


SubSourceStatus SubSource_Reset(SubSourceHandle handle) {
    SubtitleContext *ctx = (SubtitleContext *)handle;
    if (ctx == nullptr) return SUB_STAT_INV;

    if (LOGIT) ALOGD("SubSource reset %d", ctx->sId);

    std::lock_guard<std::mutex> guard(ctx->mLock);

    if (ctx->mRemote == nullptr) {
        ALOGE("Error! not connect to Remote Service!");
        return SUB_STAT_INV;
    }

    char buffer[64];
    memset(buffer, 0, 64);
    makeHeader(buffer, ctx->sId, SUBTITLE_RESET_SERVER, 7);
    strcpy(buffer+HEADER_SIZE, "reset\n");
    fmqSendDataLocked(ctx, buffer, HEADER_SIZE+7);
    return SUB_STAT_OK;
}

SubSourceStatus SubSource_Stop(SubSourceHandle handle) {
    SubtitleContext *ctx = (SubtitleContext *)handle;
    if (ctx == nullptr) return SUB_STAT_INV;

    if (LOGIT) ALOGD("SubSource destroy %d", ctx->sId);
    std::lock_guard<std::mutex> guard(ctx->mLock);
    char buffer[64];
    memset(buffer, 0, 64);
    makeHeader(buffer, ctx->sId, SUBTITLE_EXIT_SERVER, 6);
    strcpy(buffer+HEADER_SIZE, "exit\n");
    fmqSendDataLocked(ctx, buffer, HEADER_SIZE+6);
    return SUB_STAT_OK;

}

SubSourceStatus SubSource_ReportRenderTime(SubSourceHandle handle, int64_t timeUs) {
    SubtitleContext *ctx = (SubtitleContext *)handle;
    static int count = 0;
    if (ctx == nullptr) return SUB_STAT_INV;

    if (LOGIT) {
        if (count++ %1000 == 0) ALOGD("SubSource ReportRenderTime %d 0x%llx", ctx->sId, timeUs);
    }

    std::lock_guard<std::mutex> guard(ctx->mLock);
    char buffer[64];
    makeHeader(buffer, ctx->sId, SUBTITLE_RENDER_TIME, sizeof(int64_t));
    memcpy(buffer+HEADER_SIZE, &timeUs, sizeof(int64_t));
    fmqSendDataLocked(ctx, buffer, HEADER_SIZE + sizeof(int64_t));
    return SUB_STAT_OK;
}

SubSourceStatus SubSource_ReportStartPts(SubSourceHandle handle, int64_t pts) {
    SubtitleContext *ctx = (SubtitleContext *)handle;
    if (ctx == nullptr) return SUB_STAT_INV;

    if (LOGIT) ALOGD("SubSource ReportStartPts %d 0x%llx", ctx->sId, pts);

    std::lock_guard<std::mutex> guard(ctx->mLock);
    char buffer[64];
    makeHeader(buffer, ctx->sId, SUBTITLE_START_PTS, sizeof(pts));
    memcpy(buffer+HEADER_SIZE, &pts, sizeof(pts));
    fmqSendDataLocked(ctx, buffer, HEADER_SIZE + sizeof(pts));

    return SUB_STAT_OK;
}

SubSourceStatus SubSource_ReportTotalTracks(SubSourceHandle handle, int trackNum) {
    SubtitleContext *ctx = (SubtitleContext *)handle;
    if (ctx == nullptr) return SUB_STAT_INV;

    if (LOGIT) ALOGD("SubSource ReportTotalTracks %d 0x%x", ctx->sId, trackNum);

    std::lock_guard<std::mutex> guard(ctx->mLock);
    char buffer[64];
    makeHeader(buffer, ctx->sId, SUBTITLE_TOTAL_TRACK, sizeof(trackNum));
    memcpy(buffer+HEADER_SIZE, &trackNum, sizeof(trackNum));
    fmqSendDataLocked(ctx, buffer, HEADER_SIZE + sizeof(trackNum));

    return SUB_STAT_OK;
}


SubSourceStatus SubSource_ReportType(SubSourceHandle handle, int type) {
    SubtitleContext *ctx = (SubtitleContext *)handle;
    if (ctx == nullptr) return SUB_STAT_INV;

    if (LOGIT) ALOGD("SubSource ReportType %d 0x%x", ctx->sId, type);

    std::lock_guard<std::mutex> guard(ctx->mLock);
    char buffer[64];
    makeHeader(buffer, ctx->sId, SUBTITLE_SUB_TYPE, sizeof(type));
    memcpy(buffer+HEADER_SIZE, &type, sizeof(type));
    fmqSendDataLocked(ctx, buffer, HEADER_SIZE + sizeof(type));

    return SUB_STAT_OK;
}


SubSourceStatus SubSource_ReportSubTypeString(SubSourceHandle handle, const char *type) {
    SubtitleContext *ctx = (SubtitleContext *)handle;
    std::lock_guard<std::mutex> guard(ctx->mLock);
    if (ctx == nullptr) return SUB_STAT_INV;

    char *buffer = new char[strlen(type)+1+HEADER_SIZE]();
    if (buffer == nullptr) return SUB_STAT_INV;

    if (LOGIT) ALOGD("SubSource ReportTypeString %d %s", ctx->sId, type);

    makeHeader(buffer, ctx->sId, SUBTITLE_TYPE_STRING, strlen(type)+1);
    memcpy(buffer+HEADER_SIZE, type, strlen(type));
    fmqSendDataLocked(ctx, buffer, HEADER_SIZE + strlen(type)+1);

    delete[] buffer;
    return SUB_STAT_OK;
}
SubSourceStatus SubSource_ReportLauguageString(SubSourceHandle handle, const char *lang) {
    SubtitleContext *ctx = (SubtitleContext *)handle;
    std::lock_guard<std::mutex> guard(ctx->mLock);
    if (ctx == nullptr) return SUB_STAT_INV;
    if (LOGIT) ALOGD("SubSource ReportLangString %d %s", ctx->sId, lang);

    char *buffer = new char[strlen(lang)+1+HEADER_SIZE]();
    if (buffer == nullptr) return SUB_STAT_INV;

    makeHeader(buffer, ctx->sId, SUBTITLE_LANG_STRING, strlen(lang)+1);
    memcpy(buffer+HEADER_SIZE, lang, strlen(lang));
    fmqSendDataLocked(ctx, buffer, HEADER_SIZE + strlen(lang)+1);

    delete[] buffer;
    return SUB_STAT_OK;
}


SubSourceStatus SubSource_SendData(SubSourceHandle handle, const char *data, int size) {
    SubtitleContext *ctx = (SubtitleContext *)handle;
    if (ctx == nullptr) return SUB_STAT_INV;
    if (LOGIT) ALOGD("SubSource SubSource_SendData %d %d", ctx->sId, size);

    std::lock_guard<std::mutex> guard(ctx->mLock);
    char buffer[64];
    makeHeader(buffer, ctx->sId, SUBTITLE_SUB_DATA, size);
    fmqSendDataLocked(ctx, buffer, HEADER_SIZE);
    fmqSendDataLocked(ctx, data, size);
    if (LOGIT) ALOGD("SubSource SubSource_SendData end %d %d", ctx->sId, size);

    return SUB_STAT_OK;
}

//}
