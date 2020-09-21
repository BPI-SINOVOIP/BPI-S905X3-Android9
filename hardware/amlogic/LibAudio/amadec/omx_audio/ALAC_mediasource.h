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

#ifndef MEDIA_ALACMEDIASOURCE_H_
#define MEDIA_ALACMEDIASOURCE_H_

#include  "MediaSource.h"
#include  "DataSource.h"
#include  "MediaBufferGroup.h"
#include  "MetaData.h"
#include  "audio_mediasource.h"
#include  "../audio-dec.h"
#include  "MediaBufferBase.h"

namespace android
{

typedef int (*fp_read_buffer)(unsigned char *, int);

class ALAC_MediaSource : public AudioMediaSource
{
public:
    ALAC_MediaSource(void *read_buffer, aml_audio_dec_t *audec);

    status_t start(MetaData *params = NULL);
    status_t stop();
    sp<MetaData> getFormat();
    status_t read(MediaBufferBase **buffer, const ReadOptions *options = NULL);

    int  GetReadedBytes();
    int GetSampleRate();
    int GetChNum();
    int* Get_pStop_ReadBuf_Flag();
    int Set_pStop_ReadBuf_Flag(int *pStop);

    int set_ALAC_MetaData(aml_audio_dec_t *audec);
    int MediaSourceRead_buffer(unsigned char *buffer, int size);

    fp_read_buffer fpread_buffer;

    int sample_rate;
    int ChNum;
    int frame_size;
    int *pStop_ReadBuf_Flag;
    int extradata_size;
    int64_t bytes_readed_sum_pre;
    int64_t bytes_readed_sum;
protected:
    virtual ~ALAC_MediaSource();

private:
    bool mStarted;
    sp<DataSource> mDataSource;
    sp<MetaData>   mMeta;
    MediaBufferGroup *mGroup;
    int64_t mCurrentTimeUs;
    int     mBytesReaded;

    ALAC_MediaSource(const ALAC_MediaSource &);
    ALAC_MediaSource &operator=(const ALAC_MediaSource &);
};


}

#endif
