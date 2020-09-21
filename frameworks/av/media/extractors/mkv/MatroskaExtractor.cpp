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

//#define LOG_NDEBUG 0
#define LOG_TAG "MatroskaExtractor"
#include <utils/Log.h>

#include "FLACDecoder.h"
#include "MatroskaExtractor.h"

#include <media/DataSourceBase.h>
#include <media/ExtractorUtils.h>
#include <media/MediaTrack.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AUtils.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/ByteUtils.h>
#include <media/stagefright/foundation/ColorUtils.h>
#include <media/stagefright/foundation/hexdump.h>
#include <media/stagefright/MediaBufferBase.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/MetaDataUtils.h>
#include <utils/String8.h>

#include <arpa/inet.h>
#include <inttypes.h>
#include <vector>

namespace android {

struct DataSourceBaseReader : public mkvparser::IMkvReader {
    explicit DataSourceBaseReader(DataSourceBase *source)
        : mSource(source) {
    }

    virtual int Read(long long position, long length, unsigned char* buffer) {
        CHECK(position >= 0);
        CHECK(length >= 0);

        if (length == 0) {
            return 0;
        }

        ssize_t n = mSource->readAt(position, buffer, length);

        if (n <= 0) {
            return -1;
        }

        return 0;
    }

    virtual int Length(long long* total, long long* available) {
        off64_t size;
        if (mSource->getSize(&size) != OK) {
            *total = -1;
            *available = (long long)((1ull << 63) - 1);

            return 0;
        }

        if (total) {
            *total = size;
        }

        if (available) {
            *available = size;
        }

        return 0;
    }

private:
    DataSourceBase *mSource;

    DataSourceBaseReader(const DataSourceBaseReader &);
    DataSourceBaseReader &operator=(const DataSourceBaseReader &);
};

////////////////////////////////////////////////////////////////////////////////

struct BlockIterator {
    BlockIterator(MatroskaExtractor *extractor, unsigned long trackNum, unsigned long index);

    bool eos() const;

    void advance();
    void reset();

    void seek(
            int64_t seekTimeUs, bool isAudio,
            int64_t *actualFrameTimeUs);

    const mkvparser::Block *block() const;
    int64_t blockTimeUs() const;

private:
    MatroskaExtractor *mExtractor;
    long long mTrackNum;
    unsigned long mIndex;

    const mkvparser::Cluster *mCluster;
    const mkvparser::BlockEntry *mBlockEntry;
    long mBlockEntryIndex;

    void advance_l();

    BlockIterator(const BlockIterator &);
    BlockIterator &operator=(const BlockIterator &);
};

struct MatroskaSource : public MediaTrack {
    MatroskaSource(MatroskaExtractor *extractor, size_t index);

    virtual status_t start(MetaDataBase *params);
    virtual status_t stop();

    virtual status_t getFormat(MetaDataBase &);

    virtual status_t read(
            MediaBufferBase **buffer, const ReadOptions *options);

protected:
    virtual ~MatroskaSource();

private:
    enum Type {
        AVC,
        AAC,
        HEVC,
        OTHER
    };

    MatroskaExtractor *mExtractor;
    size_t mTrackIndex;
    Type mType;
    bool mIsAudio;
    BlockIterator mBlockIter;
    ssize_t mNALSizeLen;  // for type AVC or HEVC

    List<MediaBufferBase *> mPendingFrames;

    status_t advance();

    status_t setWebmBlockCryptoInfo(MediaBufferBase *mbuf);
    status_t readBlock();
    void clearPendingFrames();

    MatroskaSource(const MatroskaSource &);
    MatroskaSource &operator=(const MatroskaSource &);
};

const mkvparser::Track* MatroskaExtractor::TrackInfo::getTrack() const {
    return mExtractor->mSegment->GetTracks()->GetTrackByNumber(mTrackNum);
}

// This function does exactly the same as mkvparser::Cues::Find, except that it
// searches in our own track based vectors. We should not need this once mkvparser
// adds the same functionality.
const mkvparser::CuePoint::TrackPosition *MatroskaExtractor::TrackInfo::find(
        long long timeNs) const {
    ALOGV("mCuePoints.size %zu", mCuePoints.size());
    if (mCuePoints.empty()) {
        return NULL;
    }

    const mkvparser::CuePoint* cp = mCuePoints.itemAt(0);
    const mkvparser::Track* track = getTrack();
    if (timeNs <= cp->GetTime(mExtractor->mSegment)) {
        return cp->Find(track);
    }

    // Binary searches through relevant cues; assumes cues are ordered by timecode.
    // If we do detect out-of-order cues, return NULL.
    size_t lo = 0;
    size_t hi = mCuePoints.size();
    while (lo < hi) {
        const size_t mid = lo + (hi - lo) / 2;
        const mkvparser::CuePoint* const midCp = mCuePoints.itemAt(mid);
        const long long cueTimeNs = midCp->GetTime(mExtractor->mSegment);
        if (cueTimeNs <= timeNs) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }

    if (lo == 0) {
        return NULL;
    }

    cp = mCuePoints.itemAt(lo - 1);
    if (cp->GetTime(mExtractor->mSegment) > timeNs) {
        return NULL;
    }

    return cp->Find(track);
}

MatroskaSource::MatroskaSource(
        MatroskaExtractor *extractor, size_t index)
    : mExtractor(extractor),
      mTrackIndex(index),
      mType(OTHER),
      mIsAudio(false),
      mBlockIter(mExtractor,
                 mExtractor->mTracks.itemAt(index).mTrackNum,
                 index),
      mNALSizeLen(-1) {
    MetaDataBase &meta = mExtractor->mTracks.editItemAt(index).mMeta;

    const char *mime;
    CHECK(meta.findCString(kKeyMIMEType, &mime));

    mIsAudio = !strncasecmp("audio/", mime, 6);

    if (!strcasecmp(mime, MEDIA_MIMETYPE_VIDEO_AVC)) {
        mType = AVC;

        uint32_t dummy;
        const uint8_t *avcc;
        size_t avccSize;
        int32_t nalSizeLen = 0;
        if (meta.findInt32(kKeyNalLengthSize, &nalSizeLen)) {
            if (nalSizeLen >= 0 && nalSizeLen <= 4) {
                mNALSizeLen = nalSizeLen;
            }
        } else if (meta.findData(kKeyAVCC, &dummy, (const void **)&avcc, &avccSize)
                && avccSize >= 5u) {
            mNALSizeLen = 1 + (avcc[4] & 3);
            ALOGV("mNALSizeLen = %zd", mNALSizeLen);
        } else {
            ALOGE("No mNALSizeLen");
        }
    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_VIDEO_HEVC)) {
        mType = HEVC;

        uint32_t dummy;
        const uint8_t *hvcc;
        size_t hvccSize;
        if (meta.findData(kKeyHVCC, &dummy, (const void **)&hvcc, &hvccSize)
                && hvccSize >= 22u) {
            mNALSizeLen = 1 + (hvcc[14+7] & 3);
            ALOGV("mNALSizeLen = %zu", mNALSizeLen);
        } else {
            ALOGE("No mNALSizeLen");
        }
    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_AAC)) {
        mType = AAC;
    }
}

