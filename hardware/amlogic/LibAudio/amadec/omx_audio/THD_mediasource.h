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

#ifndef MEDIA_THDMEDIASOURCE_H_
#define MEDIA_THDMEDIASOURCE_H_

#include  "MediaSource.h"
#include  "DataSource.h"
#include  "MediaBufferGroup.h"
#include  "MetaData.h"
#include  "audio_mediasource.h"
#include  "MediaBufferBase.h"

#define TRUEHDSYNC 0xf8726fba   //main sync header
#define MAX_AUSIZE 4000         //max access unit size
#define MIN_AUSIZE 6            //min access unit size

typedef  unsigned char  uchar;
typedef  unsigned short ushort;
typedef  unsigned int   uint;



namespace android
{

typedef int (*fp_read_buffer)(unsigned char *, int);

class THD_MediaSource : public AudioMediaSource
{
public:
    THD_MediaSource(void *read_buffer);

    status_t start(MetaData *params = NULL);
    status_t stop();
    sp<MetaData> getFormat();
    status_t read(MediaBufferBase **buffer, const ReadOptions *options = NULL);

    int GetReadedBytes();
    int GetSampleRate();
    int GetChNum();
    int* Get_pStop_ReadBuf_Flag();
    int Set_pStop_ReadBuf_Flag(int *pStop);

    int SetReadedBytes(int size);
    int MediaSourceRead_buffer(unsigned char *buffer, int size);

    fp_read_buffer fpread_buffer;

    int StreamAnalysis(char *buffer);

    int sample_rate;
    int ChNum;
    int frame_size;
    int64_t bytes_readed_sum_pre;
    int64_t bytes_readed_sum;
    int* pStop_ReadBuf_Flag;

protected:
    virtual ~THD_MediaSource();

private:
    bool mStarted;
    sp<DataSource> mDataSource;
    sp<MetaData>   mMeta;
    MediaBufferGroup *mGroup;
    int64_t mCurrentTimeUs;
    int     mBytesReaded;
    bool    mSyncMain;

    THD_MediaSource(const THD_MediaSource &);
    THD_MediaSource &operator=(const THD_MediaSource &);
};


}

#endif
