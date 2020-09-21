/*
 * Copyright 2017 The Android Open Source Project
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

#include <mediaplayer2/MediaPlayer2Interface.h>

#include <media/MediaAnalyticsItem.h>
#include <media/stagefright/foundation/ABase.h>

namespace android {

struct ALooper;
struct MediaClock;
struct NuPlayer2;

struct NuPlayer2Driver : public MediaPlayer2Interface {
    explicit NuPlayer2Driver(pid_t pid, uid_t uid);

    virtual status_t initCheck() override;

    virtual status_t setDataSource(const sp<DataSourceDesc> &dsd) override;
    virtual status_t prepareNextDataSource(const sp<DataSourceDesc> &dsd) override;
    virtual status_t playNextDataSource(int64_t srcId) override;

    virtual status_t setVideoSurfaceTexture(const sp<ANativeWindowWrapper> &nww) override;

    virtual status_t getBufferingSettings(
            BufferingSettings* buffering /* nonnull */) override;
    virtual status_t setBufferingSettings(const BufferingSettings& buffering) override;

    virtual status_t prepareAsync();
    virtual status_t start();
    virtual status_t stop();
    virtual status_t pause();
    virtual bool isPlaying();
    virtual status_t setPlaybackSettings(const AudioPlaybackRate &rate);
    virtual status_t getPlaybackSettings(AudioPlaybackRate *rate);
    virtual status_t setSyncSettings(const AVSyncSettings &sync, float videoFpsHint);
    virtual status_t getSyncSettings(AVSyncSettings *sync, float *videoFps);
    virtual status_t seekTo(
            int64_t msec, MediaPlayer2SeekMode mode = MediaPlayer2SeekMode::SEEK_PREVIOUS_SYNC);
    virtual status_t getCurrentPosition(int64_t *msec);
    virtual status_t getDuration(int64_t *msec);
    virtual status_t reset();
    virtual status_t notifyAt(int64_t mediaTimeUs) override;
    virtual status_t setLooping(int loop);
    virtual status_t invoke(const Parcel &request, Parcel *reply);
    virtual void setAudioSink(const sp<AudioSink> &audioSink);
    virtual status_t setParameter(int key, const Parcel &request);
    virtual status_t getParameter(int key, Parcel *reply);

    virtual status_t getMetadata(
            const media::Metadata::Filter& ids, Parcel *records);

    virtual status_t dump(int fd, const Vector<String16> &args) const;

    virtual void onMessageReceived(const sp<AMessage> &msg) override;

    void notifySetDataSourceCompleted(int64_t srcId, status_t err);
    void notifyPrepareCompleted(int64_t srcId, status_t err);
    void notifyResetComplete(int64_t srcId);
    void notifySetSurfaceComplete(int64_t srcId);
    void notifyDuration(int64_t srcId, int64_t durationUs);
    void notifyMorePlayingTimeUs(int64_t srcId, int64_t timeUs);
    void notifyMoreRebufferingTimeUs(int64_t srcId, int64_t timeUs);
    void notifyRebufferingWhenExit(int64_t srcId, bool status);
    void notifySeekComplete(int64_t srcId);
    void notifySeekComplete_l(int64_t srcId);
    void notifyListener(int64_t srcId, int msg, int ext1 = 0, int ext2 = 0,
                        const Parcel *in = NULL);
    void notifyFlagsChanged(int64_t srcId, uint32_t flags);

    // Modular DRM
    virtual status_t prepareDrm(const uint8_t uuid[16], const Vector<uint8_t> &drmSessionId);
    virtual status_t releaseDrm();

protected:
    virtual ~NuPlayer2Driver();

private:
    enum State {
        STATE_IDLE,
        STATE_SET_DATASOURCE_PENDING,
        STATE_UNPREPARED,
        STATE_PREPARING,
        STATE_PREPARED,
        STATE_RUNNING,
        STATE_PAUSED,
        STATE_RESET_IN_PROGRESS,
        STATE_STOPPED,                  // equivalent to PAUSED
        STATE_STOPPED_AND_PREPARING,    // equivalent to PAUSED, but seeking
        STATE_STOPPED_AND_PREPARED,     // equivalent to PAUSED, but seek complete
    };

    std::string stateString(State state);

    enum {
        kWhatNotifyListener,
    };

    mutable Mutex mLock;
    Condition mCondition;

    State mState;

    status_t mAsyncResult;

    // The following are protected through "mLock"
    // >>>
    int64_t mSrcId;
    bool mSetSurfaceInProgress;
    int64_t mDurationUs;
    int64_t mPositionUs;
    bool mSeekInProgress;
    int64_t mPlayingTimeUs;
    int64_t mRebufferingTimeUs;
    int32_t mRebufferingEvents;
    bool mRebufferingAtExit;
    // <<<

    sp<ALooper> mLooper;
    sp<ALooper> mNuPlayer2Looper;
    const sp<MediaClock> mMediaClock;
    const sp<NuPlayer2> mPlayer;
    sp<AudioSink> mAudioSink;
    uint32_t mPlayerFlags;

    MediaAnalyticsItem *mAnalyticsItem;
    uid_t mClientUid;

    bool mAtEOS;
    bool mLooping;
    bool mAutoLoop;

    void updateMetrics(const char *where);
    void logMetrics(const char *where);

    status_t start_l();
    void notifyListener_l(int64_t srcId, int msg, int ext1 = 0, int ext2 = 0,
                          const Parcel *in = NULL);

    DISALLOW_EVIL_CONSTRUCTORS(NuPlayer2Driver);
};

}  // namespace android


