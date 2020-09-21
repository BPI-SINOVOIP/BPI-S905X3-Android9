/*
* Copyright (C) 2011 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* 	 http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#define LOG_NDEBUG 0
#define LOG_TAG "TSPacker"

#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <utils/threads.h>

#include <arpa/inet.h>

#include <linux/videodev2.h>
#include <hardware/hardware.h>

#include <cutils/properties.h>
#include <media/ICrypto.h>
#include <pthread.h>

#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/foundation/AHandler.h>
#include <media/stagefright/foundation/ALooper.h>

#include <media/stagefright/MediaBuffer.h>
#include <media/stagefright/MediaBufferBase.h>

#include <media/stagefright/MediaCodec.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/AudioSource.h>
#include <media/hardware/MetadataBufferType.h>

#include <gui/Surface.h>

#include <OMX_Component.h>

#include <utils/Log.h>
#include <utils/String8.h>

#include "tspack.h"
#include "esconvertor.h"

namespace android {

//#define DUMPVIDEOES

//#define DUMPVIDEOTS
//#define DUMPAUDIOES
//#define DUMPAUDIOPCM

TSPacker::TSPacker(int width, int height, int frameRate, int bitRate, int sourceType, bool hasAudio) :
    mWidth(width),
    mHeight(height),
    mFrameRate(frameRate),
    mBitRate(bitRate),
    mSourceType(sourceType),
    mHasAudio(hasAudio),
    mPrevTimeUs(-1ll),
    mStarted(false),
    mheadFinalize(0) {

    mPATContinuityCounter = 0;
    mPMTContinuityCounter = 0;
    mAudioContinuityCounter = 0;
    mVideoContinuityCounter = 0;

    initCrcTable();

    ALOGE("TSPacker construct\n");
}

TSPacker::~TSPacker() {
    ALOGE("~TSPacker");
    CHECK(!mStarted);
}

int32_t TSPacker::getFrameRate( ) const {
    ALOGE("getFrameRate %d", mFrameRate);
    Mutex::Autolock lock(mMutex);
    return mFrameRate;
}

sp<MetaData> TSPacker::getFormat() {
    ALOGE("getFormat");

    Mutex::Autolock lock(mMutex);
    sp<MetaData> meta = new MetaData;

    return meta;
}

int TSPacker::threadFunc()
{
    int err;
    MediaBufferBase *tESBuffer;
    int64_t timeUs;
    int32_t flags;
    sp<ABuffer> tsPackets;

    //get video start code header
    while (mStarted == true) {
        err = mVideoConvertor->read(&tESBuffer);
        if (err != OK) {
            usleep(1000);
            continue;
        }

        ALOGE("[%s %d] csd buffer size:%d", __FUNCTION__, __LINE__, tESBuffer->range_length());

        sp<ABuffer> csd = new ABuffer(tESBuffer->range_length());
        memcpy(csd->data(), tESBuffer->data(), tESBuffer->range_length());
        mCSD.push(csd);

        tESBuffer->release();
        tESBuffer = NULL;

        break;
    }

    while (mStarted == true) {
        err = mVideoConvertor->read(&tESBuffer);

        if (err == OK) {
            tESBuffer->meta_data().findInt64(kKeyTime, &timeUs);

            int64_t timeNow64;
            struct timeval timeNow;
            gettimeofday(&timeNow, NULL);
            int64_t timeUsPMT = (int64_t)timeNow.tv_sec*1000*1000 + (int64_t)timeNow.tv_usec;
            flags = 0;
            if (mPrevTimeUs < 0ll || mPrevTimeUs + 100000ll <= timeUsPMT || mFirstVideoFrame == 1) {
                flags |= EMIT_PCR;
                flags |= EMIT_PAT_AND_PMT;

                mPrevTimeUs = timeUsPMT;
                mFirstVideoFrame = 0;
            }

            //ALOGE("[%s %d] data:%x len:%d flags:%d timeUs:%lld", __FUNCTION__, __LINE__,
            //(char *)tESBuffer->data(), tESBuffer->range_length(), flags, timeUs);
            packetize(0, (char *)tESBuffer->data(), tESBuffer->range_length(), &tsPackets, flags, NULL, 0, 0, timeUs);

            tsPackets->meta()->setInt64("timeUs", timeUs);
            mOutputBufferQueue.push_back(tsPackets);

            tESBuffer->release();
            tESBuffer = NULL;
        } else if(mHasAudio) {
            err = mAudioConvertor->read(&tESBuffer);
            if (err != OK) {
                usleep(100);
                continue;
            }

            tESBuffer->meta_data().findInt64(kKeyTime, &timeUs);
            packetize(1, (char *)tESBuffer->data(), tESBuffer->range_length(), &tsPackets, 0, NULL, 0, 2, timeUs);
            tsPackets->meta()->setInt64("timeUs", timeUs);
            mOutputBufferQueue.push_back(tsPackets);
            tESBuffer->release();
            tESBuffer = NULL;
        } else {
            usleep(100);
        }
    }

    return 0;
}

// static
void *TSPacker::ThreadWrapper(void *me) {
    TSPacker *tspacker = static_cast<TSPacker *>(me);
    tspacker->threadFunc();
    return NULL;
}

void TSPacker::initCrcTable() {
    uint32_t poly = 0x04C11DB7;

    for (int i = 0; i < 256; i++) {
        uint32_t crc = i << 24;
        for (int j = 0; j < 8; j++) {
            crc = (crc << 1) ^ ((crc & 0x80000000) ? (poly) : 0);
        }
        mCrcTable[i] = crc;
    }
}

uint32_t TSPacker::crc32(const uint8_t *start, size_t size) const {
    uint32_t crc = 0xFFFFFFFF;
    const uint8_t *p;

    for (p = start; p < start + size; ++p) {
        crc = (crc << 8) ^ mCrcTable[((crc >> 24) ^ *p) & 0xFF];
    }

    return crc;
}

status_t TSPacker::start(MetaData *params)
{
    int err;

    Mutex::Autolock lock(mMutex);

    mFirstVideoFrame = 1;
    mFirstAudioFrame = 1;

    sp<MetaData> params_video = new MetaData;
    params_video->setInt32(kKeyWidth, mWidth);
    params_video->setInt32(kKeyHeight, mHeight);

    params_video->setInt32(kKeyFrameRate, mFrameRate);
    params_video->setInt32(kKeyBitRate, mBitRate);

    mVideoConvertor = new ESConvertor(mSourceType, 0);
    err = mVideoConvertor->start(params_video.get());

    if (mHasAudio) {
        sp<MetaData> params_audio = new MetaData;
        params_audio->setInt32(kKeyChannelCount, 2);
        params_audio->setInt32(kKeySampleRate, 48000);
        params_audio->setInt32(kKeyIsADTS, 1);
        mIsPcmAudio = 0;
        mAudioConvertor = new ESConvertor(mSourceType, 1);
        err = mAudioConvertor->start(params_audio.get());
    }

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&mThread, &attr, ThreadWrapper, this);
    pthread_attr_destroy(&attr);

#ifdef DUMPVIDEOES
	mDumpVideoEs = open("/data/temp/tspDumpVideo.es", O_CREAT | O_RDWR, 0666);
#endif

#ifdef DUMPVIDEOTS
	mDumpVideoTs = open("/data/temp/tspDumpTs.ts", O_CREAT | O_RDWR, 0666);
#endif

#ifdef DUMPAUDIOES
	mDumpAudioEs = open("/data/temp/tspDumpAAC.adts", O_CREAT | O_RDWR, 0666);
#endif

#ifdef DUMPAUDIOPCM
	mDumpAudioPCM = open("/data/temp/tspDumpPcm.pcm", O_CREAT | O_RDWR, 0666);
#endif

    mStarted = true;

    return OK;
}

status_t TSPacker::stop()
{
    ALOGE("stop");
    Mutex::Autolock lock(mMutex);

    mStarted = false;

    if (mHasAudio) {
        mAudioConvertor->stop();
    }

    mVideoConvertor->stop();

#ifdef DUMPVIDEOES
	close(mDumpVideoEs);
#endif
#ifdef DUMPVIDEOTS
	close(mDumpVideoTs);
#endif
#ifdef DUMPAUDIOES
	close(mDumpAudioEs);
#endif
#ifdef DUMPAUDIOPCM
	close(mDumpAudioPCM);
#endif

    return OK;
}

status_t TSPacker::incrementContinuityCounter(int isAudio)
{
    unsigned prevCounter;

    if (isAudio) {
        prevCounter = mAudioContinuityCounter;

        if (++mAudioContinuityCounter == 16) {
            mAudioContinuityCounter = 0;
        }

        return prevCounter;
    } else {
        prevCounter = mVideoContinuityCounter;

        if (++mVideoContinuityCounter == 16) {
            mVideoContinuityCounter = 0;
        }

        return prevCounter;
    }
}

void TSPacker::headFinalize() {
    if (mheadFinalize) {
        return;
    }

    {
        // AVC video descriptor (40)

        sp<ABuffer> descriptor = new ABuffer(6);
        uint8_t *data = descriptor->data();
        data[0] = 40;  // descriptor_tag
        data[1] = 4;  // descriptor_length

        ALOGE("[%s %d] mCSD.size:%d",__FUNCTION__, __LINE__, mCSD.size());

        if (mCSD.size() > 0) {
            CHECK_GE(mCSD.size(), 1u);
            const sp<ABuffer> &sps = mCSD.itemAt(0);
            CHECK(!memcmp("\x00\x00\x00\x01", sps->data(), 4));
            CHECK_GE(sps->size(), 7u);
            // profile_idc, constraint_set*, level_idc
            memcpy(&data[2], sps->data() + 4, 3);
        } else {
            int32_t profileIdc, levelIdc, constraintSet;

            profileIdc = 100;
            levelIdc   = 32;
            constraintSet = 12;

            data[2] = profileIdc;    // profile_idc
            data[3] = constraintSet; // constraint_set*
            data[4] = levelIdc;      // level_idc
        }

        // AVC_still_present=0, AVC_24_hour_picture_flag=0, reserved
        data[5] = 0x3f;

        ALOGE("[%s %d] make AVC video descriptor, size is 6",__FUNCTION__, __LINE__);

        mDescriptors.push_back(descriptor);
    }

    {
        // AVC timing and HRD descriptor (42)

        sp<ABuffer> descriptor = new ABuffer(4);
        uint8_t *data = descriptor->data();
        data[0] = 42;  // descriptor_tag
        data[1] = 2;  // descriptor_length

        // hrd_management_valid_flag = 0
        // reserved = 111111b
        // picture_and_timing_info_present = 0

        data[2] = 0x7e;

        // fixed_frame_rate_flag = 0
        // temporal_poc_flag = 0
        // picture_to_display_conversion_flag = 0
        // reserved = 11111b
        data[3] = 0x1f;
        ALOGE("[%s %d] make AVC timing and HRD descriptor, size is 4",__FUNCTION__, __LINE__);
        mDescriptors.push_back(descriptor);
    }

    if (mHasAudio && mIsPcmAudio) {
        // LPCM audio stream descriptor (0x83)

        int32_t channelCount;
        channelCount = 2;

        int32_t sampleRate;
        sampleRate = 48000;

        sp<ABuffer> descriptor = new ABuffer(4);
        uint8_t *data = descriptor->data();
        data[0] = 0x83;  // descriptor_tag
        data[1] = 2;  // descriptor_length

        unsigned sampling_frequency = (sampleRate == 44100) ? 1 : 2;

        data[2] = (sampling_frequency << 5) | (3 /* reserved */ << 1) | 0 /* emphasis_flag */;
        data[3] = (1 /* number_of_channels = stereo */ << 5) | 0xf /* reserved */;

        ALOGE("[%s %d] make  LPCM audio stream descriptor, size is 4",__FUNCTION__, __LINE__);

        mDescriptors.push_back(descriptor);
    }

    mheadFinalize = 1;
}

