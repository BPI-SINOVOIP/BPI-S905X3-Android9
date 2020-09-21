/*
**
** Copyright 2018, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

//#define LOG_NDEBUG 0
#define LOG_TAG "MediaPlayer2AudioOutput"
#include <mediaplayer2/MediaPlayer2AudioOutput.h>

#include <cutils/properties.h> // for property_get
#include <utils/Log.h>

#include <media/AudioPolicyHelper.h>
#include <media/AudioTrack.h>
#include <media/stagefright/foundation/ADebug.h>

namespace {

const float kMaxRequiredSpeed = 8.0f; // for PCM tracks allow up to 8x speedup.

} // anonymous namespace

namespace android {

// TODO: Find real cause of Audio/Video delay in PV framework and remove this workaround
/* static */ int MediaPlayer2AudioOutput::mMinBufferCount = 4;
/* static */ bool MediaPlayer2AudioOutput::mIsOnEmulator = false;

status_t MediaPlayer2AudioOutput::dump(int fd, const Vector<String16>& args) const {
    const size_t SIZE = 256;
    char buffer[SIZE];
    String8 result;

    result.append(" MediaPlayer2AudioOutput\n");
    snprintf(buffer, 255, "  stream type(%d), left - right volume(%f, %f)\n",
            mStreamType, mLeftVolume, mRightVolume);
    result.append(buffer);
    snprintf(buffer, 255, "  msec per frame(%f), latency (%d)\n",
            mMsecsPerFrame, (mTrack != 0) ? mTrack->latency() : -1);
    result.append(buffer);
    snprintf(buffer, 255, "  aux effect id(%d), send level (%f)\n",
            mAuxEffectId, mSendLevel);
    result.append(buffer);

    ::write(fd, result.string(), result.size());
    if (mTrack != 0) {
        mTrack->dump(fd, args);
    }
    return NO_ERROR;
}

MediaPlayer2AudioOutput::MediaPlayer2AudioOutput(audio_session_t sessionId, uid_t uid, int pid,
        const audio_attributes_t* attr, const sp<AudioSystem::AudioDeviceCallback>& deviceCallback)
    : mCallback(NULL),
      mCallbackCookie(NULL),
      mCallbackData(NULL),
      mStreamType(AUDIO_STREAM_MUSIC),
      mLeftVolume(1.0),
      mRightVolume(1.0),
      mPlaybackRate(AUDIO_PLAYBACK_RATE_DEFAULT),
      mSampleRateHz(0),
      mMsecsPerFrame(0),
      mFrameSize(0),
      mSessionId(sessionId),
      mUid(uid),
      mPid(pid),
      mSendLevel(0.0),
      mAuxEffectId(0),
      mFlags(AUDIO_OUTPUT_FLAG_NONE),
      mSelectedDeviceId(AUDIO_PORT_HANDLE_NONE),
      mRoutedDeviceId(AUDIO_PORT_HANDLE_NONE),
      mDeviceCallbackEnabled(false),
      mDeviceCallback(deviceCallback) {
    ALOGV("MediaPlayer2AudioOutput(%d)", sessionId);
    if (attr != NULL) {
        mAttributes = (audio_attributes_t *) calloc(1, sizeof(audio_attributes_t));
        if (mAttributes != NULL) {
            memcpy(mAttributes, attr, sizeof(audio_attributes_t));
            mStreamType = audio_attributes_to_stream_type(attr);
        }
    } else {
        mAttributes = NULL;
    }

    setMinBufferCount();
}

MediaPlayer2AudioOutput::~MediaPlayer2AudioOutput() {
    close();
    free(mAttributes);
    delete mCallbackData;
}

//static
void MediaPlayer2AudioOutput::setMinBufferCount() {
    char value[PROPERTY_VALUE_MAX];
    if (property_get("ro.kernel.qemu", value, 0)) {
        mIsOnEmulator = true;
        mMinBufferCount = 12;  // to prevent systematic buffer underrun for emulator
    }
}

// static
bool MediaPlayer2AudioOutput::isOnEmulator() {
    setMinBufferCount();  // benign race wrt other threads
    return mIsOnEmulator;
}

// static
int MediaPlayer2AudioOutput::getMinBufferCount() {
    setMinBufferCount();  // benign race wrt other threads
    return mMinBufferCount;
}

