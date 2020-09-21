/*
**
** Copyright 2012, The Android Open Source Project
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

#ifndef INCLUDING_FROM_AUDIOFLINGER_H
    #error This header file should only be included from AudioFlinger.h
#endif

// playback track
class Track : public TrackBase, public VolumeProvider {
public:
                        Track(  PlaybackThread *thread,
                                const sp<Client>& client,
                                audio_stream_type_t streamType,
                                const audio_attributes_t& attr,
                                uint32_t sampleRate,
                                audio_format_t format,
                                audio_channel_mask_t channelMask,
                                size_t frameCount,
                                void *buffer,
                                size_t bufferSize,
                                const sp<IMemory>& sharedBuffer,
                                audio_session_t sessionId,
                                uid_t uid,
                                audio_output_flags_t flags,
                                track_type type,
                                audio_port_handle_t portId = AUDIO_PORT_HANDLE_NONE);
    virtual             ~Track();
    virtual status_t    initCheck() const;

    static  void        appendDumpHeader(String8& result);
            void        appendDump(String8& result, bool active);
    virtual status_t    start(AudioSystem::sync_event_t event =
                                    AudioSystem::SYNC_EVENT_NONE,
                             audio_session_t triggerSession = AUDIO_SESSION_NONE);
    virtual void        stop();
            void        pause();

            void        flush();
            void        destroy();
            int         name() const { return mName; }
            void        setName(int name) {
                LOG_ALWAYS_FATAL_IF(mName >= 0 && name >= 0,
                        "%s both old name %d and new name %d are valid", __func__, mName, name);
                mName = name;
            }

    virtual uint32_t    sampleRate() const;

            audio_stream_type_t streamType() const {
                return mStreamType;
            }
            bool        isOffloaded() const
                                { return (mFlags & AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD) != 0; }
            bool        isDirect() const { return (mFlags & AUDIO_OUTPUT_FLAG_DIRECT) != 0; }
            bool        isOffloadedOrDirect() const { return (mFlags
                            & (AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD
                                    | AUDIO_OUTPUT_FLAG_DIRECT)) != 0; }

            status_t    setParameters(const String8& keyValuePairs);
            status_t    attachAuxEffect(int EffectId);
            void        setAuxBuffer(int EffectId, int32_t *buffer);
            int32_t     *auxBuffer() const { return mAuxBuffer; }
            void        setMainBuffer(effect_buffer_t *buffer) { mMainBuffer = buffer; }
            effect_buffer_t *mainBuffer() const { return mMainBuffer; }
            int         auxEffectId() const { return mAuxEffectId; }
    virtual status_t    getTimestamp(AudioTimestamp& timestamp);
            void        signal();

// implement FastMixerState::VolumeProvider interface
    virtual gain_minifloat_packed_t getVolumeLR();

    virtual status_t    setSyncEvent(const sp<SyncEvent>& event);

    virtual bool        isFastTrack() const { return (mFlags & AUDIO_OUTPUT_FLAG_FAST) != 0; }

// implement volume handling.
    media::VolumeShaper::Status applyVolumeShaper(
                                const sp<media::VolumeShaper::Configuration>& configuration,
                                const sp<media::VolumeShaper::Operation>& operation);
    sp<media::VolumeShaper::State> getVolumeShaperState(int id);
    sp<media::VolumeHandler>   getVolumeHandler() { return mVolumeHandler; }
    /** Set the computed normalized final volume of the track.
     * !masterMute * masterVolume * streamVolume * averageLRVolume */
    void                setFinalVolume(float volume);
    float               getFinalVolume() const { return mFinalVolume; }

    /** @return true if the track has changed (metadata or volume) since
     *          the last time this function was called,
     *          true if this function was never called since the track creation,
     *          false otherwise.
     *  Thread safe.
     */
    bool            readAndClearHasChanged() { return !mChangeNotified.test_and_set(); }

    using SourceMetadatas = std::vector<playback_track_metadata_t>;
    using MetadataInserter = std::back_insert_iterator<SourceMetadatas>;
    /** Copy the track metadata in the provided iterator. Thread safe. */
    virtual void    copyMetadataTo(MetadataInserter& backInserter) const;

