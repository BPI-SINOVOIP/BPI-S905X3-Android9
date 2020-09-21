/*
 * Copyright (C) 2019-2020 HAOBO Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/hidraw.h>


#include <hardware/hardware.h>
#include <system/audio.h>
#include <hardware/audio.h>
#include<stdio.h>
#include <unistd.h>
//#define am8

#ifndef am8
    #include<android/log.h>
#else
    #include <cutils/log.h>
    #include <tinyalsa/asoundlib.h>
    #include "audio_hw.h"
#endif

#include "hbg_blehid_mic.h"
#include "ringBuffer.h"
#include "hbddecode_info_interface.h"


#define LOGD(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, LOGH_TAG, fmt, ##args)
#define LOGE(fmt, args...) __android_log_print(ANDROID_LOG_ERROR, LOGH_TAG, fmt, ##args)
#define PCM_DATA_BUFFER_LEN (16*1024)//huajianwu 0621

#define HBG_VID 0x1d5a

#define _vendor_hbg                //9.0 need to add vendor目录

#ifdef _vendor_hbg
#define BLE_KEYDOWN_NAME                "/data/vendor/HBG/ble"
#define NODE_RECORDING                "/data/vendor/HBG/recording"
#define DATA_FILE                "/data/vendor/HBG/record"
#define TARGET_FILE                "/data/vendor/HBG"
#else
#define BLE_KEYDOWN_NAME                "/data/HBG/ble"
#define NODE_RECORDING                "/data/HBG/recording"
#define DATA_FILE                "/data/HBG/record"
#define TARGET_FILE                "/data/HBG"
#endif
#define UNUSED(x) (void)x      //形参未使用报错，因此加这个。要使用形参时，需要把定义函数中的UNUSED(x)给注释
#define setbit(x,y) x|=(1<<y) //将X的第Y位置1
#define clrbit(x,y) x&=~(1<<y) //将X的第Y位清0
#define BitGet(Number,pos) (((Number) >> (pos))&1) //用宏得到某数的某位


unsigned char hbg_bmic_used =0;// 1 is test for hbg
static unsigned char start_audio_cmd;
struct hbg_config *hbg_cfg;

//static char mValue[128];
static int is_read = 0;
static int64_t frames_read = 0;

static int file_no = 0;

static int running_flag = 0;
static int running_m_flag = 0;
pthread_t id;
pthread_t monit_id;
pthread_mutex_t lock;

struct RegAudioCB
{
    int in_use;
    int flag;
    struct RingBuffer *ptr;
};
static struct RegAudioCB mRegAudioCB[MAX_NUM_CALLBACK];


static void* Monit_Mic_Thread(void* arg) {
    UNUSED(arg);
    unsigned char found_mic_start =0;
    unsigned char found_mic_stop = 0;
    LOGD("---0223 monit_mic_Thread ---");
    running_m_flag = 1;
    while(running_m_flag) {
    //BLE 节点
        if(access(BLE_KEYDOWN_NAME, F_OK)== -1) {  //可以使用inotify_add_watch来优化监控！后续修改
            found_mic_start = 0;
            if(found_mic_stop == 0) {
                hbg_bmic_used = 0; //mic key is up
                found_mic_stop = 1;
                //send stop info to 遥控器
                LOGD("HBG remote is powerdown!");
            }
            usleep(2000);
        }else {
            found_mic_stop = 0;
            if(found_mic_start == 0) {
                hbg_bmic_used = 1;  //mic key is down
                found_mic_start = 1;
                //send start info to 遥控器
                LOGD("HBG remote is active!");
            }
            usleep(1000);
        }
    }
    LOGE("Monit_Mic_Thread out!");
    return NULL;
}


static void* ReceiveAudioDataThread(void* arg)//2018.11.1
{
    UNUSED(arg);
    unsigned char buf_r[4096];  //huajianwu
    int fd = -1;
    int  nread;
     int i;
    //int position_flag = 0x0;

    LOGD("---0208 ReceiveAudioDataThread ---");
    running_flag = 1;
    start_audio_cmd = 0;//2018.11.26
    while(running_flag) {
        while(start_audio_cmd) {
             //read_data_from_buffer()----This interface is implemented in the so library
            if((nread=read_data_from_buffer(buf_r,1024))> 0) {
                if(access(NODE_RECORDING, F_OK)==0)//creat the recording file node to record audio data---------test the audio 
                    dumpData(DATA_FILE,buf_r,nread);

                    for(i = 0; i<MAX_NUM_CALLBACK; i++ ) {
                         if(mRegAudioCB[i].in_use) {
                             WriteRingBuffer(mRegAudioCB[i].ptr,buf_r,nread);
                         }
                    }
                    memset(buf_r,0,sizeof(buf_r));
            } else {
                LOGD("nread read 0\n");
                usleep(2000);
                continue;
            }
        }
        if(running_flag == 0){ //kill thread!
            break;
        }
        usleep(200000);//200ms
    }
    LOGE("read thread out!");
    close(fd);
    return NULL;

}

void creat_record()//2018.11.1
{
    send_start_audio_cmd();//This interface is implemented in the so library
    start_audio_cmd = 1;
}

int registerAudioDataCB()
{
    int i;

    creat_record();

    for(i = 0; i<MAX_NUM_CALLBACK; i++ )
    {
        if(!mRegAudioCB[i].in_use)
        {
/*         if(mRegAudioCB[i].flag == 1)
				continue; */
            mRegAudioCB[i].ptr  = InitRingBuffer(PCM_DATA_BUFFER_LEN);
            mRegAudioCB[i].in_use = 1;
            //mRegAudioCB[i].flag = 1;
            LOGD("--- registerAudioDataCB %d ---",i);
            return i;
        }
    }
    return -1;
}

