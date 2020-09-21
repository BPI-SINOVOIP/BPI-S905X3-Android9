/*
 * Copyright (C) 2010 The Android Open Source Project
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

#ifndef MATROSKA_EXTRACTOR_H_

#define MATROSKA_EXTRACTOR_H_

#include "mkvparser/mkvparser.h"

#include <media/MediaExtractor.h>
#include <media/stagefright/MetaDataBase.h>
#include <utils/Vector.h>
#include <utils/threads.h>

namespace android {

struct AMessage;
class String8;

class MetaData;
struct DataSourceBaseReader;
struct MatroskaSource;

struct MatroskaExtractor : public MediaExtractor {
    explicit MatroskaExtractor(DataSourceBase *source);

    virtual size_t countTracks();

    virtual MediaTrack *getTrack(size_t index);

    virtual status_t getTrackMetaData(MetaDataBase& meta, size_t index, uint32_t flags);

    virtual status_t getMetaData(MetaDataBase& meta);

    virtual uint32_t flags() const;

    virtual const char * name() { return "MatroskaExtractor"; }

protected:
    virtual ~MatroskaExtractor();

private:
    friend struct MatroskaSource;
    friend struct BlockIterator;

    struct TrackInfo {
        unsigned long mTrackNum;
        bool mEncrypted;
        MetaDataBase mMeta;
        const MatroskaExtractor *mExtractor;
        Vector<const mkvparser::CuePoint*> mCuePoints;

        // mHeader points to memory managed by mkvparser;
        // mHeader would be deleted when mSegment is deleted
        // in ~MatroskaExtractor.
        unsigned char *mHeader;
        size_t mHeaderLen;

        const mkvparser::Track* getTrack() const;
        const mkvparser::CuePoint::TrackPosition *find(long long timeNs) const;
    };

    Mutex mLock;
    Vector<TrackInfo> mTracks;

    DataSourceBase *mDataSource;
    DataSourceBaseReader *mReader;
    mkvparser::Segment *mSegment;
    bool mExtractedThumbnails;
    bool mIsLiveStreaming;
    bool mIsWebm;
    int64_t mSeekPreRollNs;

    status_t synthesizeAVCC(TrackInfo *trackInfo, size_t index);
    status_t initTrackInfo(
            const mkvparser::Track *track,
            MetaDataBase &meta,
            TrackInfo *trackInfo);
    void addTracks();
    void findThumbnails();
    void getColorInformation(
            const mkvparser::VideoTrack *vtrack,
            MetaDataBase &meta);
    bool isLiveStreaming() const;

    MatroskaExtractor(const MatroskaExtractor &);
    MatroskaExtractor &operator=(const MatroskaExtractor &);
};

}  // namespace android

#endif  // MATROSKA_EXTRACTOR_H_