MatroskaSource::~MatroskaSource() {
    clearPendingFrames();
}

status_t MatroskaSource::start(MetaDataBase * /* params */) {
    if (mType == AVC && mNALSizeLen < 0) {
        return ERROR_MALFORMED;
    }

    mBlockIter.reset();

    return OK;
}

status_t MatroskaSource::stop() {
    clearPendingFrames();

    return OK;
}

status_t MatroskaSource::getFormat(MetaDataBase &meta) {
    meta = mExtractor->mTracks.itemAt(mTrackIndex).mMeta;
    return OK;
}

////////////////////////////////////////////////////////////////////////////////

BlockIterator::BlockIterator(
        MatroskaExtractor *extractor, unsigned long trackNum, unsigned long index)
    : mExtractor(extractor),
      mTrackNum(trackNum),
      mIndex(index),
      mCluster(NULL),
      mBlockEntry(NULL),
      mBlockEntryIndex(0) {
    reset();
}

bool BlockIterator::eos() const {
    return mCluster == NULL || mCluster->EOS();
}

void BlockIterator::advance() {
    Mutex::Autolock autoLock(mExtractor->mLock);
    advance_l();
}

void BlockIterator::advance_l() {
    for (;;) {
        long res = mCluster->GetEntry(mBlockEntryIndex, mBlockEntry);
        ALOGV("GetEntry returned %ld", res);

        long long pos;
        long len;
        if (res < 0) {
            // Need to parse this cluster some more

            CHECK_EQ(res, mkvparser::E_BUFFER_NOT_FULL);

            res = mCluster->Parse(pos, len);
            ALOGV("Parse returned %ld", res);

            if (res < 0) {
                // I/O error

                ALOGE("Cluster::Parse returned result %ld", res);

                mCluster = NULL;
                break;
            }

            continue;
        } else if (res == 0) {
            // We're done with this cluster

            const mkvparser::Cluster *nextCluster;
            res = mExtractor->mSegment->ParseNext(
                    mCluster, nextCluster, pos, len);
            ALOGV("ParseNext returned %ld", res);

            if (res != 0) {
                // EOF or error

                mCluster = NULL;
                break;
            }

            CHECK_EQ(res, 0);
            CHECK(nextCluster != NULL);
            CHECK(!nextCluster->EOS());

            mCluster = nextCluster;

            res = mCluster->Parse(pos, len);
            ALOGV("Parse (2) returned %ld", res);

            if (res < 0) {
                // I/O error

                ALOGE("Cluster::Parse returned result %ld", res);

                mCluster = NULL;
                break;
            }

            mBlockEntryIndex = 0;
            continue;
        }

        CHECK(mBlockEntry != NULL);
        CHECK(mBlockEntry->GetBlock() != NULL);
        ++mBlockEntryIndex;

        if (mBlockEntry->GetBlock()->GetTrackNumber() == mTrackNum) {
            break;
        }
    }
}

void BlockIterator::reset() {
    Mutex::Autolock autoLock(mExtractor->mLock);

    mCluster = mExtractor->mSegment->GetFirst();
    mBlockEntry = NULL;
    mBlockEntryIndex = 0;

    do {
        advance_l();
    } while (!eos() && block()->GetTrackNumber() != mTrackNum);
}

void BlockIterator::seek(
        int64_t seekTimeUs, bool isAudio,
        int64_t *actualFrameTimeUs) {
    Mutex::Autolock autoLock(mExtractor->mLock);

    *actualFrameTimeUs = -1ll;

    if (seekTimeUs > INT64_MAX / 1000ll ||
            seekTimeUs < INT64_MIN / 1000ll ||
            (mExtractor->mSeekPreRollNs > 0 &&
                    (seekTimeUs * 1000ll) < INT64_MIN + mExtractor->mSeekPreRollNs) ||
            (mExtractor->mSeekPreRollNs < 0 &&
                    (seekTimeUs * 1000ll) > INT64_MAX + mExtractor->mSeekPreRollNs)) {
        ALOGE("cannot seek to %lld", (long long) seekTimeUs);
        return;
    }

    const int64_t seekTimeNs = seekTimeUs * 1000ll - mExtractor->mSeekPreRollNs;

    mkvparser::Segment* const pSegment = mExtractor->mSegment;

    // Special case the 0 seek to avoid loading Cues when the application
    // extraneously seeks to 0 before playing.
    if (seekTimeNs <= 0) {
        ALOGV("Seek to beginning: %" PRId64, seekTimeUs);
        mCluster = pSegment->GetFirst();
        mBlockEntryIndex = 0;
        do {
            advance_l();
        } while (!eos() && block()->GetTrackNumber() != mTrackNum);
        return;
    }

    ALOGV("Seeking to: %" PRId64, seekTimeUs);

    // If the Cues have not been located then find them.
    const mkvparser::Cues* pCues = pSegment->GetCues();
    const mkvparser::SeekHead* pSH = pSegment->GetSeekHead();
    if (!pCues && pSH) {
        const size_t count = pSH->GetCount();
        const mkvparser::SeekHead::Entry* pEntry;
        ALOGV("No Cues yet");

        for (size_t index = 0; index < count; index++) {
            pEntry = pSH->GetEntry(index);

            if (pEntry->id == 0x0C53BB6B) { // Cues ID
                long len; long long pos;
                pSegment->ParseCues(pEntry->pos, pos, len);
                pCues = pSegment->GetCues();
                ALOGV("Cues found");
                break;
            }
        }

        if (!pCues) {
            ALOGE("No Cues in file");
            return;
        }
    }
    else if (!pSH) {
        ALOGE("No SeekHead");
        return;
    }

    const mkvparser::CuePoint* pCP;
    mkvparser::Tracks const *pTracks = pSegment->GetTracks();
    while (!pCues->DoneParsing()) {
        pCues->LoadCuePoint();
        pCP = pCues->GetLast();
        CHECK(pCP);

        size_t trackCount = mExtractor->mTracks.size();
        for (size_t index = 0; index < trackCount; ++index) {
            MatroskaExtractor::TrackInfo& track = mExtractor->mTracks.editItemAt(index);
            const mkvparser::Track *pTrack = pTracks->GetTrackByNumber(track.mTrackNum);
            if (pTrack && pTrack->GetType() == 1 && pCP->Find(pTrack)) { // VIDEO_TRACK
                track.mCuePoints.push_back(pCP);
            }
        }

        if (pCP->GetTime(pSegment) >= seekTimeNs) {
            ALOGV("Parsed past relevant Cue");
            break;
        }
    }

    const mkvparser::CuePoint::TrackPosition *pTP = NULL;
    const mkvparser::Track *thisTrack = pTracks->GetTrackByNumber(mTrackNum);
    if (thisTrack->GetType() == 1) { // video
        MatroskaExtractor::TrackInfo& track = mExtractor->mTracks.editItemAt(mIndex);
        pTP = track.find(seekTimeNs);
    } else {
        // The Cue index is built around video keyframes
        unsigned long int trackCount = pTracks->GetTracksCount();
        for (size_t index = 0; index < trackCount; ++index) {
            const mkvparser::Track *pTrack = pTracks->GetTrackByIndex(index);
            if (pTrack && pTrack->GetType() == 1 && pCues->Find(seekTimeNs, pTrack, pCP, pTP)) {
                ALOGV("Video track located at %zu", index);
                break;
            }
        }
    }


    // Always *search* based on the video track, but finalize based on mTrackNum
    if (!pTP) {
        ALOGE("Did not locate the video track for seeking");
        return;
    }

    mCluster = pSegment->FindOrPreloadCluster(pTP->m_pos);

    CHECK(mCluster);
    CHECK(!mCluster->EOS());

    // mBlockEntryIndex starts at 0 but m_block starts at 1
    CHECK_GT(pTP->m_block, 0);
    mBlockEntryIndex = pTP->m_block - 1;

    for (;;) {
        advance_l();

        if (eos()) break;

        if (isAudio || block()->IsKey()) {
            // Accept the first key frame
            int64_t frameTimeUs = (block()->GetTime(mCluster) + 500LL) / 1000LL;
            if (thisTrack->GetType() == 1 || frameTimeUs >= seekTimeUs) {
                *actualFrameTimeUs = frameTimeUs;
                ALOGV("Requested seek point: %" PRId64 " actual: %" PRId64,
                      seekTimeUs, *actualFrameTimeUs);
                break;
            }
        }
    }
}