void unRegisterAudioDataCB(int index)//2018.11.1
{
    //int i;

    pthread_mutex_lock(&lock);

    mRegAudioCB[index].in_use = 0;
    if (mRegAudioCB[index].ptr==NULL)
    {
        LOGD("%s ptr==NULL",__FUNCTION__);
/*
        for (i = 0; i < MAX_NUM_CALLBACK; ++i)//2018.7.6
        {
        if (mRegAudioCB[i].in_use == 1)
        {
            pthread_mutex_unlock(&lock);
            return;
        }

    } */
        send_stop_audio_cmd();
        start_audio_cmd = 0;

        pthread_mutex_unlock(&lock);//2018.6.19
        return;
    }

    DeInitRingBuffer(mRegAudioCB[index].ptr);
    LOGD("%s:DeInitRingBuffer",__FUNCTION__);
    mRegAudioCB[index].ptr = NULL;


/*
    for (i = 0; i < MAX_NUM_CALLBACK; ++i)//2018.7.6
    {
        if (mRegAudioCB[i].in_use == 1)
        {
            pthread_mutex_unlock(&lock);
            return;
        }

    } */
    send_stop_audio_cmd();
    start_audio_cmd = 0;

    if(access(DATA_FILE, F_OK)==0){//rename audio data file for test
        char file_new_name[30];
        sprintf(file_new_name,"%s/record_%d",TARGET_FILE,file_no);
        if (rename(DATA_FILE, file_new_name) == 0)
            LOGD("record file rename success!");
        if(file_no == 2)
            file_no = 0;
        else
            file_no++;
    }

    pthread_mutex_unlock(&lock);//2018.6.19
}


void startReceiveAudioData()
{
    hbg_init();
    if(running_flag == 0)
    {
        LOGD("0208creat rcv thread!");
        //rm ble
        remove(BLE_KEYDOWN_NAME);
        memset(mRegAudioCB,0,sizeof(struct RegAudioCB)*MAX_NUM_CALLBACK);
        pthread_create(&id,NULL,ReceiveAudioDataThread,NULL);
        pthread_create(&monit_id,NULL,Monit_Mic_Thread,NULL);
        pthread_mutex_init(&lock,NULL);
    }
}

void stopReceiveAudioData()
{
    //LOGD("0208stop rcv thread!");
    int i ;
    running_flag = 0;
    start_audio_cmd = 0;
    running_m_flag = 0;
    pthread_join(id,NULL);
    pthread_join(monit_id,NULL);
    for(i =0;i < MAX_NUM_CALLBACK;i++)
    {
        if(mRegAudioCB[i].in_use)
        {
            mRegAudioCB[i].in_use = 0;
            //mRegAudioCB[i].flag = 0;
            DeInitRingBuffer(mRegAudioCB[i].ptr);
            mRegAudioCB[i].ptr = NULL;
        }
    }
    pthread_mutex_destroy(&lock);
    hbg_stop_thread();

}