status_t TSPacker::packetize(
        bool isAudio,
        const char *buffer_add,
        int32_t buffer_size,
        sp<ABuffer> *packets,
        uint32_t flags,
        const uint8_t *PES_private_data, size_t PES_private_data_len,
        size_t numStuffingBytes,
        int64_t timeUs)
{
    int32_t stream_pid;
    int32_t stream_id;

    if (isAudio) {
        stream_pid = kPID_AUDIO;
        if (mIsPcmAudio) stream_id = 0xbd;
        else			       stream_id = 0xc0;
    } else {
        stream_pid = kPID_VIDEO;
        stream_id  = 0xe0;
    }

    packets->clear();

    bool alignPayload = 0;

    size_t PES_packet_length = buffer_size + 8 + numStuffingBytes;
    if (PES_private_data_len > 0) {
        PES_packet_length += PES_private_data_len + 1;
    }

    size_t numTSPackets = 1;

    {
        // Make sure the PES header fits into a single TS packet:
        size_t PES_header_size = 14 + numStuffingBytes;
        if (PES_private_data_len > 0) {
            PES_header_size += PES_private_data_len + 1;
        }

        CHECK_LE(PES_header_size, 188u - 4u);

        size_t sizeAvailableForPayload = 188 - 4 - PES_header_size;
        size_t numBytesOfPayload = buffer_size;

        if (numBytesOfPayload > sizeAvailableForPayload) {
            numBytesOfPayload = sizeAvailableForPayload;

        if (alignPayload && numBytesOfPayload > 16) {
            numBytesOfPayload -= (numBytesOfPayload % 16);
        }
    }

    // size_t numPaddingBytes = sizeAvailableForPayload - numBytesOfPayload;

    size_t numBytesOfPayloadRemaining = buffer_size - numBytesOfPayload;

    // This is how many bytes of payload each subsequent TS packet
    // can contain at most.
    sizeAvailableForPayload = 188 - 4;
    size_t sizeAvailableForAlignedPayload = sizeAvailableForPayload;
    if (alignPayload) {
        // We're only going to use a subset of the available space
        // since we need to make each fragment a multiple of 16 in size.
        sizeAvailableForAlignedPayload -= (sizeAvailableForAlignedPayload % 16);
    }

    size_t numFullTSPackets = numBytesOfPayloadRemaining / sizeAvailableForAlignedPayload;
    numTSPackets += numFullTSPackets;
    numBytesOfPayloadRemaining -= numFullTSPackets * sizeAvailableForAlignedPayload;

    // numBytesOfPayloadRemaining < sizeAvailableForAlignedPayload
    if (numFullTSPackets == 0 && numBytesOfPayloadRemaining > 0) {
        // There wasn't enough payload left to form a full aligned payload,
        // the last packet doesn't have to be aligned.
        ++numTSPackets;
    } else if (numFullTSPackets > 0
        && numBytesOfPayloadRemaining
        + sizeAvailableForAlignedPayload > sizeAvailableForPayload) {
            // The last packet emitted had a full aligned payload and together
            // with the bytes remaining does exceed the unaligned payload
            // size, so we need another packet.
            ++numTSPackets;
        }
    }

    if (flags & EMIT_PAT_AND_PMT) {
        numTSPackets += 2;
    }

    if (flags & EMIT_PCR) {
        ++numTSPackets;
    }

    sp<ABuffer> buffer = new ABuffer(numTSPackets * 188);
    uint8_t *packetDataStart = buffer->data();

    if (flags & EMIT_PAT_AND_PMT) {

        if (++mPATContinuityCounter == 16) {
            mPATContinuityCounter = 0;
        }

        uint8_t *ptr = packetDataStart;
        *ptr++ = 0x47;
        *ptr++ = 0x40;
        *ptr++ = 0x00;
        *ptr++ = 0x10 | mPATContinuityCounter;
        *ptr++ = 0x00;

        uint8_t *crcDataStart = ptr;
        *ptr++ = 0x00;
        *ptr++ = 0xb0;
        *ptr++ = 0x0d;
        *ptr++ = 0x00;
        *ptr++ = 0x00;
        *ptr++ = 0xc3;
        *ptr++ = 0x00;
        *ptr++ = 0x00;
        *ptr++ = 0x00;
        *ptr++ = 0x01;
        *ptr++ = 0xe0 | (kPID_PMT >> 8);
        *ptr++ = kPID_PMT & 0xff;

        CHECK_EQ(ptr - crcDataStart, 12);
        uint32_t crc = htonl(crc32(crcDataStart, ptr - crcDataStart));
        memcpy(ptr, &crc, 4);
        ptr += 4;

        size_t sizeLeft = packetDataStart + 188 - ptr;
        memset(ptr, 0xff, sizeLeft);

        packetDataStart += 188;

        if (++mPMTContinuityCounter == 16) {
            mPMTContinuityCounter = 0;
        }

        ptr = packetDataStart;
        *ptr++ = 0x47;
        *ptr++ = 0x40 | (kPID_PMT >> 8);
        *ptr++ = kPID_PMT & 0xff;
        *ptr++ = 0x10 | mPMTContinuityCounter;
        *ptr++ = 0x00;

        crcDataStart = ptr;
        *ptr++ = 0x02;

        *ptr++ = 0x00;	// section_length to be filled in below.
        *ptr++ = 0x00;

        *ptr++ = 0x00;
        *ptr++ = 0x01;
        *ptr++ = 0xc3;
        *ptr++ = 0x00;
        *ptr++ = 0x00;
        *ptr++ = 0xe0 | (kPID_PCR >> 8);
        *ptr++ = kPID_PCR & 0xff;

        *ptr++ = 0xf0 | (0 >> 8);
        *ptr++ = (0 & 0xff);

        headFinalize();
        size_t ES_info_length = 0;

        //***************video info******************//
        ES_info_length = 10;
        *ptr++ = 0x1b;//0x1b avc
        *ptr++ = 0xe0 | (kPID_VIDEO >> 8);
        *ptr++ = kPID_VIDEO & 0xff;

        *ptr++ = 0xf0 | (ES_info_length >> 8);
        *ptr++ = (ES_info_length & 0xff);
        {
            const sp<ABuffer> &descriptor = mDescriptors.itemAt(0);
            memcpy(ptr, descriptor->data(), descriptor->size());
            ptr += descriptor->size();
        }
        {
            const sp<ABuffer> &descriptor = mDescriptors.itemAt(1);
            memcpy(ptr, descriptor->data(), descriptor->size());
            ptr += descriptor->size();
         }
        //*****************************************//

        //***************audio info******************//
        if (mHasAudio) {
            ES_info_length = 4;
            if (mIsPcmAudio) *ptr++ = 0x83;//0x0f AAC, 0x83 PCM
            else            *ptr++ = 0x0f;

            *ptr++ = 0xe0 | (kPID_AUDIO >> 8);
            *ptr++ = kPID_AUDIO & 0xff;

            *ptr++ = 0xf0 | (ES_info_length >> 8);
            *ptr++ = (ES_info_length & 0xff);

            if (mIsPcmAudio) {
               const sp<ABuffer> &descriptor = mDescriptors.itemAt(2);
               memcpy(ptr, descriptor->data(), descriptor->size());
               ptr += descriptor->size();
               //ALOGE("[%s %d] size:%d",__FUNCTION__, __LINE__, descriptor->size());
            }
         }
            //*****************************************//

             size_t section_length = ptr - (crcDataStart + 3) + 4 /* CRC */;

             crcDataStart[1] = 0xb0 | (section_length >> 8);
             crcDataStart[2] = section_length & 0xff;
             crc = htonl(crc32(crcDataStart, ptr - crcDataStart));
             memcpy(ptr, &crc, 4);
             ptr += 4;

             sizeLeft = packetDataStart + 188 - ptr;
             memset(ptr, 0xff, sizeLeft);

             packetDataStart += 188;
          }

          if (flags & EMIT_PCR) {

               int64_t timeNow64;
               struct timeval timeNow;
               gettimeofday(&timeNow, NULL);
               int64_t nowUs = (int64_t)timeNow.tv_sec*1000*1000 + (int64_t)timeNow.tv_usec;

               //ALOGE("packetize			num:%lld latency:%lldms", timeUs, (nowUs - timeUs)/1000);

               uint64_t PCR = nowUs * 27;	// PCR based on a 27MHz clock
               uint64_t PCR_base = PCR / 300;
               uint32_t PCR_ext = PCR % 300;

               uint8_t *ptr = packetDataStart;
               *ptr++ = 0x47;
               *ptr++ = 0x40 | (kPID_PCR >> 8);
               *ptr++ = kPID_PCR & 0xff;
               *ptr++ = 0x20;
               *ptr++ = 0xb7;	// adaptation_field_length
               *ptr++ = 0x10;
               *ptr++ = (PCR_base >> 25) & 0xff;
               *ptr++ = (PCR_base >> 17) & 0xff;
               *ptr++ = (PCR_base >> 9) & 0xff;
               *ptr++ = ((PCR_base & 1) << 7) | 0x7e | ((PCR_ext >> 8) & 1);
               *ptr++ = (PCR_ext & 0xff);

               size_t sizeLeft = packetDataStart + 188 - ptr;
               memset(ptr, 0xff, sizeLeft);

               packetDataStart += 188;
            }

            uint64_t PTS = (timeUs * 9ll) / 100ll;

            if (PES_packet_length >= 65536) {
               // This really should only happen for video.
               // It's valid to set this to 0 for video according to the specs.
               PES_packet_length = 0;
            }

            size_t sizeAvailableForPayload = 188 - 4 - 14 - numStuffingBytes;
            if (PES_private_data_len > 0) {
               sizeAvailableForPayload -= PES_private_data_len + 1;
            }

            size_t copy = buffer_size;

            if (copy > sizeAvailableForPayload) {
            copy = sizeAvailableForPayload;

            if (alignPayload && copy > 16) {
                copy -= (copy % 16);
            }
        }

        size_t numPaddingBytes = sizeAvailableForPayload - copy;

        uint8_t *ptr = packetDataStart;
        *ptr++ = 0x47;
        *ptr++ = 0x40 | (stream_pid >> 8);
        *ptr++ = stream_pid & 0xff;

        *ptr++ = (numPaddingBytes > 0 ? 0x30 : 0x10) | incrementContinuityCounter(isAudio);

        if (numPaddingBytes > 0) {
            *ptr++ = numPaddingBytes - 1;
            if (numPaddingBytes >= 2) {
                *ptr++ = 0x00;
                memset(ptr, 0xff, numPaddingBytes - 2);
                ptr += numPaddingBytes - 2;
            }
        }

        *ptr++ = 0x00;
        *ptr++ = 0x00;
        *ptr++ = 0x01;
        *ptr++ = stream_id;
        *ptr++ = PES_packet_length >> 8;
        *ptr++ = PES_packet_length & 0xff;
        *ptr++ = 0x84;
        *ptr++ = (PES_private_data_len > 0) ? 0x81 : 0x80;

        size_t headerLength = 0x05 + numStuffingBytes;
        if (PES_private_data_len > 0) {
            headerLength += 1 + PES_private_data_len;
        }

        *ptr++ = headerLength;

        *ptr++ = 0x20 | (((PTS >> 30) & 7) << 1) | 1;
        *ptr++ = (PTS >> 22) & 0xff;
        *ptr++ = (((PTS >> 15) & 0x7f) << 1) | 1;
        *ptr++ = (PTS >> 7) & 0xff;
        *ptr++ = ((PTS & 0x7f) << 1) | 1;

        if (PES_private_data_len > 0) {
            *ptr++ = 0x8e;	// PES_private_data_flag, reserved.
	            memcpy(ptr, PES_private_data, PES_private_data_len);
            ptr += PES_private_data_len;
        }

        for (size_t i = 0; i < numStuffingBytes; ++i) {
            *ptr++ = 0xff;
        }

        memcpy(ptr, buffer_add, copy);
        ptr += copy;

        CHECK_EQ(ptr, packetDataStart + 188);
        packetDataStart += 188;

        size_t offset = copy;
        while (offset < buffer_size) {

        size_t sizeAvailableForPayload = 188 - 4;

        size_t copy = buffer_size - offset;

        if (copy > sizeAvailableForPayload) {
            copy = sizeAvailableForPayload;

            if (alignPayload && copy > 16) {
                copy -= (copy % 16);
           }
        }

        size_t numPaddingBytes = sizeAvailableForPayload - copy;

        uint8_t *ptr = packetDataStart;
        *ptr++ = 0x47;
        *ptr++ = 0x00 | (stream_pid >> 8);
        *ptr++ = stream_pid & 0xff;

        *ptr++ = (numPaddingBytes > 0 ? 0x30 : 0x10) | incrementContinuityCounter(isAudio);

        if (numPaddingBytes > 0) {
            *ptr++ = numPaddingBytes - 1;
            if (numPaddingBytes >= 2) {
                *ptr++ = 0x00;
                memset(ptr, 0xff, numPaddingBytes - 2);
                ptr += numPaddingBytes - 2;
            }
        }

        memcpy(ptr, buffer_add + offset, copy);
        ptr += copy;
        CHECK_EQ(ptr, packetDataStart + 188);

        offset += copy;
        packetDataStart += 188;
    }

    CHECK(packetDataStart == buffer->data() + buffer->capacity());

    *packets = buffer;

    return OK;
}


