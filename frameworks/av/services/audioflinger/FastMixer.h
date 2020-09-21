/*
 * Copyright (C) 2012 The Android Open Source Project
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

#ifndef ANDROID_AUDIO_FAST_MIXER_H
#define ANDROID_AUDIO_FAST_MIXER_H

#include <atomic>
#include "FastThread.h"
#include "StateQueue.h"
#include "FastMixerState.h"
#include "FastMixerDumpState.h"

namespace android {

class AudioMixer;

typedef StateQueue<FastMixerState> FastMixerStateQueue;

class FastMixer : public FastThread {

public:
            FastMixer();
    virtual ~FastMixer();

            FastMixerStateQueue* sq();

    virtual void setMasterMono(bool mono) { mMasterMono.store(mono); /* memory_order_seq_cst */ }
    virtual void setBoottimeOffset(int64_t boottimeOffset) {
        mBoottimeOffset.store(boottimeOffset); /* memory_order_seq_cst */
    }
private:
            FastMixerStateQueue mSQ;

    // callouts
    virtual const FastThreadState *poll();
    virtual void setNBLogWriter(NBLog::Writer *logWriter);
    virtual void onIdle();
    virtual void onExit();
    virtual bool isSubClassCommand(FastThreadState::Command command);
    virtual void onStateChange();
    virtual void onWork();

    // FIXME these former local variables need comments
    static const FastMixerState sInitial;

    FastMixerState  mPreIdle;   // copy of state before we went into idle
    int             mGenerations[FastMixerState::kMaxFastTracks];
                                // last observed mFastTracks[i].mGeneration
    NBAIO_Sink*     mOutputSink;
    int             mOutputSinkGen;
    AudioMixer*     mMixer;

    // mSinkBuffer audio format is stored in format.mFormat.
    void*           mSinkBuffer;        // used for mixer output format translation
                                        // if sink format is different than mixer output.
    size_t          mSinkBufferSize;
    uint32_t        mSinkChannelCount;
    audio_channel_mask_t mSinkChannelMask;
    void*           mMixerBuffer;       // mixer output buffer.
    size_t          mMixerBufferSize;
    audio_format_t  mMixerBufferFormat; // mixer output format: AUDIO_FORMAT_PCM_(16_BIT|FLOAT).

    enum {UNDEFINED, MIXED, ZEROED} mMixerBufferState;
    NBAIO_Format    mFormat;
    unsigned        mSampleRate;
    int             mFastTracksGen;
    FastMixerDumpState mDummyFastMixerDumpState;
    int64_t         mTotalNativeFramesWritten;  // copied to dumpState->mFramesWritten

    // next 2 fields are valid only when timestampStatus == NO_ERROR
    ExtendedTimestamp mTimestamp;
    int64_t         mNativeFramesWrittenButNotPresented;

    // accessed without lock between multiple threads.
    std::atomic_bool mMasterMono;
    std::atomic_int_fast64_t mBoottimeOffset;
};  // class FastMixer

}   // namespace android

#endif  // ANDROID_AUDIO_FAST_MIXER_H
