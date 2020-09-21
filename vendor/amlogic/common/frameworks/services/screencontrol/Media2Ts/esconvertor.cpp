/*
 * Copyright (C) 2011 The Android Open Source Project
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
#define LOG_NDEBUG 0
#define LOG_TAG "ESConvertor"

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

#include <cutils/properties.h>
#include <media/ICrypto.h>
#include <pthread.h>

#include <ABuffer.h>
#include <ADebug.h>
#include <AMessage.h>
#include <AHandler.h>
#include <ALooper.h>
#include <media/MediaCodecBuffer.h>
#include <media/stagefright/MediaBuffer.h>
#include <media/stagefright/MediaBufferBase.h>
#include <media/MediaCodecBuffer.h>
#include <media/stagefright/MediaCodec.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/AudioSource.h>
#include <media/hardware/MetadataBufferType.h>
#include <media/stagefright/foundation/avc_utils.h>
#include <media/MediaSource.h>

#include <OMX_IVCommon.h>
#include <OMX_Video.h>
#include <gui/Surface.h>

#include <OMX_Component.h>

#include <utils/Log.h>
#include <utils/String8.h>

#include "esconvertor.h"
#include "media/stagefright/foundation/avc_utils.h"

#include "../ScreenManager.h"

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <binder/MemoryBase.h>
#include <binder/MemoryHeapBase.h>

#define TEST_VIDEO_BITRATE

namespace android {

#define VIDEO_ENCODE 0
#define AUDIO_ENCODE 1

//#define ESCDUMPAUDIOAAC 1
//#define ESCDUMPAUDIOPCM 1

int ESConvertor::CanvasdataCallBack(const sp<IMemory>& data){
    int ret = NO_ERROR;
    Mutex::Autolock autoLock(mLock);

    if (mStarted == true) {
        MediaBuffer *tBuffer = new MediaBuffer(3*sizeof(unsigned));
        memcpy(tBuffer->data(), (uint8_t *)data->pointer(), 3*sizeof(unsigned));
        mFramesReceived.push_back(tBuffer);
    }else{
        mScreenManager->freeBuffer(mClientId, data);
        return !OK;
    }

    return ret;
}

ESConvertor::ESConvertor(int sourceType, int IsAudio) :
    mIsAudio(IsAudio),
    mWidth(1280),
    mHeight(720),
    mSourceType(sourceType),
    mAudioChannelCount(2),
    mAudioSampleRate(48000),
    mVideoFrameRate(30),
    mVIdeoBitRate(2000000),
    mStarted(false),
    mIsPCMAudio(1),
    mDequeueBufferTotal(0),
    mQueueBufferTotal(0),
    mIsSoftwareEncoder(false),
    mDumpYuvFd(-1),
    mDumpEsFd(-1)
{
    int fd = -1;
    fd = open("/dev/amvenc_avc", O_RDWR);
    if (fd < 0) {
        mIsSoftwareEncoder = true;
        ALOGE("Open /dev/amvenc_avc failed, use software encoder instead!\n");
    } else {
        close(fd);
        fd = -1;
    }
    ALOGE("ESConvertor construct\n");
}

ESConvertor::~ESConvertor() {
    ALOGE("~ESConvertor");
    CHECK(!mStarted);
}

nsecs_t ESConvertor::getTimestamp() {
    ALOGE("getTimestamp");
    Mutex::Autolock lock(mMutex);
    return mCurrentTimestamp;
}

status_t ESConvertor::setFrameRate(int32_t fps)
{
    ALOGE("setFrameRate");
    Mutex::Autolock lock(mMutex);
    const int MAX_FRAME_RATE = 60;
    if (fps <= 0 || fps > MAX_FRAME_RATE) {
        return BAD_VALUE;
    }
    mFrameRate = fps;
    return OK;
}

bool ESConvertor::isMetaDataStoredInVideoBuffers() const {
    ALOGE("isMetaDataStoredInVideoBuffers");
    if (mIsSoftwareEncoder) {
        return false;
    }
    else
        return true;
}

int32_t ESConvertor::getFrameRate( ) const {
    ALOGE("getFrameRate %d", mFrameRate);
    Mutex::Autolock lock(mMutex);
    return mFrameRate;
}

status_t ESConvertor::feedEncoderInputBuffers() {
    status_t err;

    if (!mInputBufferQueue.empty() && !mAvailEncoderInputIndices.empty())
    {
        sp<ABuffer> buffer = *mInputBufferQueue.begin();
        mInputBufferQueue.erase(mInputBufferQueue.begin());

        size_t bufferIndex = *mAvailEncoderInputIndices.begin();
        mAvailEncoderInputIndices.erase(mAvailEncoderInputIndices.begin());

        int64_t timeUs = 0ll;
        uint32_t flags = 0;

        if (buffer != NULL) {
            CHECK(buffer->meta()->findInt64("timeUs", &timeUs));
            memcpy(mEncoderInputBuffers.itemAt(bufferIndex)->data(),buffer->data(),buffer->size());
            if (mDumpYuvFd >= 0 && buffer->size() > 0 && mIsSoftwareEncoder) {
                write(mDumpYuvFd, buffer->data(), buffer->size());
            }

            if (!mIsSoftwareEncoder) {
                void *mediaBuffer;
                if (buffer->meta()->findPointer("mediaBuffer", &mediaBuffer) && mediaBuffer != NULL) {
                    mEncoderInputBuffers.itemAt(bufferIndex)->meta()->setPointer("mediaBuffer", mediaBuffer);
                    buffer->meta()->setPointer("mediaBuffer", NULL);
                }
            }
        } else {
            flags = MediaCodec::BUFFER_FLAG_EOS;
        }

        err = mEncoder->queueInputBuffer(bufferIndex, 0, (buffer == NULL) ? 0 : buffer->size(), timeUs, flags);

        if (err != OK) {
        ALOGE("[%s %d] queueInputBuffer fail\n", __FUNCTION__, __LINE__);
            return err;
        }
        mQueueBufferTotal++;
    }

    return OK;
}

status_t ESConvertor::initEncoder() {
    status_t err = NO_INIT;

    sp<AMessage> msg = new AMessage;
    mInputFormat = msg;

    AString outputMIME;

    if (mIsAudio == VIDEO_ENCODE) {
        mInputFormat->setString("mime", MEDIA_MIMETYPE_VIDEO_AVC);

        mInputFormat->setInt32("width", mWidth);
        mInputFormat->setInt32("height", mHeight);

        AString inputMIME;
        CHECK(mInputFormat->findString("mime", &inputMIME));

        outputMIME = MEDIA_MIMETYPE_VIDEO_AVC;
    }

    if (mIsAudio == AUDIO_ENCODE) {
        outputMIME = MEDIA_MIMETYPE_AUDIO_AAC;
    }

    sp<ALooper> mCodecLooper = new ALooper;
    mCodecLooper->setName("codec_looper");

    mCodecLooper->start(
            false /* runOnCallingThread */,
            false /* canCallJava */,
            PRIORITY_AUDIO);

    mEncoder = MediaCodec::CreateByType(
            mCodecLooper, outputMIME.c_str(), true /* encoder */);

    if (mEncoder == NULL) {
        ALOGE("[%s %d]\n", __FUNCTION__, __LINE__);
        return ERROR_UNSUPPORTED;
    }

    mOutputFormat = mInputFormat->dup();
    mOutputFormat->setString("mime", outputMIME.c_str());

    if (mIsAudio == VIDEO_ENCODE) {
        mOutputFormat->setInt32("bitrate", mVIdeoBitRate);
        mOutputFormat->setInt32("bitrate-mode", OMX_Video_ControlRateConstant);
        mOutputFormat->setInt32("frame-rate", mVideoFrameRate);
        if (mIsSoftwareEncoder) {
            mOutputFormat->setInt32("i-frame-interval", 5);
            mOutputFormat->setInt32("color-format", OMX_COLOR_FormatYUV420SemiPlanar);
            mOutputFormat->setInt32("store-metadata-in-buffers", false);
            mOutputFormat->setInt32("prepend-sps-pps-to-idr-frames", 0);
        } else {
            mOutputFormat->setInt32("i-frame-interval", 15);  // Iframes every 15 secs
            mOutputFormat->setInt32("color-format", OMX_COLOR_FormatAndroidOpaque);
            mOutputFormat->setInt32("store-metadata-in-buffers", true);
            mOutputFormat->setInt32("prepend-sps-pps-to-idr-frames", 1);
        }
        //mOutputFormat->setInt32("intra-refresh-mode", OMX_VIDEO_IntraRefreshCyclic);
        //mOutputFormat->setInt32("store-metadata-in-buffers-output", 0);

        int width, height, mbs;
        if (!mOutputFormat->findInt32("width", &width)
                || !mOutputFormat->findInt32("height", &height)) {
            ALOGE("[%s %d] ERROR_UNSUPPORTED\n", __FUNCTION__, __LINE__);
            return ERROR_UNSUPPORTED;
        }

        mbs = (((width + 15) / 16) * ((height + 15) / 16) * 10) / 100;
        //mOutputFormat->setInt32("intra-refresh-CIR-mbs", mbs);
    } else {
        mOutputFormat->setInt32("bitrate", 128000);
        mOutputFormat->setInt32("channel-count", 2);
        mOutputFormat->setInt32("sample-rate", mAudioSampleRate);
    }

    err = mEncoder->configure(
            mOutputFormat,
            NULL /* nativeWindow */,
            NULL /* crypto */,
            MediaCodec::CONFIGURE_FLAG_ENCODE);

    if (err == OK) {
        // Encoder supported prepending SPS/PPS, we don't need to emulate
        // it.
    } else {
        ALOGE("We going to manually prepend SPS and PPS to IDR frames.");
    }

    err = mEncoder->start();

    if (err != OK) {
        ALOGE("[%s %d] err:%d\n", __FUNCTION__, __LINE__, err);
        return err;
    }

    err = mEncoder->getInputBuffers(&mEncoderInputBuffers);
    if (err != OK) {
        ALOGE("[%s %d] err:%d\n", __FUNCTION__, __LINE__, err);
        return err;
    }

    err = mEncoder->getOutputBuffers(&mEncoderOutputBuffers);

    if (err != OK) {
        ALOGE("[%s %d] err:%d\n", __FUNCTION__, __LINE__, err);
        return err;
    }

    if (mIsSoftwareEncoder) {
        int value = 0;
        char prop[PROPERTY_VALUE_MAX];
        memset(prop, 0, sizeof(prop));
        if (property_get("media.screen.dumpfile", prop, NULL) > 0) {
            sscanf(prop, "%d", &value);
        }
        if (value > 0) {
            const char filename[] = "/data/rawdata.yuv";
            const char filenameEs[] = "/data/es.h264";
            ALOGD("Enable Dump yuv file, name: %s es filename: %s", filename, filenameEs);
            mDumpYuvFd= open(filename, O_CREAT | O_LARGEFILE | O_TRUNC | O_RDWR, S_IRUSR | S_IWUSR);
            if (mDumpYuvFd < 0)
                ALOGE("Dump mDumpYuvFd error! mDumpYuvFd: %d", mDumpYuvFd);

            mDumpEsFd = open(filenameEs, O_CREAT | O_LARGEFILE | O_TRUNC | O_RDWR, S_IRUSR | S_IWUSR);
            if (mDumpEsFd < 0)
                ALOGE("Dump mDumpEsFd error! mDumpEsFd: %d", mDumpEsFd);
        }
    }
    return err;
}