const mkvparser::Block *BlockIterator::block() const {
    CHECK(!eos());

    return mBlockEntry->GetBlock();
}

int64_t BlockIterator::blockTimeUs() const {
    if (mCluster == NULL || mBlockEntry == NULL) {
        return -1;
    }
    return (mBlockEntry->GetBlock()->GetTime(mCluster) + 500ll) / 1000ll;
}

////////////////////////////////////////////////////////////////////////////////

static unsigned U24_AT(const uint8_t *ptr) {
    return ptr[0] << 16 | ptr[1] << 8 | ptr[2];
}

static AString uriDebugString(const char *uri) {
    // find scheme
    AString scheme;
    for (size_t i = 0; i < strlen(uri); i++) {
        const char c = uri[i];
        if (!isascii(c)) {
            break;
        } else if (isalpha(c)) {
            continue;
        } else if (i == 0) {
            // first character must be a letter
            break;
        } else if (isdigit(c) || c == '+' || c == '.' || c =='-') {
            continue;
        } else if (c != ':') {
            break;
        }
        scheme = AString(uri, 0, i);
        scheme.append("://<suppressed>");
        return scheme;
    }
    return AString("<no-scheme URI suppressed>");
}

void MatroskaSource::clearPendingFrames() {
    while (!mPendingFrames.empty()) {
        MediaBufferBase *frame = *mPendingFrames.begin();
        mPendingFrames.erase(mPendingFrames.begin());

        frame->release();
        frame = NULL;
    }
}

status_t MatroskaSource::setWebmBlockCryptoInfo(MediaBufferBase *mbuf) {
    if (mbuf->range_length() < 1 || mbuf->range_length() - 1 > INT32_MAX) {
        // 1-byte signal
        return ERROR_MALFORMED;
    }

    const uint8_t *data = (const uint8_t *)mbuf->data() + mbuf->range_offset();
    bool encrypted = data[0] & 0x1;
    bool partitioned = data[0] & 0x2;
    if (encrypted && mbuf->range_length() < 9) {
        // 1-byte signal + 8-byte IV
        return ERROR_MALFORMED;
    }

    MetaDataBase &meta = mbuf->meta_data();
    if (encrypted) {
        uint8_t ctrCounter[16] = { 0 };
        uint32_t type;
        const uint8_t *keyId;
        size_t keyIdSize;
        const MetaDataBase &trackMeta = mExtractor->mTracks.itemAt(mTrackIndex).mMeta;
        CHECK(trackMeta.findData(kKeyCryptoKey, &type, (const void **)&keyId, &keyIdSize));
        meta.setData(kKeyCryptoKey, 0, keyId, keyIdSize);
        memcpy(ctrCounter, data + 1, 8);
        meta.setData(kKeyCryptoIV, 0, ctrCounter, 16);
        if (partitioned) {
            /*  0                   1                   2                   3
             *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
             * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
             * |  Signal Byte  |                                               |
             * +-+-+-+-+-+-+-+-+             IV                                |
             * |                                                               |
             * |               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
             * |               | num_partition |     Partition 0 offset ->     |
             * |-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-|
             * |     -> Partition 0 offset     |              ...              |
             * |-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-|
             * |             ...               |     Partition n-1 offset ->   |
             * |-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-|
             * |     -> Partition n-1 offset   |                               |
             * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               |
             * |                    Clear/encrypted sample data                |
             * |                                                               |
             * |                                                               |
             * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
             */
            if (mbuf->range_length() < 10) {
                return ERROR_MALFORMED;
            }
            uint8_t numPartitions = data[9];
            if (mbuf->range_length() - 10 < numPartitions * sizeof(uint32_t)) {
                return ERROR_MALFORMED;
            }
            std::vector<uint32_t> plainSizes, encryptedSizes;
            uint32_t prev = 0;
            uint32_t frameOffset = 10 + numPartitions * sizeof(uint32_t);
            const uint32_t *partitions = reinterpret_cast<const uint32_t*>(data + 10);
            for (uint32_t i = 0; i <= numPartitions; ++i) {
                uint32_t p_i = i < numPartitions
                        ? ntohl(partitions[i])
                        : (mbuf->range_length() - frameOffset);
                if (p_i < prev) {
                    return ERROR_MALFORMED;
                }
                uint32_t size = p_i - prev;
                prev = p_i;
                if (i % 2) {
                    encryptedSizes.push_back(size);
                } else {
                    plainSizes.push_back(size);
                }
            }
            if (plainSizes.size() > encryptedSizes.size()) {
                encryptedSizes.push_back(0);
            }
            uint32_t sizeofPlainSizes = sizeof(uint32_t) * plainSizes.size();
            uint32_t sizeofEncryptedSizes = sizeof(uint32_t) * encryptedSizes.size();
            meta.setData(kKeyPlainSizes, 0, plainSizes.data(), sizeofPlainSizes);
            meta.setData(kKeyEncryptedSizes, 0, encryptedSizes.data(), sizeofEncryptedSizes);
            mbuf->set_range(frameOffset, mbuf->range_length() - frameOffset);
        } else {
            /*
             *  0                   1                   2                   3
             *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
             *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
             *  |  Signal Byte  |                                               |
             *  +-+-+-+-+-+-+-+-+             IV                                |
             *  |                                                               |
             *  |               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
             *  |               |                                               |
             *  |-+-+-+-+-+-+-+-+                                               |
             *  :               Bytes 1..N of encrypted frame                   :
             *  |                                                               |
             *  |                                                               |
             *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
             */
            int32_t plainSizes[] = { 0 };
            int32_t encryptedSizes[] = { static_cast<int32_t>(mbuf->range_length() - 9) };
            meta.setData(kKeyPlainSizes, 0, plainSizes, sizeof(plainSizes));
            meta.setData(kKeyEncryptedSizes, 0, encryptedSizes, sizeof(encryptedSizes));
            mbuf->set_range(9, mbuf->range_length() - 9);
        }
    } else {
        /*
         *  0                   1                   2                   3
         *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
         *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         *  |  Signal Byte  |                                               |
         *  +-+-+-+-+-+-+-+-+                                               |
         *  :               Bytes 1..N of unencrypted frame                 :
         *  |                                                               |
         *  |                                                               |
         *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         */
        int32_t plainSizes[] = { static_cast<int32_t>(mbuf->range_length() - 1) };
        int32_t encryptedSizes[] = { 0 };
        meta.setData(kKeyPlainSizes, 0, plainSizes, sizeof(plainSizes));
        meta.setData(kKeyEncryptedSizes, 0, encryptedSizes, sizeof(encryptedSizes));
        mbuf->set_range(1, mbuf->range_length() - 1);
    }

    return OK;
}

