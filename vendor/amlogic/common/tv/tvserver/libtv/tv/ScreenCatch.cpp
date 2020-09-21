/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_NDEBUG 1
#define LOG_TAG "tvserver"
#define LOG_TV_TAG "ScreenCatch"

#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MetaData.h>
#include <OMX_IVCommon.h>
#include <MetadataBufferType.h>

#include <ui/GraphicBuffer.h>
#include <gui/ISurfaceComposer.h>
#include <gui/IGraphicBufferAlloc.h>
#include <OMX_Component.h>

#include <CTvLog.h>
#include <utils/String8.h>

#include <private/gui/ComposerService.h>

#include <media/stagefright/ScreenCatch.h>
#include <media/stagefright/ScreenSource.h>

#include <binder/IPCThreadState.h>
#include <binder/MemoryBase.h>
#include <binder/MemoryHeapBase.h>

#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <binder/IServiceManager.h>

#define BOUNDRY 32

#define ALIGN(x) (x + (BOUNDRY) - 1)& ~((BOUNDRY) - 1)

namespace android {

    struct ScreenCatch::ScreenCatchClient : public BnScreenMediaSourceClient {
        ScreenCatchClient(void *user)
        {
            mUser = user;
            LOGE("[%s %d] user:%x", __FUNCTION__, __LINE__, user);
        }

        virtual void notify(int msg, int ext1, int ext2, const Parcel *obj)
        {
            LOGI("notify %d, %d, %d", msg, ext1, ext2);
        }

        virtual int dataCallback(const sp<IMemory> &data)
        {
            return 0;
        }

    protected:
        void *mUser;
        virtual ~ScreenCatchClient() {}

    private:
        DISALLOW_EVIL_CONSTRUCTORS(ScreenCatchClient);
    };

    ScreenCatch::ScreenCatch(uint32_t bufferWidth, uint32_t bufferHeight, uint32_t bitSize) :
        mWidth(ALIGN(bufferWidth)),
        mHeight(bufferHeight),
        mScreenMediaSourceService(NULL),
        mColorFormat(OMX_COLOR_Format32bitARGB8888),
        mBitSize(bitSize)
    {
        LOGE("ScreenCatch: %dx%d", bufferWidth, bufferHeight);

        if (bufferWidth <= 0 || bufferHeight <= 0 || bufferWidth > 1920 || bufferHeight > 1080) {
            LOGE("Invalid dimensions %dx%d", bufferWidth, bufferHeight);
        }

        if (bitSize != 24 || bitSize != 32)
            bitSize = 32;

        mCorpX = -1;
        mCorpY = -1;
        mCorpWidth = -1;
        mCorpHeight = -1;

    }

    ScreenCatch::~ScreenCatch()
    {
        LOGE("~ScreenCatch");
    }

    void ScreenCatch::setVideoRotation(int degree)
    {
        int angle;

        LOGI("[%s %d] setVideoRotation degree:%x", __FUNCTION__, __LINE__, degree);

    }

    void ScreenCatch::setVideoCrop(int x, int y, int width, int height)
    {
        mCorpX = x;
        mCorpY = y;
        mCorpWidth = width;
        mCorpHeight = height;
    }

    static inline void yuv_to_rgb32(unsigned char y, unsigned char u, unsigned char v, unsigned char *rgb)
    {
        register int r, g, b;

        r = (1192 * (y - 16) + 1634 * (v - 128) ) >> 10;
        g = (1192 * (y - 16) - 833 * (v - 128) - 400 * (u - 128) ) >> 10;
        b = (1192 * (y - 16) + 2066 * (u - 128) ) >> 10;

        r = r > 255 ? 255 : r < 0 ? 0 : r;
        g = g > 255 ? 255 : g < 0 ? 0 : g;
        b = b > 255 ? 255 : b < 0 ? 0 : b;

        /*ARGB*/
        *rgb = (unsigned char)r;
        rgb++;
        *rgb = (unsigned char)g;
        rgb++;
        *rgb = (unsigned char)b;
        rgb++;
        *rgb = 0xff;

    }

