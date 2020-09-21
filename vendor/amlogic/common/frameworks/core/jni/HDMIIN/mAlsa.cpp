/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC mAlsa
 */

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <unistd.h>
#include <pthread.h>
#include <media/AudioRecord.h>
#include <media/AudioTrack.h>
#include <media/mediarecorder.h>
#include <binder/IPCThreadState.h>
#include <system/audio.h>
#include <android/log.h>

#include <sys/stat.h>
#include <fcntl.h>

#include <cutils/log.h>
#include <cutils/str_parms.h>
#include <cutils/properties.h>
#include "audio_global_cfg.h"
#include "mAlsa.h"
#ifdef BOARD_ALSA_AUDIO_TINY
#include <tinyalsa/asoundlib.h>
#endif

//#define LOG_TAG "LibAudioCtl"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
//#undef LOGD
//#undef LOGE
//#define LOGD printf
//#define LOGE printf
using namespace android;

#define CC_ALSA_HAS_MUTEX_LOCK              (1)

static sp<AudioRecord> glpRecorder = NULL;
static sp<AudioTrack> glpTracker = NULL;

#if CC_ALSA_HAS_MUTEX_LOCK == 1
static Mutex free_recorder_lock;
static Mutex free_tracker_lock;
static Mutex alsa_init_lock;
static Mutex alsa_uninit_lock;
static Mutex temp_buffer_lock;
static Mutex tracker_ctrl_lock;
#endif


#define temp_buffer_size    4096*5
#define save_distance_max   4096*4
#define save_distance_min   4096
#define mid_buffer_distance 2048*5

short *temp_buffer = NULL;
short *end_temp_buffer = NULL;
short volatile *record_write_pointer;
short volatile *playback_read_pointer;


//static bool gEnableNoiseGate = false;
//static bool gUserSetEnableNoiseGate = false;
static unsigned zero_count_left = 2000;
static unsigned zero_count_right = 2000;
static signed zero_left = 0;
static signed zero_right = 0;
static unsigned NOISE_HIS = 48000 * 5;
//static signed gNoiseGateThresh = 0;

#define HISTORY_BASE   (12)
#define HISTORY_NUM    (1<<12)
static signed history[2][HISTORY_NUM];

static signed sum_left = 0;
static signed sum_right = 0;
static unsigned left_pos = 0;
static unsigned right_pos = 0;

static void FreeRecorder(void) {
    LOGD("*****FreeRecorder****\n");
#if CC_DISABLE_ALSA_MODULE == 0
#if CC_ALSA_HAS_MUTEX_LOCK == 1
    Mutex::Autolock _l(free_recorder_lock);
#endif

    if (glpRecorder != NULL) {
        glpRecorder->stop();
        glpRecorder.clear();
        //glpRecorder.~sp();
        // delete glpRecorder;
       // glpRecorder = NULL;
        LOGD("FreeRecorder return\n");
    }
#endif
    LOGD("***1**FreeRecorder****\n");
}

static void FreeTracker(void) {
    LOGD("*****FreeTracker****\n");
#if CC_DISABLE_ALSA_MODULE == 0
#if CC_ALSA_HAS_MUTEX_LOCK == 1
    Mutex::Autolock _l(tracker_ctrl_lock);
#endif

    if (glpTracker != NULL) {
        glpTracker->stop();
        glpTracker.clear();
       // glpTracker.~sp();
        // delete glpTracker;
       // glpTracker = NULL;
        LOGD("FreeTracker return.\n");
    }
#endif
    LOGD("***1**FreeTracker****\n");
}


/*
static void recorderCallback(int event, void* user, void *info) {
}

static void trackerCallback(int event, void* user, void *info) {
}

*/


static int InitTempBuffer() {
    LOGD("*****InitTempBuffer**temp_buffer=%p**\n",temp_buffer);
#if CC_ALSA_HAS_MUTEX_LOCK == 1
    Mutex::Autolock _l(temp_buffer_lock);
#endif
    if (NULL == temp_buffer) {
        temp_buffer = new short[temp_buffer_size];
        if (NULL == temp_buffer) {
            return -1;
        }
        end_temp_buffer = temp_buffer + temp_buffer_size;
        record_write_pointer = temp_buffer;
        playback_read_pointer = temp_buffer;
    }
    LOGD("***1**InitTempBuffer****\n");
    return 0;
}

