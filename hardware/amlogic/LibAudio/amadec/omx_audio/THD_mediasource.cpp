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
#include "THD_mediasource.h"

extern "C" int read_buffer(unsigned char *buffer,int size);

//#define LOG_TAG "THD_Medissource"
//#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
//#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

namespace android {

THD_MediaSource::THD_MediaSource(void *read_buffer)	
{   
    ALOGI("%s %d \n",__FUNCTION__,__LINE__);
    mStarted=false;
    mMeta=new MetaData;
    mDataSource=NULL;
    mGroup=NULL;
    mBytesReaded=0;
    mCurrentTimeUs=0;
    pStop_ReadBuf_Flag=NULL;
    fpread_buffer=(fp_read_buffer)read_buffer;
    sample_rate=48000;//96000;
    ChNum=2;
    frame_size=0;
    bytes_readed_sum_pre=0;
    bytes_readed_sum=0;
    mSyncMain = false;
    mMeta->setInt32(kKeyChannelCount, 2);
    mMeta->setInt32(kKeySampleRate, 48000);
}

THD_MediaSource::~THD_MediaSource() 
{
    ALOGI("%s %d \n",__FUNCTION__,__LINE__);
    if (mStarted) {
        stop();
    }
}

int THD_MediaSource::GetSampleRate()
{	
    return sample_rate;
}

int THD_MediaSource::GetChNum()
{
    return ChNum;
}

int* THD_MediaSource::Get_pStop_ReadBuf_Flag()
{
    return pStop_ReadBuf_Flag;
}

int THD_MediaSource::Set_pStop_ReadBuf_Flag(int *pStop)
{
    pStop_ReadBuf_Flag = pStop;

    return 0;
}

int THD_MediaSource::SetReadedBytes(int size)
{
    mBytesReaded=size;
    return 0;
}

int THD_MediaSource::GetReadedBytes()
{    
    int bytes_used;
    bytes_used=bytes_readed_sum-bytes_readed_sum_pre;
    if(bytes_used<0)
    {
        ALOGI("[%s]bytes_readed_sum(%ld) < bytes_readed_sum_pre(%ld) \n",__FUNCTION__, (long)bytes_readed_sum, (long)bytes_readed_sum_pre);
        bytes_used=0;
    }
    bytes_readed_sum_pre=bytes_readed_sum;
    return bytes_used;
}

sp<MetaData> THD_MediaSource::getFormat() {
    ALOGI("%s %d \n",__FUNCTION__,__LINE__);
    return mMeta;
}

status_t THD_MediaSource::start(MetaData *params __unused)
{
    ALOGI("%s %d \n",__FUNCTION__,__LINE__);
    mGroup = new MediaBufferGroup;
    mGroup->add_buffer(MediaBufferBase::Create(4096));
    mStarted = true;
    return OK;	
}

status_t THD_MediaSource::stop()
{
    ALOGI("%s %d \n",__FUNCTION__,__LINE__);
    delete mGroup;
    mGroup = NULL;
    mStarted = false;
    return OK;
}

int THD_MediaSource::MediaSourceRead_buffer(unsigned char *buffer,int size)
{
    int readcnt=0;
    int readsum=0;
    if(fpread_buffer!=NULL)
    {
        int sleep_time=0;
        while((readsum<size)&& (*pStop_ReadBuf_Flag==0))
        {
            readcnt=fpread_buffer(buffer+readsum,size-readsum);
            if(readcnt<(size-readsum)){
                sleep_time++;
                usleep(10000);
            }
            readsum+=readcnt;
            if((sleep_time>0) && (sleep_time%100==0)){ //wait for max 10s to get audio data
                ALOGE("[%s] Can't get data from audiobuffer,wait for %d ms\n ", __FUNCTION__,sleep_time*10);
            }
        }
        bytes_readed_sum +=readsum;
        if(*pStop_ReadBuf_Flag==1)
        {
            ALOGI("[%s] End of Stream: *pStop_ReadBuf_Flag==1\n ", __FUNCTION__);
        }
        return readsum;
    }else{
        ALOGE("[%s]ERR: fpread_buffer=NULL\n ", __FUNCTION__);
        return 0;
    }
}

status_t THD_MediaSource::read(MediaBufferBase **out, const ReadOptions *options __unused)
{
    *out = NULL;
    uint     fourheader = 0;  //au size and crc check and timing
    uint     syncheader = 0;  //truehd sync words(word:four bytes)
    uchar    byte; 
    uchar    byte2[2];
    uint     ausize;          //access unit size(byte)
    //uint     i = 0;
	
    if(!mSyncMain){
        while(1)//max frame length is 4000 bytes
        {   
            ALOGI("=======find sync main=====\n");
            if(MediaSourceRead_buffer((uchar*)&byte, 1) < 1){
                ALOGI("[%s,%d]read raw data from buffer failed\n",__FUNCTION__,__LINE__);
                return ERROR_END_OF_STREAM;
            }
            fourheader = (fourheader << 8) | (syncheader >> 24);
            syncheader = (syncheader << 8) | byte;
            if(syncheader == TRUEHDSYNC){
                ausize = ((fourheader >> 16) & 0x0fff) << 1;
                ChNum = 2;
                sample_rate = 48000;//96000;
                mSyncMain = true;
                ALOGI("truehd: main sync word find ok\n");
                break;
            }
        }
#if 0
        if(i >= MAX_AUSIZE)
        {
            ALOGI("ERROR: can not find truehd main sync\n");
            return ERROR_UNSUPPORTED;
        }
#endif
    }else{
        if(MediaSourceRead_buffer((uchar*)&byte2, 2) < 2){
            ALOGI("[%s,%d]read raw data from buffer failed\n",__FUNCTION__,__LINE__);
            return ERROR_END_OF_STREAM;
        }
        ausize = (((byte2[0]<< 8) | byte2[1]) & 0x0fff) << 1; 
    }

    if((ausize < MIN_AUSIZE) || (ausize > MAX_AUSIZE)){
        ALOGI("ERROR: access unit size can not accept!!!\n");
        return ERROR_UNSUPPORTED;
    }
	
   
    MediaBufferBase *buffer;
    status_t err = mGroup->acquire_buffer(&buffer);
    if (err != OK) {
        ALOGI("acquire_buffer buffer failed.\n");
        return err;
    }

    if(syncheader == TRUEHDSYNC){
        memcpy((uchar *)(buffer->data()), &fourheader, 4);
        memcpy((uchar *)((unsigned long)buffer->data() + 4), &syncheader, 4);
        if (MediaSourceRead_buffer((uchar*)((unsigned long)buffer->data() + 8), ausize - 8) != (int)(ausize - 8)) {
            ALOGI("[%s %d]stream read failed\n",__FUNCTION__,__LINE__); 
            buffer->release();
            buffer = NULL;
            return ERROR_END_OF_STREAM;
        }
        char *ptr = (char *)((unsigned long)buffer->data() + 8);
        sample_rate = ((ptr[0] >> 4) & 0xf) & 8 ? 44100 : 48000;
        ALOGI("[%s:%d] sample_rate = %d\n", __FUNCTION__, __LINE__, sample_rate);
    }else{
        memcpy((uchar *)(buffer->data()), &byte2, 2);	
        if (MediaSourceRead_buffer((uchar*)((unsigned long)buffer->data() + 2), ausize - 2) != (int)(ausize - 2)) {
            ALOGI("[%s %d]stream read failed\n",__FUNCTION__,__LINE__); 
            buffer->release();
            buffer = NULL;
            return ERROR_END_OF_STREAM;
        }
    }
	
	
    buffer->set_range(0, ausize);
    buffer->meta_data().setInt64(kKeyTime, mCurrentTimeUs);
    buffer->meta_data().setInt32(kKeyIsSyncFrame, 1);

    *out = buffer;
    return OK;
}

}  // namespace android

