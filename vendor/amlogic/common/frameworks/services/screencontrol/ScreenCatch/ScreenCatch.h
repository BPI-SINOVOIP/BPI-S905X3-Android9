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

#ifndef ANDROID_GUI_SCREENCATCH_H
#define ANDROID_GUI_SCREENCATCH_H

#include <media/MediaSource.h>
#include <media/stagefright/MediaBuffer.h>

#include <utils/List.h>
#include <utils/RefBase.h>
#include <utils/threads.h>

#include <binder/MemoryDealer.h>

#include "../ScreenManager.h"

namespace android {
// ----------------------------------------------------------------------------

class ScreenCatch {
public:
    ScreenCatch(uint32_t bufferWidth, uint32_t bufferHeight, uint32_t bitSize, uint32_t type);

    virtual ~ScreenCatch();

    // For the MediaSource interface for use by StageFrightRecorder:
    virtual status_t start(MetaData *params);
    virtual status_t stop();
    virtual status_t read(MediaBuffer **buffer);

    void setVideoRotation(int degree);

    void setVideoCrop(int x, int y, int width, int height);

private:
    int mStart;
    int mClientId;
    Mutex mLock;
    struct ScreenCatchClient;
    static void *ThreadWrapper(void *me);
    int threadFunc();
    pthread_t mThread;

    ScreenManager* mScreenManager;

    // The permenent width and height of SMS buffers
    int mWidth;
    int mHeight;
    int mType;
    int mWidthDst;
    int mHeightDst;
    int mColorFormat;

    int32_t mCorpX;
    int32_t mCorpY;
    int32_t mCorpWidth;
    int32_t mCorpHeight;
    List<MediaBuffer*> mRawBufferQueue;
    Condition mThreadOutCondition;
};

// ----------------------------------------------------------------------------
}; // namespace android

#endif // ANDROID_GUI_SURFACEMEDIASOURCE_H
