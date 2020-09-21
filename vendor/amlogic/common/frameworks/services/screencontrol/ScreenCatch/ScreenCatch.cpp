/*
 * Copyright (C) 2011 The Android Open Source Project
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
 */
#define LOG_NDEBUG 1
#define LOG_TAG "ScreenCatch"

#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MetaData.h>
#include <OMX_IVCommon.h>
#include <MetadataBufferType.h>

#include <ui/GraphicBuffer.h>
#include <gui/ISurfaceComposer.h>
//#include <gui/IGraphicBufferAlloc.h>
#include <OMX_Component.h>

#include <utils/Log.h>
#include <utils/String8.h>

#include <private/gui/ComposerService.h>

#include "ScreenCatch.h"

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

ScreenCatch::ScreenCatch(uint32_t bufferWidth, uint32_t bufferHeight, uint32_t bitSize, uint32_t type) :
    /*mWidth(ALIGN(bufferWidth)),*/
    mWidth(bufferWidth),
    mHeight(bufferHeight),
    mType(type),
    mScreenManager(NULL),
    mColorFormat(OMX_COLOR_Format32bitARGB8888){
    ALOGE("ScreenCatch: %dx%d", bufferWidth, bufferHeight);

    if (bufferWidth <= 0 || bufferHeight <= 0 || bufferWidth > 1920 || bufferHeight > 1080) {
        ALOGE("Invalid dimensions %dx%d", bufferWidth, bufferHeight);
    }

    mCorpX = -1;
    mCorpY = -1;
    mCorpWidth = -1;
    mCorpHeight = -1;
    mRawBufferQueue.clear();
}

ScreenCatch::~ScreenCatch() {
    ALOGE("~ScreenCatch");
}

void ScreenCatch::setVideoRotation(int degree)
{
    int angle;
    ALOGI("[%s %d] setVideoRotation degree:%x", __FUNCTION__, __LINE__, degree);
}

void ScreenCatch::setVideoCrop(int x, int y, int width, int height)
{
    mCorpX = x;
    mCorpY = y;
    mCorpWidth = width;
    mCorpHeight = height;
}

static inline void yuv_to_rgb32(unsigned char y,unsigned char u,unsigned char v,unsigned char *rgb)
{
    register int r,g,b;

    r = (1192 * (y - 16) + 1634 * (v - 128) ) >> 10;
    g = (1192 * (y - 16) - 833 * (v - 128) - 400 * (u -128) ) >> 10;
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
    int x,y,z=0;
    int h,w;
    int blocks;
    unsigned char Y1, Y2, U, V;

    blocks = (width * height) * 2;

    for (h=0, z=0; h< height; h+=2) {
        for (y = 0; y < width*2; y+=2) {

            Y1 = buf[ h*width + y + 0];
            V = buf[ blocks/2 + h*width/2 + y%width + 0 ];
            Y2 = buf[ h*width + y + 1];
            U = buf[ blocks/2 + h*width/2 + y%width + 1 ];

            yuv_to_rgb32(Y1, U, V, &rgb[z]);
            yuv_to_rgb32(Y2, U, V, &rgb[z + 4]);
            z+=8;
        }
    }
}

static inline void yuv_to_rgb24(unsigned char y,unsigned char u,unsigned char v,unsigned char *rgb)
{
    register int r,g,b;

    r = (1192 * (y - 16) + 1634 * (v - 128) ) >> 10;
    g = (1192 * (y - 16) - 833 * (v - 128) - 400 * (u -128) ) >> 10;
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
    int x,y,z=0;
    int h,w;
    int blocks;
    unsigned char Y1, Y2, U, V;

    blocks = (width * height) * 2;

    for (h=0, z=0; h< height; h+=2) {
        for (y = 0; y < width*2; y+=2) {

            Y1 = buf[ h*width + y + 0];
            V = buf[ blocks/2 + h*width/2 + y%width + 0 ];
            Y2 = buf[ h*width + y + 1];
            U = buf[ blocks/2 + h*width/2 + y%width + 1 ];

            yuv_to_rgb24(Y1, U, V, &rgb[z]);
            yuv_to_rgb24(Y2, U, V, &rgb[z + 3]);
            z+=6;
        }
    }
}