protected:
    // for numerous
    friend class PlaybackThread;
    friend class MixerThread;
    friend class DirectOutputThread;
    friend class OffloadThread;

    DISALLOW_COPY_AND_ASSIGN(Track);

    // AudioBufferProvider interface
    virtual status_t getNextBuffer(AudioBufferProvider::Buffer* buffer);
    // releaseBuffer() not overridden

    // ExtendedAudioBufferProvider interface
    virtual size_t framesReady() const;
    virtual int64_t framesReleased() const;
    virtual void onTimestamp(const ExtendedTimestamp &timestamp);

    bool isPausing() const { return mState == PAUSING; }
    bool isPaused() const { return mState == PAUSED; }
    bool isResuming() const { return mState == RESUMING; }
    bool isReady() const;
    void setPaused() { mState = PAUSED; }
    void reset();
    bool isFlushPending() const { return mFlushHwPending; }
    void flushAck();
    bool isResumePending();
    void resumeAck();
    void updateTrackFrameInfo(int64_t trackFramesReleased, int64_t sinkFramesWritten,
            const ExtendedTimestamp &timeStamp);

    sp<IMemory> sharedBuffer() const { return mSharedBuffer; }

    // framesWritten is cumulative, never reset, and is shared all tracks
    // audioHalFrames is derived from output latency
    // FIXME parameters not needed, could get them from the thread
    bool presentationComplete(int64_t framesWritten, size_t audioHalFrames);
    void signalClientFlag(int32_t flag);

    /** Set that a metadata has changed and needs to be notified to backend. Thread safe. */
    void setMetadataHasChanged() { mChangeNotified.clear(); }
public:
    void triggerEvents(AudioSystem::sync_event_t type);
    virtual void invalidate();
    void disable();

    int fastIndex() const { return mFastIndex; }

protected:

    // FILLED state is used for suppressing volume ramp at begin of playing
    enum {FS_INVALID, FS_FILLING, FS_FILLED, FS_ACTIVE};
    mutable uint8_t     mFillingUpStatus;
    int8_t              mRetryCount;

    // see comment at AudioFlinger::PlaybackThread::Track::~Track for why this can't be const
    sp<IMemory>         mSharedBuffer;

    bool                mResetDone;
    const audio_stream_type_t mStreamType;
    int                 mName;
    effect_buffer_t     *mMainBuffer;

    int32_t             *mAuxBuffer;
    int                 mAuxEffectId;
    bool                mHasVolumeController;
    size_t              mPresentationCompleteFrames; // number of frames written to the
                                    // audio HAL when this track will be fully rendered
                                    // zero means not monitoring

    // access these three variables only when holding thread lock.
    LinearMap<int64_t> mFrameMap;           // track frame to server frame mapping

    ExtendedTimestamp  mSinkTimestamp;

    sp<media::VolumeHandler>  mVolumeHandler; // handles multiple VolumeShaper configs and operations

private:
    // The following fields are only for fast tracks, and should be in a subclass
    int                 mFastIndex; // index within FastMixerState::mFastTracks[];
                                    // either mFastIndex == -1 if not isFastTrack()
                                    // or 0 < mFastIndex < FastMixerState::kMaxFast because
                                    // index 0 is reserved for normal mixer's submix;
                                    // index is allocated statically at track creation time
                                    // but the slot is only used if track is active
    FastTrackUnderruns  mObservedUnderruns; // Most recently observed value of
                                    // mFastMixerDumpState.mTracks[mFastIndex].mUnderruns
    volatile float      mCachedVolume;  // combined master volume and stream type volume;
                                        // 'volatile' means accessed without lock or
                                        // barrier, but is read/written atomically
    float               mFinalVolume; // combine master volume, stream type volume and track volume
    sp<AudioTrackServerProxy>  mAudioTrackServerProxy;
    bool                mResumeToStopping; // track was paused in stopping state.
    bool                mFlushHwPending; // track requests for thread flush
    audio_output_flags_t mFlags;
    // If the last track change was notified to the client with readAndClearHasChanged
    std::atomic_flag     mChangeNotified = ATOMIC_FLAG_INIT;
};  // end of Track


