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
#define LOG_TAG "WAVExtractor"
#include <utils/Log.h>

#include "WAVExtractor.h"

#include <audio_utils/primitives.h>
#include <media/DataSourceBase.h>
#include <media/MediaTrack.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/MediaBufferGroup.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MetaData.h>
#include <utils/String8.h>
#include <cutils/bitops.h>

#define CHANNEL_MASK_USE_CHANNEL_ORDER 0

namespace android {

enum {
    WAVE_FORMAT_PCM        = 0x0001,
    WAVE_FORMAT_IEEE_FLOAT = 0x0003,
    WAVE_FORMAT_ALAW       = 0x0006,
    WAVE_FORMAT_MULAW      = 0x0007,
    WAVE_FORMAT_MSGSM      = 0x0031,
    WAVE_FORMAT_EXTENSIBLE = 0xFFFE
};

static const char* WAVEEXT_SUBFORMAT = "\x00\x00\x00\x00\x10\x00\x80\x00\x00\xAA\x00\x38\x9B\x71";
static const char* AMBISONIC_SUBFORMAT = "\x00\x00\x21\x07\xD3\x11\x86\x44\xC8\xC1\xCA\x00\x00\x00";

static uint32_t U32_LE_AT(const uint8_t *ptr) {
    return ptr[3] << 24 | ptr[2] << 16 | ptr[1] << 8 | ptr[0];
}

static uint16_t U16_LE_AT(const uint8_t *ptr) {
    return ptr[1] << 8 | ptr[0];
}

struct WAVSource : public MediaTrack {
    WAVSource(
            DataSourceBase *dataSource,
            MetaDataBase &meta,
            uint16_t waveFormat,
            int32_t bitsPerSample,
            off64_t offset, size_t size);

    virtual status_t start(MetaDataBase *params = NULL);
    virtual status_t stop();
    virtual status_t getFormat(MetaDataBase &meta);

    virtual status_t read(
            MediaBufferBase **buffer, const ReadOptions *options = NULL);

    virtual bool supportNonblockingRead() { return true; }

protected:
    virtual ~WAVSource();

private:
    static const size_t kMaxFrameSize;

    DataSourceBase *mDataSource;
    MetaDataBase &mMeta;
    uint16_t mWaveFormat;
    int32_t mSampleRate;
    int32_t mNumChannels;
    int32_t mBitsPerSample;
    off64_t mOffset;
    size_t mSize;
    bool mStarted;
    MediaBufferGroup *mGroup;
    off64_t mCurrentPos;

