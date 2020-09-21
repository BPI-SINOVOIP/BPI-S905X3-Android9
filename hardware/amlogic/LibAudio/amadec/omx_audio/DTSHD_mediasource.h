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

#ifndef MEDIA_DTSHD_MEDIASOURCE_H_
#define MEDIA_DTSHD_MEDIASOURCE_H_

#include  "MediaSource.h"
#include  "DataSource.h"
#include  "MediaBufferGroup.h"
#include  "MetaData.h"
#include  "audio_mediasource.h"
#include  "MediaBufferBase.h"

namespace android
{
//for dtsexpress stream,sometimes vbuf is full and abuf has not 40k and cause block about 5s.
#define AML_DCA_INPUT_DATA_LEN_PTIME               (10*1024)
#define AML_DCA_OMX_DECODER_NUMBUF    2
#define AML_DCA_SW_CORE_16M          0x7ffe8001
#define AML_DCA_SW_CORE_14M          0x1fffe800
#define AML_DCA_SW_CORE_24M          0xfe80007f
#define AML_DCA_SW_CORE_16             0xfe7f0180
#define AML_DCA_SW_CORE_14             0xff1f00e8
#define AML_DCA_SW_CORE_24             0x80fe7f01
#define AML_DCA_SW_SUBSTREAM_M    0x64582025
#define AML_DCA_SW_SUBSTREAM         0x58642520

typedef int (*fp_read_buffer)(unsigned char *, int);
class Dtshd_MediaSource : public AudioMediaSource
{
public:
    Dtshd_MediaSource(void *read_buffer);

    status_t start(MetaData *params = NULL);
    status_t stop();
    sp<MetaData> getFormat();
    status_t read(MediaBufferBase **buffer, const ReadOptions *options = NULL);

    int  GetReadedBytes();
    int GetSampleRate();
    virtual int SetSampleRate(int samplerate);
    int GetChNum();
    int* Get_pStop_ReadBuf_Flag();
    int Set_pStop_ReadBuf_Flag(int *pStop);
    int MediaSourceRead_buffer(unsigned char *buffer, int size);

    fp_read_buffer fpread_buffer;
    int sample_rate;
    int ChNum;
    int frame_size;
    int FrameSizeDetectFlag;
    unsigned char * FirFraBuf;
    int FirFraBuf_Len;
    int FirFraBuf_Offset;
    int FrameNumReaded;
    int *pStop_ReadBuf_Flag;
    int64_t bytes_readed_sum_pre;
    int64_t bytes_readed_sum;
protected:
    virtual ~Dtshd_MediaSource();

private:
    bool mStarted;
    sp<DataSource> mDataSource;
    sp<MetaData>   mMeta;
    MediaBufferGroup *mGroup;
    int64_t mCurrentTimeUs;
    int     mBytesReaded;
    int     block_align;
    Dtshd_MediaSource(const Dtshd_MediaSource &);
    Dtshd_MediaSource &operator=(const Dtshd_MediaSource &);
};


}

#endif