sp<ABuffer> ESConvertor::prependStartCode(
            const sp<ABuffer> &accessUnit) const {

    sp<ABuffer> dup = new ABuffer(accessUnit->size() + 4);
    memcpy(dup->data(), "\x00\x00\x00\x01", 4);
    memcpy(dup->data() + 4, accessUnit->data(), accessUnit->size());
    return dup;
}

sp<ABuffer>ESConvertor::prependCSD(sp<ABuffer> &accessUnit, sp<ABuffer> CSDBuffer){
    sp<ABuffer> dup = new ABuffer(accessUnit->size() + CSDBuffer->size());
    size_t offset = 0;

    memcpy(dup->data() + offset, CSDBuffer->data(), CSDBuffer->size());
    offset += CSDBuffer->size();

    memcpy(dup->data() + offset, accessUnit->data(), accessUnit->size());

    return dup;
}

sp<ABuffer> ESConvertor::prependADTSHeader(const sp<ABuffer> &accessUnit) const {
    CHECK_EQ(mCSDADTS.size(), 1u);
    const uint8_t *codec_specific_data = mCSDADTS.itemAt(0)->data();

    const uint32_t aac_frame_length = accessUnit->size() + 7;

    sp<ABuffer> dup = new ABuffer(aac_frame_length);

    unsigned profile = (codec_specific_data[0] >> 3) - 1;

    unsigned sampling_freq_index =
        ((codec_specific_data[0] & 7) << 1)
        | (codec_specific_data[1] >> 7);

    unsigned channel_configuration =
        (codec_specific_data[1] >> 3) & 0x0f;

    uint8_t *ptr = dup->data();

    *ptr++ = 0xff;
    *ptr++ = 0xf1;  // b11110001, ID=0, layer=0, protection_absent=1

    *ptr++ =
        profile << 6
        | sampling_freq_index << 2
        | ((channel_configuration >> 2) & 1);  // private_bit=0

    // original_copy=0, home=0, copyright_id_bit=0, copyright_id_start=0
    *ptr++ =
        (channel_configuration & 3) << 6
        | aac_frame_length >> 11;
    *ptr++ = (aac_frame_length >> 3) & 0xff;
    *ptr++ = (aac_frame_length & 7) << 5;

    // adts_buffer_fullness=0, number_of_raw_data_blocks_in_frame=0
    *ptr++ = 0;

    memcpy(ptr, accessUnit->data(), accessUnit->size());

    return dup;
}

