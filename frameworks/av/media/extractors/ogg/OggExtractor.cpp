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
#define LOG_TAG "OggExtractor"
#include <utils/Log.h>

#include "OggExtractor.h"

#include <cutils/properties.h>
#include <media/DataSourceBase.h>
#include <media/ExtractorUtils.h>
#include <media/MediaTrack.h>
#include <media/VorbisComment.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/base64.h>
#include <media/stagefright/foundation/ByteUtils.h>
#include <media/stagefright/MediaBufferBase.h>
#include <media/stagefright/MediaBufferGroup.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MetaDataBase.h>
#include <utils/String8.h>

extern "C" {
    #include <Tremolo/codec_internal.h>

    int _vorbis_unpack_books(vorbis_info *vi,oggpack_buffer *opb);
    int _vorbis_unpack_info(vorbis_info *vi,oggpack_buffer *opb);
    int _vorbis_unpack_comment(vorbis_comment *vc,oggpack_buffer *opb);
    long vorbis_packet_blocksize(vorbis_info *vi,ogg_packet *op);
}

namespace android {

struct OggSource : public MediaTrack {
    explicit OggSource(OggExtractor *extractor);

    virtual status_t getFormat(MetaDataBase &);

    virtual status_t start(MetaDataBase *params = NULL);
    virtual status_t stop();

    virtual status_t read(
            MediaBufferBase **buffer, const ReadOptions *options = NULL);

protected:
    virtual ~OggSource();

private:
    OggExtractor *mExtractor;
    bool mStarted;

    OggSource(const OggSource &);
    OggSource &operator=(const OggSource &);
};

struct MyOggExtractor {
    MyOggExtractor(
            DataSourceBase *source,
            const char *mimeType,
            size_t numHeaders,
            int64_t seekPreRollUs);
    virtual ~MyOggExtractor();

    status_t getFormat(MetaDataBase &) const;

    // Returns an approximate bitrate in bits per second.
    virtual uint64_t approxBitrate() const = 0;

    status_t seekToTime(int64_t timeUs);
    status_t seekToOffset(off64_t offset);
    virtual status_t readNextPacket(MediaBufferBase **buffer) = 0;

    status_t init();

    status_t getFileMetaData(MetaDataBase &meta) {
        meta = mFileMeta;
        return OK;
    }

protected:
    struct Page {
        uint64_t mGranulePosition;
        int32_t mPrevPacketSize;
        uint64_t mPrevPacketPos;
        uint32_t mSerialNo;
        uint32_t mPageNo;
        uint8_t mFlags;
        uint8_t mNumSegments;
        uint8_t mLace[255];
    };

    struct TOCEntry {
        off64_t mPageOffset;
        int64_t mTimeUs;
    };

    DataSourceBase *mSource;
    off64_t mOffset;
    Page mCurrentPage;
    uint64_t mCurGranulePosition;
    uint64_t mPrevGranulePosition;
    size_t mCurrentPageSize;
    bool mFirstPacketInPage;
    uint64_t mCurrentPageSamples;
    size_t mNextLaceIndex;

    const char *mMimeType;
    size_t mNumHeaders;
    int64_t mSeekPreRollUs;

    off64_t mFirstDataOffset;

    vorbis_info mVi;
    vorbis_comment mVc;

    MetaDataBase mMeta;
    MetaDataBase mFileMeta;

    Vector<TOCEntry> mTableOfContents;

    ssize_t readPage(off64_t offset, Page *page);
    status_t findNextPage(off64_t startOffset, off64_t *pageOffset);

    virtual int64_t getTimeUsOfGranule(uint64_t granulePos) const = 0;

    // Extract codec format, metadata tags, and various codec specific data;
    // the format and CSD's are required to setup the decoders for the enclosed media content.
    //
    // Valid values for `type` are:
    // 1 - bitstream identification header
    // 3 - comment header
    // 5 - codec setup header (Vorbis only)
    virtual status_t verifyHeader(MediaBufferBase *buffer, uint8_t type) = 0;

    // Read the next ogg packet from the underlying data source; optionally
    // calculate the timestamp for the output packet whilst pretending
    // that we are parsing an Ogg Vorbis stream.
    //
    // *buffer is NULL'ed out immediately upon entry, and if successful a new buffer is allocated;
    // clients are responsible for releasing the original buffer.
    status_t _readNextPacket(MediaBufferBase **buffer, bool calcVorbisTimestamp);

    int32_t getPacketBlockSize(MediaBufferBase *buffer);

    void parseFileMetaData();

    status_t findPrevGranulePosition(off64_t pageOffset, uint64_t *granulePos);

    void buildTableOfContents();

    MyOggExtractor(const MyOggExtractor &);
    MyOggExtractor &operator=(const MyOggExtractor &);
};

struct MyVorbisExtractor : public MyOggExtractor {
    explicit MyVorbisExtractor(DataSourceBase *source)
        : MyOggExtractor(source,
                MEDIA_MIMETYPE_AUDIO_VORBIS,
                /* numHeaders */ 3,
                /* seekPreRollUs */ 0) {
    }