ssize_t MediaPlayer2AudioOutput::bufferSize() const {
    Mutex::Autolock lock(mLock);
    if (mTrack == 0) {
        return NO_INIT;
    }
    return mTrack->frameCount() * mFrameSize;
}

ssize_t MediaPlayer2AudioOutput::frameCount() const {
    Mutex::Autolock lock(mLock);
    if (mTrack == 0) {
        return NO_INIT;
    }
    return mTrack->frameCount();
}

ssize_t MediaPlayer2AudioOutput::channelCount() const {
    Mutex::Autolock lock(mLock);
    if (mTrack == 0) {
        return NO_INIT;
    }
    return mTrack->channelCount();
}

ssize_t MediaPlayer2AudioOutput::frameSize() const {
    Mutex::Autolock lock(mLock);
    if (mTrack == 0) {
        return NO_INIT;
    }
    return mFrameSize;
}

uint32_t MediaPlayer2AudioOutput::latency () const {
    Mutex::Autolock lock(mLock);
    if (mTrack == 0) {
        return 0;
    }
    return mTrack->latency();
}

float MediaPlayer2AudioOutput::msecsPerFrame() const {
    Mutex::Autolock lock(mLock);
    return mMsecsPerFrame;
}

status_t MediaPlayer2AudioOutput::getPosition(uint32_t *position) const {
    Mutex::Autolock lock(mLock);
    if (mTrack == 0) {
        return NO_INIT;
    }
    return mTrack->getPosition(position);
}

status_t MediaPlayer2AudioOutput::getTimestamp(AudioTimestamp &ts) const {
    Mutex::Autolock lock(mLock);
    if (mTrack == 0) {
        return NO_INIT;
    }
    return mTrack->getTimestamp(ts);
}

// TODO: Remove unnecessary calls to getPlayedOutDurationUs()
// as it acquires locks and may query the audio driver.
//
// Some calls could conceivably retrieve extrapolated data instead of
// accessing getTimestamp() or getPosition() every time a data buffer with
// a media time is received.
//
// Calculate duration of played samples if played at normal rate (i.e., 1.0).
int64_t MediaPlayer2AudioOutput::getPlayedOutDurationUs(int64_t nowUs) const {
    Mutex::Autolock lock(mLock);
    if (mTrack == 0 || mSampleRateHz == 0) {
        return 0;
    }

    uint32_t numFramesPlayed;
    int64_t numFramesPlayedAtUs;
    AudioTimestamp ts;

    status_t res = mTrack->getTimestamp(ts);
    if (res == OK) {                 // case 1: mixing audio tracks and offloaded tracks.
        numFramesPlayed = ts.mPosition;
        numFramesPlayedAtUs = ts.mTime.tv_sec * 1000000LL + ts.mTime.tv_nsec / 1000;
        //ALOGD("getTimestamp: OK %d %lld", numFramesPlayed, (long long)numFramesPlayedAtUs);
    } else if (res == WOULD_BLOCK) { // case 2: transitory state on start of a new track
        numFramesPlayed = 0;
        numFramesPlayedAtUs = nowUs;
        //ALOGD("getTimestamp: WOULD_BLOCK %d %lld",
        //        numFramesPlayed, (long long)numFramesPlayedAtUs);
    } else {                         // case 3: transitory at new track or audio fast tracks.
        res = mTrack->getPosition(&numFramesPlayed);
        CHECK_EQ(res, (status_t)OK);
        numFramesPlayedAtUs = nowUs;
        numFramesPlayedAtUs += 1000LL * mTrack->latency() / 2; /* XXX */
        //ALOGD("getPosition: %u %lld", numFramesPlayed, (long long)numFramesPlayedAtUs);
    }

    // CHECK_EQ(numFramesPlayed & (1 << 31), 0);  // can't be negative until 12.4 hrs, test
    // TODO: remove the (int32_t) casting below as it may overflow at 12.4 hours.
    int64_t durationUs = (int64_t)((int32_t)numFramesPlayed * 1000000LL / mSampleRateHz)
            + nowUs - numFramesPlayedAtUs;
    if (durationUs < 0) {
        // Occurs when numFramesPlayed position is very small and the following:
        // (1) In case 1, the time nowUs is computed before getTimestamp() is called and
        //     numFramesPlayedAtUs is greater than nowUs by time more than numFramesPlayed.
        // (2) In case 3, using getPosition and adding mAudioSink->latency() to
        //     numFramesPlayedAtUs, by a time amount greater than numFramesPlayed.
        //
        // Both of these are transitory conditions.
        ALOGV("getPlayedOutDurationUs: negative duration %lld set to zero", (long long)durationUs);
        durationUs = 0;
    }
    ALOGV("getPlayedOutDurationUs(%lld) nowUs(%lld) frames(%u) framesAt(%lld)",
            (long long)durationUs, (long long)nowUs,
            numFramesPlayed, (long long)numFramesPlayedAtUs);
    return durationUs;
}