#if 0
int ESConvertor::threadAudioFunc()
{
    while (mStarted == true) {
        sp<ABuffer> accessUnit;
        status_t err;
        {
            Mutex::Autolock lock(mMutex);
            MediaBuffer *mbuf;
            err = mAudioSource->read(&mbuf);

            if (err != OK) {
                usleep(100);
                continue;
            }

            accessUnit = new ABuffer(mbuf->range_length());
            memcpy(accessUnit->data(), (const uint8_t *)mbuf->data() + mbuf->range_offset(), mbuf->range_length());
            mbuf->release();
            mbuf = NULL;
        }

		    int64_t timeNow64;
		    struct timeval timeNow;
		    gettimeofday(&timeNow, NULL);
		    int64_t nowUs = (int64_t)timeNow.tv_sec*1000*1000 + (int64_t)timeNow.tv_usec;
		    accessUnit->meta()->setInt64("timeUs", nowUs);

#ifdef ESCDUMPAUDIOPCM
		write(mEscDumpPcm, accessUnit->data(), accessUnit->size());
		ALOGE("[%s %d] mEscDumpPcm:%d size:%d", __FUNCTION__, __LINE__, mEscDumpPcm, accessUnit->size());
#endif

        if (mIsPCMAudio) {
            int16_t *ptr = (int16_t *)accessUnit->data();
            int16_t *stop = (int16_t *)(accessUnit->data() + accessUnit->size());
            while (ptr < stop) {
                *ptr = htons(*ptr);
                ++ptr;
            }

            static const size_t kFrameSize = 2 * sizeof(int16_t);  // stereo
            static const size_t kFramesPerAU = 80;
            static const size_t kNumAUsPerPESPacket = 6;

            if (mPartialAudioAU != NULL) {
                size_t bytesMissingForFullAU = kNumAUsPerPESPacket * kFramesPerAU * kFrameSize - mPartialAudioAU->size() + 4;

            size_t copy = accessUnit->size();
            if (copy > bytesMissingForFullAU) {
                copy = bytesMissingForFullAU;
            }

            memcpy(mPartialAudioAU->data() + mPartialAudioAU->size(), accessUnit->data(), copy);
            mPartialAudioAU->setRange(0, mPartialAudioAU->size() + copy);
            accessUnit->setRange(accessUnit->offset() + copy, accessUnit->size() - copy);

            int64_t timeUs;
            CHECK(accessUnit->meta()->findInt64("timeUs", &timeUs));

            int64_t copyUs = (int64_t)((copy / kFrameSize) * 1E6 / 48000.0);
            timeUs += copyUs;
            accessUnit->meta()->setInt64("timeUs", timeUs);

                if (bytesMissingForFullAU == copy) {
                    ALOGE("[%s %d] size:%d timeUs_temp:%llx", __FUNCTION__, __LINE__, mPartialAudioAU->size(), timeUs);
                    mOutputBufferQueue.push_back(mPartialAudioAU);
                    mPartialAudioAU.clear();
                }
            }

            while (accessUnit->size() > 0) {
                sp<ABuffer> partialAudioAU = new ABuffer(4 + kNumAUsPerPESPacket * kFrameSize * kFramesPerAU);

                uint8_t *ptr = partialAudioAU->data();
                ptr[0] = 0xa0;	// 10100000b
                ptr[1] = kNumAUsPerPESPacket;
                ptr[2] = 0;  // reserved, audio _emphasis_flag = 0

                static const unsigned kQuantizationWordLength = 0;	// 16-bit
                static const unsigned kAudioSamplingFrequency = 2;	// 48Khz
                static const unsigned kNumberOfAudioChannels = 1;  // stereo

                ptr[3] = (kQuantizationWordLength << 6) | (kAudioSamplingFrequency << 3) | kNumberOfAudioChannels;

                size_t copy = accessUnit->size();
                if (copy > partialAudioAU->size() - 4) {
                    copy = partialAudioAU->size() - 4;
                }

                memcpy(&ptr[4], accessUnit->data(), copy);

                partialAudioAU->setRange(0, 4 + copy);
                accessUnit->setRange(accessUnit->offset() + copy, accessUnit->size() - copy);

                int64_t timeUs;
                CHECK(accessUnit->meta()->findInt64("timeUs", &timeUs));

                partialAudioAU->meta()->setInt64("timeUs", timeUs);

                int64_t copyUs = (int64_t)((copy / kFrameSize) * 1E6 / 48000.0);
                timeUs += copyUs;
                accessUnit->meta()->setInt64("timeUs", timeUs);

                if (copy == partialAudioAU->capacity() - 4) {
                    ALOGE("[%s %d] size:%d timeUs:%llx", __FUNCTION__, __LINE__, partialAudioAU->size(), timeUs);
                    mOutputBufferQueue.push_back(partialAudioAU);
                    partialAudioAU.clear();
                    continue;
                }

                mPartialAudioAU = partialAudioAU;
            }

            continue;
        }

		    mInputBufferQueue.push_back(accessUnit);
		    feedEncoderInputBuffers();

        for (;;) {
            size_t bufferIndex;
            size_t offset;
            size_t size;
            int64_t timeUs;
            uint32_t flags;

            if (mStarted != true)
                break;

            err = mEncoder->dequeueOutputBuffer(&bufferIndex, &offset, &size, &timeUs, &flags);

            if (err != OK || size == 0) {
                usleep(1);
                break;
            }

            if (flags & MediaCodec::BUFFER_FLAG_EOS || flags & MediaCodec::BUFFER_FLAG_CODECCONFIG) {
                //TODO
                //usleep(1);
                sp<ABuffer> buffer = new ABuffer(size);

                memcpy(buffer->data(), mEncoderOutputBuffers.itemAt(bufferIndex)->base() + offset, size);

                mCSDADTS.push(buffer);
                break;
            } else {
                sp<ABuffer> buffer = new ABuffer(size);

                memcpy(buffer->data(), mEncoderOutputBuffers.itemAt(bufferIndex)->base() + offset, size);

                buffer = prependADTSHeader(buffer);

                buffer->meta()->setInt64("timeUs", timeUs);
#ifdef ESCDUMPAUDIOAAC
				write(mEscDumpAAC, buffer->data(), buffer->size());
				ALOGE("[%s %d] mEscDumpAAC:%d size:%d", __FUNCTION__, __LINE__, mEscDumpAAC, buffer->size());
#endif
                mOutputBufferQueue.push_back(buffer);
            }

            mEncoder->releaseOutputBuffer(bufferIndex);

            if (flags & MediaCodec::BUFFER_FLAG_EOS) {
                break;
            }
        }

        for (;;) {
            size_t bufferIndex;
            err = mEncoder->dequeueInputBuffer(&bufferIndex);

            if (err != OK) {
                break;
            }

            mAvailEncoderInputIndices.push_back(bufferIndex);
        }
    }

    ALOGE("[%s %d] audio thread out\n", __FUNCTION__, __LINE__);
    return OK;
}

