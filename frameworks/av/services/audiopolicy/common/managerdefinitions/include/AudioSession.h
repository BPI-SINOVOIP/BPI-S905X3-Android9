/*
 * Copyright (C) 2015 The Android Open Source Project
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

#pragma once

#include <system/audio.h>
#include <utils/Errors.h>
#include <utils/RefBase.h>
#include <utils/Errors.h>
#include <utils/KeyedVector.h>
#include <media/AudioPolicy.h>
#include <media/IAudioPolicyServiceClient.h>
#include "AudioSessionInfoProvider.h"

namespace android {

class AudioPolicyClientInterface;

class AudioSession : public RefBase, public AudioSessionInfoUpdateListener
{
public:
    AudioSession(audio_session_t session,
                 audio_source_t inputSource,
                 audio_format_t format,
                 uint32_t sampleRate,
                 audio_channel_mask_t channelMask,
                 audio_input_flags_t flags,
                 uid_t uid,
                 bool isSoundTrigger,
                 AudioMix* policyMix,
                 AudioPolicyClientInterface *clientInterface);

    status_t dump(int fd, int spaces, int index) const;

    audio_session_t session() const { return mRecordClientInfo.session; }
    audio_source_t inputSource()const { return mRecordClientInfo.source; }
    audio_format_t format() const { return mConfig.format; }
    uint32_t sampleRate() const { return mConfig.sample_rate; }
    audio_channel_mask_t channelMask() const { return mConfig.channel_mask; }
    audio_input_flags_t flags() const { return mFlags; }
    uid_t uid() const { return mRecordClientInfo.uid; }
    void setUid(uid_t uid) { mRecordClientInfo.uid = uid; }
    bool matches(const sp<AudioSession> &other) const;
    bool isSoundTrigger() const { return mIsSoundTrigger; }
    void setSilenced(bool silenced) { mSilenced = silenced; }
    bool isSilenced() const { return mSilenced; }
    uint32_t openCount() const { return mOpenCount; } ;
    uint32_t activeCount() const { return mActiveCount; } ;

    uint32_t changeOpenCount(int delta);
    uint32_t changeActiveCount(int delta);

    void setInfoProvider(AudioSessionInfoProvider *provider);
    // implementation of AudioSessionInfoUpdateListener
    virtual void onSessionInfoUpdate() const;

private:
    record_client_info_t mRecordClientInfo;
    const struct audio_config_base mConfig;
    const audio_input_flags_t mFlags;
    bool  mIsSoundTrigger;
    bool mSilenced;
    uint32_t  mOpenCount;
    uint32_t  mActiveCount;
    AudioMix* mPolicyMix; // non NULL when used by a dynamic policy
    AudioPolicyClientInterface* mClientInterface;
    const AudioSessionInfoProvider* mInfoProvider;
};

class AudioSessionCollection :
    public DefaultKeyedVector<audio_session_t, sp<AudioSession> >,
    public AudioSessionInfoUpdateListener
{
public:
    status_t addSession(audio_session_t session,
                             const sp<AudioSession>& audioSession,
                             AudioSessionInfoProvider *provider);

    status_t removeSession(audio_session_t session);

    uint32_t getOpenCount() const;

    AudioSessionCollection getActiveSessions() const;
    size_t getActiveSessionCount() const;
    bool hasActiveSession() const;
    bool isSourceActive(audio_source_t source) const;
    audio_source_t getHighestPrioritySource(bool activeOnly) const;

    // implementation of AudioSessionInfoUpdateListener
    virtual void onSessionInfoUpdate() const;

    status_t dump(int fd, int spaces) const;
};

} // namespace android