status_t MediaPlayer2AudioOutput::getFramesWritten(uint32_t *frameswritten) const {
    Mutex::Autolock lock(mLock);
    if (mTrack == 0) {
        return NO_INIT;
    }
    ExtendedTimestamp ets;
    status_t status = mTrack->getTimestamp(&ets);
    if (status == OK || status == WOULD_BLOCK) {
        *frameswritten = (uint32_t)ets.mPosition[ExtendedTimestamp::LOCATION_CLIENT];
    }
    return status;
}

status_t MediaPlayer2AudioOutput::setParameters(const String8& keyValuePairs) {
    Mutex::Autolock lock(mLock);
    if (mTrack == 0) {
        return NO_INIT;
    }
    return mTrack->setParameters(keyValuePairs);
}

String8  MediaPlayer2AudioOutput::getParameters(const String8& keys) {
    Mutex::Autolock lock(mLock);
    if (mTrack == 0) {
        return String8::empty();
    }
    return mTrack->getParameters(keys);
}

void MediaPlayer2AudioOutput::setAudioAttributes(const audio_attributes_t * attributes) {
    Mutex::Autolock lock(mLock);
    if (attributes == NULL) {
        free(mAttributes);
        mAttributes = NULL;
    } else {
        if (mAttributes == NULL) {
            mAttributes = (audio_attributes_t *) calloc(1, sizeof(audio_attributes_t));
        }
        memcpy(mAttributes, attributes, sizeof(audio_attributes_t));
        mStreamType = audio_attributes_to_stream_type(attributes);
    }
}

void MediaPlayer2AudioOutput::setAudioStreamType(audio_stream_type_t streamType) {
    Mutex::Autolock lock(mLock);
    // do not allow direct stream type modification if attributes have been set
    if (mAttributes == NULL) {
        mStreamType = streamType;
    }
}

void MediaPlayer2AudioOutput::close_l() {
    mTrack.clear();
}

