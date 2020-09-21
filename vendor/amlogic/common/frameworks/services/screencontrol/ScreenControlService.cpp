/** @file ScreenControlService.cpp
 *  @par Copyright:
 *  - Copyright 2011 Amlogic Inc as unpublished work
 *  All Rights Reserved
 *  - The information contained herein is the confidential property
 *  of Amlogic.  The use, copying, transfer or disclosure of such information
 *  is prohibited except by express written agreement with Amlogic Inc.
 *  @author   liangzhuo xie
 *  @version  1.0
 *  @date     2018/08/18
 *  @par function description:
 *  - screen capture
 *  - screen record
 *  @warning This class may explode in your face.
 *  @note If you inherit anything from this class, you're doomed.
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "ScreenControlService"

#include "ScreenControlService.h"
#include <stdlib.h>
#include <string.h>
#include <utils/Errors.h>
#include <utils/Timers.h>

#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MetaData.h>
#include <OMX_IVCommon.h>
#include <MetadataBufferType.h>

#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <binder/MemoryHeapBase.h>
#include <binder/MemoryBase.h>

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <media/stagefright/MediaBuffer.h>

#include <Media2Ts/tspack.h>

#include <gui/ISurfaceComposer.h>
#include <OMX_Component.h>

#include <utils/Log.h>
#include <utils/String8.h>

#include <private/gui/ComposerService.h>

#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <linux/videodev2.h>
#include <hardware/hardware.h>

#include <ScreenCatch/ScreenCatch.h>
#include <ui/PixelFormat.h>
#include <ui/DisplayInfo.h>

#include <system/graphics.h>

#include <SkImageEncoder.h>
#include <SkData.h>
#include <SkColorSpace.h>

namespace android {
class DeathNotifier: public IBinder::DeathRecipient
{
    public:
        DeathNotifier(sp<ScreenControlService> screencontrolservice) {
            mScreenControlService = screencontrolservice;
        }

        void binderDied(const wp<IBinder>& who) {
            ALOGW("native screen control client binder died!");
            mScreenControlService->release();
        }
    private:
        sp<ScreenControlService> mScreenControlService;
};
} // namespace android

namespace android {

ScreenControlService::ScreenControlService() {
}

ScreenControlService::~ScreenControlService() {
    ALOGE("~ScreenControlService");
}

ScreenControlService* ScreenControlService::getInstance() {
    ScreenControlService *mScreenControl = new ScreenControlService();
    return mScreenControl;
}

void ScreenControlService::instantiate() {
    android::status_t ret = defaultServiceManager()->addService(
            String16("screen_control"), new ScreenControlService());

    if (ret != android::OK) {
        ALOGE("Couldn't register screen_control service!");
    }
    ALOGI("instantiate add service result:%d", ret);

}

int ScreenControlService::startScreenRecord(int32_t width, int32_t height, int32_t frameRate, int32_t bitRate, int32_t limitTimeSec, int32_t sourceType, const char* filename) {
    ALOGI("startScreenRecord width:%d, height:%d, frameRate:%d, bitRate:%d, limitTimeSec:%d, sourceType:%d, filename:%s\n", width, height, frameRate, bitRate, limitTimeSec, sourceType, filename);

    int err;
    int video_dump_size = 0;
    int32_t dump_time = 0;
    int32_t limit_time = limitTimeSec * frameRate;
    MediaBufferBase *tVideoBuffer;

    ProcessState::self()->startThreadPool();

    int video_file = open(filename, O_CREAT | O_RDWR, 0666);

    sp<TSPacker> mTSPacker = new TSPacker(width, height, frameRate, bitRate, sourceType, 0);
    err = mTSPacker->start();

    if (err != OK) {
        ALOGE("[%s %d]\n", __FUNCTION__, __LINE__);
        return OK;
    }

    while (1) {
        tVideoBuffer = NULL;
        err = mTSPacker->read(&tVideoBuffer);

        if (err != OK) {
            usleep(1);
            continue;
        }

	dump_time++;

        write(video_file, tVideoBuffer->data(),tVideoBuffer->range_length());
        video_dump_size += tVideoBuffer->range_length();
        ALOGI("[%s %d] video limit_time:%d dump_time:%d size:%d dump_size:%d\n", __FUNCTION__, __LINE__, limit_time, dump_time, tVideoBuffer->range_length(), video_dump_size);

        tVideoBuffer->release();
        tVideoBuffer = NULL;

	if (dump_time > limit_time)
	    break;
    }

    ALOGI("tspacker stop\n");

    mTSPacker->stop();
    close(video_file);
    return OK;
}

int ScreenControlService::startScreenCap(int32_t left, int32_t top, int32_t right, int32_t bottom, int32_t width, int32_t height, int32_t sourceType, const char* filename) {
    ALOGI("startScreenCap left:%d, top:%d, right:%d, bottom:%d, width:%d, height:%d, sourceType:%d, filename:%s\n", left, top, right, bottom, width, height, sourceType, filename);

    int status;
    int dumpfd;
    int count = 0;
    uint32_t f = PIXEL_FORMAT_RGBA_8888;
    status_t result = NO_ERROR;
    ScreenCatch* mScreenCatch;
    const size_t size = width * height * 4;

    //ProcessState::self()->startThreadPool();

    sp<MemoryHeapBase> memoryBase(new MemoryHeapBase(size, 0, "screen-capture"));
    void* const base = memoryBase->getBase();

    if (base != MAP_FAILED) {
        mScreenCatch = new ScreenCatch(width, height, 32, sourceType);
        mScreenCatch->setVideoCrop(left, top, right, bottom);

        MetaData* pMeta;
        pMeta = new MetaData();
        pMeta->setInt32(kKeyColorFormat, OMX_COLOR_Format32bitARGB8888);
        mScreenCatch->start(pMeta);

        MediaBuffer *buffer;

        while (count < 1) {
            status = mScreenCatch->read(&buffer);
            if (status != OK) {
                usleep(100);
                continue;
            }

            count++;
            dumpfd = open(filename, O_CREAT | O_RDWR | O_TRUNC, 0644);
            ALOGE("[%s %d] dump:%s size:%d", __FUNCTION__, __LINE__, filename, buffer->size());
            memcpy(base, buffer->data(), buffer->size());

            const SkImageInfo info = SkImageInfo::Make(width, height, flinger2skia(f), kPremul_SkAlphaType, nullptr);
            SkPixmap pixmap(info, base, width * bytesPerPixel(f));
            struct FDWStream final : public SkWStream {
                size_t fBytesWritten = 0;
                int fFd;
                FDWStream(int f) : fFd(f) {}
                size_t bytesWritten() const override {
                    return fBytesWritten;
                }
                bool write(const void* buffer, size_t size) override {
                    fBytesWritten += size;
                    return size == 0 || ::write(fFd, buffer, size) > 0;
                }
            } fdStream(dumpfd);

            (void)SkEncodeImage(&fdStream, pixmap, SkEncodedImageFormat::kJPEG, 100);

            close(dumpfd);
            buffer->release();
            buffer = NULL;
        }

        memoryBase.clear();
        mScreenCatch->stop();
        pMeta->clear();
        delete mScreenCatch;
    } else {
        result = UNKNOWN_ERROR;
    }

    ALOGE("[%s %d] startScreenCap finish", __FUNCTION__, __LINE__);
    return result;
}

int ScreenControlService::startScreenCapBuffer(int32_t left, int32_t top, int32_t right, int32_t bottom, int32_t width, int32_t height, int32_t sourceType, void *dstBuffer, int32_t *dstBufferSize) {
    ALOGI("[%s] left:%d, top:%d, right:%d, bottom:%d, width:%d, height:%d, sourceType:%d\n",
		__func__, left, top, right, bottom, width, height, sourceType);

    int status;
    int count = 0;
    uint32_t f = PIXEL_FORMAT_RGBA_8888;
    status_t result = NO_ERROR;
    ScreenCatch* mScreenCatch;
    const size_t size = width * height * 4;

    mScreenCatch = new ScreenCatch(width, height, 32, sourceType);
    mScreenCatch->setVideoCrop(left, top, right, bottom);

    MetaData* pMeta;
    pMeta = new MetaData();
    pMeta->setInt32(kKeyColorFormat, OMX_COLOR_Format32bitARGB8888);
    mScreenCatch->start(pMeta);

    MediaBuffer *buffer;

    while (count < 1) {
        status = mScreenCatch->read(&buffer);
        if (status != OK) {
            usleep(50);
            continue;
        }

        count++;
        ALOGE("[%s %d] readed size:%d", __FUNCTION__, __LINE__, buffer->size());
		memcpy(dstBuffer, buffer->data(), buffer->size());
		*dstBufferSize = buffer->size();

        buffer->release();
        buffer = NULL;
    }

    mScreenCatch->stop();
    pMeta->clear();
    delete mScreenCatch;
	
    return result;
}


SkColorType ScreenControlService::flinger2skia(PixelFormat f) {
    switch (f) {
        case PIXEL_FORMAT_RGB_565:
            return kRGB_565_SkColorType;
        default:
            return kN32_SkColorType;
    }
}

int ScreenControlService::notifyProcessDied (const sp<IBinder> &binder) {
    ALOGI("notifyProcessDied");
    if (binder == NULL) {
        ALOGE("notifyProcessDied binder is NULL");
        return -1;
    }
    binder->linkToDeath(mDeathNotifier);
    return NO_ERROR;
}

int ScreenControlService::release() {
    ALOGI("release");
    return NO_ERROR;
}

} // namespace android