#endif

int ESConvertor::videoDequeueInputBuffer()
{
    int err;
    size_t bufferIndex;

    err = mEncoder->dequeueInputBuffer(&bufferIndex);
    if (err != OK) {
        return !OK;
    }

    mAvailEncoderInputIndices.push_back(bufferIndex);
    mDequeueBufferTotal++;

    return OK;
}

int ESConvertor::videoFeedInputBuffer() {

    int err;
    unsigned buff_info[3];
    MediaBuffer* tBuffer = NULL;

    if (mStarted == false)
        return !OK;

    {
RETRY:
        int64_t pts;
        MediaBuffer *tBuffer = new MediaBuffer(3*sizeof(unsigned));
        err = mScreenManager->readBuffer(mClientId, mBufferGet, &pts);

        if (err == OK && mStarted != false) {
            memcpy(tBuffer->data(), (uint8_t *)mBufferGet->pointer(), 3*sizeof(unsigned));
            mFramesReceived.push_back(tBuffer);
            goto RETRY;
        }
    }

    if (!mFramesReceived.empty())
    {
        tBuffer = *mFramesReceived.begin();
        mFramesReceived.erase(mFramesReceived.begin());

        sp<ABuffer> accessUnit = new ABuffer(12);

        memcpy((uint8_t *)accessUnit->data(), (uint8_t *)tBuffer->data(), 12);
        memcpy(&buff_info[0], (uint8_t *)tBuffer->data(), 12);

        int64_t timeNow64;
        struct timeval timeNow;
        gettimeofday(&timeNow, NULL);
        int64_t nowUs = (int64_t)timeNow.tv_sec*1000*1000 + (int64_t)timeNow.tv_usec;

        tBuffer->meta_data().setInt32(kKeyBufferID, 0xf);
        tBuffer->setObserver(this);
        tBuffer->add_ref();
        accessUnit->meta()->setPointer("mediaBuffer", tBuffer);
        accessUnit->meta()->setInt64("timeUs", nowUs);

        mInputBufferQueue.push_back(accessUnit);
        feedEncoderInputBuffers();
    }
    return OK;
}