status_t MediaPlayer2AudioOutput::open(
        uint32_t sampleRate, int channelCount, audio_channel_mask_t channelMask,
        audio_format_t format, int bufferCount,
        AudioCallback cb, void *cookie,
        audio_output_flags_t flags,
        const audio_offload_info_t *offloadInfo,
        bool doNotReconnect,
        uint32_t suggestedFrameCount) {
    ALOGV("open(%u, %d, 0x%x, 0x%x, %d, %d 0x%x)", sampleRate, channelCount, channelMask,
                format, bufferCount, mSessionId, flags);

    // offloading is only supported in callback mode for now.
    // offloadInfo must be present if offload flag is set
    if (((flags & AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD) != 0) &&
            ((cb == NULL) || (offloadInfo == NULL))) {
        return BAD_VALUE;
    }

    // compute frame count for the AudioTrack internal buffer
    size_t frameCount;
    if ((flags & AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD) != 0) {
        frameCount = 0; // AudioTrack will get frame count from AudioFlinger
    } else {
        // try to estimate the buffer processing fetch size from AudioFlinger.
        // framesPerBuffer is approximate and generally correct, except when it's not :-).
        uint32_t afSampleRate;
        size_t afFrameCount;
        if (AudioSystem::getOutputFrameCount(&afFrameCount, mStreamType) != NO_ERROR) {
            return NO_INIT;
        }
        if (AudioSystem::getOutputSamplingRate(&afSampleRate, mStreamType) != NO_ERROR) {
            return NO_INIT;
        }
        const size_t framesPerBuffer =
                (unsigned long long)sampleRate * afFrameCount / afSampleRate;

        if (bufferCount == 0) {
            // use suggestedFrameCount
            bufferCount = (suggestedFrameCount + framesPerBuffer - 1) / framesPerBuffer;
        }
        // Check argument bufferCount against the mininum buffer count
        if (bufferCount != 0 && bufferCount < mMinBufferCount) {
            ALOGV("bufferCount (%d) increased to %d", bufferCount, mMinBufferCount);
            bufferCount = mMinBufferCount;
        }
        // if frameCount is 0, then AudioTrack will get frame count from AudioFlinger
        // which will be the minimum size permitted.
        frameCount = bufferCount * framesPerBuffer;
    }

    if (channelMask == CHANNEL_MASK_USE_CHANNEL_ORDER) {
        channelMask = audio_channel_out_mask_from_count(channelCount);
        if (0 == channelMask) {
            ALOGE("open() error, can\'t derive mask for %d audio channels", channelCount);
            return NO_INIT;
        }
    }

    Mutex::Autolock lock(mLock);
    mCallback = cb;
    mCallbackCookie = cookie;

    sp<AudioTrack> t;
    CallbackData *newcbd = NULL;

    ALOGV("creating new AudioTrack");

    if (mCallback != NULL) {
        newcbd = new CallbackData(this);
        t = new AudioTrack(
                mStreamType,
                sampleRate,
                format,
                channelMask,
                frameCount,
                flags,
                CallbackWrapper,
                newcbd,
                0,  // notification frames
                mSessionId,
                AudioTrack::TRANSFER_CALLBACK,
                offloadInfo,
                mUid,
                mPid,
                mAttributes,
                doNotReconnect,
                1.0f,  // default value for maxRequiredSpeed
                mSelectedDeviceId);
    } else {
        // TODO: Due to buffer memory concerns, we use a max target playback speed
        // based on mPlaybackRate at the time of open (instead of kMaxRequiredSpeed),
        // also clamping the target speed to 1.0 <= targetSpeed <= kMaxRequiredSpeed.
        const float targetSpeed =
                std::min(std::max(mPlaybackRate.mSpeed, 1.0f), kMaxRequiredSpeed);
        ALOGW_IF(targetSpeed != mPlaybackRate.mSpeed,
                "track target speed:%f clamped from playback speed:%f",
                targetSpeed, mPlaybackRate.mSpeed);
        t = new AudioTrack(
                mStreamType,
                sampleRate,
                format,
                channelMask,
                frameCount,
                flags,
                NULL, // callback
                NULL, // user data
                0, // notification frames
                mSessionId,
                AudioTrack::TRANSFER_DEFAULT,
                NULL, // offload info
                mUid,
                mPid,
                mAttributes,
                doNotReconnect,
                targetSpeed,
                mSelectedDeviceId);
    }

    if ((t == 0) || (t->initCheck() != NO_ERROR)) {
        ALOGE("Unable to create audio track");
        delete newcbd;
        // t goes out of scope, so reference count drops to zero
        return NO_INIT;
    } else {
        // successful AudioTrack initialization implies a legacy stream type was generated
        // from the audio attributes
        mStreamType = t->streamType();
    }

    CHECK((t != NULL) && ((mCallback == NULL) || (newcbd != NULL)));

    mCallbackData = newcbd;
    ALOGV("setVolume");
    t->setVolume(mLeftVolume, mRightVolume);

    mSampleRateHz = sampleRate;
    mFlags = flags;
    mMsecsPerFrame = 1E3f / (mPlaybackRate.mSpeed * sampleRate);
    mFrameSize = t->frameSize();
    mTrack = t;

    return updateTrack_l();
}

status_t MediaPlayer2AudioOutput::updateTrack_l() {
    if (mTrack == NULL) {
        return NO_ERROR;
    }

    status_t res = NO_ERROR;
    // Note some output devices may give us a direct track even though we don't specify it.
    // Example: Line application b/17459982.
    if ((mTrack->getFlags()
            & (AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD | AUDIO_OUTPUT_FLAG_DIRECT)) == 0) {
        res = mTrack->setPlaybackRate(mPlaybackRate);
        if (res == NO_ERROR) {
            mTrack->setAuxEffectSendLevel(mSendLevel);
            res = mTrack->attachAuxEffect(mAuxEffectId);
        }
    }
    mTrack->setOutputDevice(mSelectedDeviceId);
    if (mDeviceCallbackEnabled) {
        mTrack->addAudioDeviceCallback(mDeviceCallback.promote());
    }
    ALOGV("updateTrack_l() DONE status %d", res);
    return res;
}

status_t MediaPlayer2AudioOutput::start() {
    ALOGV("start");
    Mutex::Autolock lock(mLock);
    if (mCallbackData != NULL) {
        mCallbackData->endTrackSwitch();
    }
    if (mTrack != 0) {
        mTrack->setVolume(mLeftVolume, mRightVolume);
        mTrack->setAuxEffectSendLevel(mSendLevel);
        status_t status = mTrack->start();
        return status;
    }
    return NO_INIT;
}