//
int regist_callBack_stream()
{

//    LOGD("--- RegisterAudioDataCB---");
    return registerAudioDataCB();
}
void unregist_callBack_stream(int channel)
{
    //if(hbg_bmic_used== 0)//20180315  bug empty point
    //	return;

    LOGD("--- unRegisterAudioDataCB %d---",channel);
    unRegisterAudioDataCB(channel);
}

int is_hbg_hidraw(){
    char *device = "/dev/hidraw";
    int i=0;
    int openfd,res;
    char device_to_be_open[64];
    struct hidraw_devinfo info;
    memset(&info, 0x0, sizeof(info));
    for(i=0;i<8;i++)
    {
        memset(device_to_be_open, 0x0, sizeof(device_to_be_open));
        sprintf(device_to_be_open,"%s%d",device,i);
        openfd = open(device_to_be_open, O_RDWR);
        if (openfd < 0) {
            LOGD("Unable to open device %s", device_to_be_open);
            continue;
        }
        else
        {
            res=ioctl(openfd, HIDIOCGRAWINFO, &info);
            if (res < 0) {
                LOGD("HIDIOCGRAWINFO");
            } else {
                LOGD("vendor: 0x%04hx", info.vendor);
                if( (info.vendor & 0x0000FFFF) != HBG_VID )
                {
                    LOGD("next hidraw..");
                    close(openfd);
                    continue;
                }else{
                    close(openfd);
                    return 1;
                }
            }
        }
    }
    return 0;
}

////----------------------------------function callback----------

/** audio_stream_in implementation **/
 uint32_t in_hbg_get_sample_rate(const struct audio_stream *stream)
{
    UNUSED(stream);
    //LOGD("get sample rate is 16K! \n");
    return 16000;
}

 int in_hbg_set_sample_rate(struct audio_stream *stream, uint32_t rate)
{
    UNUSED(stream);
    UNUSED(rate);
    LOGD("in_set_sample_rate");
    return 0;
}

 size_t in_hbg_get_buffer_size(const struct audio_stream *stream)
{
    UNUSED(stream);
    // struct stub_stream_in *in = (struct stub_stream_in *)stream;
    int bsize =2048;
    //LOGD("in_get_buffer_size %d",bsize); //pcm buff szie

    return bsize;//mRegAudioCB[in->hbg_channel].ptr->validDataLen;
}

 uint32_t in_hbg_get_channels(const struct audio_stream *stream)
{
    UNUSED(stream);
    //LOGD("in_hbg_get_channels!");
    return AUDIO_CHANNEL_IN_MONO;
}
 int in_hbg_set_format(struct audio_stream *stream, audio_format_t format)
{
    UNUSED(stream);
    LOGD("in_hbg_set_format!");
    if (format != AUDIO_FORMAT_PCM_16_BIT) {
        return -ENOSYS;
    } else {
        return 0;
    }
}

 audio_format_t in_hbg_get_format(const struct audio_stream *stream)
{
    UNUSED(stream);
    //LOGD("in_hbg_get_format!");
    return AUDIO_FORMAT_PCM_16_BIT;
}

 int in_hbg_standby(struct audio_stream *stream)
{
    UNUSED(stream);
    return 0;
}

 int in_hbg_dump(const struct audio_stream *stream, int fd)
{
    UNUSED(stream);
    UNUSED(fd);
    return 0;
}

 int in_hbg_add_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    UNUSED(stream);
    UNUSED(effect);
    return 0;
}

 int in_hbg_remove_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    UNUSED(stream);
    UNUSED(effect);
    return 0;
}

 int in_hbg_set_parameters(struct audio_stream *stream, const char *kvpairs)
{
    UNUSED(stream);
    UNUSED(kvpairs);
    return 0;
}

 char * in_hbg_get_parameters(const struct audio_stream *stream,
                                const char *keys)
{
    UNUSED(stream);
    UNUSED(keys);
    return strdup("");
}

 int in_hbg_set_gain(struct audio_stream_in *stream, float gain)
{
    UNUSED(stream);
    UNUSED(gain);
    return 0;
}

