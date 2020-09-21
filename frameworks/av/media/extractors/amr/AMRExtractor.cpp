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

//#define LOG_NDEBUG 0
#define LOG_TAG "AMRExtractor"
#include <utils/Log.h>

#include "AMRExtractor.h"

#include <media/DataSourceBase.h>
#include <media/MediaTrack.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/MediaBufferGroup.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MetaData.h>
#include <utils/String8.h>

namespace android {

class AMRSource : public MediaTrack {
public:
    AMRSource(
            DataSourceBase *source,
            MetaDataBase &meta,
            bool isWide,
            const off64_t *offset_table,
            size_t offset_table_length);

    virtual status_t start(MetaDataBase *params = NULL);
    virtual status_t stop();

    virtual status_t getFormat(MetaDataBase &);

    virtual status_t read(
            MediaBufferBase **buffer, const ReadOptions *options = NULL);

protected:
    virtual ~AMRSource();

private:
    DataSourceBase *mDataSource;
    MetaDataBase mMeta;
    bool mIsWide;

    off64_t mOffset;
    int64_t mCurrentTimeUs;
    bool mStarted;
    MediaBufferGroup *mGroup;

    off64_t mOffsetTable[OFFSET_TABLE_LEN];
    size_t mOffsetTableLength;

    AMRSource(const AMRSource &);
    AMRSource &operator=(const AMRSource &);
};

////////////////////////////////////////////////////////////////////////////////

static size_t getFrameSize(bool isWide, unsigned FT) {
    static const size_t kFrameSizeNB[16] = {
        95, 103, 118, 134, 148, 159, 204, 244,
        39, 43, 38, 37, // SID
        0, 0, 0, // future use
        0 // no data
    };
    static const size_t kFrameSizeWB[16] = {
        132, 177, 253, 285, 317, 365, 397, 461, 477,
        40, // SID
        0, 0, 0, 0, // future use
        0, // speech lost
        0 // no data
    };

    if (FT > 15 || (isWide && FT > 9 && FT < 14) || (!isWide && FT > 11 && FT < 15)) {
        ALOGE("illegal AMR frame type %d", FT);
        return 0;
    }

    size_t frameSize = isWide ? kFrameSizeWB[FT] : kFrameSizeNB[FT];

    // Round up bits to bytes and add 1 for the header byte.
    frameSize = (frameSize + 7) / 8 + 1;

    return frameSize;
}

static status_t getFrameSizeByOffset(DataSourceBase *source,
        off64_t offset, bool isWide, size_t *frameSize) {
    uint8_t header;
    ssize_t count = source->readAt(offset, &header, 1);
    if (count == 0) {
        return ERROR_END_OF_STREAM;
    } else if (count < 0) {
        return ERROR_IO;
    }

    unsigned FT = (header >> 3) & 0x0f;

    *frameSize = getFrameSize(isWide, FT);
    if (*frameSize == 0) {
        return ERROR_MALFORMED;
    }
    return OK;
}

static bool SniffAMR(
        DataSourceBase *source, bool *isWide, float *confidence) {
    char header[9];

    if (source->readAt(0, header, sizeof(header)) != sizeof(header)) {
        return false;
    }

    if (!memcmp(header, "#!AMR\n", 6)) {
        if (isWide != nullptr) {
            *isWide = false;
        }
        *confidence = 0.5;

        return true;
    } else if (!memcmp(header, "#!AMR-WB\n", 9)) {
        if (isWide != nullptr) {
            *isWide = true;
        }
        *confidence = 0.5;

        return true;
    }

    return false;
}

AMRExtractor::AMRExtractor(DataSourceBase *source)
    : mDataSource(source),
      mInitCheck(NO_INIT),
      mOffsetTableLength(0) {
    float confidence;
    if (!SniffAMR(mDataSource, &mIsWide, &confidence)) {
        return;
    }

    mMeta.setCString(
            kKeyMIMEType, mIsWide ? MEDIA_MIMETYPE_AUDIO_AMR_WB
                                  : MEDIA_MIMETYPE_AUDIO_AMR_NB);

    mMeta.setInt32(kKeyChannelCount, 1);
    mMeta.setInt32(kKeySampleRate, mIsWide ? 16000 : 8000);

    off64_t offset = mIsWide ? 9 : 6;
    off64_t streamSize;
    size_t frameSize, numFrames = 0;
    int64_t duration = 0;

    if (mDataSource->getSize(&streamSize) == OK) {
         while (offset < streamSize) {
             status_t status = getFrameSizeByOffset(source, offset, mIsWide, &frameSize);
             if (status == ERROR_END_OF_STREAM) {
                 break;
             } else if (status != OK) {
                return;
            }

            if ((numFrames % 50 == 0) && (numFrames / 50 < OFFSET_TABLE_LEN)) {
                CHECK_EQ(mOffsetTableLength, numFrames / 50);
                mOffsetTable[mOffsetTableLength] = offset - (mIsWide ? 9: 6);
                mOffsetTableLength ++;
            }

            offset += frameSize;
            duration += 20000;  // Each frame is 20ms
            numFrames ++;
        }

        mMeta.setInt64(kKeyDuration, duration);
    }

    mInitCheck = OK;
}

AMRExtractor::~AMRExtractor() {
}

status_t AMRExtractor::getMetaData(MetaDataBase &meta) {
    meta.clear();

    if (mInitCheck == OK) {
        meta.setCString(kKeyMIMEType, mIsWide ? "audio/amr-wb" : "audio/amr");
    }

    return OK;
}

size_t AMRExtractor::countTracks() {
    return mInitCheck == OK ? 1 : 0;
}

MediaTrack *AMRExtractor::getTrack(size_t index) {
    if (mInitCheck != OK || index != 0) {
        return NULL;
    }

    return new AMRSource(mDataSource, mMeta, mIsWide,
            mOffsetTable, mOffsetTableLength);
}

status_t AMRExtractor::getTrackMetaData(MetaDataBase &meta, size_t index, uint32_t /* flags */) {
    if (mInitCheck != OK || index != 0) {
        return UNKNOWN_ERROR;
    }

    meta = mMeta;
    return OK;
}

////////////////////////////////////////////////////////////////////////////////

AMRSource::AMRSource(
        DataSourceBase *source, MetaDataBase &meta,
        bool isWide, const off64_t *offset_table, size_t offset_table_length)
    : mDataSource(source),
      mMeta(meta),
      mIsWide(isWide),
      mOffset(mIsWide ? 9 : 6),
      mCurrentTimeUs(0),
      mStarted(false),
      mGroup(NULL),
      mOffsetTableLength(offset_table_length) {
    if (mOffsetTableLength > 0 && mOffsetTableLength <= OFFSET_TABLE_LEN) {
        memcpy ((char*)mOffsetTable, (char*)offset_table, sizeof(off64_t) * mOffsetTableLength);
    }
}

AMRSource::~AMRSource() {
    if (mStarted) {
        stop();
    }
}

status_t AMRSource::start(MetaDataBase * /* params */) {
    CHECK(!mStarted);

    mOffset = mIsWide ? 9 : 6;
    mCurrentTimeUs = 0;
    mGroup = new MediaBufferGroup;
    mGroup->add_buffer(MediaBufferBase::Create(128));
    mStarted = true;

    return OK;
}

status_t AMRSource::stop() {
    CHECK(mStarted);

    delete mGroup;
    mGroup = NULL;

    mStarted = false;
    return OK;
}

status_t AMRSource::getFormat(MetaDataBase &meta) {
    meta = mMeta;
    return OK;
}

status_t AMRSource::read(
        MediaBufferBase **out, const ReadOptions *options) {
    *out = NULL;

    int64_t seekTimeUs;
    ReadOptions::SeekMode mode;
    if (mOffsetTableLength > 0 && options && options->getSeekTo(&seekTimeUs, &mode)) {
        size_t size;
        int64_t seekFrame = seekTimeUs / 20000ll;  // 20ms per frame.
        mCurrentTimeUs = seekFrame * 20000ll;

        size_t index = seekFrame < 0 ? 0 : seekFrame / 50;
        if (index >= mOffsetTableLength) {
            index = mOffsetTableLength - 1;
        }

        mOffset = mOffsetTable[index] + (mIsWide ? 9 : 6);

        for (size_t i = 0; i< seekFrame - index * 50; i++) {
            status_t err;
            if ((err = getFrameSizeByOffset(mDataSource, mOffset,
                            mIsWide, &size)) != OK) {
                return err;
            }
            mOffset += size;
        }
    }

    uint8_t header;
    ssize_t n = mDataSource->readAt(mOffset, &header, 1);

    if (n < 1) {
        return ERROR_END_OF_STREAM;
    }

    if (header & 0x83) {
        // Padding bits must be 0.

        ALOGE("padding bits must be 0, header is 0x%02x", header);

        return ERROR_MALFORMED;
    }

    unsigned FT = (header >> 3) & 0x0f;

    size_t frameSize = getFrameSize(mIsWide, FT);
    if (frameSize == 0) {
        return ERROR_MALFORMED;
    }

    MediaBufferBase *buffer;
    status_t err = mGroup->acquire_buffer(&buffer);
    if (err != OK) {
        return err;
    }

    n = mDataSource->readAt(mOffset, buffer->data(), frameSize);

    if (n != (ssize_t)frameSize) {
        buffer->release();
        buffer = NULL;

        if (n < 0) {
            return ERROR_IO;
        } else {
            // only partial frame is available, treat it as EOS.
            mOffset += n;
            return ERROR_END_OF_STREAM;
        }
    }

    buffer->set_range(0, frameSize);
    buffer->meta_data().setInt64(kKeyTime, mCurrentTimeUs);
    buffer->meta_data().setInt32(kKeyIsSyncFrame, 1);

    mOffset += frameSize;
    mCurrentTimeUs += 20000;  // Each frame is 20ms

    *out = buffer;

    return OK;
}

////////////////////////////////////////////////////////////////////////////////

extern "C" {
// This is the only symbol that needs to be exported
__attribute__ ((visibility ("default")))
MediaExtractor::ExtractorDef GETEXTRACTORDEF() {
    return {
        MediaExtractor::EXTRACTORDEF_VERSION,
        UUID("c86639c9-2f31-40ac-a715-fa01b4493aaf"),
        1,
        "AMR Extractor",
        [](
                DataSourceBase *source,
                float *confidence,
                void **,
                MediaExtractor::FreeMetaFunc *) -> MediaExtractor::CreatorFunc {
            if (SniffAMR(source, nullptr, confidence)) {
                return [](
                        DataSourceBase *source,
                        void *) -> MediaExtractor* {
                    return new AMRExtractor(source);};
            }
            return NULL;
        }
    };
}

} // extern "C"

}  // namespace android
