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

#ifndef ANDROID_MEDIAPLAYER2_H
#define ANDROID_MEDIAPLAYER2_H

#include <media/AVSyncSettings.h>
#include <media/AudioResamplerPublic.h>
#include <media/BufferingSettings.h>
#include <media/Metadata.h>
#include <media/mediaplayer_common.h>
#include <mediaplayer2/MediaPlayer2Interface.h>
#include <mediaplayer2/MediaPlayer2Types.h>

#include <utils/Errors.h>
#include <utils/Mutex.h>
#include <utils/RefBase.h>
#include <utils/String16.h>
#include <utils/Vector.h>
#include <system/audio-base.h>

namespace android {

struct ANativeWindowWrapper;
struct DataSourceDesc;
class MediaPlayer2AudioOutput;

// ref-counted object for callbacks
class MediaPlayer2Listener: virtual public RefBase
{
public:
    virtual void notify(int64_t srcId, int msg, int ext1, int ext2, const Parcel *obj) = 0;
};

class MediaPlayer2 : public MediaPlayer2InterfaceListener
{
public:
    ~MediaPlayer2();

    static sp<MediaPlayer2> Create();
    static status_t DumpAll(int fd, const Vector<String16>& args);

            void            disconnect();

            status_t        getSrcId(int64_t *srcId);
            status_t        setDataSource(const sp<DataSourceDesc> &dsd);
            status_t        prepareNextDataSource(const sp<DataSourceDesc> &dsd);
            status_t        playNextDataSource(int64_t srcId);
            status_t        setVideoSurfaceTexture(const sp<ANativeWindowWrapper>& nww);
            status_t        setListener(const sp<MediaPlayer2Listener>& listener);
            status_t        getBufferingSettings(BufferingSettings* buffering /* nonnull */);
            status_t        setBufferingSettings(const BufferingSettings& buffering);
            status_t        prepareAsync();
            status_t        start();
            status_t        stop();
            status_t        pause();
            bool            isPlaying();
            mediaplayer2_states getMediaPlayer2State();
            status_t        setPlaybackSettings(const AudioPlaybackRate& rate);
            status_t        getPlaybackSettings(AudioPlaybackRate* rate /* nonnull */);
            status_t        setSyncSettings(const AVSyncSettings& sync, float videoFpsHint);
            status_t        getSyncSettings(
                                    AVSyncSettings* sync /* nonnull */,
                                    float* videoFps /* nonnull */);
            status_t        getVideoWidth(int *w);
            status_t        getVideoHeight(int *h);
            status_t        seekTo(
                    int64_t msec,
                    MediaPlayer2SeekMode mode = MediaPlayer2SeekMode::SEEK_PREVIOUS_SYNC);
            status_t        notifyAt(int64_t mediaTimeUs);
            status_t        getCurrentPosition(int64_t *msec);
            status_t        getDuration(int64_t *msec);
            status_t        reset();
            status_t        setAudioStreamType(audio_stream_type_t type);
            status_t        getAudioStreamType(audio_stream_type_t *type);
            status_t        setLooping(int loop);
            bool            isLooping();
            status_t        setVolume(float leftVolume, float rightVolume);
            void            notify(int64_t srcId, int msg, int ext1, int ext2,
                                   const Parcel *obj = NULL);
            status_t        invoke(const Parcel& request, Parcel *reply);
            status_t        setMetadataFilter(const Parcel& filter);
            status_t        getMetadata(bool update_only, bool apply_filter, Parcel *metadata);
            status_t        setAudioSessionId(audio_session_t sessionId);
            audio_session_t getAudioSessionId();
            status_t        setAuxEffectSendLevel(float level);
            status_t        attachAuxEffect(int effectId);
            status_t        setParameter(int key, const Parcel& request);
            status_t        getParameter(int key, Parcel* reply);

            // Modular DRM
            status_t        prepareDrm(const uint8_t uuid[16], const Vector<uint8_t>& drmSessionId);
            status_t        releaseDrm();
            // AudioRouting
            status_t        setOutputDevice(audio_port_handle_t deviceId);
            audio_port_handle_t getRoutedDeviceId();
            status_t        enableAudioDeviceCallback(bool enabled);

            status_t        dump(int fd, const Vector<String16>& args);

private:
    MediaPlayer2();
    bool init();

    // @param type Of the metadata to be tested.
    // @return true if the metadata should be dropped according to
    //              the filters.
    bool shouldDropMetadata(media::Metadata::Type type) const;

    // Add a new element to the set of metadata updated. Noop if
    // the element exists already.
    // @param type Of the metadata to be recorded.
    void addNewMetadataUpdate(media::Metadata::Type type);

    // Disconnect from the currently connected ANativeWindow.
    void disconnectNativeWindow_l();

    status_t setAudioAttributes_l(const Parcel &request);

    void clear_l();
    status_t seekTo_l(int64_t msec, MediaPlayer2SeekMode mode);
    status_t prepareAsync_l();
    status_t getDuration_l(int64_t *msec);
    status_t reset_l();
    status_t checkStateForKeySet_l(int key);

    pid_t                       mPid;
    uid_t                       mUid;
    sp<MediaPlayer2Interface>   mPlayer;
    sp<MediaPlayer2AudioOutput> mAudioOutput;
    int64_t                     mSrcId;
    thread_id_t                 mLockThreadId;
    mutable Mutex               mLock;
    Mutex                       mNotifyLock;
    sp<MediaPlayer2Listener>    mListener;
    media_player2_internal_states mCurrentState;
    int64_t                     mCurrentPosition;
    MediaPlayer2SeekMode        mCurrentSeekMode;
    int64_t                     mSeekPosition;
    MediaPlayer2SeekMode        mSeekMode;
    audio_stream_type_t         mStreamType;
    Parcel*                     mAudioAttributesParcel;
    bool                        mLoop;
    float                       mLeftVolume;
    float                       mRightVolume;
    int                         mVideoWidth;
    int                         mVideoHeight;
    audio_session_t             mAudioSessionId;
    audio_attributes_t *        mAudioAttributes;
    float                       mSendLevel;

    sp<ANativeWindowWrapper>    mConnectedWindow;

    // Metadata filters.
    media::Metadata::Filter mMetadataAllow;  // protected by mLock
    media::Metadata::Filter mMetadataDrop;  // protected by mLock

    // Metadata updated. For each MEDIA_INFO_METADATA_UPDATE
    // notification we try to update mMetadataUpdated which is a
    // set: no duplicate.
    // getMetadata clears this set.
    media::Metadata::Filter mMetadataUpdated;  // protected by mLock
};

}; // namespace android

#endif // ANDROID_MEDIAPLAYER2_H
