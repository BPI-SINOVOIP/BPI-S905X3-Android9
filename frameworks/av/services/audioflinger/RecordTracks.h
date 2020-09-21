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

// record track
class RecordTrack : public TrackBase {
public:
                        RecordTrack(RecordThread *thread,
                                const sp<Client>& client,
                                const audio_attributes_t& attr,
                                uint32_t sampleRate,
                                audio_format_t format,
                                audio_channel_mask_t channelMask,
                                size_t frameCount,
                                void *buffer,
                                size_t bufferSize,
                                audio_session_t sessionId,
                                uid_t uid,
                                audio_input_flags_t flags,
                                track_type type,
                                audio_port_handle_t portId = AUDIO_PORT_HANDLE_NONE);
    virtual             ~RecordTrack();
    virtual status_t    initCheck() const;

    virtual status_t    start(AudioSystem::sync_event_t event, audio_session_t triggerSession);
    virtual void        stop();

            void        destroy();

    virtual void        invalidate();
            // clear the buffer overflow flag
            void        clearOverflow() { mOverflow = false; }
            // set the buffer overflow flag and return previous value
            bool        setOverflow() { bool tmp = mOverflow; mOverflow = true;
                                                return tmp; }

    static  void        appendDumpHeader(String8& result);
            void        appendDump(String8& result, bool active);

            void        handleSyncStartEvent(const sp<SyncEvent>& event);
            void        clearSyncStartEvent();

            void        updateTrackFrameInfo(int64_t trackFramesReleased,
                                             int64_t sourceFramesRead,
                                             uint32_t halSampleRate,
                                             const ExtendedTimestamp &timestamp);

    virtual bool        isFastTrack() const { return (mFlags & AUDIO_INPUT_FLAG_FAST) != 0; }

            void        setSilenced(bool silenced) { if (!isPatchTrack()) mSilenced = silenced; }
            bool        isSilenced() const { return mSilenced; }

            status_t    getActiveMicrophones(std::vector<media::MicrophoneInfo>* activeMicrophones);

private:
    friend class AudioFlinger;  // for mState

    DISALLOW_COPY_AND_ASSIGN(RecordTrack);

    // AudioBufferProvider interface
    virtual status_t getNextBuffer(AudioBufferProvider::Buffer* buffer);
    // releaseBuffer() not overridden

    bool                mOverflow;  // overflow on most recent attempt to fill client buffer

            AudioBufferProvider::Buffer mSink;  // references client's buffer sink in shared memory

            // sync event triggering actual audio capture. Frames read before this event will
            // be dropped and therefore not read by the application.
            sp<SyncEvent>                       mSyncStartEvent;

            // number of captured frames to drop after the start sync event has been received.
            // when < 0, maximum frames to drop before starting capture even if sync event is
            // not received
            ssize_t                             mFramesToDrop;

            // used by resampler to find source frames
            ResamplerBufferProvider            *mResamplerBufferProvider;

            // used by the record thread to convert frames to proper destination format
            RecordBufferConverter              *mRecordBufferConverter;
            audio_input_flags_t                mFlags;

            bool                               mSilenced;
};

// playback track, used by PatchPanel
class PatchRecord : virtual public RecordTrack, public PatchProxyBufferProvider {
public:

    PatchRecord(RecordThread *recordThread,
                uint32_t sampleRate,
                audio_channel_mask_t channelMask,
                audio_format_t format,
                size_t frameCount,
                void *buffer,
                size_t bufferSize,
                audio_input_flags_t flags);
    virtual             ~PatchRecord();

    // AudioBufferProvider interface
    virtual status_t getNextBuffer(AudioBufferProvider::Buffer* buffer);
    virtual void releaseBuffer(AudioBufferProvider::Buffer* buffer);

    // PatchProxyBufferProvider interface
    virtual status_t    obtainBuffer(Proxy::Buffer *buffer,
                                     const struct timespec *timeOut = NULL);
    virtual void        releaseBuffer(Proxy::Buffer *buffer);

    void setPeerProxy(PatchProxyBufferProvider *proxy) { mPeerProxy = proxy; }

private:
    sp<ClientProxy>             mProxy;
    PatchProxyBufferProvider*   mPeerProxy;
    struct timespec             mPeerTimeout;
};  // end of PatchRecord