int send_zero_data(unsigned char *data,int len)
{
    unsigned char fakedata[8] = {0x01,0x00,0x01,0x00,0x00,0x00,0x00,0x00};
    //16K 16bit 100ms 320byte
    unsigned char zero_data[len];
    unsigned int i = 0;

    LOGD("%s:into   data:%p len:%d",__FUNCTION__,data,len);
    for(i = 0; i<sizeof(zero_data)/sizeof(fakedata);i++)
        memcpy((char *)zero_data + i*sizeof(fakedata), (char *)fakedata, sizeof(fakedata));

    if (data==NULL)
    {
        LOGD("%s data==NULL",__FUNCTION__);
        return 0;
    }
    memcpy(data, zero_data, len);

    return 0;
}

 ssize_t in_hbg_read(struct audio_stream_in *stream, void* buffer,
                       size_t bytes)
{
    #ifndef am8
    struct stub_stream_in *in = (struct stub_stream_in *)stream;
    #else
    struct aml_stream_in *in = (struct aml_stream_in *)stream;
    #endif

    int timeOut =0;
    ssize_t cnt = -1;
    int flag = 1;
    //int len =0;
    #if 0
    cnt = ReadRingBuffer2(mRegAudioCB[in->hbg_channel].ptr, buffer, bytes);
    //LOGD("in_hbg_read %d \n",bytes);
    while(cnt == 0)
    {
        usleep(5000);
        cnt = ReadRingBuffer2(mRegAudioCB[in->hbg_channel].ptr, buffer, bytes);
        timeOut++;
        if(timeOut > 400) //timeout
        {
            LOGD("read tout:%d %d",in->hbg_channel,(int)cnt);
            creat_record();
            break;
        }
    }
    #else

    pthread_mutex_lock(&lock);
    //LOGD("%s: thread_lock",__FUNCTION__);

    if (mRegAudioCB[in->hbg_channel].ptr != NULL)
    {
        cnt = ReadRingBuffer2(mRegAudioCB[in->hbg_channel].ptr, buffer, bytes);
        while(cnt == 0)
        {
            usleep(5000);
            cnt = ReadRingBuffer2(mRegAudioCB[in->hbg_channel].ptr, buffer, bytes);
            timeOut++;
            //LOGD("cnt == 0  timeOut:%d channel:%d",timeOut,in->hbg_channel);
            if ((flag == 1) && (timeOut > 200))
            {
                creat_record();
                flag = 0;
            }

            if(timeOut > 400) //timeout
            {
                LOGD("read tout:%d %d",in->hbg_channel,(int)cnt);
                creat_record();
                break;
            }

        }
        //LOGD("%s: cnt=%d",__FUNCTION__,cnt);
    }else{
        LOGD("mRegAudioCB[].ptr == NULL");

        if (mRegAudioCB[0].in_use == 0)
        {
            creat_record();
            mRegAudioCB[0].ptr  = InitRingBuffer(PCM_DATA_BUFFER_LEN);
            mRegAudioCB[0].in_use = 1;
            //LOGD("%s: cnt==0 mRegAudioCB[0].in_use = 1",__FUNCTION__);
        }else{
            //LOGD("%s: cnt==0 channel:%d",__FUNCTION__,in->hbg_channel);
            cnt = send_zero_data(buffer,bytes);
        }
    }
    pthread_mutex_unlock(&lock);
#endif
    if (cnt > 0) {
        frames_read += cnt; // / audio_stream_in_frame_size(stream);
        is_read = 1;
    }
    return cnt;
}

 uint32_t in_hbg_get_input_frames_lost(struct audio_stream_in *stream)
{
    UNUSED(stream);
    LOGD("in_hbg_get_input_frames_lost!");
    return 0;
}


 int in_hbg_get_hbg_capture_position(const struct audio_stream_in *stream,
                                   int64_t *frames, int64_t *time)
{
    if (stream == NULL || frames == NULL || time == NULL) {
        return -EINVAL;
    }
    //LOGD("in_hbg_get_hbg_capture_position!");
#if 1
    /* #ifndef am8
    struct stub_stream_in *in = (struct stub_stream_in *)stream;
    #else
    struct aml_stream_in *in = (struct aml_stream_in *)stream;
    #endif*/

    int ret = -ENOSYS;


    if (is_read)
    {
        struct timespec timestamp;
        unsigned int avail = 0;
        *frames = frames_read + avail;
        clock_gettime(CLOCK_REALTIME, &timestamp);	
        *time = timestamp.tv_sec * 1000000000LL + timestamp.tv_nsec;
        ret = 0;
        //LOGD("get_hbg_capture_position!");
    }

    return ret;
#else
    return -ENOSYS;
#endif

}