ssize_t MediaPlayer2AudioOutput::write(const void* buffer, size_t size, bool blocking) {
    Mutex::Autolock lock(mLock);
    LOG_ALWAYS_FATAL_IF(mCallback != NULL, "Don't call write if supplying a callback.");

    //ALOGV("write(%p, %u)", buffer, size);
    if (mTrack != 0) {
        return mTrack->write(buffer, size, blocking);
    }
    return NO_INIT;
}

void MediaPlayer2AudioOutput::stop() {
    ALOGV("stop");
    Mutex::Autolock lock(mLock);
    if (mTrack != 0) {
        mTrack->stop();
    }
}

void MediaPlayer2AudioOutput::flush() {
    ALOGV("flush");
    Mutex::Autolock lock(mLock);
    if (mTrack != 0) {
        mTrack->flush();
    }
}

void MediaPlayer2AudioOutput::pause() {
    ALOGV("pause");
    Mutex::Autolock lock(mLock);
    if (mTrack != 0) {
        mTrack->pause();
    }
}

void MediaPlayer2AudioOutput::close() {
    ALOGV("close");
    sp<AudioTrack> track;
    {
        Mutex::Autolock lock(mLock);
        track = mTrack;
        close_l(); // clears mTrack
    }
    // destruction of the track occurs outside of mutex.
}

void MediaPlayer2AudioOutput::setVolume(float left, float right) {
    ALOGV("setVolume(%f, %f)", left, right);
    Mutex::Autolock lock(mLock);
    mLeftVolume = left;
    mRightVolume = right;
    if (mTrack != 0) {
        mTrack->setVolume(left, right);
    }
}

status_t MediaPlayer2AudioOutput::setPlaybackRate(const AudioPlaybackRate &rate) {
    ALOGV("setPlaybackRate(%f %f %d %d)",
                rate.mSpeed, rate.mPitch, rate.mFallbackMode, rate.mStretchMode);
    Mutex::Autolock lock(mLock);
    if (mTrack == 0) {
        // remember rate so that we can set it when the track is opened
        mPlaybackRate = rate;
        return OK;
    }
    status_t res = mTrack->setPlaybackRate(rate);
    if (res != NO_ERROR) {
        return res;
    }
    // rate.mSpeed is always greater than 0 if setPlaybackRate succeeded
    CHECK_GT(rate.mSpeed, 0.f);
    mPlaybackRate = rate;
    if (mSampleRateHz != 0) {
        mMsecsPerFrame = 1E3f / (rate.mSpeed * mSampleRateHz);
    }
    return res;
}

status_t MediaPlayer2AudioOutput::getPlaybackRate(AudioPlaybackRate *rate) {
    ALOGV("setPlaybackRate");
    Mutex::Autolock lock(mLock);
    if (mTrack == 0) {
        return NO_INIT;
    }
    *rate = mTrack->getPlaybackRate();
    return NO_ERROR;
}

status_t MediaPlayer2AudioOutput::setAuxEffectSendLevel(float level) {
    ALOGV("setAuxEffectSendLevel(%f)", level);
    Mutex::Autolock lock(mLock);
    mSendLevel = level;
    if (mTrack != 0) {
        return mTrack->setAuxEffectSendLevel(level);
    }
    return NO_ERROR;
}

status_t MediaPlayer2AudioOutput::attachAuxEffect(int effectId) {
    ALOGV("attachAuxEffect(%d)", effectId);
    Mutex::Autolock lock(mLock);
    mAuxEffectId = effectId;
    if (mTrack != 0) {
        return mTrack->attachAuxEffect(effectId);
    }
    return NO_ERROR;
}

status_t MediaPlayer2AudioOutput::setOutputDevice(audio_port_handle_t deviceId) {
    ALOGV("setOutputDevice(%d)", deviceId);
    Mutex::Autolock lock(mLock);
    mSelectedDeviceId = deviceId;
    if (mTrack != 0) {
        return mTrack->setOutputDevice(deviceId);
    }
    return NO_ERROR;
}

status_t MediaPlayer2AudioOutput::getRoutedDeviceId(audio_port_handle_t* deviceId) {
    ALOGV("getRoutedDeviceId");
    Mutex::Autolock lock(mLock);
    if (mTrack != 0) {
        mRoutedDeviceId = mTrack->getRoutedDeviceId();
    }
    *deviceId = mRoutedDeviceId;
    return NO_ERROR;
}