static void MuteTempBuffer() {
    LOGD("*****MuteTempBuffer****\n");
#if CC_ALSA_HAS_MUTEX_LOCK == 1
    Mutex::Autolock _l(temp_buffer_lock);
    Mutex::Autolock _2(tracker_ctrl_lock);
#endif
    if ((temp_buffer != NULL) && (glpTracker != NULL)) {
        playback_read_pointer = record_write_pointer;
        memset(temp_buffer, 0, temp_buffer_size);
        for (int i = 0; i < 10; i++) {
            glpTracker->write(temp_buffer, temp_buffer_size);
        }
    }
    LOGD("***1**MuteTempBuffer****\n");
}

static void FreeTempBuffer() {
    LOGD("*****FreeTempBuffer****\n");
#if CC_ALSA_HAS_MUTEX_LOCK == 1
    Mutex::Autolock _l(temp_buffer_lock);
#endif
    if (temp_buffer != NULL) {
        delete[] temp_buffer;
        temp_buffer = NULL;
        end_temp_buffer = NULL;
        record_write_pointer = NULL;
        playback_read_pointer = NULL;
    }
    LOGD("***1**FreeTempBuffer****\n");
}



static void ResetRecordWritePointer() {
#if CC_ALSA_HAS_MUTEX_LOCK == 1
    Mutex::Autolock _l(temp_buffer_lock);
#endif
    record_write_pointer = playback_read_pointer;
}

static void ResetPlaybackReadPointer() {
#if CC_ALSA_HAS_MUTEX_LOCK == 1
    Mutex::Autolock _l(temp_buffer_lock);
#endif
    playback_read_pointer = record_write_pointer;
}

static int GetWriteSpace(short volatile *WritePoint, short volatile *ReadPoint) {
    unsigned int space;

    if (WritePoint >= ReadPoint)
        space = temp_buffer_size - (WritePoint - ReadPoint);
    else
        space = ReadPoint - WritePoint;

    return space;
}
static int GetReadSpace(short volatile *WritePoint, short volatile *ReadPoint) {
    unsigned int space;

    if (WritePoint >= ReadPoint)
        space = WritePoint - ReadPoint;
    else
        space = temp_buffer_size - (ReadPoint - WritePoint);

    return space;
}

static void recorderCallback(int event, void* user, void *info) {
//#if CC_AUD_SRC_IN_BUF_ANDROID
    if (AudioRecord::EVENT_MORE_DATA == event) {
        AudioRecord::Buffer *pbuf = (AudioRecord::Buffer*)info;

        if (glpRecorder==NULL) return;

#if CC_ALSA_HAS_MUTEX_LOCK == 1
        Mutex::Autolock _l(temp_buffer_lock);
#endif
        int available_write_space;
        int i;
        available_write_space = GetWriteSpace(record_write_pointer,playback_read_pointer);

        // LOGE("~~~~~available_write_space = %d \n", available_write_space);

        if(available_write_space <= save_distance_min )
        {
            //memcpy((short *)record_write_pointer, pbuf->raw, pbuf->size);
            LOGE("[%s]: *********Throw a frame data away!!!!!!!!\n", __FUNCTION__);
        }
        else
        {
            unsigned int offset_bytes = 2*(end_temp_buffer-record_write_pointer);
            unsigned int copy_bytes;
            int sampNum;

            if(offset_bytes >= pbuf->size)
            {
                memcpy((short *)record_write_pointer, pbuf->raw, pbuf->size);
                record_write_pointer += pbuf->size/2;

                if(record_write_pointer == end_temp_buffer)
                record_write_pointer = temp_buffer;
            }
            else
            {
                memcpy((short *)record_write_pointer, pbuf->raw, offset_bytes);

                copy_bytes = offset_bytes;
                offset_bytes = pbuf->size - copy_bytes;
                record_write_pointer = temp_buffer;

                memcpy((short *)record_write_pointer, (short *)pbuf->raw + (copy_bytes/2), offset_bytes);

                record_write_pointer += offset_bytes/2;
            }
        }

        //LOGD("--------RecordCallback, pbuf->size:%d, pbuf->frameCount:%d\n", pbuf->size, pbuf->frameCount);
        //LOGD("--------Record----offset_bytes:%d\n", 2*(record_write_pointer-temp_buffer));

    } else if (AudioRecord::EVENT_OVERRUN == event) {
        LOGE("[%s]: AudioRecord::EVENT_OVERRUN\n", __FUNCTION__);
    } else if (AudioRecord::EVENT_MARKER == event) {
        LOGD("[%s]: AudioRecord::EVENT_MARKER\n", __FUNCTION__);
    } else if (AudioRecord::EVENT_NEW_POS == event) {
        LOGD("[%s]: AudioRecord::EVENT_NEW_POS\n", __FUNCTION__);
    }
//#endif // CC_AUD_SRC_IN_BUF_ANDROID
}