    void nv21_to_rgb32(unsigned char *buf, unsigned char *rgb, int width, int height)
    {
        int x, y, z = 0;
        int h, w;
        int blocks;
        unsigned char Y1, Y2, U, V;

        blocks = (width * height) * 2;

        for (h = 0, z = 0; h < height; h += 2) {
            for (y = 0; y < width * 2; y += 2) {

                Y1 = buf[ h * width + y + 0];
                V = buf[ blocks / 2 + h * width / 2 + y % width + 0 ];
                Y2 = buf[ h * width + y + 1];
                U = buf[ blocks / 2 + h * width / 2 + y % width + 1 ];

                yuv_to_rgb32(Y1, U, V, &rgb[z]);
                yuv_to_rgb32(Y2, U, V, &rgb[z + 4]);
                z += 8;
            }
        }
    }

    static inline void yuv_to_rgb24(unsigned char y, unsigned char u, unsigned char v, unsigned char *rgb)
    {
        register int r, g, b;

        r = (1192 * (y - 16) + 1634 * (v - 128) ) >> 10;
        g = (1192 * (y - 16) - 833 * (v - 128) - 400 * (u - 128) ) >> 10;
        b = (1192 * (y - 16) + 2066 * (u - 128) ) >> 10;

        r = r > 255 ? 255 : r < 0 ? 0 : r;
        g = g > 255 ? 255 : g < 0 ? 0 : g;
        b = b > 255 ? 255 : b < 0 ? 0 : b;

        /*ARGB*/
        *rgb = (unsigned char)r;
        rgb++;
        *rgb = (unsigned char)g;
        rgb++;
        *rgb = (unsigned char)b;
    }

    void nv21_to_rgb24(unsigned char *buf, unsigned char *rgb, int width, int height)
    {
        int x, y, z = 0;
        int h, w;
        int blocks;
        unsigned char Y1, Y2, U, V;

        blocks = (width * height) * 2;

        for (h = 0, z = 0; h < height; h += 2) {
            for (y = 0; y < width * 2; y += 2) {

                Y1 = buf[ h * width + y + 0];
                V = buf[ blocks / 2 + h * width / 2 + y % width + 0 ];
                Y2 = buf[ h * width + y + 1];
                U = buf[ blocks / 2 + h * width / 2 + y % width + 1 ];

                yuv_to_rgb24(Y1, U, V, &rgb[z]);
                yuv_to_rgb24(Y2, U, V, &rgb[z + 3]);
                z += 6;
            }
        }
    }

    int ScreenCatch::threadFunc()
    {
        int64_t pts;
        int status;

        sp<MemoryHeapBase> newMemoryHeap = new MemoryHeapBase(mWidth * mHeight * 3 / 2);
        sp<MemoryBase> buffer = new MemoryBase(newMemoryHeap, 0, mWidth * mHeight * 3 / 2);

        LOGV("[%s %d] empty:%d", __FUNCTION__, __LINE__, mRawBufferQueue.empty());

        while (mStart == true) {

            status = mScreenMediaSourceService->readBuffer(mClientId, buffer, &pts);

            if (status != OK && mStart == true) {
                usleep(10000);
                continue;
            }

            if (mStart != true)
                break;

            MediaBuffer *accessUnit;

            if (OMX_COLOR_Format24bitRGB888 == mColorFormat) { //rgb 24bit
                accessUnit = new MediaBuffer(mWidth * mHeight * 3);
                nv21_to_rgb24((unsigned char *)buffer->pointer(), (unsigned char *)accessUnit->data(), mWidth, mHeight);
                accessUnit->set_range(0, mWidth * mHeight * 3);
            } else if (OMX_COLOR_Format32bitARGB8888 == mColorFormat) { //rgba 32bit
                accessUnit = new MediaBuffer(mWidth * mHeight * 4);
                nv21_to_rgb32((unsigned char *)buffer->pointer(), (unsigned char *)accessUnit->data(), mWidth, mHeight);
                accessUnit->set_range(0, mWidth * mHeight * 4);
            } else if (OMX_COLOR_FormatYUV420SemiPlanar ==  mColorFormat) { //nv21
                accessUnit = new MediaBuffer(mWidth * mHeight * 3 / 2);
                memcpy((unsigned char *)accessUnit->data(), (unsigned char *)buffer->pointer(), mWidth * mHeight * 3 / 2);
                accessUnit->set_range(0, mWidth * mHeight * 3 / 2);
            }
            mRawBufferQueue.push_back(accessUnit);
        }

        LOGE("[%s %d] thread out", __FUNCTION__, __LINE__);

        mThreadOutCondition.signal();

        return 0;
    }