status_t MatroskaSource::readBlock() {
    CHECK(mPendingFrames.empty());

    if (mBlockIter.eos()) {
        return ERROR_END_OF_STREAM;
    }

    const mkvparser::Block *block = mBlockIter.block();

    int64_t timeUs = mBlockIter.blockTimeUs();

    for (int i = 0; i < block->GetFrameCount(); ++i) {
        MatroskaExtractor::TrackInfo *trackInfo = &mExtractor->mTracks.editItemAt(mTrackIndex);
        const mkvparser::Block::Frame &frame = block->GetFrame(i);
        size_t len = frame.len;
        if (SIZE_MAX - len < trackInfo->mHeaderLen) {
            return ERROR_MALFORMED;
        }

        len += trackInfo->mHeaderLen;
        MediaBufferBase *mbuf = MediaBufferBase::Create(len);
        uint8_t *data = static_cast<uint8_t *>(mbuf->data());
        if (trackInfo->mHeader) {
            memcpy(data, trackInfo->mHeader, trackInfo->mHeaderLen);
        }

        mbuf->meta_data().setInt64(kKeyTime, timeUs);
        mbuf->meta_data().setInt32(kKeyIsSyncFrame, block->IsKey());

        status_t err = frame.Read(mExtractor->mReader, data + trackInfo->mHeaderLen);
        if (err == OK
                && mExtractor->mIsWebm
                && trackInfo->mEncrypted) {
            err = setWebmBlockCryptoInfo(mbuf);
        }

        if (err != OK) {
            mPendingFrames.clear();

            mBlockIter.advance();
            mbuf->release();
            return err;
        }

        mPendingFrames.push_back(mbuf);
    }

    mBlockIter.advance();

    return OK;
}

status_t MatroskaSource::read(
        MediaBufferBase **out, const ReadOptions *options) {
    *out = NULL;

    int64_t targetSampleTimeUs = -1ll;

    int64_t seekTimeUs;
    ReadOptions::SeekMode mode;
    if (options && options->getSeekTo(&seekTimeUs, &mode)) {
        if (mode == ReadOptions::SEEK_FRAME_INDEX) {
            return ERROR_UNSUPPORTED;
        }

        if (!mExtractor->isLiveStreaming()) {
            clearPendingFrames();

            // The audio we want is located by using the Cues to seek the video
            // stream to find the target Cluster then iterating to finalize for
            // audio.
            int64_t actualFrameTimeUs;
            mBlockIter.seek(seekTimeUs, mIsAudio, &actualFrameTimeUs);
            if (mode == ReadOptions::SEEK_CLOSEST) {
                targetSampleTimeUs = actualFrameTimeUs;
            }
        }
    }

    while (mPendingFrames.empty()) {
        status_t err = readBlock();

        if (err != OK) {
            clearPendingFrames();

            return err;
        }
    }

    MediaBufferBase *frame = *mPendingFrames.begin();
    mPendingFrames.erase(mPendingFrames.begin());

    if ((mType != AVC && mType != HEVC) || mNALSizeLen == 0) {
        if (targetSampleTimeUs >= 0ll) {
            frame->meta_data().setInt64(
                    kKeyTargetTime, targetSampleTimeUs);
        }

        *out = frame;

        return OK;
    }

    // Each input frame contains one or more NAL fragments, each fragment
    // is prefixed by mNALSizeLen bytes giving the fragment length,
    // followed by a corresponding number of bytes containing the fragment.
    // We output all these fragments into a single large buffer separated
    // by startcodes (0x00 0x00 0x00 0x01).
    //
    // When mNALSizeLen is 0, we assume the data is already in the format
    // desired.

    const uint8_t *srcPtr =
        (const uint8_t *)frame->data() + frame->range_offset();

    size_t srcSize = frame->range_length();

    size_t dstSize = 0;
    MediaBufferBase *buffer = NULL;
    uint8_t *dstPtr = NULL;

    for (int32_t pass = 0; pass < 2; ++pass) {
        size_t srcOffset = 0;
        size_t dstOffset = 0;
        while (srcOffset + mNALSizeLen <= srcSize) {
            size_t NALsize;
            switch (mNALSizeLen) {
                case 1: NALsize = srcPtr[srcOffset]; break;
                case 2: NALsize = U16_AT(srcPtr + srcOffset); break;
                case 3: NALsize = U24_AT(srcPtr + srcOffset); break;
                case 4: NALsize = U32_AT(srcPtr + srcOffset); break;
                default:
                    TRESPASS();
            }

            if (srcOffset + mNALSizeLen + NALsize <= srcOffset + mNALSizeLen) {
                frame->release();
                frame = NULL;

                return ERROR_MALFORMED;
            } else if (srcOffset + mNALSizeLen + NALsize > srcSize) {
                break;
            }

            if (pass == 1) {
                memcpy(&dstPtr[dstOffset], "\x00\x00\x00\x01", 4);

                if (frame != buffer) {
                    memcpy(&dstPtr[dstOffset + 4],
                           &srcPtr[srcOffset + mNALSizeLen],
                           NALsize);
                }
            }

            dstOffset += 4;  // 0x00 00 00 01
            dstOffset += NALsize;

            srcOffset += mNALSizeLen + NALsize;
        }

        if (srcOffset < srcSize) {
            // There were trailing bytes or not enough data to complete
            // a fragment.

            frame->release();
            frame = NULL;

            return ERROR_MALFORMED;
        }

        if (pass == 0) {
            dstSize = dstOffset;

            if (dstSize == srcSize && mNALSizeLen == 4) {
                // In this special case we can re-use the input buffer by substituting
                // each 4-byte nal size with a 4-byte start code
                buffer = frame;
            } else {
                buffer = MediaBufferBase::Create(dstSize);
            }

            int64_t timeUs;
            CHECK(frame->meta_data().findInt64(kKeyTime, &timeUs));
            int32_t isSync;
            CHECK(frame->meta_data().findInt32(kKeyIsSyncFrame, &isSync));

            buffer->meta_data().setInt64(kKeyTime, timeUs);
            buffer->meta_data().setInt32(kKeyIsSyncFrame, isSync);

            dstPtr = (uint8_t *)buffer->data();
        }
    }

    if (frame != buffer) {
        frame->release();
        frame = NULL;
    }

    if (targetSampleTimeUs >= 0ll) {
        buffer->meta_data().setInt64(
                kKeyTargetTime, targetSampleTimeUs);
    }

    *out = buffer;

    return OK;
}

