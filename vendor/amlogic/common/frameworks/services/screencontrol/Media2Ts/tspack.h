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

#ifndef ANDROID_GUI_TSPACK_H
#define ANDROID_GUI_TSPACK_H

#include <media/MediaSource.h>
#include <media/stagefright/MediaBuffer.h>
#include <media/stagefright/MediaCodec.h>
#include <media/stagefright/AudioSource.h>

#include <hardware/hardware.h>
#include "../../../../../hardware/amlogic/screen_source/aml_screen.h"

#include <media/stagefright/foundation/AHandler.h>
#include <pthread.h>

#include "esconvertor.h"

namespace android {
// ----------------------------------------------------------------------------

class TSPacker : public MediaSource,
                                public MediaBufferObserver {
public:
    TSPacker(int width, int height, int frameRate, int bitRate, int sourceType, bool hasAudio);

    virtual ~TSPacker();

    // For the MediaSource interface for use by StageFrightRecorder:
    virtual status_t start(MetaData *params = NULL);
    virtual status_t stop();
    virtual status_t read(MediaBufferBase **buffer,
            const ReadOptions *options = NULL);
    virtual sp<MetaData> getFormat();

    // Get / Set the frame rate used for encoding. Default fps = 30
    status_t setFrameRate(int32_t fps) ;
    int32_t getFrameRate( ) const;

    // The call for the StageFrightRecorder to tell us that
    // it is done using the MediaBuffer data so that its state
    // can be set to FREE for dequeuing
    virtual void signalBufferReturned(MediaBufferBase* buffer);
    // end of MediaSource interface

    // getTimestamp retrieves the timestamp associated with the image
    // set by the most recent call to read()
    //
    // The timestamp is in nanoseconds, and is monotonically increasing. Its
    // other semantics (zero point, etc) are source-dependent and should be
    // documented by the source.
    int64_t getTimestamp();

    // isMetaDataStoredInVideoBuffers tells the encoder whether we will
    // pass metadata through the buffers. Currently, it is force set to true
    bool isMetaDataStoredInVideoBuffers() const;

    // To be called before start()
    status_t setMaxAcquiredBufferCount(size_t count);

    status_t packetize(
            bool isAudio, const char *buffer_add,
            int32_t buffer_size,
            sp<ABuffer> *packets,
            uint32_t flags,
            const uint8_t *PES_private_data, size_t PES_private_data_len,
            size_t numStuffingBytes, int64_t timeUs);
    void headFinalize();

    status_t incrementContinuityCounter(int isAudio);

private:
    mutable Mutex mMutex;
    int mFrameRate;
    int64_t mCurrentTimestamp;
    bool mStarted;
    int mWidth;
    int mHeight;
    int mSourceType;
    int mBitRate;
    bool mHasAudio;
    bool mIsPcmAudio;
    int mheadFinalize;

    sp<ESConvertor> mVideoConvertor;
    sp<ESConvertor> mAudioConvertor;

    pthread_t mThread;
    int threadFunc();
    static void *ThreadWrapper(void *me);
    enum {
        EMIT_PAT_AND_PMT                = 1,
        EMIT_PCR                        = 2,
        IS_ENCRYPTED                    = 4,
        PREPEND_SPS_PPS_TO_IDR_FRAMES   = 8,
    };
    List<sp<ABuffer> > mOutputBufferQueue;
    unsigned mPATContinuityCounter;
    unsigned mPMTContinuityCounter;
    unsigned mAudioContinuityCounter;
    unsigned mVideoContinuityCounter;

    enum {
        kPID_PMT = 0x100,
        kPID_PCR = 0x1000,
        kPID_VIDEO = 0x1100,
        kPID_AUDIO = 0x1110,
    };
    uint32_t mCrcTable[256];
    Vector<sp<ABuffer> > mCSD;
    Vector<sp<ABuffer> > mDescriptors;
    void initCrcTable();
    uint32_t crc32(const uint8_t *start, size_t size) const;
    int64_t mPrevTimeUs;
    int mFirstVideoFrame;
    int mFirstAudioFrame;

    int mDumpVideoEs;
    int mDumpVideoTs;
    int mDumpAudioEs;
    int mDumpAudioPCM;
    Vector<sp<ABuffer> > mProgramInfoDescriptors;
};

// ----------------------------------------------------------------------------
}; // namespace android

#endif // ANDROID_GUI_SURFACEMEDIASOURCE_H