    void *ScreenCatch::ThreadWrapper(void *me)
    {

        ScreenCatch *Convertor = static_cast<ScreenCatch *>(me);
        Convertor->threadFunc();
        return NULL;
    }

    status_t ScreenCatch::start(MetaData *params)
    {
        LOGE("[%s %d] mWidth:%d mHeight:%d", __FUNCTION__, __LINE__, mWidth, mHeight);
        AutoMutex _l(mLock);

        status_t status;
        int64_t pts;
        int client_id;

        sp<IServiceManager> sm = defaultServiceManager();
        sp<IBinder> binder = sm->getService(String16("media.screenmediasource"));
        mScreenMediaSourceService = interface_cast<IScreenMediaSource>(binder);

        sp<ScreenCatchClient> mIScreenSourceClient = new ScreenCatchClient(this);

        LOGE("[%s %d] mWidth:%d mHeight:%d", __FUNCTION__, __LINE__, mWidth, mHeight);

        mScreenMediaSourceService->registerClient(mIScreenSourceClient, mWidth, mHeight, 1, SCREENMEDIASOURC_RAWDATA_TYPE, &client_id, NULL);

        LOGE("[%s %d] client_id:%d", __FUNCTION__, __LINE__, client_id);

        mClientId = client_id;

        if (status != OK) {
            LOGE("setResolutionRatio fail");
            return !OK;
        }

        LOGV("[%s %d] mCorpX:%d mCorpY:%d mCorpWidth:%d mCorpHeight:%d", __FUNCTION__, __LINE__,  mCorpX, mCorpY, mCorpWidth, mCorpHeight);

        if (mCorpX != -1)
            mScreenMediaSourceService->setVideoCrop(client_id, mCorpX, mCorpY, mCorpWidth, mCorpHeight);


        status = mScreenMediaSourceService->start(client_id);

        if (status != OK) {
            mScreenMediaSourceService->unregisterClient(mClientId);
            LOGE("ScreenMediaSourceService start fail");
            return !OK;
        }

        if (!(params->findInt32(kKeyColorFormat, &mColorFormat)
                && (mColorFormat != OMX_COLOR_FormatYUV420SemiPlanar
                    || mColorFormat != OMX_COLOR_Format24bitRGB888
                    || mColorFormat != OMX_COLOR_Format32bitARGB8888)))
            mColorFormat = OMX_COLOR_Format32bitARGB8888;

        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
        pthread_create(&mThread, &attr, ThreadWrapper, this);
        pthread_attr_destroy(&attr);

        mStart = true;

        LOGV("[%s %d]", __FUNCTION__, __LINE__);
        return OK;
    }

    status_t ScreenCatch::stop()
    {
        LOGV("[%s %d]", __FUNCTION__, __LINE__);
        AutoMutex _l(mLock);
        mStart = false;

        mThreadOutCondition.waitRelative(mLock, 1000000000000);
        LOGV("[%s %d]", __FUNCTION__, __LINE__);

        while (!mRawBufferQueue.empty()) {

            LOGV("[%s %d] free buffer", __FUNCTION__, __LINE__);

            MediaBuffer *rawBuffer = *mRawBufferQueue.begin();
            mRawBufferQueue.erase(mRawBufferQueue.begin());
            rawBuffer->release();
        }

        mScreenMediaSourceService->stop(mClientId);
        mScreenMediaSourceService->unregisterClient(mClientId);

        return OK;
    }

    status_t ScreenCatch::read(MediaBuffer **buffer)
    {
        AutoMutex _l(mLock);

        if (!mRawBufferQueue.empty()) {
            MediaBuffer *rawBuffer = *mRawBufferQueue.begin();
            mRawBufferQueue.erase(mRawBufferQueue.begin());
            *buffer = rawBuffer;
            return OK;
        }

        return !OK;
    }

    status_t ScreenCatch::free(MediaBuffer *buffer)
    {
        AutoMutex _l(mLock);
        buffer->release();
        return OK;
    }

} // end of namespace android