// playback track, used by DuplicatingThread
class OutputTrack : public Track {
public:

    class Buffer : public AudioBufferProvider::Buffer {
    public:
        void *mBuffer;
    };

                        OutputTrack(PlaybackThread *thread,
                                DuplicatingThread *sourceThread,
                                uint32_t sampleRate,
                                audio_format_t format,
                                audio_channel_mask_t channelMask,
                                size_t frameCount,
                                uid_t uid);
    virtual             ~OutputTrack();

    virtual status_t    start(AudioSystem::sync_event_t event =
                                    AudioSystem::SYNC_EVENT_NONE,
                             audio_session_t triggerSession = AUDIO_SESSION_NONE);
    virtual void        stop();
            bool        write(void* data, uint32_t frames);
            bool        bufferQueueEmpty() const { return mBufferQueue.size() == 0; }
            bool        isActive() const { return mActive; }
    const wp<ThreadBase>& thread() const { return mThread; }

            void        copyMetadataTo(MetadataInserter& backInserter) const override;
    /** Set the metadatas of the upstream tracks. Thread safe. */
            void        setMetadatas(const SourceMetadatas& metadatas);

private:
    status_t            obtainBuffer(AudioBufferProvider::Buffer* buffer,
                                     uint32_t waitTimeMs);
    void                clearBufferQueue();

    void                restartIfDisabled();

    // Maximum number of pending buffers allocated by OutputTrack::write()
    static const uint8_t kMaxOverFlowBuffers = 10;

    Vector < Buffer* >          mBufferQueue;
    AudioBufferProvider::Buffer mOutBuffer;
    bool                        mActive;
    DuplicatingThread* const    mSourceThread; // for waitTimeMs() in write()
    sp<AudioTrackClientProxy>   mClientProxy;
    /** Attributes of the source tracks.
     *
     * This member must be accessed with mTrackMetadatasMutex taken.
     * There is one writer (duplicating thread) and one reader (downstream mixer).
     *
     * That means that the duplicating thread can block the downstream mixer
     * thread and vice versa for the time of the copy.
     * If this becomes an issue, the metadata could be stored in an atomic raw pointer,
     * and a exchange with nullptr and delete can be used.
     * Alternatively a read-copy-update might be implemented.
     */
    SourceMetadatas mTrackMetadatas;
    /** Protects mTrackMetadatas against concurrent access. */
    mutable std::mutex mTrackMetadatasMutex;
};  // end of OutputTrack

// playback track, used by PatchPanel
class PatchTrack : public Track, public PatchProxyBufferProvider {
public:

                        PatchTrack(PlaybackThread *playbackThread,
                                   audio_stream_type_t streamType,
                                   uint32_t sampleRate,
                                   audio_channel_mask_t channelMask,
                                   audio_format_t format,
                                   size_t frameCount,
                                   void *buffer,
                                   size_t bufferSize,
                                   audio_output_flags_t flags);
    virtual             ~PatchTrack();

    virtual status_t    start(AudioSystem::sync_event_t event =
                                    AudioSystem::SYNC_EVENT_NONE,
                             audio_session_t triggerSession = AUDIO_SESSION_NONE);

    // AudioBufferProvider interface
    virtual status_t getNextBuffer(AudioBufferProvider::Buffer* buffer);
    virtual void releaseBuffer(AudioBufferProvider::Buffer* buffer);

    // PatchProxyBufferProvider interface
    virtual status_t    obtainBuffer(Proxy::Buffer* buffer,
                                     const struct timespec *timeOut = NULL);
    virtual void        releaseBuffer(Proxy::Buffer* buffer);

            void setPeerProxy(PatchProxyBufferProvider *proxy) { mPeerProxy = proxy; }

private:
            void restartIfDisabled();

    sp<ClientProxy>             mProxy;
    PatchProxyBufferProvider*   mPeerProxy;
    struct timespec             mPeerTimeout;
};  // end of PatchTrack