////////////////////////////////////////////////////////////////////////////////

MatroskaExtractor::MatroskaExtractor(DataSourceBase *source)
    : mDataSource(source),
      mReader(new DataSourceBaseReader(mDataSource)),
      mSegment(NULL),
      mExtractedThumbnails(false),
      mIsWebm(false),
      mSeekPreRollNs(0) {
    off64_t size;
    mIsLiveStreaming =
        (mDataSource->flags()
            & (DataSourceBase::kWantsPrefetching
                | DataSourceBase::kIsCachingDataSource))
        && mDataSource->getSize(&size) != OK;

    mkvparser::EBMLHeader ebmlHeader;
    long long pos;
    if (ebmlHeader.Parse(mReader, pos) < 0) {
        return;
    }

    if (ebmlHeader.m_docType && !strcmp("webm", ebmlHeader.m_docType)) {
        mIsWebm = true;
    }

    long long ret =
        mkvparser::Segment::CreateInstance(mReader, pos, mSegment);

    if (ret) {
        CHECK(mSegment == NULL);
        return;
    }

    // from mkvparser::Segment::Load(), but stop at first cluster
    ret = mSegment->ParseHeaders();
    if (ret == 0) {
        long len;
        ret = mSegment->LoadCluster(pos, len);
        if (ret >= 1) {
            // no more clusters
            ret = 0;
        }
    } else if (ret > 0) {
        ret = mkvparser::E_BUFFER_NOT_FULL;
    }

    if (ret < 0) {
        char uri[1024];
        if(!mDataSource->getUri(uri, sizeof(uri))) {
            uri[0] = '\0';
        }
        ALOGW("Corrupt %s source: %s", mIsWebm ? "webm" : "matroska",
                uriDebugString(uri).c_str());
        delete mSegment;
        mSegment = NULL;
        return;
    }

#if 0
    const mkvparser::SegmentInfo *info = mSegment->GetInfo();
    ALOGI("muxing app: %s, writing app: %s",
         info->GetMuxingAppAsUTF8(),
         info->GetWritingAppAsUTF8());
#endif

    addTracks();
}

MatroskaExtractor::~MatroskaExtractor() {
    delete mSegment;
    mSegment = NULL;

    delete mReader;
    mReader = NULL;
}

size_t MatroskaExtractor::countTracks() {
    return mTracks.size();
}

MediaTrack *MatroskaExtractor::getTrack(size_t index) {
    if (index >= mTracks.size()) {
        return NULL;
    }

    return new MatroskaSource(this, index);
}

status_t MatroskaExtractor::getTrackMetaData(
        MetaDataBase &meta,
        size_t index, uint32_t flags) {
    if (index >= mTracks.size()) {
        return UNKNOWN_ERROR;
    }

    if ((flags & kIncludeExtensiveMetaData) && !mExtractedThumbnails
            && !isLiveStreaming()) {
        findThumbnails();
        mExtractedThumbnails = true;
    }

    meta = mTracks.itemAt(index).mMeta;
    return OK;
}

bool MatroskaExtractor::isLiveStreaming() const {
    return mIsLiveStreaming;
}

static int bytesForSize(size_t size) {
    // use at most 28 bits (4 times 7)
    CHECK(size <= 0xfffffff);

    if (size > 0x1fffff) {
        return 4;
    } else if (size > 0x3fff) {
        return 3;
    } else if (size > 0x7f) {
        return 2;
    }
    return 1;
}

static void storeSize(uint8_t *data, size_t &idx, size_t size) {
    int numBytes = bytesForSize(size);
    idx += numBytes;

    data += idx;
    size_t next = 0;
    while (numBytes--) {
        *--data = (size & 0x7f) | next;
        size >>= 7;
        next = 0x80;
    }
}

static void addESDSFromCodecPrivate(
        MetaDataBase &meta,
        bool isAudio, const void *priv, size_t privSize) {

    int privSizeBytesRequired = bytesForSize(privSize);
    int esdsSize2 = 14 + privSizeBytesRequired + privSize;
    int esdsSize2BytesRequired = bytesForSize(esdsSize2);
    int esdsSize1 = 4 + esdsSize2BytesRequired + esdsSize2;
    int esdsSize1BytesRequired = bytesForSize(esdsSize1);
    size_t esdsSize = 1 + esdsSize1BytesRequired + esdsSize1;
    uint8_t *esds = new uint8_t[esdsSize];

    size_t idx = 0;
    esds[idx++] = 0x03;
    storeSize(esds, idx, esdsSize1);
    esds[idx++] = 0x00; // ES_ID
    esds[idx++] = 0x00; // ES_ID
    esds[idx++] = 0x00; // streamDependenceFlag, URL_Flag, OCRstreamFlag
    esds[idx++] = 0x04;
    storeSize(esds, idx, esdsSize2);
    esds[idx++] = isAudio ? 0x40   // Audio ISO/IEC 14496-3
                          : 0x20;  // Visual ISO/IEC 14496-2
    for (int i = 0; i < 12; i++) {
        esds[idx++] = 0x00;
    }
    esds[idx++] = 0x05;
    storeSize(esds, idx, privSize);
    memcpy(esds + idx, priv, privSize);

    meta.setData(kKeyESDS, 0, esds, esdsSize);

    delete[] esds;
    esds = NULL;
}