    virtual uint64_t approxBitrate() const;

    virtual status_t readNextPacket(MediaBufferBase **buffer) {
        return _readNextPacket(buffer, /* calcVorbisTimestamp = */ true);
    }

protected:
    virtual int64_t getTimeUsOfGranule(uint64_t granulePos) const {
        if (granulePos > INT64_MAX / 1000000ll) {
            return INT64_MAX;
        }
        return granulePos * 1000000ll / mVi.rate;
    }

    virtual status_t verifyHeader(MediaBufferBase *buffer, uint8_t type);
};

struct MyOpusExtractor : public MyOggExtractor {
    static const int32_t kOpusSampleRate = 48000;
    static const int64_t kOpusSeekPreRollUs = 80000; // 80 ms

    explicit MyOpusExtractor(DataSourceBase *source)
        : MyOggExtractor(source, MEDIA_MIMETYPE_AUDIO_OPUS, /*numHeaders*/ 2, kOpusSeekPreRollUs),
          mChannelCount(0),
          mCodecDelay(0),
          mStartGranulePosition(-1) {
    }

    virtual uint64_t approxBitrate() const {
        return 0;
    }

    virtual status_t readNextPacket(MediaBufferBase **buffer);

protected:
    virtual int64_t getTimeUsOfGranule(uint64_t granulePos) const;
    virtual status_t verifyHeader(MediaBufferBase *buffer, uint8_t type);

private:
    status_t verifyOpusHeader(MediaBufferBase *buffer);
    status_t verifyOpusComments(MediaBufferBase *buffer);
    uint32_t getNumSamplesInPacket(MediaBufferBase *buffer) const;