status_t MediaPlayer2AudioOutput::enableAudioDeviceCallback(bool enabled) {
    ALOGV("enableAudioDeviceCallback, %d", enabled);
    Mutex::Autolock lock(mLock);
    mDeviceCallbackEnabled = enabled;
    if (mTrack != 0) {
        status_t status;
        if (enabled) {
            status = mTrack->addAudioDeviceCallback(mDeviceCallback.promote());
        } else {
            status = mTrack->removeAudioDeviceCallback(mDeviceCallback.promote());
        }
        return status;
    }
    return NO_ERROR;
}

// static
void MediaPlayer2AudioOutput::CallbackWrapper(
        int event, void *cookie, void *info) {
    //ALOGV("callbackwrapper");
    CallbackData *data = (CallbackData*)cookie;
    // lock to ensure we aren't caught in the middle of a track switch.
    data->lock();
    MediaPlayer2AudioOutput *me = data->getOutput();
    AudioTrack::Buffer *buffer = (AudioTrack::Buffer *)info;
    if (me == NULL) {
        // no output set, likely because the track was scheduled to be reused
        // by another player, but the format turned out to be incompatible.
        data->unlock();
        if (buffer != NULL) {
            buffer->size = 0;
        }
        return;
    }

    switch(event) {
    case AudioTrack::EVENT_MORE_DATA: {
        size_t actualSize = (*me->mCallback)(
                me, buffer->raw, buffer->size, me->mCallbackCookie,
                CB_EVENT_FILL_BUFFER);

        // Log when no data is returned from the callback.
        // (1) We may have no data (especially with network streaming sources).
        // (2) We may have reached the EOS and the audio track is not stopped yet.
        // Note that AwesomePlayer/AudioPlayer will only return zero size when it reaches the EOS.
        // NuPlayer2Renderer will return zero when it doesn't have data (it doesn't block to fill).
        //
        // This is a benign busy-wait, with the next data request generated 10 ms or more later;
        // nevertheless for power reasons, we don't want to see too many of these.

        ALOGV_IF(actualSize == 0 && buffer->size > 0, "callbackwrapper: empty buffer returned");

        buffer->size = actualSize;
        } break;

    case AudioTrack::EVENT_STREAM_END:
        // currently only occurs for offloaded callbacks
        ALOGV("callbackwrapper: deliver EVENT_STREAM_END");
        (*me->mCallback)(me, NULL /* buffer */, 0 /* size */,
                me->mCallbackCookie, CB_EVENT_STREAM_END);
        break;

    case AudioTrack::EVENT_NEW_IAUDIOTRACK :
        ALOGV("callbackwrapper: deliver EVENT_TEAR_DOWN");
        (*me->mCallback)(me,  NULL /* buffer */, 0 /* size */,
                me->mCallbackCookie, CB_EVENT_TEAR_DOWN);
        break;

    case AudioTrack::EVENT_UNDERRUN:
        // This occurs when there is no data available, typically
        // when there is a failure to supply data to the AudioTrack.  It can also
        // occur in non-offloaded mode when the audio device comes out of standby.
        //
        // If an AudioTrack underruns it outputs silence. Since this happens suddenly
        // it may sound like an audible pop or glitch.
        //
        // The underrun event is sent once per track underrun; the condition is reset
        // when more data is sent to the AudioTrack.
        ALOGD("callbackwrapper: EVENT_UNDERRUN (discarded)");
        break;

    default:
        ALOGE("received unknown event type: %d inside CallbackWrapper !", event);
    }

    data->unlock();
}

audio_session_t MediaPlayer2AudioOutput::getSessionId() const
{
    Mutex::Autolock lock(mLock);
    return mSessionId;
}

uint32_t MediaPlayer2AudioOutput::getSampleRate() const
{
    Mutex::Autolock lock(mLock);
    if (mTrack == 0) {
        return 0;
    }
    return mTrack->getSampleRate();
}

int64_t MediaPlayer2AudioOutput::getBufferDurationInUs() const
{
    Mutex::Autolock lock(mLock);
    if (mTrack == 0) {
        return 0;
    }
    int64_t duration;
    if (mTrack->getBufferDurationInUs(&duration) != OK) {
        return 0;
    }
    return duration;
}

} // namespace android