int ESConvertor::videoSwEncoderFeedInputBuffer() {

    int err;
    unsigned buff_info[3];
    MediaBuffer *tBufferRec = NULL;
    if (mStarted == false)
        return !OK;

    {
RETRY:
        int64_t pts;
        sp<ABuffer> accessUnit = new ABuffer(mWidth * mHeight * 3 >> 1);
        err = mScreenManager->readBuffer(mClientId, mBufferGet, &pts);

        if (err == OK && mStarted != false) {
            memcpy(accessUnit->data(), (uint8_t *)mBufferGet->pointer(), mWidth * mHeight * 3 >> 1);

            int64_t timeNow64;
            struct timeval timeNow;
            gettimeofday(&timeNow, NULL);
            int64_t nowUs = (int64_t)timeNow.tv_sec*1000*1000 + (int64_t)timeNow.tv_usec;

            accessUnit->meta()->setInt64("timeUs", nowUs);
            mInputBufferQueue.push_back(accessUnit);
            goto RETRY;
        }
    }
    feedEncoderInputBuffers();
    return OK;
}

int ESConvertor::videoDequeueOutputBuffer() {
    int err;

    if (mStarted == false)
        return !OK;

    size_t bufferIndex;
    size_t offset;
    size_t size;
    int64_t timeUs;
    uint32_t flags;

#ifdef TEST_VIDEO_BITRATE
	static int64_t mCurrentTime = 0;
	static int mBitratePerSecond = 0;
	static int mFrameNum = 0;
	static int mTotalTime = 0;
	static int mTotalSize = 0;
#endif

    err = mEncoder->dequeueOutputBuffer( &bufferIndex, &offset, &size, &timeUs, &flags);

    if (err != OK) {
        return !OK;
    }

    if (flags & MediaCodec::BUFFER_FLAG_EOS) {
        //TODO
        ALOGE("[%s %d] err:%d\n", __FUNCTION__, __LINE__, err);
        return !OK;
    } else {
        sp<ABuffer> buffer = new ABuffer(size);

        memcpy(buffer->data(), mEncoderOutputBuffers.itemAt(bufferIndex)->base() + offset, size);
        if (mDumpEsFd >= 0 && size > 0 && mIsSoftwareEncoder) {
            write(mDumpEsFd, buffer->data(), size);
        }

        if (flags & MediaCodec::BUFFER_FLAG_CODECCONFIG) {
            //store ppssps header
            mCSDbuffer = buffer;
            mOutputBufferQueue.push_back(buffer);
        } else {
            //push queue
            if (IsIDR(buffer->base(),buffer->size())) {
                buffer = prependCSD(buffer, mCSDbuffer);
            }
            buffer->meta()->setInt64("timeUs", timeUs);
            mOutputBufferQueue.push_back(buffer);
        }

#ifdef TEST_VIDEO_BITRATE
    ALOGE("[%s %d] currtime:%lld mFrameNum:%d size:%d\n", __FUNCTION__, __LINE__, mCurrentTime, mFrameNum, size);
    int64_t timeTemp = systemTime(SYSTEM_TIME_MONOTONIC)/1000000000;
    mFrameNum++;
    mTotalSize += size;
    if (timeTemp > mCurrentTime) {
        ALOGE("[%s %d] currtime:%lld mFrameNum:%d Size:%d MB:%.3f Mbits:%.3f\n", __FUNCTION__, __LINE__,
            mCurrentTime, mFrameNum, mBitratePerSecond, (float)mBitratePerSecond/(float)1048576, (float)mBitratePerSecond*(float)8/(float)1048576);
        mBitratePerSecond = size;
        mCurrentTime = timeTemp;
        mFrameNum = 1;
        mTotalTime++;

        ALOGE("[%s %d] Totel time:%d Size:%d MB:%.3f Mbits:%.3f\n", __FUNCTION__, __LINE__,
            mTotalTime, mTotalSize, (float)mTotalSize/(float)1048576/(float)mTotalTime, (float)mTotalSize*(float)8/(float)1048576/(float)mTotalTime);
    } else {
        mBitratePerSecond += size;
    }

#endif
    }

    mEncoder->releaseOutputBuffer(bufferIndex);

    if (flags & MediaCodec::BUFFER_FLAG_EOS) {
        ALOGE("[%s %d]\n", __FUNCTION__, __LINE__);
        return !OK;
    }

    return OK;
}