    uint8_t mChannelCount;
    uint16_t mCodecDelay;
    int64_t mStartGranulePosition;
};

////////////////////////////////////////////////////////////////////////////////

OggSource::OggSource(OggExtractor *extractor)
    : mExtractor(extractor),
      mStarted(false) {
}

OggSource::~OggSource() {
    if (mStarted) {
        stop();
    }
}

status_t OggSource::getFormat(MetaDataBase &meta) {
    return mExtractor->mImpl->getFormat(meta);
}

status_t OggSource::start(MetaDataBase * /* params */) {
    if (mStarted) {
        return INVALID_OPERATION;
    }

    mStarted = true;

    return OK;
}

status_t OggSource::stop() {
    mStarted = false;

    return OK;
}

status_t OggSource::read(
        MediaBufferBase **out, const ReadOptions *options) {
    *out = NULL;

    int64_t seekTimeUs;
    ReadOptions::SeekMode mode;
    if (options && options->getSeekTo(&seekTimeUs, &mode)) {
        status_t err = mExtractor->mImpl->seekToTime(seekTimeUs);
        if (err != OK) {
            return err;
        }
    }

    MediaBufferBase *packet;
    status_t err = mExtractor->mImpl->readNextPacket(&packet);

    if (err != OK) {
        return err;
    }

#if 0
    int64_t timeUs;
    if (packet->meta_data().findInt64(kKeyTime, &timeUs)) {
        ALOGI("found time = %lld us", timeUs);
    } else {
        ALOGI("NO time");
    }
#endif

    packet->meta_data().setInt32(kKeyIsSyncFrame, 1);

    *out = packet;

    return OK;
}

////////////////////////////////////////////////////////////////////////////////

MyOggExtractor::MyOggExtractor(
        DataSourceBase *source,
        const char *mimeType,
        size_t numHeaders,
        int64_t seekPreRollUs)
    : mSource(source),
      mOffset(0),
      mCurGranulePosition(0),
      mPrevGranulePosition(0),
      mCurrentPageSize(0),
      mFirstPacketInPage(true),
      mCurrentPageSamples(0),
      mNextLaceIndex(0),
      mMimeType(mimeType),
      mNumHeaders(numHeaders),
      mSeekPreRollUs(seekPreRollUs),
      mFirstDataOffset(-1) {
    mCurrentPage.mNumSegments = 0;

    vorbis_info_init(&mVi);
    vorbis_comment_init(&mVc);
}

MyOggExtractor::~MyOggExtractor() {
    vorbis_comment_clear(&mVc);
    vorbis_info_clear(&mVi);
}

status_t MyOggExtractor::getFormat(MetaDataBase &meta) const {
    meta = mMeta;
    return OK;
}

status_t MyOggExtractor::findNextPage(
        off64_t startOffset, off64_t *pageOffset) {
    *pageOffset = startOffset;

    for (;;) {
        char signature[4];
        ssize_t n = mSource->readAt(*pageOffset, &signature, 4);

        if (n < 4) {
            *pageOffset = 0;

            return (n < 0) ? n : (status_t)ERROR_END_OF_STREAM;
        }

        if (!memcmp(signature, "OggS", 4)) {
            if (*pageOffset > startOffset) {
                ALOGV("skipped %lld bytes of junk to reach next frame",
                     (long long)(*pageOffset - startOffset));
            }

            return OK;
        }

        ++*pageOffset;
    }
}

// Given the offset of the "current" page, find the page immediately preceding
// it (if any) and return its granule position.
// To do this we back up from the "current" page's offset until we find any
// page preceding it and then scan forward to just before the current page.
status_t MyOggExtractor::findPrevGranulePosition(
        off64_t pageOffset, uint64_t *granulePos) {
    *granulePos = 0;

    off64_t prevPageOffset = 0;
    off64_t prevGuess = pageOffset;
    for (;;) {
        if (prevGuess >= 5000) {
            prevGuess -= 5000;
        } else {
            prevGuess = 0;
        }

        ALOGV("backing up %lld bytes", (long long)(pageOffset - prevGuess));

        status_t err = findNextPage(prevGuess, &prevPageOffset);
        if (err == ERROR_END_OF_STREAM) {
            // We are at the last page and didn't back off enough;
            // back off 5000 bytes more and try again.
            continue;
        } else if (err != OK) {
            return err;
        }

        if (prevPageOffset < pageOffset || prevGuess == 0) {
            break;
        }
    }

    if (prevPageOffset == pageOffset) {
        // We did not find a page preceding this one.
        return UNKNOWN_ERROR;
    }

    ALOGV("prevPageOffset at %lld, pageOffset at %lld",
            (long long)prevPageOffset, (long long)pageOffset);

    for (;;) {
        Page prevPage;
        ssize_t n = readPage(prevPageOffset, &prevPage);

        if (n <= 0) {
            return (status_t)n;
        }

        prevPageOffset += n;

        if (prevPageOffset == pageOffset) {
            *granulePos = prevPage.mGranulePosition;
            return OK;
        }
    }
}

status_t MyOggExtractor::seekToTime(int64_t timeUs) {
    timeUs -= mSeekPreRollUs;
    if (timeUs < 0) {
        timeUs = 0;
    }

    if (mTableOfContents.isEmpty()) {
        // Perform approximate seeking based on avg. bitrate.
        uint64_t bps = approxBitrate();
        if (bps <= 0) {
            return INVALID_OPERATION;
        }

        off64_t pos = timeUs * bps / 8000000ll;

        ALOGV("seeking to offset %lld", (long long)pos);
        return seekToOffset(pos);
    }

    size_t left = 0;
    size_t right_plus_one = mTableOfContents.size();
    while (left < right_plus_one) {
        size_t center = left + (right_plus_one - left) / 2;

        const TOCEntry &entry = mTableOfContents.itemAt(center);

        if (timeUs < entry.mTimeUs) {
            right_plus_one = center;
        } else if (timeUs > entry.mTimeUs) {
            left = center + 1;
        } else {
            left = center;
            break;
        }
    }

    if (left == mTableOfContents.size()) {
        --left;
    }

    const TOCEntry &entry = mTableOfContents.itemAt(left);

    ALOGV("seeking to entry %zu / %zu at offset %lld",
         left, mTableOfContents.size(), (long long)entry.mPageOffset);

    return seekToOffset(entry.mPageOffset);
}

status_t MyOggExtractor::seekToOffset(off64_t offset) {
    if (mFirstDataOffset >= 0 && offset < mFirstDataOffset) {
        // Once we know where the actual audio data starts (past the headers)
        // don't ever seek to anywhere before that.
        offset = mFirstDataOffset;
    }

    off64_t pageOffset;
    status_t err = findNextPage(offset, &pageOffset);

    if (err != OK) {
        return err;
    }

    // We found the page we wanted to seek to, but we'll also need
    // the page preceding it to determine how many valid samples are on
    // this page.
    findPrevGranulePosition(pageOffset, &mPrevGranulePosition);

    mOffset = pageOffset;

    mCurrentPageSize = 0;
    mFirstPacketInPage = true;
    mCurrentPageSamples = 0;
    mCurrentPage.mNumSegments = 0;
    mCurrentPage.mPrevPacketSize = -1;
    mNextLaceIndex = 0;

    // XXX what if new page continues packet from last???

    return OK;
}

ssize_t MyOggExtractor::readPage(off64_t offset, Page *page) {
    uint8_t header[27];
    ssize_t n;
    if ((n = mSource->readAt(offset, header, sizeof(header)))
            < (ssize_t)sizeof(header)) {
        ALOGV("failed to read %zu bytes at offset %#016llx, got %zd bytes",
                sizeof(header), (long long)offset, n);

        if (n < 0) {
            return n;
        } else if (n == 0) {
            return ERROR_END_OF_STREAM;
        } else {
            return ERROR_IO;
        }
    }

    if (memcmp(header, "OggS", 4)) {
        return ERROR_MALFORMED;
    }

    if (header[4] != 0) {
        // Wrong version.

        return ERROR_UNSUPPORTED;
    }

    page->mFlags = header[5];

    if (page->mFlags & ~7) {
        // Only bits 0-2 are defined in version 0.
        return ERROR_MALFORMED;
    }

    page->mGranulePosition = U64LE_AT(&header[6]);

#if 0
    printf("granulePosition = %llu (0x%llx)\n",
           page->mGranulePosition, page->mGranulePosition);
#endif

    page->mSerialNo = U32LE_AT(&header[14]);
    page->mPageNo = U32LE_AT(&header[18]);

    page->mNumSegments = header[26];
    if (mSource->readAt(
                offset + sizeof(header), page->mLace, page->mNumSegments)
            < (ssize_t)page->mNumSegments) {
        return ERROR_IO;
    }

    size_t totalSize = 0;;
    for (size_t i = 0; i < page->mNumSegments; ++i) {
        totalSize += page->mLace[i];
    }

#if 0
    String8 tmp;
    for (size_t i = 0; i < page->mNumSegments; ++i) {
        char x[32];
        sprintf(x, "%s%u", i > 0 ? ", " : "", (unsigned)page->mLace[i]);

        tmp.append(x);
    }

    ALOGV("%c %s", page->mFlags & 1 ? '+' : ' ', tmp.string());
#endif

    return sizeof(header) + page->mNumSegments + totalSize;
}

status_t MyOpusExtractor::readNextPacket(MediaBufferBase **out) {
    if (mOffset <= mFirstDataOffset && mStartGranulePosition < 0) {
        // The first sample might not start at time 0; find out where by subtracting
        // the number of samples on the first page from the granule position
        // (position of last complete sample) of the first page. This happens
        // the first time before we attempt to read a packet from the first page.
        MediaBufferBase *mBuf;
        uint32_t numSamples = 0;
        uint64_t curGranulePosition = 0;
        while (true) {
            status_t err = _readNextPacket(&mBuf, /* calcVorbisTimestamp = */false);
            if (err != OK && err != ERROR_END_OF_STREAM) {
                return err;
            }
            // First two pages are header pages.
            if (err == ERROR_END_OF_STREAM || mCurrentPage.mPageNo > 2) {
                if (mBuf != NULL) {
                    mBuf->release();
                    mBuf = NULL;
                }
                break;
            }
            curGranulePosition = mCurrentPage.mGranulePosition;
            numSamples += getNumSamplesInPacket(mBuf);
            mBuf->release();
            mBuf = NULL;
        }

        if (curGranulePosition > numSamples) {
            mStartGranulePosition = curGranulePosition - numSamples;
        } else {
            mStartGranulePosition = 0;
        }
        seekToOffset(0);
    }

    status_t err = _readNextPacket(out, /* calcVorbisTimestamp = */false);
    if (err != OK) {
        return err;
    }

    int32_t currentPageSamples;
    // Calculate timestamps by accumulating durations starting from the first sample of a page;
    // We assume that we only seek to page boundaries.
    if ((*out)->meta_data().findInt32(kKeyValidSamples, &currentPageSamples)) {
        // first packet in page
        if (mOffset == mFirstDataOffset) {
            currentPageSamples -= mStartGranulePosition;
            (*out)->meta_data().setInt32(kKeyValidSamples, currentPageSamples);
        }
        mCurGranulePosition = mCurrentPage.mGranulePosition - currentPageSamples;
    }

    int64_t timeUs = getTimeUsOfGranule(mCurGranulePosition);
    (*out)->meta_data().setInt64(kKeyTime, timeUs);

    uint32_t frames = getNumSamplesInPacket(*out);
    mCurGranulePosition += frames;
    return OK;
}

uint32_t MyOpusExtractor::getNumSamplesInPacket(MediaBufferBase *buffer) const {
    if (buffer == NULL || buffer->range_length() < 1) {
        return 0;
    }

    uint8_t *data = (uint8_t *)buffer->data() + buffer->range_offset();
    uint8_t toc = data[0];
    uint8_t config = (toc >> 3) & 0x1f;
    uint32_t frameSizesUs[] = {
        10000, 20000, 40000, 60000, // 0...3
        10000, 20000, 40000, 60000, // 4...7
        10000, 20000, 40000, 60000, // 8...11
        10000, 20000,               // 12...13
        10000, 20000,               // 14...15
        2500, 5000, 10000, 20000,   // 16...19
        2500, 5000, 10000, 20000,   // 20...23
        2500, 5000, 10000, 20000,   // 24...27
        2500, 5000, 10000, 20000    // 28...31
    };
    uint32_t frameSizeUs = frameSizesUs[config];

    uint32_t numFrames;
    uint8_t c = toc & 3;
    switch (c) {
    case 0:
        numFrames = 1;
        break;
    case 1:
    case 2:
        numFrames = 2;
        break;
    case 3:
        if (buffer->range_length() < 3) {
            numFrames = 0;
        } else {
            numFrames = data[2] & 0x3f;
        }
        break;
    default:
        TRESPASS();
    }

    uint32_t numSamples = frameSizeUs * numFrames * kOpusSampleRate / 1000000;
    return numSamples;
}

status_t MyOggExtractor::_readNextPacket(MediaBufferBase **out, bool calcVorbisTimestamp) {
    *out = NULL;

    MediaBufferBase *buffer = NULL;
    int64_t timeUs = -1;

    for (;;) {
        size_t i;
        size_t packetSize = 0;
        bool gotFullPacket = false;
        for (i = mNextLaceIndex; i < mCurrentPage.mNumSegments; ++i) {
            uint8_t lace = mCurrentPage.mLace[i];

            packetSize += lace;

            if (lace < 255) {
                gotFullPacket = true;
                ++i;
                break;
            }
        }

        if (mNextLaceIndex < mCurrentPage.mNumSegments) {
            off64_t dataOffset = mOffset + 27 + mCurrentPage.mNumSegments;
            for (size_t j = 0; j < mNextLaceIndex; ++j) {
                dataOffset += mCurrentPage.mLace[j];
            }

            size_t fullSize = packetSize;
            if (buffer != NULL) {
                fullSize += buffer->range_length();
            }
            if (fullSize > 16 * 1024 * 1024) { // arbitrary limit of 16 MB packet size
                if (buffer != NULL) {
                    buffer->release();
                }
                ALOGE("b/36592202");
                return ERROR_MALFORMED;
            }
            MediaBufferBase *tmp = MediaBufferBase::Create(fullSize);
            if (tmp == NULL) {
                if (buffer != NULL) {
                    buffer->release();
                }
                ALOGE("b/36592202");
                return ERROR_MALFORMED;
            }
            if (buffer != NULL) {
                memcpy(tmp->data(), buffer->data(), buffer->range_length());
                tmp->set_range(0, buffer->range_length());
                buffer->release();
            } else {
                tmp->set_range(0, 0);
            }
            buffer = tmp;

            ssize_t n = mSource->readAt(
                    dataOffset,
                    (uint8_t *)buffer->data() + buffer->range_length(),
                    packetSize);

            if (n < (ssize_t)packetSize) {
                buffer->release();
                ALOGV("failed to read %zu bytes at %#016llx, got %zd bytes",
                        packetSize, (long long)dataOffset, n);
                return ERROR_IO;
            }

            buffer->set_range(0, fullSize);

            mNextLaceIndex = i;

            if (gotFullPacket) {
                // We've just read the entire packet.

                if (mFirstPacketInPage) {
                    buffer->meta_data().setInt32(
                            kKeyValidSamples, mCurrentPageSamples);
                    mFirstPacketInPage = false;
                }

                if (calcVorbisTimestamp) {
                    int32_t curBlockSize = getPacketBlockSize(buffer);
                    if (mCurrentPage.mPrevPacketSize < 0) {
                        mCurrentPage.mPrevPacketSize = curBlockSize;
                        mCurrentPage.mPrevPacketPos =
                                mCurrentPage.mGranulePosition - mCurrentPageSamples;
                        timeUs = mCurrentPage.mPrevPacketPos * 1000000ll / mVi.rate;
                    } else {
                        // The effective block size is the average of the two overlapped blocks
                        int32_t actualBlockSize =
                                (curBlockSize + mCurrentPage.mPrevPacketSize) / 2;
                        timeUs = mCurrentPage.mPrevPacketPos * 1000000ll / mVi.rate;
                        // The actual size output by the decoder will be half the effective
                        // size, due to the overlap
                        mCurrentPage.mPrevPacketPos += actualBlockSize / 2;
                        mCurrentPage.mPrevPacketSize = curBlockSize;
                    }
                    buffer->meta_data().setInt64(kKeyTime, timeUs);
                }
                *out = buffer;

                return OK;
            }

            // fall through, the buffer now contains the start of the packet.
        }

        CHECK_EQ(mNextLaceIndex, mCurrentPage.mNumSegments);

        mOffset += mCurrentPageSize;
        ssize_t n = readPage(mOffset, &mCurrentPage);

        if (n <= 0) {
            if (buffer) {
                buffer->release();
                buffer = NULL;
            }

            ALOGV("readPage returned %zd", n);

            return n < 0 ? n : (status_t)ERROR_END_OF_STREAM;
        }

        // Prevent a harmless unsigned integer overflow by clamping to 0
        if (mCurrentPage.mGranulePosition >= mPrevGranulePosition) {
            mCurrentPageSamples =
                    mCurrentPage.mGranulePosition - mPrevGranulePosition;
        } else {
            mCurrentPageSamples = 0;
        }
        mFirstPacketInPage = true;

        mPrevGranulePosition = mCurrentPage.mGranulePosition;

        mCurrentPageSize = n;
        mNextLaceIndex = 0;

        if (buffer != NULL) {
            if ((mCurrentPage.mFlags & 1) == 0) {
                // This page does not continue the packet, i.e. the packet
                // is already complete.

                if (timeUs >= 0) {
                    buffer->meta_data().setInt64(kKeyTime, timeUs);
                }

                buffer->meta_data().setInt32(
                        kKeyValidSamples, mCurrentPageSamples);
                mFirstPacketInPage = false;

                *out = buffer;

                return OK;
            }
        }
    }
}

status_t MyOggExtractor::init() {
    mMeta.setCString(kKeyMIMEType, mMimeType);

    status_t err;
    MediaBufferBase *packet;
    for (size_t i = 0; i < mNumHeaders; ++i) {
        // ignore timestamp for configuration packets
        if ((err = _readNextPacket(&packet, /* calcVorbisTimestamp = */ false)) != OK) {
            return err;
        }
        ALOGV("read packet of size %zu\n", packet->range_length());
        err = verifyHeader(packet, /* type = */ i * 2 + 1);
        packet->release();
        packet = NULL;
        if (err != OK) {
            return err;
        }
    }

    mFirstDataOffset = mOffset + mCurrentPageSize;

    off64_t size;
    uint64_t lastGranulePosition;
    if (!(mSource->flags() & DataSourceBase::kIsCachingDataSource)
            && mSource->getSize(&size) == OK
            && findPrevGranulePosition(size, &lastGranulePosition) == OK) {
        // Let's assume it's cheap to seek to the end.
        // The granule position of the final page in the stream will
        // give us the exact duration of the content, something that
        // we can only approximate using avg. bitrate if seeking to
        // the end is too expensive or impossible (live streaming).

        int64_t durationUs = getTimeUsOfGranule(lastGranulePosition);

        mMeta.setInt64(kKeyDuration, durationUs);

        buildTableOfContents();
    }

    return OK;
}

void MyOggExtractor::buildTableOfContents() {
    off64_t offset = mFirstDataOffset;
    Page page;
    ssize_t pageSize;
    while ((pageSize = readPage(offset, &page)) > 0) {
        mTableOfContents.push();

        TOCEntry &entry =
            mTableOfContents.editItemAt(mTableOfContents.size() - 1);

        entry.mPageOffset = offset;
        entry.mTimeUs = getTimeUsOfGranule(page.mGranulePosition);

        offset += (size_t)pageSize;
    }

    // Limit the maximum amount of RAM we spend on the table of contents,
    // if necessary thin out the table evenly to trim it down to maximum
    // size.

    static const size_t kMaxTOCSize = 8192;
    static const size_t kMaxNumTOCEntries = kMaxTOCSize / sizeof(TOCEntry);

    size_t numerator = mTableOfContents.size();

    if (numerator > kMaxNumTOCEntries) {
        size_t denom = numerator - kMaxNumTOCEntries;

        size_t accum = 0;
        for (ssize_t i = mTableOfContents.size() - 1; i >= 0; --i) {
            accum += denom;
            if (accum >= numerator) {
                mTableOfContents.removeAt(i);
                accum -= numerator;
            }
        }
    }
}

int32_t MyOggExtractor::getPacketBlockSize(MediaBufferBase *buffer) {
    const uint8_t *data =
        (const uint8_t *)buffer->data() + buffer->range_offset();

    size_t size = buffer->range_length();

    ogg_buffer buf;
    buf.data = (uint8_t *)data;
    buf.size = size;
    buf.refcount = 1;
    buf.ptr.owner = NULL;

    ogg_reference ref;
    ref.buffer = &buf;
    ref.begin = 0;
    ref.length = size;
    ref.next = NULL;

    ogg_packet pack;
    pack.packet = &ref;
    pack.bytes = ref.length;
    pack.b_o_s = 0;
    pack.e_o_s = 0;
    pack.granulepos = 0;
    pack.packetno = 0;

    return vorbis_packet_blocksize(&mVi, &pack);
}

int64_t MyOpusExtractor::getTimeUsOfGranule(uint64_t granulePos) const {
    uint64_t pcmSamplePosition = 0;
    if (granulePos > mCodecDelay) {
        pcmSamplePosition = granulePos - mCodecDelay;
    }
    if (pcmSamplePosition > INT64_MAX / 1000000ll) {
        return INT64_MAX;
    }
    return pcmSamplePosition * 1000000ll / kOpusSampleRate;
}

status_t MyOpusExtractor::verifyHeader(MediaBufferBase *buffer, uint8_t type) {
    switch (type) {
        // there are actually no header types defined in the Opus spec; we choose 1 and 3 to mean
        // header and comments such that we can share code with MyVorbisExtractor.
        case 1:
            return verifyOpusHeader(buffer);
        case 3:
            return verifyOpusComments(buffer);
        default:
            return INVALID_OPERATION;
    }
}

status_t MyOpusExtractor::verifyOpusHeader(MediaBufferBase *buffer) {
    const size_t kOpusHeaderSize = 19;
    const uint8_t *data =
        (const uint8_t *)buffer->data() + buffer->range_offset();

    size_t size = buffer->range_length();

    if (size < kOpusHeaderSize
            || memcmp(data, "OpusHead", 8)
            || /* version = */ data[8] != 1) {
        return ERROR_MALFORMED;
    }

    mChannelCount = data[9];
    mCodecDelay = U16LE_AT(&data[10]);

    mMeta.setData(kKeyOpusHeader, 0, data, size);
    mMeta.setInt32(kKeySampleRate, kOpusSampleRate);
    mMeta.setInt32(kKeyChannelCount, mChannelCount);
    mMeta.setInt64(kKeyOpusSeekPreRoll /* ns */, kOpusSeekPreRollUs * 1000 /* = 80 ms*/);
    mMeta.setInt64(kKeyOpusCodecDelay /* ns */,
            mCodecDelay /* sample/s */ * 1000000000ll / kOpusSampleRate);

    return OK;
}

status_t MyOpusExtractor::verifyOpusComments(MediaBufferBase *buffer) {
    // add artificial framing bit so we can reuse _vorbis_unpack_comment
    int32_t commentSize = buffer->range_length() + 1;
    auto tmp = heapbuffer<uint8_t>(commentSize);
    uint8_t *commentData = tmp.get();
    if (commentData == nullptr) {
        return ERROR_MALFORMED;
    }

    memcpy(commentData,
            (uint8_t *)buffer->data() + buffer->range_offset(),
            buffer->range_length());

    ogg_buffer buf;
    buf.data = commentData;
    buf.size = commentSize;
    buf.refcount = 1;
    buf.ptr.owner = NULL;

    ogg_reference ref;
    ref.buffer = &buf;
    ref.begin = 0;
    ref.length = commentSize;
    ref.next = NULL;

    oggpack_buffer bits;
    oggpack_readinit(&bits, &ref);

    // skip 'OpusTags'
    const char *OpusTags = "OpusTags";
    const int32_t headerLen = strlen(OpusTags);
    int32_t framingBitOffset = headerLen;
    for (int i = 0; i < headerLen; ++i) {
        char chr = oggpack_read(&bits, 8);
        if (chr != OpusTags[i]) {
            return ERROR_MALFORMED;
        }
    }

    int32_t vendorLen = oggpack_read(&bits, 32);
    framingBitOffset += 4;
    if (vendorLen < 0 || vendorLen > commentSize - 8) {
        return ERROR_MALFORMED;
    }
    // skip vendor string
    framingBitOffset += vendorLen;
    for (int i = 0; i < vendorLen; ++i) {
        oggpack_read(&bits, 8);
    }

    int32_t n = oggpack_read(&bits, 32);
    framingBitOffset += 4;
    if (n < 0 || n > ((commentSize - oggpack_bytes(&bits)) >> 2)) {
        return ERROR_MALFORMED;
    }
    for (int i = 0; i < n; ++i) {
        int32_t len = oggpack_read(&bits, 32);
        framingBitOffset += 4;
        if (len  < 0 || len  > (commentSize - oggpack_bytes(&bits))) {
            return ERROR_MALFORMED;
        }
        framingBitOffset += len;
        for (int j = 0; j < len; ++j) {
            oggpack_read(&bits, 8);
        }
    }
    if (framingBitOffset < 0 || framingBitOffset >= commentSize) {
        return ERROR_MALFORMED;
    }
    commentData[framingBitOffset] = 1;

    buf.data = commentData + headerLen;
    buf.size = commentSize - headerLen;
    buf.refcount = 1;
    buf.ptr.owner = NULL;

    ref.buffer = &buf;
    ref.begin = 0;
    ref.length = commentSize - headerLen;
    ref.next = NULL;

    oggpack_readinit(&bits, &ref);
    int err = _vorbis_unpack_comment(&mVc, &bits);
    if (0 != err) {
        return ERROR_MALFORMED;
    }

    parseFileMetaData();
    return OK;
}

status_t MyVorbisExtractor::verifyHeader(
        MediaBufferBase *buffer, uint8_t type) {
    const uint8_t *data =
        (const uint8_t *)buffer->data() + buffer->range_offset();

    size_t size = buffer->range_length();

    if (size < 7 || data[0] != type || memcmp(&data[1], "vorbis", 6)) {
        return ERROR_MALFORMED;
    }

    ogg_buffer buf;
    buf.data = (uint8_t *)data;
    buf.size = size;
    buf.refcount = 1;
    buf.ptr.owner = NULL;

    ogg_reference ref;
    ref.buffer = &buf;
    ref.begin = 0;
    ref.length = size;
    ref.next = NULL;

    oggpack_buffer bits;
    oggpack_readinit(&bits, &ref);

    if (oggpack_read(&bits, 8) != type) {
        return ERROR_MALFORMED;
    }
    for (size_t i = 0; i < 6; ++i) {
        oggpack_read(&bits, 8);  // skip 'vorbis'
    }

    switch (type) {
        case 1:
        {
            if (0 != _vorbis_unpack_info(&mVi, &bits)) {
                return ERROR_MALFORMED;
            }

            mMeta.setData(kKeyVorbisInfo, 0, data, size);
            mMeta.setInt32(kKeySampleRate, mVi.rate);
            mMeta.setInt32(kKeyChannelCount, mVi.channels);
            mMeta.setInt32(kKeyBitRate, mVi.bitrate_nominal);

            ALOGV("lower-bitrate = %ld", mVi.bitrate_lower);
            ALOGV("upper-bitrate = %ld", mVi.bitrate_upper);
            ALOGV("nominal-bitrate = %ld", mVi.bitrate_nominal);
            ALOGV("window-bitrate = %ld", mVi.bitrate_window);
            ALOGV("blocksizes: %d/%d",
                    vorbis_info_blocksize(&mVi, 0),
                    vorbis_info_blocksize(&mVi, 1)
                    );

            off64_t size;
            if (mSource->getSize(&size) == OK) {
                uint64_t bps = approxBitrate();
                if (bps != 0) {
                    mMeta.setInt64(kKeyDuration, size * 8000000ll / bps);
                }
            }
            break;
        }

        case 3:
        {
            if (0 != _vorbis_unpack_comment(&mVc, &bits)) {
                return ERROR_MALFORMED;
            }

            parseFileMetaData();
            break;
        }

        case 5:
        {
            if (0 != _vorbis_unpack_books(&mVi, &bits)) {
                return ERROR_MALFORMED;
            }

            mMeta.setData(kKeyVorbisBooks, 0, data, size);
            break;
        }
    }

    return OK;
}

uint64_t MyVorbisExtractor::approxBitrate() const {
    if (mVi.bitrate_nominal != 0) {
        return mVi.bitrate_nominal;
    }

    return (mVi.bitrate_lower + mVi.bitrate_upper) / 2;
}


void MyOggExtractor::parseFileMetaData() {
    mFileMeta.setCString(kKeyMIMEType, MEDIA_MIMETYPE_CONTAINER_OGG);

    for (int i = 0; i < mVc.comments; ++i) {
        const char *comment = mVc.user_comments[i];
        size_t commentLength = mVc.comment_lengths[i];
        parseVorbisComment(&mFileMeta, comment, commentLength);
        //ALOGI("comment #%d: '%s'", i + 1, mVc.user_comments[i]);
    }
}


////////////////////////////////////////////////////////////////////////////////

OggExtractor::OggExtractor(DataSourceBase *source)
    : mDataSource(source),
      mInitCheck(NO_INIT),
      mImpl(NULL) {
    for (int i = 0; i < 2; ++i) {
        if (mImpl != NULL) {
            delete mImpl;
        }
        if (i == 0) {
            mImpl = new MyVorbisExtractor(mDataSource);
        } else {
            mImpl = new MyOpusExtractor(mDataSource);
        }
        mInitCheck = mImpl->seekToOffset(0);

        if (mInitCheck == OK) {
            mInitCheck = mImpl->init();
            if (mInitCheck == OK) {
                break;
            }
        }
    }
}

OggExtractor::~OggExtractor() {
    delete mImpl;
    mImpl = NULL;
}

size_t OggExtractor::countTracks() {
    return mInitCheck != OK ? 0 : 1;
}

MediaTrack *OggExtractor::getTrack(size_t index) {
    if (index >= 1) {
        return NULL;
    }

    return new OggSource(this);
}

status_t OggExtractor::getTrackMetaData(
        MetaDataBase &meta,
        size_t index, uint32_t /* flags */) {
    if (index >= 1) {
        return UNKNOWN_ERROR;
    }

    return mImpl->getFormat(meta);
}

status_t OggExtractor::getMetaData(MetaDataBase &meta) {
    return mImpl->getFileMetaData(meta);
}

static MediaExtractor* CreateExtractor(
        DataSourceBase *source,
        void *) {
    return new OggExtractor(source);
}

static MediaExtractor::CreatorFunc Sniff(
        DataSourceBase *source,
        float *confidence,
        void **,
        MediaExtractor::FreeMetaFunc *) {
    char tmp[4];
    if (source->readAt(0, tmp, 4) < 4 || memcmp(tmp, "OggS", 4)) {
        return NULL;
    }

    *confidence = 0.2f;

    return CreateExtractor;
}

extern "C" {
// This is the only symbol that needs to be exported
__attribute__ ((visibility ("default")))
MediaExtractor::ExtractorDef GETEXTRACTORDEF() {
    return {
        MediaExtractor::EXTRACTORDEF_VERSION,
        UUID("8cc5cd06-f772-495e-8a62-cba9649374e9"),
        1, // version
        "Ogg Extractor",
        Sniff
    };
}

} // extern "C"

}  // namespace android
