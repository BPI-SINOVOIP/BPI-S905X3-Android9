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
#include "MP3_mediasource.h"

extern "C" int read_buffer(unsigned char *buffer,int size);

//#define LOG_TAG "MP3_Mediasource"
//#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
//#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

#define kMaxReadBytes 4096
#define MaxFrameSize 4096
static const uint32_t kMask = 0xfffe0c00;

namespace android {
    
uint32_t U32_AT(const uint8_t *ptr) 
{
    return ptr[0] << 24 | ptr[1] << 16 | ptr[2] << 8 | ptr[3];
}
inline bool MP3_MediaSource::GetMPEGAudioFrameSize(uint32_t header, int *frame_size,
        int *out_sampling_rate, int *out_channels, int *out_bitrate, int *out_num_samples) {
    *frame_size = 0;

    if (out_sampling_rate) {
        *out_sampling_rate = 0;
    }

    if (out_channels) {
        *out_channels = 0;
    }

    if (out_bitrate) {
        *out_bitrate = 0;
    }

    if (out_num_samples) {
        *out_num_samples = 1152;
    }

    if ((header & 0xffe00000) != 0xffe00000) {
        return false;
    }

    unsigned version = (header >> 19) & 3;

    if (version == 0x01) {
        return false;
    }

    unsigned layer = (header >> 17) & 3;

    if (layer == 0x00) {
        return false;
    }

    //unsigned protection = (header >> 16) & 1;

    unsigned bitrate_index = (header >> 12) & 0x0f;

    if (bitrate_index == 0 || bitrate_index == 0x0f) {
        // Disallow "free" bitrate.
        return false;
    }

    unsigned sampling_rate_index = (header >> 10) & 3;

    if (sampling_rate_index == 3) {
        return false;
    }

    static const int kSamplingRateV1[] = { 44100, 48000, 32000 };
    int sampling_rate = kSamplingRateV1[sampling_rate_index];
    if (version == 2 /* V2 */) {
        sampling_rate /= 2;
    } else if (version == 0 /* V2.5 */) {
        sampling_rate /= 4;
    }

    unsigned padding = (header >> 9) & 1;

    if (layer == 3) {
        // layer I

        static const int kBitrateV1[] = {
            32, 64, 96, 128, 160, 192, 224, 256,
            288, 320, 352, 384, 416, 448
        };

        static const int kBitrateV2[] = {
            32, 48, 56, 64, 80, 96, 112, 128,
            144, 160, 176, 192, 224, 256
        };

        int bitrate =
            (version == 3 /* V1 */)
                ? kBitrateV1[bitrate_index - 1]
                : kBitrateV2[bitrate_index - 1];

        if (out_bitrate) {
            *out_bitrate = bitrate;
        }

        *frame_size = (12000 * bitrate / sampling_rate + padding) * 4;

        if (out_num_samples) {
            *out_num_samples = 384;
        }
    } else {
        // layer II or III

        static const int kBitrateV1L2[] = {
            32, 48, 56, 64, 80, 96, 112, 128,
            160, 192, 224, 256, 320, 384
        };

        static const int kBitrateV1L3[] = {
            32, 40, 48, 56, 64, 80, 96, 112,
            128, 160, 192, 224, 256, 320
        };

        static const int kBitrateV2[] = {
            8, 16, 24, 32, 40, 48, 56, 64,
            80, 96, 112, 128, 144, 160
        };

        int bitrate;
        if (version == 3 /* V1 */) {
            bitrate = (layer == 2 /* L2 */)
                ? kBitrateV1L2[bitrate_index - 1]
                : kBitrateV1L3[bitrate_index - 1];

            if (out_num_samples) {
                *out_num_samples = 1152;
            }
        } else {
            // V2 (or 2.5)

            bitrate = kBitrateV2[bitrate_index - 1];
            if (out_num_samples) {
                *out_num_samples = (layer == 1 /* L3 */) ? 576 : 1152;
            }
        }

        if (out_bitrate) {
            *out_bitrate = bitrate;
        }

        if (version == 3 /* V1 */) {
            *frame_size = 144000 * bitrate / sampling_rate + padding;
        } else {
            // V2 or V2.5
            size_t tmp = (layer == 1 /* L3 */) ? 72000 : 144000;
            *frame_size = tmp * bitrate / sampling_rate + padding;
        }
    }

    if (out_sampling_rate) {
        *out_sampling_rate = sampling_rate;
    }

    if (out_channels) {
        int channel_mode = (header >> 6) & 3;

        *out_channels = (channel_mode == 3) ? 1 : 2;
    }

    return true;
}
int MP3_MediaSource::MediaSourceRead_buffer(unsigned char *buffer,int size)
{
    int readcnt=0;
    int readsum=0;

    if(fpread_buffer!=NULL){
        int sleep_time=0;
        while((readsum<size) && (*pStop_ReadBuf_Flag==0)){
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

unsigned int MP3_MediaSource::Find_header(unsigned char *buffer, int *frame_size,
                        int *out_sampling_rate, int *out_channels,int *out_bitrate, int *out_num_samples)
{   
    unsigned int readedbytes = 0;
    unsigned char header_buffer[5];
    
    readedbytes = MediaSourceRead_buffer(header_buffer,4);

    if (readedbytes < 4) 
    {
        ALOGI("Read MP3 header is fail [%s %d]",__FUNCTION__,__LINE__);
        return 0;
    }
    
    while(1){
        uint32_t header = U32_AT((const uint8_t *)header_buffer);
        if(GetMPEGAudioFrameSize(header, frame_size, out_sampling_rate,out_channels,
                            out_bitrate, out_num_samples)){
            break;
        }
        MediaSourceRead_buffer(&header_buffer[4],1);
        memmove(&header_buffer[0],&header_buffer[1],4);
        readedbytes++;
        if (*pStop_ReadBuf_Flag==1){
            ALOGI("Stop_ReadBuf_Flag==1 stop read_buf [%s %d]",__FUNCTION__,__LINE__);
            return 0;
        }
    }
    memcpy(&buffer[0],&header_buffer[0],4);
    return readedbytes;
}

bool MP3_MediaSource::refind_header(unsigned char *buffer, int *frame_size, unsigned char *header_buffer,
                                int offset, int *readbytes)
{   
    int readedbytes;
    unsigned char *pos_buffer = buffer;
    
    memcpy(&header_buffer[0],pos_buffer,4);
    pos_buffer += 4;
    
    for(readedbytes = 4; readedbytes < offset; readedbytes++){
        uint32_t header = U32_AT((const uint8_t *)header_buffer);
        
        if(GetMPEGAudioFrameSize(header, frame_size, NULL,NULL,NULL, NULL)){
            *readbytes = readedbytes;
            return true;
        }
        memcpy(&header_buffer[4],pos_buffer++,1);
        memmove(&header_buffer[0],&header_buffer[1],4);
        if (*pStop_ReadBuf_Flag==1){
            ALOGI("Stop_ReadBuf_Flag==1 stop read_buf [%s %d]",__FUNCTION__,__LINE__);
            return false;
        }
    }
    return false;
}


unsigned int MP3_MediaSource::Resync(uint32_t pos)
{
    unsigned char header_buffer[10];
    uint32_t first_header = 0;
    uint32_t test_header = 0;
    int offset = 0;
    int readbytes = 0;
    //uint32_t counter = 0;
    int framesize = 0;
    uint32_t position = pos;
    uint8_t *buffer;
    int left_bytes = 0;
    ResyncOccurFlag=2;
    SyncWordPosArray[0] = bytes_readed_sum;
BeginResync:

    ALOGI("Resync start!\n");
    left_bytes = 0;
    buffer = buf;
    offset = 0;
    Resync_framesize[0] = 0;
    Resync_framesize[1] = 0;
    Resync_framesize[2] = 0;
    
    if(position == 0){
        readbytes = Find_header(header_buffer,&framesize,NULL,NULL,NULL,NULL);
        first_header = U32_AT((const uint8_t *)header_buffer);
        memcpy(buf,&header_buffer[0],4);
        Resync_framesize[0] = framesize;
    }else{
        Resync_framesize[0] = framesize = frame_size;
    }
    SyncWordPosArray[1] = bytes_readed_sum-4;//first syncword pos:
    first_header = U32_AT((const uint8_t *)header_buffer);
    position = 0;
    
    if(MediaSourceRead_buffer(buf+4,framesize-4) != (framesize-4)){
        ALOGE("Read MP3 data is fail [%s %d]",__FUNCTION__,__LINE__);
        return ERROR_END_OF_STREAM;
    }
    offset += framesize;
    Resynced_flag = 1;

	//ALOGI("first offset = [%d]\n",offset);
    
    do{
        readbytes = MediaSourceRead_buffer(header_buffer,4);
        memcpy(buffer+offset,&header_buffer[0],4);
        offset += 4;
        test_header = U32_AT((const uint8_t *)header_buffer);
        
Compare:
        if(!GetMPEGAudioFrameSize(test_header,&framesize,NULL,NULL,NULL,NULL) || 
                                                ((test_header & kMask) != (first_header & kMask)))
        {
            if(refind_header(buffer+4, &framesize, &header_buffer[0],offset-4,&readbytes) == false)
            {
                goto BeginResync;       //can't find first header in loaded data
            }else{
                first_header = U32_AT((const uint8_t *)header_buffer); //reset the first header
                Resynced_flag = 1;
                Resync_framesize[0] = framesize;
                Resync_framesize[1] = 0;
                offset -= readbytes;
                memmove(buffer,buffer+readbytes,offset);
                if(framesize > offset){
                    readbytes = MediaSourceRead_buffer(buffer+offset,framesize - offset);
                    offset = framesize;
                    continue;
                }else{
                    test_header = U32_AT((const uint8_t *)(buffer+framesize));
                }       goto Compare;
            }   
        }else{
            Resynced_flag ++;
            SyncWordPosArray[Resynced_flag] = bytes_readed_sum-4;//first syncword pos:  
            if(Resynced_flag > 3){
                  ALOGI("Resynced out![%s %d]",__FUNCTION__,__LINE__);
                  Resynced_flag = 3;
                  break;
            }
            
            left_bytes = offset - 4 - Resync_framesize[0] - Resync_framesize[1];
            Resync_framesize[Resynced_flag-1] = framesize;
                                    
            if(left_bytes == 0){
                readbytes = MediaSourceRead_buffer(buffer+offset,framesize - 4);
                offset += (framesize - 4);
            }else if((left_bytes < framesize) && (left_bytes > 0)){
                readbytes = MediaSourceRead_buffer(buffer+offset,framesize - left_bytes - 4);
                offset += (framesize - left_bytes - 4);
            }else if(left_bytes > framesize){
                test_header = U32_AT((const uint8_t *)(buffer+framesize+Resync_framesize[Resynced_flag-2]));
                goto Compare;
            }else{
                ALOGE("offset [%d] is invalid!left_bytes [%d] framesize [%d] [%s %d]",
                                offset,left_bytes,framesize,__FUNCTION__,__LINE__);
            }
        }
    } while (Resynced_flag != 3);
    ALOGI("SyncWordPosArray:%ld/ %ld/ %ld/ %ld/ %ld",
            (long)SyncWordPosArray[0], (long)SyncWordPosArray[1], (long)SyncWordPosArray[2], (long)SyncWordPosArray[3], (long)SyncWordPosArray[4]);
    Resync_pos = 0;
    return first_header;
}

int MP3_MediaSource::set_MP3_MetaData(aml_audio_dec_t *audec)
{
    mMeta->setInt32(kKeyChannelCount, audec->channels);
    ChNum = audec->channels;
    mMeta->setInt32(kKeySampleRate, audec->samplerate);
    sample_rate = audec->samplerate;
    
    ALOGI("channel_number = [%d], samplerate = [%d]\n",ChNum,sample_rate);
    
    return 0;
}


//-------------------------------------------------------------------------
MP3_MediaSource::MP3_MediaSource(void *read_buffer, aml_audio_dec_t *audec, int *exit)  
{   
    ALOGI("[%s] in line (%d) \n",__FUNCTION__,__LINE__);
    mStarted=false;
    mMeta=new MetaData;
    mDataSource=NULL;
    mGroup=NULL;
    mCurrentTimeUs=0;
    pStop_ReadBuf_Flag=exit;
    fpread_buffer=(fp_read_buffer)read_buffer;
    sample_rate=0;
    ChNum=0;
    frame_size=0;
    bitrate = 0;
    num_samples = 0;
    Resynced_flag = 0;
    buf = NULL;
    mFixedHeader = 0;
    start_flag = 1;
    
    set_MP3_MetaData(audec);
    buf=(uint8_t *) malloc (sizeof(uint8_t)*kMaxReadBytes);
    ResyncOccurFlag=0;
    memset(SyncWordPosArray,0,sizeof(int64_t)*5);
    if(buf == NULL){
        ALOGE("Malloc buffer is fail! [%s] [%d]\n",__FUNCTION__,__LINE__);
    }
    
}

MP3_MediaSource::~MP3_MediaSource() 
{
    ALOGI("[%s] in line (%d) \n",__FUNCTION__,__LINE__);
    
    free( buf ); 
    buf = NULL;
    
    if (mStarted)
        stop();
}
int MP3_MediaSource::GetSampleRate()
{   
    return sample_rate;
}

int MP3_MediaSource::GetChNum()
{
    return ChNum;
}

int* MP3_MediaSource::Get_pStop_ReadBuf_Flag()
{
    return pStop_ReadBuf_Flag;
}

int MP3_MediaSource::Set_pStop_ReadBuf_Flag(int *pStop)
{
    pStop_ReadBuf_Flag = pStop;
    return 0;
}

int MP3_MediaSource::GetReadedBytes()
{    int bytes_used = 0;

     if(ResyncOccurFlag){
         if(ResyncOccurFlag==2){
            bytes_used=SyncWordPosArray[ResyncOccurFlag]-SyncWordPosArray[0];
         }else{
            bytes_used=SyncWordPosArray[ResyncOccurFlag]-SyncWordPosArray[ResyncOccurFlag-1];
         }
         ResyncOccurFlag++;
         if(ResyncOccurFlag==5) 
            ResyncOccurFlag=0;  
     }else{
         bytes_used=frame_size;//bytes_readed_sum-bytes_readed_sum_pre;
     }
     if(bytes_used<0)
     {
        ALOGI("[%s]WARING: bytes_readed_sum(%ld) < bytes_readed_sum_pre(%ld) \n",__FUNCTION__,
                                                        (long)bytes_readed_sum, (long)bytes_readed_sum_pre);
        bytes_used=0;
     }
     bytes_readed_sum_pre=bytes_readed_sum;
     return bytes_used;
}

sp<MetaData> MP3_MediaSource::getFormat() {
    ALOGI("[%s] in line (%d) \n",__FUNCTION__,__LINE__);
    return mMeta;
}

status_t MP3_MediaSource::start(MetaData *params __unused)
{
    ALOGI("[%s] in line (%d) \n",__FUNCTION__,__LINE__);
    mGroup = new MediaBufferGroup;
    mGroup->add_buffer(MediaBufferBase::Create(MaxFrameSize));
    mStarted = true;
    return OK;  
}

status_t MP3_MediaSource::stop()
{
    ALOGI("[%s] in line (%d) \n",__FUNCTION__,__LINE__);
    delete mGroup;
    mGroup = NULL;
    mStarted = false;
    return OK;
}

status_t MP3_MediaSource::read(MediaBufferBase **out, const ReadOptions *options __unused)
{
    *out = NULL;
    uint32_t readedbytes = 0;
    unsigned char header_buffer[5];
    frame_size = 0;
    //uint32_t header = 0;
    
    if (*pStop_ReadBuf_Flag==1){
        ALOGI("Stop_ReadBuf_Flag==1 stop read_buf [%s %d]",__FUNCTION__,__LINE__);
        return ERROR_END_OF_STREAM;
    }

    if(start_flag == 1){
        ALOGI("Start Resync the begin of stream [%s %d]",__FUNCTION__,__LINE__);
        mFixedHeader = Resync(0);
        start_flag = 0;
    }
    
    MediaBufferBase *buffer;
    status_t err = mGroup->acquire_buffer(&buffer);
    if (err != OK) {
        return err;
    }

Read_data:
    
    if(Resynced_flag){  //read data from resync buffer
        if (*pStop_ReadBuf_Flag==1){
            ALOGI("Stop_ReadBuf_Flag==1 stop read_buf [%s %d]",__FUNCTION__,__LINE__);
            buffer->release();
            buffer = NULL;
            return ERROR_END_OF_STREAM;
        }
        frame_size = Resync_framesize[3-Resynced_flag];
        memcpy((unsigned char*)(buffer->data()),&buf[Resync_pos],frame_size);
        Resync_pos += frame_size;
        Resynced_flag--;
    }else{
        readedbytes = Find_header(header_buffer,&frame_size,&sample_rate,&ChNum,&bitrate,&num_samples);
        
        uint32_t header = U32_AT((const uint8_t *)header_buffer);
        
        if((header & kMask) != (mFixedHeader & kMask)){
            if (*pStop_ReadBuf_Flag==1){
                ALOGI("Stop_ReadBuf_Flag==1 stop read_buf [%s %d]",__FUNCTION__,__LINE__);
                buffer->release();
                buffer = NULL;
                return ERROR_END_OF_STREAM;
            }
            while(ResyncOccurFlag && *pStop_ReadBuf_Flag==0 )
               usleep(10000);
            mFixedHeader = Resync(4);
            memcpy(&buf[0],&header_buffer[0],4);
            ALOGI("[%s]Resync the MP3! \n",__FUNCTION__);
            goto Read_data;
        }
            
        //ALOGI("frame_size = %d,sample_rate = %d,ChNum = %d, bitrate = %d, num_samples = %d readedbytes = %d\n",
        //                                  frame_size,sample_rate,ChNum,bitrate,num_samples,readedbytes);
    
        if(frame_size<=0)
        {  
            ALOGE("WARNING: Invalid frame_size %d \n",frame_size);
            return ERROR_END_OF_STREAM;
        }
    
        memcpy((unsigned char*)(buffer->data()),&header_buffer[0],4);
    
        if (MediaSourceRead_buffer(((unsigned char*)buffer->data()+4),(frame_size-4)) != (frame_size-4)){
            buffer->release();
            buffer = NULL;
            return ERROR_END_OF_STREAM;
        }
    }
    
    buffer->set_range(0, frame_size);
    buffer->meta_data().setInt64(kKeyTime, mCurrentTimeUs);
    buffer->meta_data().setInt32(kKeyIsSyncFrame, 1);
    
    *out = buffer;
     return OK;
}




}  // namespace android