int ESConvertor::threadVideoFunc() {
    int err;
    int wait_time = 0;
    bool first_pkt = 0;

    while (1) {
        videoDequeueInputBuffer();
        if (mIsSoftwareEncoder) {
            videoSwEncoderFeedInputBuffer();
        } else {
            videoFeedInputBuffer();
        }

        err = videoDequeueOutputBuffer();

        if (err == !OK)
            usleep(2000);

        if (mStarted == false)
            break;
    }

THREADOUT:
    mThreadOutCondition.signal();
    ALOGE("[%s %d] mDequeueBufferTotal:%lld mQueueBufferTotal:%lld video thread out\n", __FUNCTION__, __LINE__, mDequeueBufferTotal, mQueueBufferTotal);
    return OK;
}

int ESConvertor::threadFunc() {
    if (mIsAudio == AUDIO_ENCODE){}      //return threadAudioFunc();
    else if (mIsAudio == VIDEO_ENCODE) return threadVideoFunc();
    else                              return !OK;

    return 0;
}

// static
void *ESConvertor::ThreadWrapper(void *me) {
    ESConvertor *Convertor = static_cast<ESConvertor *>(me);
    Convertor->threadFunc();
    return NULL;
}

void ESConvertor::setVideoCrop(int x, int y, int width, int height){
    mCorpX = x;
    mCorpY = y;
    mCorpWidth = width;
    mCorpHeight = height;
}

