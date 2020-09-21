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

#ifndef ANDROID_GUI_ESCONVERTOR_H
#define ANDROID_GUI_ESCONVERTOR_H

#include <media/MediaSource.h>
#include <media/stagefright/MediaBuffer.h>
#include <media/stagefright/MediaCodec.h>
#include <media/stagefright/AudioSource.h>

#include <media/stagefright/foundation/AHandler.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <media/MediaCodecBuffer.h>
#include <pthread.h>

#include <binder/MemoryDealer.h>
#include <binder/MemoryBase.h>
#include <binder/MemoryHeapBase.h>
#include "../ScreenManager.h"

namespace android {
// ----------------------------------------------------------------------------

class ESConvertor : public MediaSource,
                                public MediaBufferObserver {
public:
    ESConvertor(int sourceType, int IsAudio);

    virtual ~ESConvertor();

    // For the MediaSource interface for use by StageFrightRecorder:
    virtual status_t start(MetaData *params = NULL);
    virtual status_t stop();
    virtual status_t read(MediaBufferBase **buffer,
            const ReadOptions *options = NULL);
    virtual sp<MetaData> getFormat();

    void setVideoCrop(int x, int y, int width, int height);

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

    // To be called before start()
    status_t setUseAbsoluteTimestamps();

    int CanvasdataCallBack(const sp<IMemory>& data);

private:
    enum {
        kWhatDoMoreWork,
        kWhatRequestIDRFrame,
        kWhatShutdown,
        kWhatMediaPullerNotify,
        kWhatEncoderActivity,
    };
    Mutex mLock;
    typedef struct FrameBufferInfo_s{
        unsigned char* buf_ptr;
        unsigned canvas;
        int64_t timestampUs;
    }FrameBufferInfo;

    pthread_t mThread;

    status_t feedEncoderInputBuffers();
    status_t initEncoder();
    sp<ABuffer> prependCSD(sp<ABuffer> &accessUnit, sp<ABuffer> CSDBuffer);
    sp<ABuffer> prependStartCode(const sp<ABuffer> &accessUnit) const;
    static void *ThreadWrapper(void *me);
    sp<ABuffer> prependADTSHeader( const sp<ABuffer> &accessUnit) const;
    int threadFunc();
    int threadAudioFunc();
    int threadVideoFunc();
    int videoDequeueInputBuffer();
    int videoFeedInputBuffer();
    int videoSwEncoderFeedInputBuffer();
    int videoDequeueOutputBuffer();
    int mWidth;
    int mHeight;
    int mSourceType;
    int mVideoFrameRate;
    int mVIdeoBitRate;
    bool mDoMoreWorkPending;
    bool mIsSoftwareEncoder;
    sp<MetaData> meta;

    int mIsAudio;
    int mIsPCMAudio;
    sp<AMessage> mInputFormat;
    sp<AMessage> mOutputFormat;
    sp<AMessage> mEncoderActivityNotify;
    int64_t mStartTimeNs;
    int mAudioChannelCount;
    int mAudioSampleRate;
    int mDumpYuvFd;
    int mDumpEsFd;

    Vector<sp<ABuffer> > mCSDADTS;

    size_t mMaxAcquiredBufferCount;
    bool mUseAbsoluteTimestamps;
    mutable Mutex mMutex;
    int32_t mFrameRate;
    int64_t mCurrentTimestamp;
    bool mStarted;
    int32_t mStoped;
    sp<ABuffer> mPartialAudioAU;
    sp<AudioSource> mAudioSource;

    sp<MediaCodec> mEncoder;
    Vector<sp<MediaCodecBuffer> > mEncoderInputBuffers;
    Vector<sp<MediaCodecBuffer> > mEncoderOutputBuffers;

    List<size_t> mAvailEncoderInputIndices;

    List<sp<ABuffer> > mInputBufferQueue;
    List<sp<ABuffer> > mOutputBufferQueue;
    List<MediaBuffer*> mFramesReceived;
    sp<IGraphicBufferProducer> mGraphicBufferProducer;

    int32_t mClientId;
    sp<MemoryHeapBase> mNewMemoryHeap;
    sp<MemoryBase> mBufferGet;
    sp<MemoryBase> mBufferRelease;
    sp<ABuffer> mCSDbuffer;
    ScreenManager* mScreenManager;
    int64_t mDequeueBufferTotal;
    int64_t mQueueBufferTotal;

    int mEscDumpAAC;
    int mEscDumpPcm;
    int mEscDumpVideo;

    int32_t mCorpX;
    int32_t mCorpY;
    int32_t mCorpWidth;
    int32_t mCorpHeight;

    Condition mThreadOutCondition;
    int32_t mVideoFrameRemain;
};

// ----------------------------------------------------------------------------
}; // namespace android

#endif // ANDROID_GUI_SURFACEMEDIASOURCE_H
