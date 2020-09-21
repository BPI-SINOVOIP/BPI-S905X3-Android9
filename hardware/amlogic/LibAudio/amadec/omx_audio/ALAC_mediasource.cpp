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
#include "ALAC_mediasource.h"
#include "AmMetaDataExt.h"

extern "C" int read_buffer(unsigned char *buffer,int size);

//#define LOG_TAG "ALAC_Medissource"
//#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
//#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

#define MaxFrameSize 16384

namespace android {

int ALAC_MediaSource::set_ALAC_MetaData(aml_audio_dec_t *audec)
{
	mMeta->setInt32(kKeyChannelCount, audec->channels);
	ChNum = audec->channels;
	mMeta->setInt32(kKeySampleRate, audec->samplerate);
	sample_rate = audec->samplerate;
	mMeta->setInt32(kKeyExtraDataSize, audec->extradata_size);
	extradata_size = audec->extradata_size;
	mMeta->setData(kKeyExtraData,0,audec->extradata,audec->extradata_size);

	ALOGI("channel_number = [%d], samplerate = [%d], extradata_size = [%d]\n",ChNum,sample_rate,extradata_size);
	
	return 0;
}
int ALAC_MediaSource::MediaSourceRead_buffer(unsigned char *buffer,int size)
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
		  if(sleep_time > 200){ //wait for max 2s to get audio data
		  	ALOGE("[%s] Can't get data from audiobuffer,wait for %d ms\n ", __FUNCTION__,sleep_time*10);
		  }
   	   }
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

//-------------------------------------------------------------------------
ALAC_MediaSource::ALAC_MediaSource(void *read_buffer, aml_audio_dec_t *audec)	
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
	bytes_readed_sum=0;
	bytes_readed_sum_pre=0;
	set_ALAC_MetaData(audec);
	
}
ALAC_MediaSource::~ALAC_MediaSource() 
{
	ALOGI("[%s] in line (%d) \n",__FUNCTION__,__LINE__);
    if (mStarted)
        stop();
}
int ALAC_MediaSource::GetSampleRate()
{	
	return sample_rate;
}

int ALAC_MediaSource::GetChNum()
{
	return ChNum;
}

int* ALAC_MediaSource::Get_pStop_ReadBuf_Flag()
{
	return pStop_ReadBuf_Flag;
}

int ALAC_MediaSource::Set_pStop_ReadBuf_Flag(int *pStop)
{
	pStop_ReadBuf_Flag = pStop;
	return 0;
}

int ALAC_MediaSource::GetReadedBytes()
{
    int bytes_used = 0;
    bytes_used=bytes_readed_sum-bytes_readed_sum_pre;
    if (bytes_used < 0)
    {
        ALOGI("[%s]WARING: bytes_readed_sum(%ld) < bytes_readed_sum_pre(%ld) \n",__FUNCTION__,
        (long)bytes_readed_sum, (long)bytes_readed_sum_pre);
        bytes_used=0;
    }
    bytes_readed_sum_pre=bytes_readed_sum;
    return bytes_used;
}

sp<MetaData> ALAC_MediaSource::getFormat() {
	ALOGI("[%s] in line (%d) \n",__FUNCTION__,__LINE__);
    return mMeta;
}

status_t ALAC_MediaSource::start(MetaData *params __unused)
{
	ALOGI("[%s] in line (%d) \n",__FUNCTION__,__LINE__);
    mGroup = new MediaBufferGroup;
	mGroup->add_buffer(MediaBufferBase::Create(MaxFrameSize));
	mStarted = true;
    return OK;	
}

status_t ALAC_MediaSource::stop()
{
	ALOGI("[%s] in line (%d) \n",__FUNCTION__,__LINE__);
	delete mGroup;
    mGroup = NULL;
    mStarted = false;
    return OK;
}

status_t ALAC_MediaSource::read(MediaBufferBase **out, const ReadOptions *options __unused)
{
	*out = NULL;
	unsigned char header_buffer[5];
	frame_size = 0;

	if (*pStop_ReadBuf_Flag==1){
		 ALOGI("Stop_ReadBuf_Flag==1 stop read_buf [%s %d]",__FUNCTION__,__LINE__);
         return ERROR_END_OF_STREAM;
	}
	
	MediaSourceRead_buffer(header_buffer,4);
	while(1){
		if((header_buffer[0] == 0x11)&&(header_buffer[1] == 0x22)&&(header_buffer[2] == 0x33)&&(header_buffer[3] == 0x44)){
			break;
		}
		MediaSourceRead_buffer(&header_buffer[4],1);
		header_buffer[0] = header_buffer[1];
		header_buffer[1] = header_buffer[2];
		header_buffer[2] = header_buffer[3];
		header_buffer[3] = header_buffer[4];
		
		if (*pStop_ReadBuf_Flag==1){
		 ALOGI("Stop_ReadBuf_Flag==1 stop read_buf [%s %d]",__FUNCTION__,__LINE__);
         return ERROR_END_OF_STREAM;
		}
	}
	
	MediaSourceRead_buffer(header_buffer,2);
	frame_size = header_buffer[0]<<8 | header_buffer[1];
	//ALOGI("frame_size = %d \n",frame_size);
	
	if(frame_size<=0)
	{  
	   ALOGV("WARNING: Invalid frame_size %d \n",frame_size);
	   return ERROR_END_OF_STREAM;
	}
	
    MediaBufferBase *buffer;
    status_t err = mGroup->acquire_buffer(&buffer);
    if (err != OK) {
        return err;
    }
	
    if (MediaSourceRead_buffer((unsigned char*)(buffer->data()),frame_size) != frame_size)
	{
		buffer->release();
		buffer = NULL;
		return ERROR_END_OF_STREAM;
	}
	bytes_readed_sum+=frame_size;
	buffer->set_range(0, frame_size);
	buffer->meta_data().setInt64(kKeyTime, mCurrentTimeUs);
	
    *out = buffer;
	 return OK;
}




}  // namespace android