static void ReleaseMediaBufferReference(const sp<ABuffer> &accessUnit) {
    void *mbuf;
    if (accessUnit->meta()->findPointer("mediaBuffer", &mbuf)
            && mbuf != NULL) {
        ALOGE("releasing mbuf %p", mbuf);

        accessUnit->meta()->setPointer("mediaBuffer", NULL);

        static_cast<MediaBuffer *>(mbuf)->release();
        mbuf = NULL;
    }
}

status_t TSPacker::read( MediaBufferBase **buffer,
				 const ReadOptions *options)
{
    Mutex::Autolock lock(mMutex);

    int64_t timeUs;

    *buffer = NULL;

    if (mOutputBufferQueue.empty()) {
        return !OK;
    }

    sp<ABuffer> aBuffer = *mOutputBufferQueue.begin();

    MediaBuffer *tBuffer = new MediaBuffer(aBuffer->size() + 16);

    if (tBuffer->data() == 0) {
        ALOGE("buffer is cant malloc");
        return !OK;
    }

    mOutputBufferQueue.erase(mOutputBufferQueue.begin());

    //ALOGE("[%s %d] aBuffer:%x aBuffer->size:%d tBuffer:%x", __FUNCTION__, __LINE__, aBuffer->data(), aBuffer->size(), tBuffer->data());

    memcpy(tBuffer->data(), aBuffer->data(), aBuffer->size());

    aBuffer->meta()->findInt64("timeUs", &timeUs);

    tBuffer->set_range(0,aBuffer->size());

    *buffer = (MediaBufferBase *)tBuffer;

    (*buffer)->setObserver(this);
    (*buffer)->add_ref();
    (*buffer)->meta_data().setInt64(kKeyTime, 0);

    return OK;
}

void TSPacker::signalBufferReturned(MediaBufferBase *buffer) {

    Mutex::Autolock lock(mMutex);

    buffer->setObserver(0);
    buffer->release();
}

} // end of namespace android