status_t addVorbisCodecInfo(
        MetaDataBase &meta,
        const void *_codecPrivate, size_t codecPrivateSize) {
    // hexdump(_codecPrivate, codecPrivateSize);

    if (codecPrivateSize < 1) {
        return ERROR_MALFORMED;
    }

    const uint8_t *codecPrivate = (const uint8_t *)_codecPrivate;

    if (codecPrivate[0] != 0x02) {
        return ERROR_MALFORMED;
    }

    // codecInfo starts with two lengths, len1 and len2, that are
    // "Xiph-style-lacing encoded"...

    size_t offset = 1;
    size_t len1 = 0;
    while (offset < codecPrivateSize && codecPrivate[offset] == 0xff) {
        if (len1 > (SIZE_MAX - 0xff)) {
            return ERROR_MALFORMED; // would overflow
        }
        len1 += 0xff;
        ++offset;
    }
    if (offset >= codecPrivateSize) {
        return ERROR_MALFORMED;
    }
    if (len1 > (SIZE_MAX - codecPrivate[offset])) {
        return ERROR_MALFORMED; // would overflow
    }
    len1 += codecPrivate[offset++];

    size_t len2 = 0;
    while (offset < codecPrivateSize && codecPrivate[offset] == 0xff) {
        if (len2 > (SIZE_MAX - 0xff)) {
            return ERROR_MALFORMED; // would overflow
        }
        len2 += 0xff;
        ++offset;
    }
    if (offset >= codecPrivateSize) {
        return ERROR_MALFORMED;
    }
    if (len2 > (SIZE_MAX - codecPrivate[offset])) {
        return ERROR_MALFORMED; // would overflow
    }
    len2 += codecPrivate[offset++];

    if (len1 > SIZE_MAX - len2 || offset > SIZE_MAX - (len1 + len2) ||
            codecPrivateSize < offset + len1 + len2) {
        return ERROR_MALFORMED;
    }

    if (codecPrivate[offset] != 0x01) {
        return ERROR_MALFORMED;
    }
    meta.setData(kKeyVorbisInfo, 0, &codecPrivate[offset], len1);

    offset += len1;
    if (codecPrivate[offset] != 0x03) {
        return ERROR_MALFORMED;
    }

    offset += len2;
    if (codecPrivate[offset] != 0x05) {
        return ERROR_MALFORMED;
    }

    meta.setData(
            kKeyVorbisBooks, 0, &codecPrivate[offset],
            codecPrivateSize - offset);

    return OK;
}

static status_t addFlacMetadata(
        MetaDataBase &meta,
        const void *codecPrivate, size_t codecPrivateSize) {
    // hexdump(codecPrivate, codecPrivateSize);

    meta.setData(kKeyFlacMetadata, 0, codecPrivate, codecPrivateSize);

    int32_t maxInputSize = 64 << 10;
    FLACDecoder *flacDecoder = FLACDecoder::Create();
    if (flacDecoder != NULL
            && flacDecoder->parseMetadata((const uint8_t*)codecPrivate, codecPrivateSize) == OK) {
        FLAC__StreamMetadata_StreamInfo streamInfo = flacDecoder->getStreamInfo();
        maxInputSize = streamInfo.max_framesize;
        if (maxInputSize == 0) {
            // In case max framesize is not available, use raw data size as max framesize,
            // assuming there is no expansion.
            if (streamInfo.max_blocksize != 0
                    && streamInfo.channels != 0
                    && ((streamInfo.bits_per_sample + 7) / 8) >
                        INT32_MAX / streamInfo.max_blocksize / streamInfo.channels) {
                delete flacDecoder;
                return ERROR_MALFORMED;
            }
            maxInputSize = ((streamInfo.bits_per_sample + 7) / 8)
                * streamInfo.max_blocksize * streamInfo.channels;
        }
    }
    meta.setInt32(kKeyMaxInputSize, maxInputSize);

    delete flacDecoder;
    return OK;
}

status_t MatroskaExtractor::synthesizeAVCC(TrackInfo *trackInfo, size_t index) {
    BlockIterator iter(this, trackInfo->mTrackNum, index);
    if (iter.eos()) {
        return ERROR_MALFORMED;
    }

    const mkvparser::Block *block = iter.block();
    if (block->GetFrameCount() <= 0) {
        return ERROR_MALFORMED;
    }

    const mkvparser::Block::Frame &frame = block->GetFrame(0);
    auto tmpData = heapbuffer<unsigned char>(frame.len);
    long n = frame.Read(mReader, tmpData.get());
    if (n != 0) {
        return ERROR_MALFORMED;
    }

    if (!MakeAVCCodecSpecificData(trackInfo->mMeta, tmpData.get(), frame.len)) {
        return ERROR_MALFORMED;
    }

    // Override the synthesized nal length size, which is arbitrary
    trackInfo->mMeta.setInt32(kKeyNalLengthSize, 0);
    return OK;
}

static inline bool isValidInt32ColourValue(long long value) {
    return value != mkvparser::Colour::kValueNotPresent
            && value >= INT32_MIN
            && value <= INT32_MAX;
}

static inline bool isValidUint16ColourValue(long long value) {
    return value != mkvparser::Colour::kValueNotPresent
            && value >= 0
            && value <= UINT16_MAX;
}

static inline bool isValidPrimary(const mkvparser::PrimaryChromaticity *primary) {
    return primary != NULL && primary->x >= 0 && primary->x <= 1
             && primary->y >= 0 && primary->y <= 1;
}

void MatroskaExtractor::getColorInformation(
        const mkvparser::VideoTrack *vtrack, MetaDataBase &meta) {
    const mkvparser::Colour *color = vtrack->GetColour();
    if (color == NULL) {
        return;
    }

    // Color Aspects
    {
        int32_t primaries = 2; // ISO unspecified
        int32_t transfer = 2; // ISO unspecified
        int32_t coeffs = 2; // ISO unspecified
        bool fullRange = false; // default
        bool rangeSpecified = false;

        if (isValidInt32ColourValue(color->primaries)) {
            primaries = color->primaries;
        }
        if (isValidInt32ColourValue(color->transfer_characteristics)) {
            transfer = color->transfer_characteristics;
        }
        if (isValidInt32ColourValue(color->matrix_coefficients)) {
            coeffs = color->matrix_coefficients;
        }
        if (color->range != mkvparser::Colour::kValueNotPresent
                && color->range != 0 /* MKV unspecified */) {
            // We only support MKV broadcast range (== limited) and full range.
            // We treat all other value as the default limited range.
            fullRange = color->range == 2 /* MKV fullRange */;
            rangeSpecified = true;
        }

        ColorAspects aspects;
        ColorUtils::convertIsoColorAspectsToCodecAspects(
                primaries, transfer, coeffs, fullRange, aspects);
        meta.setInt32(kKeyColorPrimaries, aspects.mPrimaries);
        meta.setInt32(kKeyTransferFunction, aspects.mTransfer);
        meta.setInt32(kKeyColorMatrix, aspects.mMatrixCoeffs);
        meta.setInt32(
                kKeyColorRange, rangeSpecified ? aspects.mRange : ColorAspects::RangeUnspecified);
    }

    // HDR Static Info
    {
        HDRStaticInfo info, nullInfo; // nullInfo is a fully unspecified static info
        memset(&info, 0, sizeof(info));
        memset(&nullInfo, 0, sizeof(nullInfo));
        if (isValidUint16ColourValue(color->max_cll)) {
            info.sType1.mMaxContentLightLevel = color->max_cll;
        }
        if (isValidUint16ColourValue(color->max_fall)) {
            info.sType1.mMaxFrameAverageLightLevel = color->max_fall;
        }
        const mkvparser::MasteringMetadata *mastering = color->mastering_metadata;
        if (mastering != NULL) {
            // Convert matroska values to HDRStaticInfo equivalent values for each fully specified
            // group. See CTA-681.3 section 3.2.1 for more info.
            if (mastering->luminance_max >= 0.5 && mastering->luminance_max < 65535.5) {
                info.sType1.mMaxDisplayLuminance = (uint16_t)(mastering->luminance_max + 0.5);
            }
            if (mastering->luminance_min >= 0.00005 && mastering->luminance_min < 6.55355) {
                // HDRStaticInfo Type1 stores min luminance scaled 10000:1
                info.sType1.mMinDisplayLuminance =
                    (uint16_t)(10000 * mastering->luminance_min + 0.5);
            }
            // HDRStaticInfo Type1 stores primaries scaled 50000:1
            if (isValidPrimary(mastering->white_point)) {
                info.sType1.mW.x = (uint16_t)(50000 * mastering->white_point->x + 0.5);
                info.sType1.mW.y = (uint16_t)(50000 * mastering->white_point->y + 0.5);
            }
            if (isValidPrimary(mastering->r) && isValidPrimary(mastering->g)
                    && isValidPrimary(mastering->b)) {
                info.sType1.mR.x = (uint16_t)(50000 * mastering->r->x + 0.5);
                info.sType1.mR.y = (uint16_t)(50000 * mastering->r->y + 0.5);
                info.sType1.mG.x = (uint16_t)(50000 * mastering->g->x + 0.5);
                info.sType1.mG.y = (uint16_t)(50000 * mastering->g->y + 0.5);
                info.sType1.mB.x = (uint16_t)(50000 * mastering->b->x + 0.5);
                info.sType1.mB.y = (uint16_t)(50000 * mastering->b->y + 0.5);
            }
        }
        // Only advertise static info if at least one of the groups have been specified.
        if (memcmp(&info, &nullInfo, sizeof(info)) != 0) {
            info.mID = HDRStaticInfo::kType1;
            meta.setData(kKeyHdrStaticInfo, 'hdrS', &info, sizeof(info));
        }
    }
}

