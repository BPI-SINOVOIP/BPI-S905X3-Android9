/*
 * Copyright (C) 2009 The Android Open Source Project
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

#ifndef MPEG4_EXTRACTOR_H_

#define MPEG4_EXTRACTOR_H_

#include <arpa/inet.h>

#include <media/DataSourceBase.h>
#include <media/MediaExtractor.h>
#include <media/stagefright/MetaDataBase.h>
#include <media/stagefright/foundation/AString.h>
#include <utils/KeyedVector.h>
#include <utils/List.h>
#include <utils/String8.h>
#include <utils/Vector.h>

namespace android {
struct AMessage;
class DataSourceBase;
struct CachedRangedDataSource;
class SampleTable;
class String8;
namespace heif {
class ItemTable;
}
using heif::ItemTable;

struct SidxEntry {
    size_t mSize;
    uint32_t mDurationUs;
};

struct Trex {
    uint32_t track_ID;
    uint32_t default_sample_description_index;
    uint32_t default_sample_duration;
    uint32_t default_sample_size;
    uint32_t default_sample_flags;
};

class MPEG4Extractor : public MediaExtractor {
public:
    explicit MPEG4Extractor(DataSourceBase *source, const char *mime = NULL);

    virtual size_t countTracks();
    virtual MediaTrack *getTrack(size_t index);
    virtual status_t getTrackMetaData(MetaDataBase& meta, size_t index, uint32_t flags);

    virtual status_t getMetaData(MetaDataBase& meta);
    virtual uint32_t flags() const;
    virtual const char * name() { return "MPEG4Extractor"; }

protected:
    virtual ~MPEG4Extractor();

private:

    struct PsshInfo {
        uint8_t uuid[16];
        uint32_t datalen;
        uint8_t *data;
    };
    struct Track {
        Track *next;
        MetaDataBase meta;
        uint32_t timescale;
        sp<SampleTable> sampleTable;
        bool includes_expensive_metadata;
        bool skipTrack;
        bool has_elst;
        int64_t elst_media_time;
        uint64_t elst_segment_duration;
        bool subsample_encryption;
    };

    Vector<SidxEntry> mSidxEntries;
    off64_t mMoofOffset;
    bool mMoofFound;
    bool mMdatFound;

    Vector<PsshInfo> mPssh;

    Vector<Trex> mTrex;

    DataSourceBase *mDataSource;
    CachedRangedDataSource *mCachedSource;
    status_t mInitCheck;
    uint32_t mHeaderTimescale;
    bool mIsQT;
    bool mIsHeif;
    bool mHasMoovBox;
    bool mPreferHeif;

    Track *mFirstTrack, *mLastTrack;

    MetaDataBase mFileMetaData;

    Vector<uint32_t> mPath;
    String8 mLastCommentMean;
    String8 mLastCommentName;
    String8 mLastCommentData;

    KeyedVector<uint32_t, AString> mMetaKeyMap;

    status_t readMetaData();
    status_t parseChunk(off64_t *offset, int depth);
    status_t parseITunesMetaData(off64_t offset, size_t size);
    status_t parseColorInfo(off64_t offset, size_t size);
    status_t parse3GPPMetaData(off64_t offset, size_t size, int depth);
    void parseID3v2MetaData(off64_t offset);
    status_t parseQTMetaKey(off64_t data_offset, size_t data_size);
    status_t parseQTMetaVal(int32_t keyId, off64_t data_offset, size_t data_size);

    status_t updateAudioTrackInfoFromESDS_MPEG4Audio(
            const void *esds_data, size_t esds_size);

    static status_t verifyTrack(Track *track);

    sp<ItemTable> mItemTable;

    status_t parseTrackHeader(off64_t data_offset, off64_t data_size);

    status_t parseSegmentIndex(off64_t data_offset, size_t data_size);

    Track *findTrackByMimePrefix(const char *mimePrefix);

    status_t parseAC3SampleEntry(off64_t offset);
    status_t parseAC3SpecificBox(off64_t offset, uint16_t sampleRate);

    MPEG4Extractor(const MPEG4Extractor &);
    MPEG4Extractor &operator=(const MPEG4Extractor &);
};

bool SniffMPEG4(
        DataSourceBase *source, String8 *mimeType, float *confidence,
        sp<AMessage> *);

}  // namespace android

#endif  // MPEG4_EXTRACTOR_H_
