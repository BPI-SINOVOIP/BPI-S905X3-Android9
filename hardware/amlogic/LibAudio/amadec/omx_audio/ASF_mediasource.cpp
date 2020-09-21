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
#include "ASF_mediasource.h"
#include "audio-dec.h"
#include "AmMetaDataExt.h"

extern "C" int read_buffer(unsigned char *buffer,int size);

//#define LOG_TAG "Asf_Medissource"
//#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
//#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)


namespace android {

int Asf_MediaSource::set_Asf_MetaData(aml_audio_dec_t *audec)
{   
    Asf_audio_info_t *paudio_info=(Asf_audio_info_t*)audec->extradata;
    
    sample_rate=audec->samplerate=paudio_info->sample_rate;
    ChNum      =audec->channels  =paudio_info->channels>2? 2: paudio_info->channels;
    block_align=paudio_info->block_align;
    mMeta->setInt32(kKeySampleRate,paudio_info->sample_rate );
    mMeta->setInt32(kKeyChannelCount, paudio_info->channels);
    mMeta->setInt32(kKeyBitRate,paudio_info->bitrate);
/*	
    if(paudio_info->codec_id==CODEC_ID_WMAV1_FFMPEG)
        paudio_info->codec_id=CODEC_ID_WMAV1_OMX;
    else if(paudio_info->codec_id==CODEC_ID_WMAV2_FFMPEG)
        paudio_info->codec_id=CODEC_ID_WMAV2_OMX;
*/	
    mMeta->setInt32(kKeyCodecID,paudio_info->codec_id);
    mMeta->setData(kKeyExtraData,0,paudio_info->extradata,paudio_info->extradata_size);
    mMeta->setInt32(kKeyExtraDataSize, paudio_info->extradata_size);
    mMeta->setInt32(kKeyBlockAlign,paudio_info->block_align);
    
    ALOGI("ASF-->channels/%d samplerate/%d bitrate/%d  codec_ID/0x%x extradata_size/%d block_align/%d\n",ChNum,sample_rate,
    paudio_info->bitrate,paudio_info->codec_id,paudio_info->extradata_size,block_align);

    if(ChNum==1 && paudio_info->codec_id==CODEC_ID_WMAV2){
        ChNum=2;
        ALOGI("ASF-->here ChNum:change mono to stero!\n");
    }
    return 0;
}
int Asf_MediaSource::MediaSourceRead_buffer(unsigned char *buffer,int size)
{
   int readcnt=0;
   int readsum=0;
   if(fpread_buffer!=NULL)
   {   int sleep_time=0;
       while((readsum<size) && (*pStop_ReadBuf_Flag==0))
       {
          readcnt=fpread_buffer(buffer+readsum,size-readsum);
          if(readcnt<(size-readsum)){
             sleep_time++;
             usleep(10000);
          }
          readsum+=readcnt; 
       }
       bytes_readed_sum +=readsum;
       if(*pStop_ReadBuf_Flag==1)
       {
            ALOGI("[%s] End of Stream: *pStop_ReadBuf_Flag==1\n ", __FUNCTION__);
       }
       return readsum;
   }else{
        ALOGI("[%s]ERR: fpread_buffer=NULL\n ", __FUNCTION__);
        return 0;
   }
}

//-------------------------------------------------------------------------
Asf_MediaSource::Asf_MediaSource(void *read_buffer, aml_audio_dec_t *audec) 
{   
    ALOGI("[%s] in line (%d) \n",__FUNCTION__,__LINE__);
    mStarted=false;
    mMeta=new MetaData;
    mDataSource=NULL;
    mGroup=NULL;
    mCurrentTimeUs=0;
    pStop_ReadBuf_Flag=NULL;
    fpread_buffer=(fp_read_buffer)read_buffer;
    sample_rate=0;
    ChNum=0;
    frame_size=0;
    block_align=0;
    bytes_readed_sum_pre=0;
    bytes_readed_sum=0;
    set_Asf_MetaData(audec);
    
}
Asf_MediaSource::~Asf_MediaSource() 
{
    ALOGI("[%s] in line (%d) \n",__FUNCTION__,__LINE__);
    if (mStarted)
        stop();
}
int Asf_MediaSource::GetSampleRate()
{   
    return sample_rate;
}

int Asf_MediaSource::GetChNum()
{
    return ChNum;
}

int* Asf_MediaSource::Get_pStop_ReadBuf_Flag()
{
    return pStop_ReadBuf_Flag;
}

int Asf_MediaSource::Set_pStop_ReadBuf_Flag(int *pStop)
{
    pStop_ReadBuf_Flag = pStop;
    return 0;
}

int Asf_MediaSource::GetReadedBytes()
{    
     return block_align;
}

sp<MetaData> Asf_MediaSource::getFormat() {
    ALOGI("[%s] in line (%d) \n",__FUNCTION__,__LINE__);
    return mMeta;
}

status_t Asf_MediaSource::start(MetaData *params __unused)
{
    ALOGI("[%s] in line (%d) \n",__FUNCTION__,__LINE__);
    mGroup = new MediaBufferGroup;
    mGroup->add_buffer(MediaBufferBase::Create(block_align));
    mStarted = true;
    return OK;  
}

status_t Asf_MediaSource::stop()
{
    ALOGI("[%s] in line (%d) \n",__FUNCTION__,__LINE__);
    delete mGroup;
    mGroup = NULL;
    mStarted = false;
    return OK;
}

status_t Asf_MediaSource::read(MediaBufferBase **out, const ReadOptions *options __unused)
{
    *out = NULL;
    //int readedbytes = 0;
    //unsigned char header_buffer[5];
    frame_size = 0;

    if (*pStop_ReadBuf_Flag==1){
         ALOGI("Stop_ReadBuf_Flag==1 stop read_buf [%s %d]",__FUNCTION__,__LINE__);
         return ERROR_END_OF_STREAM;
    }

    MediaBufferBase *buffer;
    status_t err = mGroup->acquire_buffer(&buffer);
    if (err != OK) {
        return err;
    }
    
    if (MediaSourceRead_buffer((unsigned char*)(buffer->data()),block_align) != block_align)
    {
        buffer->release();
        buffer = NULL;
        return ERROR_END_OF_STREAM;
    }
    
    buffer->set_range(0, block_align);
    buffer->meta_data().setInt64(kKeyTime, mCurrentTimeUs);
    
    *out = buffer;
     return OK;
}




}  // namespace android