status_t MatroskaExtractor::initTrackInfo(
        const mkvparser::Track *track, MetaDataBase &meta, TrackInfo *trackInfo) {
    trackInfo->mTrackNum = track->GetNumber();
    trackInfo->mMeta = meta;
    trackInfo->mExtractor = this;
    trackInfo->mEncrypted = false;
    trackInfo->mHeader = NULL;
    trackInfo->mHeaderLen = 0;

    for(size_t i = 0; i < track->GetContentEncodingCount(); i++) {
        const mkvparser::ContentEncoding *encoding = track->GetContentEncodingByIndex(i);
        for(size_t j = 0; j < encoding->GetEncryptionCount(); j++) {
            const mkvparser::ContentEncoding::ContentEncryption *encryption;
            encryption = encoding->GetEncryptionByIndex(j);
            trackInfo->mMeta.setData(kKeyCryptoKey, 0, encryption->key_id, encryption->key_id_len);
            trackInfo->mEncrypted = true;
            break;
        }

        for(size_t j = 0; j < encoding->GetCompressionCount(); j++) {
            const mkvparser::ContentEncoding::ContentCompression *compression;
            compression = encoding->GetCompressionByIndex(j);
            ALOGV("compression algo %llu settings_len %lld",
                compression->algo, compression->settings_len);
            if (compression->algo == 3
                    && compression->settings
                    && compression->settings_len > 0) {
                trackInfo->mHeader = compression->settings;
                trackInfo->mHeaderLen = compression->settings_len;
            }
        }
    }

    return OK;
}

