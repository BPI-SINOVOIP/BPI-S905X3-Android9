/*
 * Copyright (C) 2014 The Android Open Source Project
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

#ifndef MIDI_EXTRACTOR_H_
#define MIDI_EXTRACTOR_H_

#include <media/DataSourceBase.h>
#include <media/MediaExtractor.h>
#include <media/stagefright/MediaBufferBase.h>
#include <media/stagefright/MediaBufferGroup.h>
#include <media/stagefright/MetaDataBase.h>
#include <media/MidiIoWrapper.h>
#include <utils/String8.h>
#include <libsonivox/eas.h>

namespace android {

class MidiEngine {
public:
    explicit MidiEngine(DataSourceBase *dataSource,
            MetaDataBase *fileMetadata,
            MetaDataBase *trackMetadata);
    ~MidiEngine();

    status_t initCheck();

    status_t allocateBuffers();
    status_t releaseBuffers();
    status_t seekTo(int64_t positionUs);
    MediaBufferBase* readBuffer();
private:
    MidiIoWrapper *mIoWrapper;
    MediaBufferGroup *mGroup;
    EAS_DATA_HANDLE mEasData;
    EAS_HANDLE mEasHandle;
    const S_EAS_LIB_CONFIG* mEasConfig;
    bool mIsInitialized;
};

class MidiExtractor : public MediaExtractor {

public:
    explicit MidiExtractor(DataSourceBase *source);

    virtual size_t countTracks();
    virtual MediaTrack *getTrack(size_t index);
    virtual status_t getTrackMetaData(MetaDataBase& meta, size_t index, uint32_t flags);

    virtual status_t getMetaData(MetaDataBase& meta);
    virtual const char * name() { return "MidiExtractor"; }

protected:
    virtual ~MidiExtractor();

private:
    DataSourceBase *mDataSource;
    status_t mInitCheck;
    MetaDataBase mFileMetadata;

    // There is only one track
    MetaDataBase mTrackMetadata;

    MidiEngine *mEngine;

    EAS_DATA_HANDLE     mEasData;
    EAS_HANDLE          mEasHandle;
    EAS_PCM*            mAudioBuffer;
    EAS_I32             mPlayTime;
    EAS_I32             mDuration;
    EAS_STATE           mState;
    EAS_FILE            mFileLocator;

    MidiExtractor(const MidiExtractor &);
    MidiExtractor &operator=(const MidiExtractor &);

};

bool SniffMidi(DataSourceBase *source, float *confidence);

}  // namespace android

#endif  // MIDI_EXTRACTOR_H_