status_t ESConvertor::start(MetaData *params) {
    Mutex::Autolock lock(mMutex);

    CHECK(!mStarted);
    status_t err = -1;
    int32_t client_id;
    mStartTimeNs = 0;
    int64_t startTimeUs;

    ALOGE("[%s %d] mIsAudio:%d\n", __FUNCTION__, __LINE__, mIsAudio);

    if (mIsAudio == VIDEO_ENCODE) {
        if (params) {
            params->findInt32(kKeyWidth, &mWidth);
            params->findInt32(kKeyHeight, &mHeight);
            params->findInt32(kKeyFrameRate, &mVideoFrameRate);
            params->findInt32(kKeyBitRate, &mVIdeoBitRate);
        }

        ALOGE("[%s %d] ESConvertor get video info mWidth:%d mHeight:%d mVideoFrameRate:%d mVIdeoBitRate:%d mSourceType:%d\n", __FUNCTION__, __LINE__,
            mWidth, mHeight, mVideoFrameRate, mVIdeoBitRate, mSourceType);

        mScreenManager = ScreenManager::instantiate();
        if (mIsSoftwareEncoder) {
            mScreenManager->mIsScreenRecord = true;
            mScreenManager->init(mWidth, mHeight, mSourceType, mVideoFrameRate, SCREENCONTROL_RAWDATA_TYPE, &client_id, NULL);
        } else {
            mScreenManager->init(mWidth, mHeight, mSourceType, mVideoFrameRate, SCREENCONTROL_CANVAS_TYPE, &client_id, NULL);
        }
        mScreenManager->setVideoCrop(client_id, mCorpX, mCorpY, mCorpWidth, mCorpHeight);

        mClientId = client_id;

        if (mIsSoftwareEncoder) {
            mNewMemoryHeap = NULL;
            mNewMemoryHeap = new MemoryHeapBase(mWidth * mHeight * 3 >> 1);
            mBufferGet = new MemoryBase(mNewMemoryHeap, 0, mWidth * mHeight * 3 >> 1);
        } else {
            sp<MemoryHeapBase> newMemoryHeap = NULL;
            newMemoryHeap = new MemoryHeapBase(128*sizeof(unsigned));
            mBufferGet = new MemoryBase(newMemoryHeap, 0, 3*sizeof(unsigned));
            newMemoryHeap = new MemoryHeapBase(128*sizeof(unsigned));
            mBufferRelease = new MemoryBase(newMemoryHeap, 0, 3*sizeof(unsigned));
        }
    } else {
        int isADTS = 0;
        if (params) {
            params->findInt32(kKeyChannelCount, &mAudioChannelCount);
            params->findInt32(kKeySampleRate, &mAudioSampleRate);
            params->findInt32(kKeyIsADTS, &isADTS);
            mIsPCMAudio = !isADTS;
        }

#ifdef ESCDUMPAUDIOAAC
		mEscDumpAAC = open("/data/temp/EscDumpAAC.aac", O_CREAT | O_RDWR, 0666);
#endif

#ifdef ESCDUMPAUDIOPCM
		mEscDumpPcm = open("/data/temp/EscDumpPcm.pcm", O_CREAT | O_RDWR, 0666);
#endif

        ALOGE("[%s %d] ESConvertor get audio info mAudioChannelCount:%d mAudioSampleRate:%d mIsPCMAudio:%d isADTS:%d\n", __FUNCTION__, __LINE__,
            mAudioChannelCount, mAudioSampleRate, mIsPCMAudio, isADTS);

        //AudioSystem::setDeviceConnectionState(AUDIO_DEVICE_IN_REMOTE_SUBMIX, AUDIO_POLICY_DEVICE_STATE_AVAILABLE, 0);
        //AudioSystem::setDeviceConnectionState(AUDIO_DEVICE_OUT_REMOTE_SUBMIX, AUDIO_POLICY_DEVICE_STATE_AVAILABLE, 0);

        //create audio source
        //mAudioSource = new AudioSource(AUDIO_SOURCE_REMOTE_SUBMIX,mAudioSampleRate /* sampleRate */,mAudioChannelCount /* channelCount */);

        if (mAudioSource == NULL) {
            ALOGE("[%s %d] mAudioSource calloc fail:%x\n", __FUNCTION__, __LINE__, &mAudioSource);
            return !OK;
        }

        status_t err = mAudioSource->initCheck();
        if (err != OK) {
            ALOGE("[%s %d] mAudioSource initCheck fail err:%d\n", __FUNCTION__, __LINE__, err);
            return !OK;
        }

        sp<MetaData> params_audio_source = new MetaData;
        params_audio_source->setInt64(kKeyTime, 1ll);

        err = mAudioSource->start(params_audio_source.get());
        if (err != OK) {
            ALOGE("[%s %d] mAudioSource start fail err:%d\n", __FUNCTION__, __LINE__, err);
            return !OK;
        }
        ALOGE("[%s %d] mAudioSource start err:%d\n", __FUNCTION__, __LINE__, err);
    }

    if (!(mIsAudio == 1 && mIsPCMAudio == 1))
        initEncoder();

    if (mIsAudio == VIDEO_ENCODE)
        mScreenManager->start(client_id);

    mStarted = true;

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&mThread, &attr, ThreadWrapper, this);
    pthread_attr_destroy(&attr);

    return OK;
}

status_t ESConvertor::setMaxAcquiredBufferCount(size_t count) {
    ALOGE("setMaxAcquiredBufferCount(%d)", count);
    Mutex::Autolock lock(mMutex);

    CHECK_GT(count, 1);
    mMaxAcquiredBufferCount = count;

    return OK;
}

status_t ESConvertor::setUseAbsoluteTimestamps() {
    ALOGE("setUseAbsoluteTimestamps");
    Mutex::Autolock lock(mMutex);
    mUseAbsoluteTimestamps = true;

    return OK;
}