void MatroskaExtractor::addTracks() {
    const mkvparser::Tracks *tracks = mSegment->GetTracks();

    for (size_t index = 0; index < tracks->GetTracksCount(); ++index) {
        const mkvparser::Track *track = tracks->GetTrackByIndex(index);

        if (track == NULL) {
            // Apparently this is currently valid (if unexpected) behaviour
            // of the mkv parser lib.
            continue;
        }

        const char *const codecID = track->GetCodecId();
        ALOGV("codec id = %s", codecID);
        ALOGV("codec name = %s", track->GetCodecNameAsUTF8());

        if (codecID == NULL) {
            ALOGW("unknown codecID is not supported.");
            continue;
        }

        size_t codecPrivateSize;
        const unsigned char *codecPrivate =
            track->GetCodecPrivate(codecPrivateSize);

        enum { VIDEO_TRACK = 1, AUDIO_TRACK = 2 };

        MetaDataBase meta;

        status_t err = OK;

        switch (track->GetType()) {
            case VIDEO_TRACK:
            {
                const mkvparser::VideoTrack *vtrack =
                    static_cast<const mkvparser::VideoTrack *>(track);

                if (!strcmp("V_MPEG4/ISO/AVC", codecID)) {
                    meta.setCString(kKeyMIMEType, MEDIA_MIMETYPE_VIDEO_AVC);
                    meta.setData(kKeyAVCC, 0, codecPrivate, codecPrivateSize);
                } else if (!strcmp("V_MPEGH/ISO/HEVC", codecID)) {
                    meta.setCString(kKeyMIMEType, MEDIA_MIMETYPE_VIDEO_HEVC);
                    if (codecPrivateSize > 0) {
                        meta.setData(kKeyHVCC, kTypeHVCC, codecPrivate, codecPrivateSize);
                    } else {
                        ALOGW("HEVC is detected, but does not have configuration.");
                        continue;
                    }
                } else if (!strcmp("V_MPEG4/ISO/ASP", codecID)) {
                    if (codecPrivateSize > 0) {
                        meta.setCString(
                                kKeyMIMEType, MEDIA_MIMETYPE_VIDEO_MPEG4);
                        addESDSFromCodecPrivate(
                                meta, false, codecPrivate, codecPrivateSize);
                    } else {
                        ALOGW("%s is detected, but does not have configuration.",
                                codecID);
                        continue;
                    }
                } else if (!strcmp("V_VP8", codecID)) {
                    meta.setCString(kKeyMIMEType, MEDIA_MIMETYPE_VIDEO_VP8);
                } else if (!strcmp("V_VP9", codecID)) {
                    meta.setCString(kKeyMIMEType, MEDIA_MIMETYPE_VIDEO_VP9);
                    if (codecPrivateSize > 0) {
                      // 'csd-0' for VP9 is the Blob of Codec Private data as
                      // specified in http://www.webmproject.org/vp9/profiles/.
                        meta.setData(
                              kKeyVp9CodecPrivate, 0, codecPrivate,
                              codecPrivateSize);
                    }
                } else {
                    ALOGW("%s is not supported.", codecID);
                    continue;
                }

                const long long width = vtrack->GetWidth();
                const long long height = vtrack->GetHeight();
                if (width <= 0 || width > INT32_MAX) {
                    ALOGW("track width exceeds int32_t, %lld", width);
                    continue;
                }
                if (height <= 0 || height > INT32_MAX) {
                    ALOGW("track height exceeds int32_t, %lld", height);
                    continue;
                }
                meta.setInt32(kKeyWidth, (int32_t)width);
                meta.setInt32(kKeyHeight, (int32_t)height);

                // setting display width/height is optional
                const long long displayUnit = vtrack->GetDisplayUnit();
                const long long displayWidth = vtrack->GetDisplayWidth();
                const long long displayHeight = vtrack->GetDisplayHeight();
                if (displayWidth > 0 && displayWidth <= INT32_MAX
                        && displayHeight > 0 && displayHeight <= INT32_MAX) {
                    switch (displayUnit) {
                    case 0: // pixels
                        meta.setInt32(kKeyDisplayWidth, (int32_t)displayWidth);
                        meta.setInt32(kKeyDisplayHeight, (int32_t)displayHeight);
                        break;
                    case 1: // centimeters
                    case 2: // inches
                    case 3: // aspect ratio
                    {
                        // Physical layout size is treated the same as aspect ratio.
                        // Note: displayWidth and displayHeight are never zero as they are
                        // checked in the if above.
                        const long long computedWidth =
                                std::max(width, height * displayWidth / displayHeight);
                        const long long computedHeight =
                                std::max(height, width * displayHeight / displayWidth);
                        if (computedWidth <= INT32_MAX && computedHeight <= INT32_MAX) {
                            meta.setInt32(kKeyDisplayWidth, (int32_t)computedWidth);
                            meta.setInt32(kKeyDisplayHeight, (int32_t)computedHeight);
                        }
                        break;
                    }
                    default: // unknown display units, perhaps future version of spec.
                        break;
                    }
                }

                getColorInformation(vtrack, meta);

                break;
            }

            case AUDIO_TRACK:
            {
                const mkvparser::AudioTrack *atrack =
                    static_cast<const mkvparser::AudioTrack *>(track);

                if (!strcmp("A_AAC", codecID)) {
                    meta.setCString(kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_AAC);
                    CHECK(codecPrivateSize >= 2);

                    addESDSFromCodecPrivate(
                            meta, true, codecPrivate, codecPrivateSize);
                } else if (!strcmp("A_VORBIS", codecID)) {
                    meta.setCString(kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_VORBIS);

                    err = addVorbisCodecInfo(
                            meta, codecPrivate, codecPrivateSize);
                } else if (!strcmp("A_OPUS", codecID)) {
                    meta.setCString(kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_OPUS);
                    meta.setData(kKeyOpusHeader, 0, codecPrivate, codecPrivateSize);
                    meta.setInt64(kKeyOpusCodecDelay, track->GetCodecDelay());
                    meta.setInt64(kKeyOpusSeekPreRoll, track->GetSeekPreRoll());
                    mSeekPreRollNs = track->GetSeekPreRoll();
                } else if (!strcmp("A_MPEG/L3", codecID)) {
                    meta.setCString(kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_MPEG);
                } else if (!strcmp("A_FLAC", codecID)) {
                    meta.setCString(kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_FLAC);
                    err = addFlacMetadata(meta, codecPrivate, codecPrivateSize);
                } else {
                    ALOGW("%s is not supported.", codecID);
                    continue;
                }

                meta.setInt32(kKeySampleRate, atrack->GetSamplingRate());
                meta.setInt32(kKeyChannelCount, atrack->GetChannels());
                break;
            }

            default:
                continue;
        }

        const char *language = track->GetLanguage();
        if (language != NULL) {
           char lang[4];
           strncpy(lang, language, 3);
           lang[3] = '\0';
           meta.setCString(kKeyMediaLanguage, lang);
        }

        if (err != OK) {
            ALOGE("skipping track, codec specific data was malformed.");
            continue;
        }

        long long durationNs = mSegment->GetDuration();
        meta.setInt64(kKeyDuration, (durationNs + 500) / 1000);

        mTracks.push();
        size_t n = mTracks.size() - 1;
        TrackInfo *trackInfo = &mTracks.editItemAt(n);
        initTrackInfo(track, meta, trackInfo);

        if (!strcmp("V_MPEG4/ISO/AVC", codecID) && codecPrivateSize == 0) {
            // Attempt to recover from AVC track without codec private data
            err = synthesizeAVCC(trackInfo, n);
            if (err != OK) {
                mTracks.pop();
            }
        }
    }
}

void MatroskaExtractor::findThumbnails() {
    for (size_t i = 0; i < mTracks.size(); ++i) {
        TrackInfo *info = &mTracks.editItemAt(i);

        const char *mime;
        CHECK(info->mMeta.findCString(kKeyMIMEType, &mime));

        if (strncasecmp(mime, "video/", 6)) {
            continue;
        }

        BlockIterator iter(this, info->mTrackNum, i);
        int32_t j = 0;
        int64_t thumbnailTimeUs = 0;
        size_t maxBlockSize = 0;
        while (!iter.eos() && j < 20) {
            if (iter.block()->IsKey()) {
                ++j;

                size_t blockSize = 0;
                for (int k = 0; k < iter.block()->GetFrameCount(); ++k) {
                    blockSize += iter.block()->GetFrame(k).len;
                }

                if (blockSize > maxBlockSize) {
                    maxBlockSize = blockSize;
                    thumbnailTimeUs = iter.blockTimeUs();
                }
            }
            iter.advance();
        }
        info->mMeta.setInt64(kKeyThumbnailTime, thumbnailTimeUs);
    }
}

status_t MatroskaExtractor::getMetaData(MetaDataBase &meta) {
    meta.setCString(
            kKeyMIMEType,
            mIsWebm ? "video/webm" : MEDIA_MIMETYPE_CONTAINER_MATROSKA);

    return OK;
}

uint32_t MatroskaExtractor::flags() const {
    uint32_t x = CAN_PAUSE;
    if (!isLiveStreaming()) {
        x |= CAN_SEEK_BACKWARD | CAN_SEEK_FORWARD | CAN_SEEK;
    }

    return x;
}

bool SniffMatroska(
        DataSourceBase *source, float *confidence) {
    DataSourceBaseReader reader(source);
    mkvparser::EBMLHeader ebmlHeader;
    long long pos;
    if (ebmlHeader.Parse(&reader, pos) < 0) {
        return false;
    }

    *confidence = 0.6;

    return true;
}


extern "C" {
// This is the only symbol that needs to be exported
__attribute__ ((visibility ("default")))
MediaExtractor::ExtractorDef GETEXTRACTORDEF() {
    return {
        MediaExtractor::EXTRACTORDEF_VERSION,
        UUID("abbedd92-38c4-4904-a4c1-b3f45f899980"),
        1,
        "Matroska Extractor",
        [](
                DataSourceBase *source,
                float *confidence,
                void **,
                MediaExtractor::FreeMetaFunc *) -> MediaExtractor::CreatorFunc {
            if (SniffMatroska(source, confidence)) {
                return [](
                        DataSourceBase *source,
                        void *) -> MediaExtractor* {
                    return new MatroskaExtractor(source);};
            }
            return NULL;
        }
    };
}

} // extern "C"

}  // namespace android
