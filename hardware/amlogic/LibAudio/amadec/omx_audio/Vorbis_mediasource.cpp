/*
 * Copyright (C) 2018 Amlogic Corporation.
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
 *
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <android/log.h>
#include <cutils/properties.h>
#include "Vorbis_mediasource.h"
#include "audio-dec.h"


//#define LOG_TAG "Vorbis_Medissource"
//#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
//#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
extern "C" int read_buffer(unsigned char *buffer, int size);
namespace android
{

Vorbis_MediaSource::Vorbis_MediaSource(void *read_buffer, aml_audio_dec_t *audec)
{
    ALOGI("%s %d \n", __FUNCTION__, __LINE__);

    mStarted = false;
    mMeta = new MetaData;
    mDataSource = NULL;
    mGroup = NULL;
    mBytesReaded = 0;
    mCurrentTimeUs = 0;
    pStop_ReadBuf_Flag = NULL;
    fpread_buffer = (fp_read_buffer)read_buffer;
    sample_rate = audec->samplerate;
    ChNum      = audec->channels > 2 ? 2 : audec->channels;
    bytes_readed_sum_pre = 0;
    bytes_readed_sum = 0;
    FrameNumReaded = 0;
    packt_size = 0;
    mMeta->setInt32(kKeyChannelCount, audec->channels > 0 ? audec->channels : 2);
    mMeta->setInt32(kKeySampleRate, audec->samplerate > 0 ? audec->samplerate : 48000);

}


Vorbis_MediaSource::~Vorbis_MediaSource()
{
    ALOGI("%s %d \n", __FUNCTION__, __LINE__);
    if (mStarted) {
        stop();
    }
}


int Vorbis_MediaSource::GetSampleRate()
{
    return sample_rate;
}

int Vorbis_MediaSource::GetChNum()
{
    return ChNum;
}

int* Vorbis_MediaSource::Get_pStop_ReadBuf_Flag()
{
    return pStop_ReadBuf_Flag;
}

int Vorbis_MediaSource::Set_pStop_ReadBuf_Flag(int *pStop)
{
    pStop_ReadBuf_Flag = pStop;
    return 0;
}

int Vorbis_MediaSource::GetReadedBytes()
{
    if (1/*FrameNumReaded<2*/) {
        int bytes_used;
        bytes_used = bytes_readed_sum - bytes_readed_sum_pre;
        if (bytes_used < 0) {
            ALOGI("[%s]bytes_readed_sum(%ld) < bytes_readed_sum_pre(%ld) \n", __FUNCTION__, (long)bytes_readed_sum, (long)bytes_readed_sum_pre);
            bytes_used = 0;
        }
        bytes_readed_sum_pre = bytes_readed_sum;
        return bytes_used;
    } else {
        return packt_size;
    }
}

sp<MetaData> Vorbis_MediaSource::getFormat()
{
    ALOGI("%s %d \n", __FUNCTION__, __LINE__);
    return mMeta;
}

status_t  Vorbis_MediaSource::start(MetaData *params __unused)
{
    ALOGI("%s %d \n", __FUNCTION__, __LINE__);
    mGroup = new MediaBufferGroup;
    mGroup->add_buffer(MediaBufferBase::Create(8192));
    mStarted = true;
    return OK;
}

status_t Vorbis_MediaSource::stop()
{
    ALOGI("%s %d \n", __FUNCTION__, __LINE__);
    delete mGroup;
    mGroup = NULL;
    mStarted = false;
    return OK;
}


int Vorbis_MediaSource::MediaSourceRead_buffer(unsigned char *buffer, int size)
{
    int readcnt = 0;
    int readsum = 0;
    if (fpread_buffer != NULL) {
        int sleep_time = 0;
        while ((readsum < size) && (*pStop_ReadBuf_Flag == 0)) {
            readcnt = fpread_buffer(buffer + readsum, size - readsum);
            if (readcnt < (size - readsum)) {
                sleep_time++;
                usleep(10000);
            }
            readsum += readcnt;
            if (sleep_time > 200) { //wait for max 2s to get audio data
                ALOGE("[%s] Can't get data from audiobuffer,wait for %d ms\n ", __FUNCTION__, sleep_time * 10);
                return 0;
            }
        }

        if (*pStop_ReadBuf_Flag == 1) {
            ALOGI("[%s] End of Stream: *pStop_ReadBuf_Flag==1\n ", __FUNCTION__);
        }
        return readsum;
    } else {
        ALOGE("[%s]ERR: fpread_buffer=NULL\n ", __FUNCTION__);
        return 0;
    }
}