static void trackerCallback(int event, void* user, void *info) {
//#if CC_AUD_SRC_IN_BUF_ANDROID
    if (AudioTrack::EVENT_MORE_DATA == event) {
        AudioTrack::Buffer *pbuf = (AudioTrack::Buffer*)info;

        if (glpTracker == NULL) return;

#if CC_ALSA_HAS_MUTEX_LOCK == 1
        Mutex::Autolock _l(temp_buffer_lock);
#endif
        int available_read_space;
        available_read_space = GetReadSpace(record_write_pointer,playback_read_pointer);

        // LOGE("~~~~~available_read_space = %d \n", available_read_space);

        if(available_read_space <= save_distance_min )
        {
            //memcpy(pbuf->raw, (short *)playback_read_pointer, pbuf->size);
            record_write_pointer = temp_buffer;
            playback_read_pointer = record_write_pointer + mid_buffer_distance;
            LOGE("[%s]: ********Throw a frame data away!!!!!!!!\n", __FUNCTION__);
        }
        else
        {
            unsigned int offset_bytes = 2*(end_temp_buffer-playback_read_pointer);
            unsigned int copy_bytes;

            if(offset_bytes >= pbuf->size)
            {
                memcpy(pbuf->raw, (short *)playback_read_pointer, pbuf->size);
                memset((void *)playback_read_pointer,0, pbuf->size);
                playback_read_pointer += pbuf->size/2;

                if(playback_read_pointer == end_temp_buffer)
                playback_read_pointer = temp_buffer;
            }
            else
            {
                memcpy(pbuf->raw, (short *)playback_read_pointer, offset_bytes);
                memset((void *)playback_read_pointer,0, offset_bytes);
                copy_bytes = offset_bytes;
                offset_bytes = pbuf->size - copy_bytes;
                playback_read_pointer = temp_buffer;
                memcpy((short *)pbuf->raw + (copy_bytes/2), (short *)playback_read_pointer, offset_bytes);
                memset((void *)playback_read_pointer,0, offset_bytes);
                playback_read_pointer += offset_bytes/2;
            }
            //DoDumpData(pbuf->raw, pbuf->size);
        }

        //LOGD("----------PlaybackCallback, pbuf->size:%d, pbuf->frameCount:%d\n", pbuf->size, pbuf->frameCount);
        //LOGD("----------Playback----offset_bytes:%d\n", 2*(playback_read_pointer-temp_buffer));

    } else if (AudioTrack::EVENT_UNDERRUN == event) {
        LOGE("[%s]: AudioTrack::EVENT_UNDERRUN\n", __FUNCTION__);
    } else if (AudioTrack::EVENT_LOOP_END == event) {
        LOGD("[%s]: AudioTrack::EVENT_LOOP_END\n", __FUNCTION__);
    } else if (AudioTrack::EVENT_MARKER == event) {
        LOGD("[%s]: AudioTrack::EVENT_MARKER\n", __FUNCTION__);
    } else if (AudioTrack::EVENT_NEW_POS == event) {
        LOGD("[%s]: AudioTrack::EVENT_NEW_POS\n", __FUNCTION__);
    } else if (AudioTrack::EVENT_BUFFER_END == event) {
        LOGD("[%s]: AudioTrack::EVENT_BUFFER_END\n", __FUNCTION__);
    }
//#endif // CC_AUD_SRC_IN_BUF_ANDROID
}