    WAVSource(const WAVSource &);
    WAVSource &operator=(const WAVSource &);
};

WAVExtractor::WAVExtractor(DataSourceBase *source)
    : mDataSource(source),
      mValidFormat(false),
      mChannelMask(CHANNEL_MASK_USE_CHANNEL_ORDER) {
    mInitCheck = init();
}

WAVExtractor::~WAVExtractor() {
}

status_t WAVExtractor::getMetaData(MetaDataBase &meta) {
    meta.clear();
    if (mInitCheck == OK) {
        meta.setCString(kKeyMIMEType, MEDIA_MIMETYPE_CONTAINER_WAV);
    }

    return OK;
}

size_t WAVExtractor::countTracks() {
    return mInitCheck == OK ? 1 : 0;
}

MediaTrack *WAVExtractor::getTrack(size_t index) {
    if (mInitCheck != OK || index > 0) {
        return NULL;
    }

    return new WAVSource(
            mDataSource, mTrackMeta,
            mWaveFormat, mBitsPerSample, mDataOffset, mDataSize);
}

status_t WAVExtractor::getTrackMetaData(
        MetaDataBase &meta,
        size_t index, uint32_t /* flags */) {
    if (mInitCheck != OK || index > 0) {
        return UNKNOWN_ERROR;
    }

    meta = mTrackMeta;
    return OK;
}

status_t WAVExtractor::init() {
    uint8_t header[12];
    if (mDataSource->readAt(
                0, header, sizeof(header)) < (ssize_t)sizeof(header)) {
        return NO_INIT;
    }

    if (memcmp(header, "RIFF", 4) || memcmp(&header[8], "WAVE", 4)) {
        return NO_INIT;
    }

    size_t totalSize = U32_LE_AT(&header[4]);

    off64_t offset = 12;
    size_t remainingSize = totalSize;
    while (remainingSize >= 8) {
        uint8_t chunkHeader[8];
        if (mDataSource->readAt(offset, chunkHeader, 8) < 8) {
            return NO_INIT;
        }

        remainingSize -= 8;
        offset += 8;

        uint32_t chunkSize = U32_LE_AT(&chunkHeader[4]);

        if (chunkSize > remainingSize) {
            return NO_INIT;
        }

        if (!memcmp(chunkHeader, "fmt ", 4)) {
            if (chunkSize < 16) {
                return NO_INIT;
            }

            uint8_t formatSpec[40];
            if (mDataSource->readAt(offset, formatSpec, 2) < 2) {
                return NO_INIT;
            }

            mWaveFormat = U16_LE_AT(formatSpec);
            if (mWaveFormat != WAVE_FORMAT_PCM
                    && mWaveFormat != WAVE_FORMAT_IEEE_FLOAT
                    && mWaveFormat != WAVE_FORMAT_ALAW
                    && mWaveFormat != WAVE_FORMAT_MULAW
                    && mWaveFormat != WAVE_FORMAT_MSGSM
                    && mWaveFormat != WAVE_FORMAT_EXTENSIBLE) {
                return ERROR_UNSUPPORTED;
            }

            uint8_t fmtSize = 16;
            if (mWaveFormat == WAVE_FORMAT_EXTENSIBLE) {
                fmtSize = 40;
            }
            if (mDataSource->readAt(offset, formatSpec, fmtSize) < fmtSize) {
                return NO_INIT;
            }

            mNumChannels = U16_LE_AT(&formatSpec[2]);

            if (mNumChannels < 1 || mNumChannels > 8) {
                ALOGE("Unsupported number of channels (%d)", mNumChannels);
                return ERROR_UNSUPPORTED;
            }

            if (mWaveFormat != WAVE_FORMAT_EXTENSIBLE) {
                if (mNumChannels != 1 && mNumChannels != 2) {
                    ALOGW("More than 2 channels (%d) in non-WAVE_EXT, unknown channel mask",
                            mNumChannels);
                }
            }

            mSampleRate = U32_LE_AT(&formatSpec[4]);

            if (mSampleRate == 0) {
                return ERROR_MALFORMED;
            }

            mBitsPerSample = U16_LE_AT(&formatSpec[14]);

            if (mWaveFormat == WAVE_FORMAT_EXTENSIBLE) {
                uint16_t validBitsPerSample = U16_LE_AT(&formatSpec[18]);
                if (validBitsPerSample != mBitsPerSample) {
                    if (validBitsPerSample != 0) {
                        ALOGE("validBits(%d) != bitsPerSample(%d) are not supported",
                                validBitsPerSample, mBitsPerSample);
                        return ERROR_UNSUPPORTED;
                    } else {
                        // we only support valitBitsPerSample == bitsPerSample but some WAV_EXT
                        // writers don't correctly set the valid bits value, and leave it at 0.
                        ALOGW("WAVE_EXT has 0 valid bits per sample, ignoring");
                    }
                }

                mChannelMask = U32_LE_AT(&formatSpec[20]);
                ALOGV("numChannels=%d channelMask=0x%x", mNumChannels, mChannelMask);
                if ((mChannelMask >> 18) != 0) {
                    ALOGE("invalid channel mask 0x%x", mChannelMask);
                    return ERROR_MALFORMED;
                }

                if ((mChannelMask != CHANNEL_MASK_USE_CHANNEL_ORDER)
                        && (popcount(mChannelMask) != mNumChannels)) {
                    ALOGE("invalid number of channels (%d) in channel mask (0x%x)",
                            popcount(mChannelMask), mChannelMask);
                    return ERROR_MALFORMED;
                }

                // In a WAVE_EXT header, the first two bytes of the GUID stored at byte 24 contain
                // the sample format, using the same definitions as a regular WAV header
                mWaveFormat = U16_LE_AT(&formatSpec[24]);
                if (memcmp(&formatSpec[26], WAVEEXT_SUBFORMAT, 14) &&
                    memcmp(&formatSpec[26], AMBISONIC_SUBFORMAT, 14)) {
                    ALOGE("unsupported GUID");
                    return ERROR_UNSUPPORTED;
                }
            }

            if (mWaveFormat == WAVE_FORMAT_PCM) {
                if (mBitsPerSample != 8 && mBitsPerSample != 16
                    && mBitsPerSample != 24 && mBitsPerSample != 32) {
                    return ERROR_UNSUPPORTED;
                }
            } else if (mWaveFormat == WAVE_FORMAT_IEEE_FLOAT) {
                if (mBitsPerSample != 32) {  // TODO we don't support double
                    return ERROR_UNSUPPORTED;
                }
            }
            else if (mWaveFormat == WAVE_FORMAT_MSGSM) {
                if (mBitsPerSample != 0) {
                    return ERROR_UNSUPPORTED;
                }
            } else if (mWaveFormat == WAVE_FORMAT_MULAW || mWaveFormat == WAVE_FORMAT_ALAW) {
                if (mBitsPerSample != 8) {
                    return ERROR_UNSUPPORTED;
                }
            } else {
                return ERROR_UNSUPPORTED;
            }

            mValidFormat = true;
        } else if (!memcmp(chunkHeader, "data", 4)) {
            if (mValidFormat) {
                mDataOffset = offset;
                mDataSize = chunkSize;

                mTrackMeta.clear();

                switch (mWaveFormat) {
                    case WAVE_FORMAT_PCM:
                    case WAVE_FORMAT_IEEE_FLOAT:
                        mTrackMeta.setCString(
                                kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_RAW);
                        break;
                    case WAVE_FORMAT_ALAW:
                        mTrackMeta.setCString(
                                kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_G711_ALAW);
                        break;
                    case WAVE_FORMAT_MSGSM:
                        mTrackMeta.setCString(
                                kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_MSGSM);
                        break;
                    default:
                        CHECK_EQ(mWaveFormat, (uint16_t)WAVE_FORMAT_MULAW);
                        mTrackMeta.setCString(
                                kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_G711_MLAW);
                        break;
                }

                mTrackMeta.setInt32(kKeyChannelCount, mNumChannels);
                mTrackMeta.setInt32(kKeyChannelMask, mChannelMask);
                mTrackMeta.setInt32(kKeySampleRate, mSampleRate);
                mTrackMeta.setInt32(kKeyPcmEncoding, kAudioEncodingPcm16bit);

                int64_t durationUs = 0;
                if (mWaveFormat == WAVE_FORMAT_MSGSM) {
                    // 65 bytes decode to 320 8kHz samples
                    durationUs =
                        1000000LL * (mDataSize / 65 * 320) / 8000;
                } else {
                    size_t bytesPerSample = mBitsPerSample >> 3;

                    if (!bytesPerSample || !mNumChannels)
                        return ERROR_MALFORMED;

                    size_t num_samples = mDataSize / (mNumChannels * bytesPerSample);

                    if (!mSampleRate)
                        return ERROR_MALFORMED;

                    durationUs =
                        1000000LL * num_samples / mSampleRate;
                }

                mTrackMeta.setInt64(kKeyDuration, durationUs);

                return OK;
            }
        }

        offset += chunkSize;
    }

    return NO_INIT;
}

const size_t WAVSource::kMaxFrameSize = 32768;

WAVSource::WAVSource(
        DataSourceBase *dataSource,
        MetaDataBase &meta,
        uint16_t waveFormat,
        int32_t bitsPerSample,
        off64_t offset, size_t size)
    : mDataSource(dataSource),
      mMeta(meta),
      mWaveFormat(waveFormat),
      mSampleRate(0),
      mNumChannels(0),
      mBitsPerSample(bitsPerSample),
      mOffset(offset),
      mSize(size),
      mStarted(false),
      mGroup(NULL) {
    CHECK(mMeta.findInt32(kKeySampleRate, &mSampleRate));
    CHECK(mMeta.findInt32(kKeyChannelCount, &mNumChannels));

    mMeta.setInt32(kKeyMaxInputSize, kMaxFrameSize);
}

WAVSource::~WAVSource() {
    if (mStarted) {
        stop();
    }
}

status_t WAVSource::start(MetaDataBase * /* params */) {
    ALOGV("WAVSource::start");

    CHECK(!mStarted);

    // some WAV files may have large audio buffers that use shared memory transfer.
    mGroup = new MediaBufferGroup(4 /* buffers */, kMaxFrameSize);

    if (mBitsPerSample == 8) {
        // As a temporary buffer for 8->16 bit conversion.
        mGroup->add_buffer(MediaBufferBase::Create(kMaxFrameSize));
    }

    mCurrentPos = mOffset;

    mStarted = true;

    return OK;
}

status_t WAVSource::stop() {
    ALOGV("WAVSource::stop");

    CHECK(mStarted);

    delete mGroup;
    mGroup = NULL;

    mStarted = false;

    return OK;
}

status_t WAVSource::getFormat(MetaDataBase &meta) {
    ALOGV("WAVSource::getFormat");

    meta = mMeta;
    return OK;
}

status_t WAVSource::read(
        MediaBufferBase **out, const ReadOptions *options) {
    *out = NULL;

    if (options != nullptr && options->getNonBlocking() && !mGroup->has_buffers()) {
        return WOULD_BLOCK;
    }

    int64_t seekTimeUs;
    ReadOptions::SeekMode mode;
    if (options != NULL && options->getSeekTo(&seekTimeUs, &mode)) {
        int64_t pos = 0;

        if (mWaveFormat == WAVE_FORMAT_MSGSM) {
            // 65 bytes decode to 320 8kHz samples
            int64_t samplenumber = (seekTimeUs * mSampleRate) / 1000000;
            int64_t framenumber = samplenumber / 320;
            pos = framenumber * 65;
        } else {
            pos = (seekTimeUs * mSampleRate) / 1000000 * mNumChannels * (mBitsPerSample >> 3);
        }
        if (pos > (off64_t)mSize) {
            pos = mSize;
        }
        mCurrentPos = pos + mOffset;
    }

    MediaBufferBase *buffer;
    status_t err = mGroup->acquire_buffer(&buffer);
    if (err != OK) {
        return err;
    }

    // make sure that maxBytesToRead is multiple of 3, in 24-bit case
    size_t maxBytesToRead =
        mBitsPerSample == 8 ? kMaxFrameSize / 2 : 
        (mBitsPerSample == 24 ? 3*(kMaxFrameSize/3): kMaxFrameSize);

    size_t maxBytesAvailable =
        (mCurrentPos - mOffset >= (off64_t)mSize)
            ? 0 : mSize - (mCurrentPos - mOffset);

    if (maxBytesToRead > maxBytesAvailable) {
        maxBytesToRead = maxBytesAvailable;
    }

    if (mWaveFormat == WAVE_FORMAT_MSGSM) {
        // Microsoft packs 2 frames into 65 bytes, rather than using separate 33-byte frames,
        // so read multiples of 65, and use smaller buffers to account for ~10:1 expansion ratio
        if (maxBytesToRead > 1024) {
            maxBytesToRead = 1024;
        }
        maxBytesToRead = (maxBytesToRead / 65) * 65;
    } else {
        // read only integral amounts of audio unit frames.
        const size_t inputUnitFrameSize = mNumChannels * mBitsPerSample / 8;
        maxBytesToRead -= maxBytesToRead % inputUnitFrameSize;
    }

    ssize_t n = mDataSource->readAt(
            mCurrentPos, buffer->data(),
            maxBytesToRead);

    if (n <= 0) {
        buffer->release();
        buffer = NULL;

        return ERROR_END_OF_STREAM;
    }

    buffer->set_range(0, n);

    // TODO: add capability to return data as float PCM instead of 16 bit PCM.
    if (mWaveFormat == WAVE_FORMAT_PCM) {
        if (mBitsPerSample == 8) {
            // Convert 8-bit unsigned samples to 16-bit signed.

            // Create new buffer with 2 byte wide samples
            MediaBufferBase *tmp;
            CHECK_EQ(mGroup->acquire_buffer(&tmp), (status_t)OK);
            tmp->set_range(0, 2 * n);

            memcpy_to_i16_from_u8((int16_t *)tmp->data(), (const uint8_t *)buffer->data(), n);
            buffer->release();
            buffer = tmp;
        } else if (mBitsPerSample == 24) {
            // Convert 24-bit signed samples to 16-bit signed in place
            const size_t numSamples = n / 3;

            memcpy_to_i16_from_p24((int16_t *)buffer->data(), (const uint8_t *)buffer->data(), numSamples);
            buffer->set_range(0, 2 * numSamples);
        }  else if (mBitsPerSample == 32) {
            // Convert 32-bit signed samples to 16-bit signed in place
            const size_t numSamples = n / 4;

            memcpy_to_i16_from_i32((int16_t *)buffer->data(), (const int32_t *)buffer->data(), numSamples);
            buffer->set_range(0, 2 * numSamples);
        }
    } else if (mWaveFormat == WAVE_FORMAT_IEEE_FLOAT) {
        if (mBitsPerSample == 32) {
            // Convert 32-bit float samples to 16-bit signed in place
            const size_t numSamples = n / 4;

            memcpy_to_i16_from_float((int16_t *)buffer->data(), (const float *)buffer->data(), numSamples);
            buffer->set_range(0, 2 * numSamples);
        }
    }

    int64_t timeStampUs = 0;

    if (mWaveFormat == WAVE_FORMAT_MSGSM) {
        timeStampUs = 1000000LL * (mCurrentPos - mOffset) * 320 / 65 / mSampleRate;
    } else {
        size_t bytesPerSample = mBitsPerSample >> 3;
        timeStampUs = 1000000LL * (mCurrentPos - mOffset)
                / (mNumChannels * bytesPerSample) / mSampleRate;
    }

    buffer->meta_data().setInt64(kKeyTime, timeStampUs);

    buffer->meta_data().setInt32(kKeyIsSyncFrame, 1);
    mCurrentPos += n;

    *out = buffer;

    return OK;
}

////////////////////////////////////////////////////////////////////////////////

static MediaExtractor* CreateExtractor(
        DataSourceBase *source,
        void *) {
    return new WAVExtractor(source);
}

static MediaExtractor::CreatorFunc Sniff(
        DataSourceBase *source,
        float *confidence,
        void **,
        MediaExtractor::FreeMetaFunc *) {
    char header[12];
    if (source->readAt(0, header, sizeof(header)) < (ssize_t)sizeof(header)) {
        return NULL;
    }

    if (memcmp(header, "RIFF", 4) || memcmp(&header[8], "WAVE", 4)) {
        return NULL;
    }

    MediaExtractor *extractor = new WAVExtractor(source);
    int numTracks = extractor->countTracks();
    delete extractor;
    if (numTracks == 0) {
        return NULL;
    }

    *confidence = 0.3f;

    return CreateExtractor;
}

extern "C" {
// This is the only symbol that needs to be exported
__attribute__ ((visibility ("default")))
MediaExtractor::ExtractorDef GETEXTRACTORDEF() {
    return {
        MediaExtractor::EXTRACTORDEF_VERSION,
        UUID("7d613858-5837-4a38-84c5-332d1cddee27"),
        1, // version
        "WAV Extractor",
        Sniff
    };
}

} // extern "C"

} // namespace android