status_t ESConvertor::stop()
{
    ALOGE("[%s %d] mIsAudio:%d mStarted:%d", __FUNCTION__, __LINE__, mIsAudio, mStarted);
    Mutex::Autolock lock(mMutex);

    if (!mStarted) {
        return OK;
    }
    mStarted = false;
    if (mIsSoftwareEncoder) {
        if (mIsAudio == VIDEO_ENCODE) {
            mThreadOutCondition.waitRelative(mLock, 1000000000000);
            sp<ABuffer> accessUnit = NULL;
            while (!mInputBufferQueue.empty()) {
                accessUnit = *mInputBufferQueue.begin();
                mInputBufferQueue.erase(mInputBufferQueue.begin());
                accessUnit.clear();
            }
            mScreenManager->stop(mClientId);
        } else {
            mAudioSource->stop();
            mAudioSource.clear();
        }
    } else {
        if (mIsAudio == VIDEO_ENCODE) {
            mThreadOutCondition.waitRelative(mLock, 1000000000000);
            MediaBuffer* tBuffer = NULL;
            while (!mFramesReceived.empty()) {
                tBuffer = *mFramesReceived.begin();
                mFramesReceived.erase(mFramesReceived.begin());
                memcpy(mBufferRelease->pointer(), tBuffer->data(), 3*sizeof(unsigned));
                mScreenManager->freeBuffer(mClientId, mBufferRelease);
                tBuffer->release();
            }

            sp<ABuffer> accessUnit = NULL;
            while (!mInputBufferQueue.empty()) {
                accessUnit = *mInputBufferQueue.begin();
                mInputBufferQueue.erase(mInputBufferQueue.begin());
                memcpy(mBufferRelease->pointer(), accessUnit->data(), 3*sizeof(unsigned));
                mScreenManager->freeBuffer(mClientId, mBufferRelease);
                accessUnit.clear();
            }
            mScreenManager->stop(mClientId);
        } else {
            mAudioSource->stop();
            mAudioSource.clear();
            //AudioSystem::setDeviceConnectionState(AUDIO_DEVICE_IN_REMOTE_SUBMIX, AUDIO_POLICY_DEVICE_STATE_UNAVAILABLE, 0);
            //AudioSystem::setDeviceConnectionState(AUDIO_DEVICE_OUT_REMOTE_SUBMIX, AUDIO_POLICY_DEVICE_STATE_UNAVAILABLE, 0);
        }

    }
    if (mEncoder != NULL) {
        mEncoder->stop();
        mEncoder->release();
        mEncoder.clear();

        mEncoderInputBuffers.clear();
        mEncoderOutputBuffers.clear();
    }

#ifdef ESCDUMPAUDIOAAC
    close(mEscDumpAAC);
#endif

#ifdef ESCDUMPAUDIOPCM
    close(mEscDumpPcm);
#endif

    if (mDumpEsFd >= 0) {
        close(mDumpEsFd);
        mDumpEsFd = -1;
        ALOGD("Dump es File finish!");
    }
    if (mDumpYuvFd >= 0) {
        close(mDumpYuvFd);
        mDumpYuvFd = -1;
        ALOGD("Dump yuv File finish!");
    }

    if (mIsAudio == VIDEO_ENCODE)
        mScreenManager->uninit(mClientId);

    return OK;

}

sp<MetaData> ESConvertor::getFormat()
{
    ALOGE("getFormat");

    Mutex::Autolock lock(mMutex);
    sp<MetaData> meta = new MetaData;

    meta->setInt32(kKeyWidth, mWidth);
    meta->setInt32(kKeyHeight, mHeight);
    meta->setInt32(kKeyColorFormat, OMX_COLOR_FormatYUV420Planar);
    meta->setInt32(kKeyStride, mWidth);
    meta->setInt32(kKeySliceHeight, mHeight);

    meta->setInt32(kKeyFrameRate, mFrameRate);
    meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_VIDEO_RAW);
    return meta;
}

status_t ESConvertor::read( MediaBufferBase **buffer,
                                    const ReadOptions *options)
{
    Mutex::Autolock lock(mMutex);

    int64_t timeUs;
    *buffer = NULL;

    if (mOutputBufferQueue.empty()) {
        return !OK;
    }

    sp<ABuffer> aBuffer = *mOutputBufferQueue.begin();
    mOutputBufferQueue.erase(mOutputBufferQueue.begin());

    aBuffer->meta()->findInt64("timeUs", &timeUs);

    MediaBuffer *tBuffer = new MediaBuffer(aBuffer->size() + 16);

    memcpy(tBuffer->data(), aBuffer->data(), aBuffer->size());

    tBuffer->set_range(0,aBuffer->size());

    tBuffer->meta_data().setInt32(kKeyBufferID, 0);
    *buffer = tBuffer;

    (*buffer)->setObserver(this);
    (*buffer)->add_ref();
    (*buffer)->meta_data().setInt64(kKeyTime, timeUs);

    return OK;
}

void ESConvertor::signalBufferReturned(MediaBufferBase *buffer) {
    int32_t bufferID = 0;
    buffer->meta_data().findInt32(kKeyBufferID, &bufferID);
    if (!mIsSoftwareEncoder) {
        if (bufferID == 0xf && mIsAudio == VIDEO_ENCODE) {
            unsigned buff_info[3] = {0,0,0};
            memcpy(buff_info, buffer->data(), 3*sizeof(unsigned));
            memcpy(mBufferRelease->pointer(), buffer->data(), 3*sizeof(unsigned));

            //ALOGE("[%s %d] buff_info[0]:%x buff_info[1]:%x buff_info[2]:%x mClientId:%d pointer:%x",
            //__FUNCTION__, __LINE__, buff_info[0], buff_info[1], buff_info[2], mClientId, mBufferRelease->pointer());
            mScreenManager->freeBuffer(mClientId, mBufferRelease);
        }
    }
    buffer->setObserver(0);
    buffer->release();
}

} // end of namespace android