#if 0
static int get_aml_card(){
    int card = -1, err = 0;
    int fd = -1;
    unsigned fileSize = 512;
    char *read_buf = NULL, *pd = NULL;
    static const char *const SOUND_CARDS_PATH = "/proc/asound/cards";
    fd = open(SOUND_CARDS_PATH, O_RDONLY);
    if (fd < 0) {
        LOGE("ERROR: failed to open config file %s error: %d\n", SOUND_CARDS_PATH, errno);
        close(fd);
        return -EINVAL;
    }

    read_buf = (char *)malloc(fileSize);
    if (!read_buf) {
        LOGE("Failed to malloc read_buf");
        close(fd);
        return -ENOMEM;
    }
    memset(read_buf, 0x0, fileSize);
    err = read(fd, read_buf, fileSize);
    if (fd < 0) {
        LOGE("ERROR: failed to read config file %s error: %d\n", SOUND_CARDS_PATH, errno);
        close(fd);
        return -EINVAL;
    }
    pd = strstr(read_buf, "AML");
    card = *(pd - 3) - '0';

OUT:
    free(read_buf);
    close(fd);
    return card;
}


/*
0  :ADC
1  :HDMI
*/
static int audio_select_source(int flag){
#ifdef BOARD_ALSA_AUDIO_TINY
    struct mixer *pmixer = NULL;
    struct mixer_ctl *pctl;
    unsigned int card = -1;
    card = get_aml_card();
    LOGE("Default sound card is %d \n",card);
    pmixer = mixer_open(card);
    if (NULL == pmixer) {
        LOGE("[%s:%d] Failed to open mixer\n", __FUNCTION__, __LINE__);
        goto err_exit;
    }
    pctl = mixer_get_ctl_by_name(pmixer,"Audio In Source");
    if (NULL == pctl) {
        LOGE("[%s:%d] Failed to get mixer control for:%s\n", __FUNCTION__, __LINE__,"Audio In Source");
        goto err_exit;
    }
    if (mixer_ctl_set_value(pctl, 0, flag) != 0) {
        LOGE("[%s:%d] Failed to set value:%d\n", __FUNCTION__, __LINE__, flag);
        goto err_exit;
    }
    mixer_close(pmixer);
    return 0;
err_exit:
    if (NULL != pmixer) {
        mixer_close(pmixer);
    }
#endif
    return -1;
}
#endif