/*
static void dump_pcm_bin(char *path, char *buf, int size)
{
    FILE *fp = fopen(path, "ab+");
    if (fp != NULL) {
        fwrite(buf, 1, size, fp);
        fclose(fp);
    }
}
*/

status_t Vorbis_MediaSource::read(MediaBufferBase **out, const ReadOptions *options __unused)
{

    *out = NULL;
    //int read_bytes_per_time;
    MediaBufferBase *buffer;
    //int byte_readed = 0;
    status_t err;

    int SyncFlag = 0;// readedbytes = 0;
    int pkt_size = 0;
    uint8_t ptr_head[4] = {0};
    int buf_getted = 0;
    //for android N, need player to insert the numPageSamples at the end of input buffer
    //android M and before,which is done by OMXCodec itself.
    int32_t numPageSamples  = -1;
    //android N and later
#if  ANDROID_PLATFORM_SDK_VERSION >= 24
    int32_t insert_byte = 4;
#else
    int32_t insert_byte = 0;
#endif
re_read:

    //----------------------------------
    if (MediaSourceRead_buffer(&ptr_head[0], 4) < 4) {
        ALOGI("WARNING: fpread_buffer readbytes failed [%s %d]!\n", __FUNCTION__, __LINE__);
        return ERROR_END_OF_STREAM;
    }

    while (!SyncFlag) {
        if (ptr_head[0] == 'H' && ptr_head[1] == 'E' && ptr_head[2] == 'A' && ptr_head[3] == 'D') {
            SyncFlag = 1;
            break;
        }
        ptr_head[0] = ptr_head[1];
        ptr_head[1] = ptr_head[2];
        ptr_head[2] = ptr_head[3];
        if (MediaSourceRead_buffer(&ptr_head[3], 1) < 1) {
            ALOGI("WARNING: fpread_buffer readbytes failed [%s %d]!\n", __FUNCTION__, __LINE__);
            return ERROR_END_OF_STREAM;
        }

        if (*pStop_ReadBuf_Flag == 1) {
            ALOGI("Stop_ReadBuf_Flag==1 stop read_buf [%s %d]", __FUNCTION__, __LINE__);
            return ERROR_END_OF_STREAM;
        }
    }

    if (MediaSourceRead_buffer((unsigned char*)(&pkt_size), 4) < 4) {
        ALOGI("WARNING: fpread_buffer readbytes failed [%s %d]!\n", __FUNCTION__, __LINE__);
        return ERROR_END_OF_STREAM;
    }

    if (!buf_getted) {
        buf_getted = 1;
        err = mGroup->acquire_buffer(&buffer);
        if (err != OK) {
            ALOGE("[%s %d] mGroup->acquire_buffer ERR!\n", __FUNCTION__, __LINE__);
            return err;
        }
    }
    //mediasource should wait abuf data util player finish it
    while (MediaSourceRead_buffer((unsigned char*)(buffer->data()), pkt_size) < pkt_size && (*pStop_ReadBuf_Flag == 0)) {
        ALOGW("WARNING: fpread_buffer readbytes failed [%s %d]!\n", __FUNCTION__, __LINE__);
    }
    if (*pStop_ReadBuf_Flag == 1) {
        buffer->release();
        buffer = NULL;
        return ERROR_END_OF_STREAM;
    }

    if (insert_byte > 0) {
        buffer->set_range(0, pkt_size + insert_byte);
        memcpy((unsigned char*)(buffer->data()) + pkt_size, &numPageSamples, sizeof(numPageSamples));
    }
    buffer->meta_data().setInt64(kKeyTime, mCurrentTimeUs);
    buffer->meta_data().setInt32(kKeyIsSyncFrame, 1);
    bytes_readed_sum += (pkt_size + 8);
    ALOGV("vorbis package size %d,num %d\n", pkt_size, FrameNumReaded);
    FrameNumReaded++;
    if (FrameNumReaded == 2) {
        goto re_read;
    }
    if (FrameNumReaded > 3) {
        packt_size = pkt_size;
        buffer->meta_data().setInt32(kKeyValidSamples, -1);
    }
    *out = buffer;
    return OK;
}


}  // namespace android