int ScreenCatch::threadFunc()
{
    int64_t pts;
    int status;

    sp<MemoryHeapBase> newMemoryHeap = new MemoryHeapBase(mWidth*mHeight*3/2);
    sp<MemoryBase> buffer = new MemoryBase(newMemoryHeap, 0, mWidth*mHeight*3/2);

    ALOGE("[%s %d] empty:%d", __FUNCTION__, __LINE__, mRawBufferQueue.empty());

    while (mStart == true) {
        status = mScreenManager->readBuffer(mClientId, buffer, &pts);

        if (status != OK && mStart == true) {
            usleep(100);
            continue;
        }

        if (mStart != true)
            break;

        {
            Mutex::Autolock autoLock(mLock);
            MediaBuffer* accessUnit;

            if (OMX_COLOR_Format24bitRGB888 == mColorFormat) {//rgb 24bit
                accessUnit = new MediaBuffer(mWidth*mHeight*3);
                if (accessUnit != NULL) {
                    nv21_to_rgb24((unsigned char *)buffer->pointer(), (unsigned char *)accessUnit->data(), mWidth, mHeight);
                    accessUnit->set_range(0, mWidth*mHeight*3);
                }
            } else if (OMX_COLOR_Format32bitARGB8888 == mColorFormat) {//rgba 32bit
                accessUnit = new MediaBuffer(mWidth*mHeight*4);
                if (accessUnit != NULL) {
                    nv21_to_rgb32((unsigned char *)buffer->pointer(), (unsigned char *)accessUnit->data(), mWidth, mHeight);
                    accessUnit->set_range(0, mWidth*mHeight*4);
                }
            } else if (OMX_COLOR_FormatYUV420SemiPlanar ==  mColorFormat){//nv21
                accessUnit = new MediaBuffer(mWidth*mHeight*3/2);
                if (accessUnit != NULL) {
                    memcpy((unsigned char *)accessUnit->data(), (unsigned char *)buffer->pointer(), mWidth*mHeight*3/2);
                    accessUnit->set_range(0, mWidth*mHeight*3/2);
                }
            }

            if (accessUnit != NULL)
                mRawBufferQueue.push_back(accessUnit);
        }
    }

    //buffer->decStrong(this);
    buffer.clear();
    //newMemoryHeap->decStrong(this);
    newMemoryHeap.clear();

    return 0;
}

void *ScreenCatch::ThreadWrapper(void *me) {
    ScreenCatch *Convertor = static_cast<ScreenCatch *>(me);
    Convertor->threadFunc();
    return NULL;
}

status_t ScreenCatch::start(MetaData *params)
{
    ALOGE("[%s %d] mWidth:%d mHeight:%d", __FUNCTION__, __LINE__, mWidth, mHeight);
    Mutex::Autolock autoLock(mLock);

    status_t status = 0;
    int64_t pts;
    int client_id;

    mScreenManager = ScreenManager::instantiate();
    ALOGE("[%s %d] mWidth:%d mHeight:%d", __FUNCTION__, __LINE__, mWidth, mHeight);

    mScreenManager->init(mWidth, mHeight, mType, 1, SCREENCONTROL_RAWDATA_TYPE, &client_id, NULL);

    ALOGE("[%s %d] client_id:%d, mType:%d", __FUNCTION__, __LINE__, client_id, mType);

    mClientId = client_id;

    if (status != OK) {
        ALOGE("setResolutionRatio fail");
        return !OK;
    }

    ALOGE("[%s %d] mCorpX:%d mCorpY:%d mCorpWidth:%d mCorpHeight:%d", __FUNCTION__, __LINE__,  mCorpX, mCorpY, mCorpWidth, mCorpHeight);

    if (mCorpX != -1)
        mScreenManager->setVideoCrop(client_id, mCorpX, mCorpY, mCorpWidth, mCorpHeight);

    status = mScreenManager->start(client_id);

    if (status != OK) {
        mScreenManager->uninit(mClientId);
        ALOGE("ScreenControlService start fail");
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

    ALOGE("[%s %d]", __FUNCTION__, __LINE__);
    return OK;
}

status_t ScreenCatch::stop()
{
    ALOGE("[%s %d]", __FUNCTION__, __LINE__);
    status_t ret = OK;
    Mutex::Autolock autoLock(mLock);
    mStart = false;
    void *dummy;
    pthread_join(mThread, &dummy);
    ALOGE("[%s %d]", __FUNCTION__, __LINE__);
    ret = static_cast<status_t>(reinterpret_cast<uintptr_t>(dummy));
    ALOGE("[%s %d], ret = %d", __FUNCTION__, __LINE__, ret);

    while (!mRawBufferQueue.empty()) {
		    ALOGE("[%s %d] free buffer", __FUNCTION__, __LINE__);
        MediaBuffer* rawBuffer = *mRawBufferQueue.begin();
        mRawBufferQueue.erase(mRawBufferQueue.begin());
        if (rawBuffer != NULL)
            rawBuffer->release();
    }

    mScreenManager->stop(mClientId);
    mScreenManager->uninit(mClientId);

    return ret;
}

status_t ScreenCatch::read(MediaBuffer **buffer)
{
    Mutex::Autolock autoLock(mLock);

    if (!mRawBufferQueue.empty()) {
        MediaBuffer* rawBuffer = *mRawBufferQueue.begin();
        mRawBufferQueue.erase(mRawBufferQueue.begin());
        *buffer = rawBuffer;
        return OK;
    }

    return !OK;
}

} // end of namespace android