int mAlsaInit(int tm_sleep, int init_flag, int track_rate) {
#if CC_DISABLE_ALSA_MODULE == 0
    int tmp_ret;
    int record_rate = 48000;

    LOGD("Enter mAlsaInit function.\n");

#if CC_ALSA_HAS_MUTEX_LOCK == 1
    Mutex::Autolock _l(alsa_init_lock);
#endif

    mAlsaUninit(0);
    //audio_select_source(1);

    if (InitTempBuffer() != 0) {
        LOGE("[%s:%d] Failed to create temp_buffer!\n", __FUNCTION__, __LINE__);
        return 0;
    }
    property_set("sys.hdmiIn.Capture","true");
    if (init_flag & CC_FLAG_CREATE_RECORD) {
        LOGD("Start to create recorder.\n");

        LOGD("Recorder rate = %d.\n", record_rate);

        // create an AudioRecord object
        //glpRecorder = new AudioRecord();
        if (glpRecorder == NULL) {
            LOGE("Error creating AudioRecord instance: new AudioRecord class failed.\n");
            return -1;
        }

        tmp_ret = glpRecorder->set(AUDIO_SOURCE_DEFAULT, record_rate, AUDIO_FORMAT_PCM_16_BIT, AUDIO_CHANNEL_IN_STEREO, 0, 
        // (AudioRecord::record_flags) 0, // flags
                recorderCallback,// callback_t
                NULL,// void* user
                0, // notificationFrames,
                false, // threadCanCallJava
                0); // sessionId)

        tmp_ret = glpRecorder->initCheck();
        if (tmp_ret != NO_ERROR) {
            LOGE("Error creating AudioRecord instance: initialization check failed. status = %d\n", tmp_ret);
            FreeRecorder();
            return -1;
        }

        if (init_flag & CC_FLAG_START_RECORD) {
            glpRecorder->start();
        }

        LOGD(" create recorder finished.\n");
        if (init_flag & CC_FLAG_SOP_RECORD) {
            mAlsaStopRecorder();
            FreeRecorder();
            LOGD("stop recorder finish.\n");

        }  
    }

    if (init_flag & CC_FLAG_CREATE_TRACK) {
        LOGD("Start to create Tracker.\n");

        LOGD("Tracker rate = %d.\n", track_rate);

        glpTracker = new AudioTrack();
        if (glpTracker == 0) {
            LOGE("Error creating AudioTrack instance: new AudioTrack class failed.\n");
            FreeRecorder();
            return -1;
        }

        tmp_ret = glpTracker->set(AUDIO_STREAM_DEFAULT, track_rate, AUDIO_FORMAT_PCM_16_BIT, AUDIO_CHANNEL_OUT_STEREO, 0, // frameCount
                (audio_output_flags_t)0, // flags
                trackerCallback, 0, // user when callback
                0, // notificationFrames
                NULL, // shared buffer
                0);

        tmp_ret = glpTracker->initCheck();
        if (tmp_ret != NO_ERROR) {
            LOGE("Error creating AudioTrack instance: initialization check failed.status = %d\n", tmp_ret);
            FreeRecorder();
            FreeTracker();
            return -1;
        }

        if (init_flag & CC_FLAG_START_TRACK) {
            glpTracker->start();
        }

        LOGD("End to create Tracker.\n");
    }

    if (tm_sleep > 0)
        sleep(tm_sleep);

    LOGD("Exit mAlsaInit function sucess.\n");

    return 0;
#else
    return 0;
#endif
}

int mAlsaUninit(int tm_sleep) {
#if CC_DISABLE_ALSA_MODULE == 0
    LOGD("Enter mAlsaUninit function.\n");

#if CC_ALSA_HAS_MUTEX_LOCK == 1
    Mutex::Autolock _l(alsa_uninit_lock);
#endif
    LOGD("mAlsaUninit invoke audio_select_source.\n");
    //audio_select_source(0);
    property_set("sys.hdmiIn.Capture","false");
    // mAlsaStopRecorder();
    FreeRecorder();
    MuteTempBuffer();
    // mAlsaStopTracker();
    FreeTracker();
    FreeTempBuffer();
    // property_set("sys.hdmiIn.Capture","false");

    if (tm_sleep > 0)
        sleep(tm_sleep);

    LOGD("Exit mAlsaUninit function sucess.\n");

    return 0;
#else
    return 0;
#endif
}

int mAlsaStartRecorder(void) {
#if CC_DISABLE_ALSA_MODULE == 0
    if (glpRecorder != NULL) {
        glpRecorder->start();
        return 1;
    }

    return 0;
#else
    return 0;
#endif
}

int mAlsaStopRecorder(void) {
#if CC_DISABLE_ALSA_MODULE == 0
    if (glpRecorder != NULL) {
        glpRecorder->stop();
        return 1;
    }

    return 0;
#else
    return 0;
#endif
}

void mAlsaStartTracker(void) {
#if CC_DISABLE_ALSA_MODULE == 0
    if (glpTracker != NULL) {
        glpTracker->start();
    }
#endif
}

void mAlsaStopTracker(void) {
#if CC_DISABLE_ALSA_MODULE == 0
    if (glpTracker != NULL) {
        glpTracker->stop();
    }
#endif
}
