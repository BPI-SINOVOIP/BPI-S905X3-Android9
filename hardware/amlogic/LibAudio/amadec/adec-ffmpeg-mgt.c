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

#define LOG_TAG "amadec"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <dlfcn.h>

#include "audiodsp_update_format.h"
#include "adec_omx_brige.h"
#include "adec_reg.h"
#include <adec-pts-mgt.h>
#include <adec_write.h>
#include "Amsysfsutils.h"
#include "amconfigutils.h"

#ifndef USE_AOUT_IN_ADEC
#include <audio_dtv_ad.h>
#include <aml_hw_mixer.h>
#include <aml_volume_utils.h>
#endif
#include <audio-dec.h>
#include <adec-external-ctrl.h>
#include <amthreadpool.h>
#include <cutils/properties.h>

extern int read_buffer(unsigned char *buffer, int size);
void *audio_decode_loop(void *args);
void *audio_dtsdecode_loop(void *args);
void *audio_getpackage_loop(void *args);
void *ad_audio_getpackage_loop(void *args);
void *ad_audio_decode_loop(void *args);
static void stop_decode_thread(aml_audio_dec_t *audec);
#ifndef USE_AOUT_IN_ADEC
static int set_sysfs_int(const char *path, int val);
#define DTV_APTS_LOOKUP_PATH "/sys/class/tsync/apts_lookup"
#endif
#define AVCODEC_MAX_AUDIO_FRAME_SIZE 500*1024
#define AD_MIXER_BUF_SIZE  256 * 1024

/*audio decoder list structure*/
typedef struct {
    //enum CodecID codec_id;
    int codec_id;
    char    name[64];
} audio_lib_t;


#ifdef USE_AOUT_IN_ADEC
audio_lib_t audio_lib_list[] = {
    {ACODEC_FMT_AAC, "libfaad_sys.so"},
    {ACODEC_FMT_AAC_LATM, "libfaad_sys.so"},
    {ACODEC_FMT_APE, "libape.so"},
    {ACODEC_FMT_MPEG, "libmad_sys.so"},
    {ACODEC_FMT_MPEG2, "libmad_sys.so"},
    {ACODEC_FMT_MPEG1, "libmad_sys.so"},
    {ACODEC_FMT_FLAC, "libflac.so"},
    {ACODEC_FMT_COOK, "libcook.so"},
    {ACODEC_FMT_RAAC, "libraac.so"},
    {ACODEC_FMT_AMR, "libamr.so"},

    {ACODEC_FMT_PCM_S16BE, "libpcm.so"},
    {ACODEC_FMT_PCM_S16LE, "libpcm.so"},
    {ACODEC_FMT_PCM_U8, "libpcm.so"},
    {ACODEC_FMT_PCM_BLURAY, "libpcm.so"},
    {ACODEC_FMT_WIFIDISPLAY, "libpcm.so"},
    {ACODEC_FMT_ALAW, "libpcm.so"},
    {ACODEC_FMT_MULAW, "libpcm.so"},
    {ACODEC_FMT_ADPCM, "libadpcm.so"},
    {ACODEC_FMT_DRA, "libdra.so"}
} ;
#else
audio_lib_t audio_lib_list[] = {
    {ACODEC_FMT_AAC, "libfaad.so"},
    {ACODEC_FMT_AAC_LATM, "libfaad.so"},
    {ACODEC_FMT_APE, "libape.so"},
    {ACODEC_FMT_MPEG, "libmad.so"},
    {ACODEC_FMT_MPEG2, "libmad.so"},
    {ACODEC_FMT_MPEG1, "libmad.so"},
    {ACODEC_FMT_FLAC, "libflac.so"},
    {ACODEC_FMT_COOK, "libcook.so"},
    {ACODEC_FMT_RAAC, "libraac.so"},
    {ACODEC_FMT_AMR, "libamr.so"},

    {ACODEC_FMT_PCM_S16BE, "libpcm.so"},
    {ACODEC_FMT_PCM_S16LE, "libpcm.so"},
    {ACODEC_FMT_PCM_U8, "libpcm.so"},
    {ACODEC_FMT_PCM_BLURAY, "libpcm.so"},
    {ACODEC_FMT_WIFIDISPLAY, "libpcm.so"},
    {ACODEC_FMT_ALAW, "libpcm.so"},
    {ACODEC_FMT_MULAW, "libpcm.so"},
    {ACODEC_FMT_ADPCM, "libadpcm.so"},
    {ACODEC_FMT_DRA, "libdra.so"}
} ;
#endif

#ifndef USE_AOUT_IN_ADEC
static unsigned long adec_apts_lookup(unsigned long offset)
{
    unsigned int pts = 0;
    int ret;
    char buff[32];

    ret = amsysfs_set_sysfs_int(DTV_APTS_LOOKUP_PATH, offset);
    if (ret == 0) {
        ret = amsysfs_get_sysfs_str(DTV_APTS_LOOKUP_PATH, buff, sizeof(buff));
        if (ret == 0) {
            //adec_print("adec_apts_lookup   get pts %s\n",buff);
            ret = sscanf(buff, "0x%x\n", &pts);
        } else {
            adec_print("amsysfs_get_sysfs_str DTV_APTS_LOOKUP_PATH failed\n");
        }
    } else {
        adec_print("amsysfs_set_sysfs_int DTV_APTS_LOOKUP_PATH failed\n");
    }
    if (pts == (unsigned int) - 1) {
        pts = 0;
    }
    return (unsigned long)pts;
}
#endif

int find_audio_lib(aml_audio_dec_t *audec)
{
    int i;
    int num;
    audio_lib_t *f;

    adec_print("[%s %d]audec->format/%d audec->codec_id/0x%x\n", __FUNCTION__, __LINE__, audec->format, audec->codec_id);
    num = ARRAY_SIZE(audio_lib_list);
    audio_decoder_operations_t *adec_ops = audec->adec_ops;
#ifndef USE_AOUT_IN_ADEC
    audio_decoder_operations_t *ad_adec_ops = audec->ad_adec_ops;
#endif
    //-------------------------
    if (find_omx_lib(audec)) {
        return 0;
    }
    //-----------------------
    for (i = 0; i < num; i++) {
        f = &audio_lib_list[i];
        if (f->codec_id == audec->format) {
            void *fd = dlopen(audio_lib_list[i].name, RTLD_NOW);
            if (fd != 0) {
                adec_ops->init    = dlsym(fd, "audio_dec_init");
                adec_ops->decode  = dlsym(fd, "audio_dec_decode");
                adec_ops->release = dlsym(fd, "audio_dec_release");
                adec_ops->getinfo = dlsym(fd, "audio_dec_getinfo");

#ifndef USE_AOUT_IN_ADEC
                ad_adec_ops->init    = dlsym(fd, "audio_dec_init");
                ad_adec_ops->decode  = dlsym(fd, "audio_dec_decode");
                ad_adec_ops->release = dlsym(fd, "audio_dec_release");
                ad_adec_ops->getinfo = dlsym(fd, "audio_dec_getinfo");
#endif
            } else {
                adec_print("cant find decoder lib\n");
                return -1;
            }
            return 0;
        }
    }
    return -1;
}

static audio_decoder_operations_t AudioArmDecoder = {
    "FFmpegDecoder",
    AUDIO_ARM_DECODER,
    0,
    0,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    0,
    0,
    0,
    0,
    0,
    { 0 },
    0,
    0,
};
#ifndef USE_AOUT_IN_ADEC

static audio_decoder_operations_t ad_AudioArmDecoder = {
    "ad_FFmpegDecoder",
    AUDIO_ARM_DECODER,
    0,
    0,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    0,
    0,
    0,
    0,
    0,
    { 0 },
    0,
    0,
};
#endif

#ifdef USE_AOUT_IN_ADEC
static int FFmpegDecoderInit(audio_decoder_operations_t *adec_ops __unused)
{
    return 0;
}
static int FFmpegDecode(audio_decoder_operations_t *adec_ops __unused, char *outbuf __unused, int *outlen __unused, char *inbuf __unused, int inlen __unused)
{
    return 0;
}
static int FFmpegDecoderRelease(audio_decoder_operations_t *adec_ops __unused)
{
    //aml_audio_dec_t *audec = (aml_audio_dec_t *)(adec_ops->priv_data);
    return 0;
}
audio_decoder_operations_t AudioFFmpegDecoder = {
    .name = "FFmpegDecoder",
    .nAudioDecoderType = AUDIO_FFMPEG_DECODER,
    .init = FFmpegDecoderInit,
    .decode = FFmpegDecode,
    .release = FFmpegDecoderRelease,
    .getinfo = NULL,
};
#endif
int package_list_free(aml_audio_dec_t * audec)
{
    lp_lock(&(audec->pack_list.tslock));
    while (audec->pack_list.pack_num) {
        struct package * p = audec->pack_list.first;
        audec->pack_list.first = audec->pack_list.first->next;
        free(p->data);
        free(p);
        audec->pack_list.pack_num--;
    }
    lp_unlock(&(audec->pack_list.tslock));
    return 0;
}
static int64_t gettime_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}


int package_list_init(aml_audio_dec_t * audec)
{
    audec->pack_list.first = NULL;
    audec->pack_list.pack_num = 0;
    audec->pack_list.current = NULL;
    lp_lock_init(&(audec->pack_list.tslock), NULL);
    return 0;
}

int package_add(aml_audio_dec_t * audec, char * data, int size)
{
    lp_lock(&(audec->pack_list.tslock));
    if (audec->pack_list.pack_num == 4) { //enough
        lp_unlock(&(audec->pack_list.tslock));
        return -2;
    }
    struct package *p = malloc(sizeof(struct package));
    if (!p) { //malloc failed
        lp_unlock(&(audec->pack_list.tslock));
        return -1;
    }
    p->data = data;
    p->size = size;
    if (audec->pack_list.pack_num == 0) { //first package
        audec->pack_list.first = p;
        audec->pack_list.current = p;
        audec->pack_list.pack_num = 1;
    } else {
        audec->pack_list.current->next = p;
        audec->pack_list.current = p;
        audec->pack_list.pack_num++;
    }
    lp_unlock(&(audec->pack_list.tslock));
    return 0;
}

struct package * package_get(aml_audio_dec_t * audec)
{
    lp_lock(&(audec->pack_list.tslock));
    if (audec->pack_list.pack_num == 0) {
        lp_unlock(&(audec->pack_list.tslock));
        return NULL;
    }
    struct package *p = audec->pack_list.first;
    if (audec->pack_list.pack_num == 1) {
        audec->pack_list.first = NULL;
        audec->pack_list.pack_num = 0;
        audec->pack_list.current = NULL;
    } else if (audec->pack_list.pack_num > 1) {
        audec->pack_list.first = audec->pack_list.first->next;
        audec->pack_list.pack_num--;
    }
    lp_unlock(&(audec->pack_list.tslock));
    return p;
}
#ifndef USE_AOUT_IN_ADEC
int ad_package_list_free(aml_audio_dec_t * audec)
{
    lp_lock(&(audec->ad_pack_list.tslock));
    while (audec->ad_pack_list.pack_num) {
        struct package * p = audec->ad_pack_list.first;
        audec->ad_pack_list.first = audec->ad_pack_list.first->next;
        free(p->data);
        free(p);
        audec->ad_pack_list.pack_num--;
    }
    lp_unlock(&(audec->ad_pack_list.tslock));
    return 0;
}

int ad_package_list_init(aml_audio_dec_t * audec)
{
    audec->ad_pack_list.first = NULL;
    audec->ad_pack_list.pack_num = 0;
    audec->ad_pack_list.current = NULL;
    lp_lock_init(&(audec->ad_pack_list.tslock), NULL);
    return 0;
}

int ad_package_add(aml_audio_dec_t * audec, char * data, int size)
{
    lp_lock(&(audec->ad_pack_list.tslock));
    if (audec->ad_pack_list.pack_num == 8) { //enough
        lp_unlock(&(audec->ad_pack_list.tslock));
        return -2;
    }
    struct package *p = malloc(sizeof(struct package));
    if (!p) { //malloc failed
        lp_unlock(&(audec->ad_pack_list.tslock));
        return -1;
    }
    p->data = data;
    p->size = size;
    if (audec->ad_pack_list.pack_num == 0) { //first package
        audec->ad_pack_list.first = p;
        audec->ad_pack_list.current = p;
        audec->ad_pack_list.pack_num = 1;
    } else {
        audec->ad_pack_list.current->next = p;
        audec->ad_pack_list.current = p;
        audec->ad_pack_list.pack_num++;
    }
    lp_unlock(&(audec->ad_pack_list.tslock));
    return 0;
}

struct package * ad_package_get(aml_audio_dec_t * audec)
{
    lp_lock(&(audec->ad_pack_list.tslock));
    if (audec->ad_pack_list.pack_num == 0) {
        lp_unlock(&(audec->ad_pack_list.tslock));
        return NULL;
    }
    struct package *p = audec->ad_pack_list.first;
    if (audec->ad_pack_list.pack_num == 1) {
        audec->ad_pack_list.first = NULL;
        audec->ad_pack_list.pack_num = 0;
        audec->ad_pack_list.current = NULL;
    } else if (audec->ad_pack_list.pack_num > 1) {
        audec->ad_pack_list.first = audec->ad_pack_list.first->next;
        audec->ad_pack_list.pack_num--;
    }
    lp_unlock(&(audec->ad_pack_list.tslock));
    return p;
}
#endif


int armdec_stream_read(dsp_operations_t *dsp_ops, char *buffer, int size)
{
    int read_size = 0;
    aml_audio_dec_t *audec = (aml_audio_dec_t *)dsp_ops->audec;
    read_size = read_pcm_buffer(buffer, audec->g_bst, size);
    audec->out_len_after_last_valid_pts += read_size;
    return read_size;
}

int armdec_stream_read_raw(dsp_operations_t *dsp_ops, char *buffer, int size)
{
    aml_audio_dec_t *audec = (aml_audio_dec_t *)dsp_ops->audec;
    return read_pcm_buffer(buffer, audec->g_bst_raw, size);
}

#ifdef USE_AOUT_IN_ADEC
unsigned long  armdec_get_pts(dsp_operations_t *dsp_ops)
{
    unsigned long val = 0, offset;
    unsigned long pts;
    int data_width, channels, samplerate;
    unsigned long long frame_nums ;
    unsigned long delay_pts;
    char value[PROPERTY_VALUE_MAX];
    aml_audio_dec_t *audec = (aml_audio_dec_t *)dsp_ops->audec;
    audio_out_operations_t * aout_ops = &audec->aout_ops;
    float  track_speed = 1.0f;
    if (aout_ops != NULL) {
        if (aout_ops->track_rate != 8.8f) {
            track_speed = aout_ops->track_rate;
        }
    }
    switch (audec->g_bst->data_width) {
    case AV_SAMPLE_FMT_U8:
        data_width = 8;
        break;
    case AV_SAMPLE_FMT_S16:
        data_width = 16;
        break;
    case AV_SAMPLE_FMT_S32:
        data_width = 32;
        break;
    default:
        data_width = 16;
    }

    int pts_delta = 0;
    if (property_get("media.libplayer.pts_delta", value, NULL) > 0) {
        pts_delta = atoi(value);
    }

    channels = audec->g_bst->channels;
    samplerate = audec->g_bst->samplerate;
    if (!channels || !samplerate) {
        adec_print("warning ::::zero  channels %d, sample rate %d \n", channels, samplerate);
        if (!samplerate) {
            samplerate = 48000;
        }
        if (!channels) {
            channels = 2;
        }
    }
    offset = audec->decode_offset;
    if (dsp_ops->dsp_file_fd >= 0) {
        if (audec->g_bst->format != ACODEC_FMT_COOK && audec->g_bst->format != ACODEC_FMT_RAAC) {
            //when first  look up apts,set offset 0
            if (!audec->first_apts_lookup_over) {
                offset = 0;
            }
            ioctl(dsp_ops->dsp_file_fd, AMSTREAM_IOC_APTS_LOOKUP, &offset);
        }
        //for cook/raac should wait to get first apts from decoder
        else {
            int wait_count = 10;
            while (offset == 0xffffffff && wait_count-- > 0) {
                amthreadpool_thread_usleep(10000);
            }
            offset = audec->decode_offset;
            if (offset == 0xffffffff) {
                adec_print(" cook/raac get apts 100 ms timeout \n");
            }

        }
    } else {
        //adec_print("====abuf have not open!\n");
    }

    if (am_getconfig_bool("media.arm.audio.apts_add")) {
        offset = 0;
    }
    pts = offset;
    if (!audec->first_apts_lookup_over) {
        audec->last_valid_pts = pts;
        audec->first_apts_lookup_over = 1;
        return pts;
    }

    if (audec->use_get_out_posion && audec->aout_ops.get_out_position) {
        /*add by zz*/
        int64_t postion, time_us;
        struct timespec timenow;
        int ret;
        ret = audec->aout_ops.get_out_position(audec, &postion, &time_us);
        if (!ret) {
            int decodered_samples;
            int cache_samples;
            int delay_us, t_us;
            decodered_samples = audec->decode_pcm_offset * 8 / (channels * data_width);
            cache_samples = decodered_samples - postion;
            if (cache_samples < 0) {
                cache_samples = 0;
            }
            delay_us = 1000 * (cache_samples * 1000 / samplerate);
            clock_gettime(CLOCK_MONOTONIC, &timenow);
            t_us = timenow.tv_sec * 1000000LL + timenow.tv_nsec / 1000 - time_us;
            if (t_us > 0) {
                delay_us -= t_us;
            }
            if (delay_us < 0) {
                delay_us = 0;
            }
            delay_pts = delay_us * 90 / 1000;
            if (pts == 0) {
                int outsamples = postion - audec->last_out_postion ;
                /*delay_us out samples after last refresh pts*/
                delay_us = 1000 * (outsamples * 1000 / samplerate);
                delay_us = delay_us * track_speed;
                if (delay_us < 0) {
                    delay_us = 0;
                }
                pts = audec->last_valid_pts + delay_us * 90 / 1000;
                audec->last_valid_pts = pts;
                pts += 90000 / 1000 * pts_delta;
                audec->last_out_postion = postion;
                audec->last_get_postion_time_us = time_us;
                return pts;
            }
            audec->last_out_postion = postion;
            audec->last_get_postion_time_us = time_us;
        } else {
            delay_pts = 0;/*audio track not ready? used buf_level add for pts*/
        }
    } else {
        delay_pts = 0;
    }
    if (delay_pts == 0) {
        if (pts == 0) {
            if (audec->last_valid_pts) {
                pts = audec->last_valid_pts;
            }
            frame_nums = (audec->out_len_after_last_valid_pts * 8 / (data_width * channels));
            pts += (frame_nums * 90000 / samplerate);
            pts += 90000 / 1000 * pts_delta;
            //if (pts < 0)
            //   pts = 0;
            //adec_print("decode_offset:%d out_pcm:%d   pts:%d \n",decode_offset,out_len_after_last_valid_pts,pts);
            return pts;
        }
        {
            int len = audec->g_bst->buf_level + audec->pcm_cache_size;
            frame_nums = (len * 8 / (data_width * channels));
            delay_pts = (frame_nums * 90000 / samplerate);
        }
    }
    delay_pts = delay_pts * track_speed;
    if (pts > delay_pts) {
        pts -= delay_pts;
    } else {
        pts = 0;
    }
    val = pts;
    audec->last_valid_pts = pts;
    audec->out_len_after_last_valid_pts = 0;
    //adec_print("====get pts:%ld offset:%ld frame_num:%lld delay:%ld \n",val,decode_offset,frame_nums,delay_pts);

    val += 90000 / 1000 * pts_delta; // for a/v sync test,some times audio ahead video +28ms.so add +15ms to apts to .....
    //if (val < 0)
    //   val = 0;
    return val;
}
#else
unsigned long  armdec_get_pts(dsp_operations_t *dsp_ops)
{
    unsigned long val = 0, offset;
    unsigned long pts;
    int data_width, channels, samplerate;
    unsigned long long frame_nums ;
    char value[PROPERTY_VALUE_MAX];
    aml_audio_dec_t *audec = (aml_audio_dec_t *)dsp_ops->audec;
    audio_out_operations_t * aout_ops = &audec->aout_ops;
    float  track_speed = 1.0f;
    if (aout_ops != NULL) {
        if (aout_ops->track_rate != 8.8f) {
            track_speed = aout_ops->track_rate;
        }
    }
    switch (audec->g_bst->data_width) {
    case AV_SAMPLE_FMT_U8:
        data_width = 8;
        break;
    case AV_SAMPLE_FMT_S16:
        data_width = 16;
        break;
    case AV_SAMPLE_FMT_S32:
        data_width = 32;
        break;
    default:
        data_width = 16;
    }

    int pts_delta = 0;
    if (property_get("media.libplayer.pts_delta", value, NULL) > 0) {
        pts_delta = atoi(value);
    }

    channels = audec->g_bst->channels;
    samplerate = audec->g_bst->samplerate;
    if (!channels || !samplerate) {
        adec_print("warning ::::zero  channels %d, sample rate %d \n", channels, samplerate);
        if (!samplerate) {
            samplerate = 48000;
        }
        if (!channels) {
            channels = 2;
        }
    }
    offset = audec->decode_offset;
    if (dsp_ops->dsp_file_fd >= 0) {
        if (audec->g_bst->format != ACODEC_FMT_COOK && audec->g_bst->format != ACODEC_FMT_RAAC) {
            //when first  look up apts,set offset 0
            if (!audec->first_apts_lookup_over) {
                offset = 0;
            }
            ioctl(dsp_ops->dsp_file_fd, AMSTREAM_IOC_APTS_LOOKUP, &offset);
        }
        //for cook/raac should wait to get first apts from decoder
        else {
            int wait_count = 10;
            while (offset == 0xffffffff && wait_count-- > 0) {
                amthreadpool_thread_usleep(10000);
            }
            offset = audec->decode_offset;
            if (offset == 0xffffffff) {
                adec_print(" cook/raac get apts 100 ms timeout \n");
            }

        }
    } else {

        if (!audec->first_apts_lookup_over) {
            offset = 0;
            adec_print("====first_apts_lookup_over:%lx offset:%ld \n", val, offset);
        } else {
            offset  = audec->decode_offset;
        }

        pts =  adec_apts_lookup(offset);
    }

    if (!audec->first_apts_lookup_over) {
        adec_print("====get pts:%lx offset:%ld \n", pts, offset);
        audec->last_valid_pts = pts;
        audec->first_apts_lookup_over = 1;
        return pts;
    }
    {
        if ((pts == 0) || (audec->last_valid_pts == pts)) {
            pts = (unsigned long) - 1;
            return pts;
        }
    }

    audec->last_valid_pts = pts;
    frame_nums = (audec->g_bst->buf_level * 8 / (data_width * channels));
    pts -= (frame_nums * 90000 / samplerate);
    val = pts;
    audec->out_len_after_last_valid_pts = 0;
    adec_print("====get pts:%lx offset:%ld frame_num:%lld \n", val, audec->decode_offset, frame_nums);

    val += 90000 / 1000 * pts_delta; // for a/v sync test,some times audio ahead video +28ms.so add +15ms to apts to .....
    //if (val < 0)
    //   val = 0;
    return val;
}
#endif
unsigned long  armdec_get_pcrscr(dsp_operations_t *dsp_ops)
{
    unsigned int val;
    if (dsp_ops->dsp_file_fd < 0) {
        //adec_print("read error!! audiodsp have not opened\n");
        return -1;
    }
    ioctl(dsp_ops->dsp_file_fd, AMSTREAM_IOC_PCRSCR, &val);
    return val;
}
int armdec_set_pts(dsp_operations_t *dsp_ops, unsigned long apts)
{
    if (dsp_ops->dsp_file_fd < 0) {
        adec_print("armdec_set_apts err!\n");
        return -1;
    }
    ioctl(dsp_ops->dsp_file_fd, AMSTREAM_IOC_SET_APTS, &apts);
    return 0;
}
int armdec_set_skip_bytes(dsp_operations_t* dsp_ops __unused, unsigned int bytes __unused)
{
    return  0;
}
#ifndef USE_AOUT_IN_ADEC
static int set_sysfs_int(const char *path, int val)
{
    return amsysfs_set_sysfs_int(path, val);
}
#endif
int get_decoder_status(void *p, struct adec_status *adec)
{
    aml_audio_dec_t *audec = (aml_audio_dec_t *)p;
    if (audec && audec->g_bst) {
        adec->channels = audec->g_bst->channels;
        adec->sample_rate = audec->g_bst->samplerate;
        adec->resolution = audec->g_bst->data_width;
        adec->error_count = audec->nDecodeErrCount; //need count
        adec->status = (audec->state > INITTED) ? 1 : 0;
        return 0;
    } else {
        return -1;
    }
}

int get_decoder_info(void *p)
{
    aml_audio_dec_t *audec = (aml_audio_dec_t *)p;
    int ret = -1;
    if (audec && audec->adec_ops) {
        AudioInfo   g_AudioInfo = {0};
        audio_decoder_operations_t *adec_ops  = audec->adec_ops;
        if (audec->format == ACODEC_FMT_AAC || audec->format == ACODEC_FMT_MPEG ||
                audec->format == ACODEC_FMT_MPEG1 || audec->format == ACODEC_FMT_MPEG2 ||
                audec->format == ACODEC_FMT_AAC_LATM) {
            adec_ops->getinfo(audec->adec_ops, &g_AudioInfo);
            audec->error_num = g_AudioInfo.error_num; //need count
            ret = 0;
        }else if (audec->format == ACODEC_FMT_AC3 || audec->format == ACODEC_FMT_EAC3){
            audec->error_num = audec->error_nb_frames; //need count
            ret = 0;
        }
        //adec_print("get_decoder_info: type :%d,error_num:%d\n",audec->format,audec->error_num);
    }
    return ret;
}

/**
 * \brief register audio decoder
 * \param audec pointer to audec ,codec_type
 * \return 0 on success otherwise -1 if an error occurred
 */
#ifdef USE_AOUT_IN_ADEC
 int RegisterDecode(aml_audio_dec_t *audec, int type)
{
    switch (type) {
    case AUDIO_ARM_DECODER:
        memset(&AudioArmDecoder, 0, sizeof(audio_decoder_operations_t));
        audec->adec_ops = &AudioArmDecoder;
        find_audio_lib(audec);
        audec->adec_ops->priv_data = audec;
        break;
    case AUDIO_FFMPEG_DECODER:
        audec->adec_ops = &AudioFFmpegDecoder;
        audec->adec_ops->priv_data = audec;
        break;
    default:
        audec->adec_ops = &AudioFFmpegDecoder;
        audec->adec_ops->priv_data = audec;
        break;
    }
    return 0;
}
#else
int RegisterDecode(aml_audio_dec_t *audec)
{
    memset(&AudioArmDecoder, 0, sizeof(audio_decoder_operations_t));
    audec->adec_ops = &AudioArmDecoder;
#ifndef USE_AOUT_IN_ADEC
    memset(&ad_AudioArmDecoder, 0, sizeof(audio_decoder_operations_t));
    audec->ad_adec_ops = &ad_AudioArmDecoder;
#endif
    find_audio_lib(audec);
    audec->adec_ops->priv_data = audec;

    return 0;
}
#endif
static int InBufferInit(aml_audio_dec_t *audec)
{
    int ret = uio_init(audec);
    if (ret < 0) {
        adec_print("uio init error! \n");
        return -1;
    }
    return 0;
}
static int InBufferRelease(aml_audio_dec_t *audec)
{
    //    close(audec->fd_uio);
    //    audec->fd_uio=-1;
    uio_deinit(audec);
    return 0;
}


static int OutBufferInit(aml_audio_dec_t *audec)
{
    audec->g_bst = malloc(sizeof(buffer_stream_t));
    if (!audec->g_bst) {
        adec_print("[%s %d]g_bst malloc failed! \n", __FUNCTION__, __LINE__);
        audec->g_bst = NULL;
        return -1;
    } else {
        adec_print("[%s %d] audec->g_bst/%p", __FUNCTION__, __LINE__, audec->g_bst);
    }

    memset(audec->g_bst, 0, sizeof(buffer_stream_t));

    if (audec->adec_ops->nOutBufSize <= 0) { //set default if not set
        audec->adec_ops->nOutBufSize = DEFAULT_PCM_BUFFER_SIZE;
    }

    int ret = init_buff(audec->g_bst, audec->adec_ops->nOutBufSize);
    if (ret == -1) {
        adec_print("[%s %d]pcm buffer init failed !\n", __FUNCTION__, __LINE__);
        return -1;
    }
    adec_print("[%s %d]pcm buffer init ok buf_size:%d\n", __FUNCTION__, __LINE__, audec->g_bst->buf_length);

    audec->g_bst->data_width = audec->data_width = AV_SAMPLE_FMT_S16;

    if (audec->channels > 0) {
        audec->g_bst->channels = audec->channels;
    }
    if (audec->samplerate > 0) {
        audec->g_bst->samplerate = audec->samplerate;
    }
    audec->g_bst->format = audec->format;

    return 0;
}
static int OutBufferInit_raw(aml_audio_dec_t *audec)
{
    audec->g_bst_raw = malloc(sizeof(buffer_stream_t));
    if (!audec->g_bst_raw) {
        adec_print("[%s %d]g_bst_raw malloc failed!\n", __FUNCTION__, __LINE__);
        audec->g_bst_raw = NULL;
        return -1;
    } else {
        adec_print("[%s %d] audec->audec->g_bst_raw/%p", __FUNCTION__, __LINE__, audec->g_bst_raw);
    }

    if (audec->adec_ops->nOutBufSize <= 0) { //set default if not set
        audec->adec_ops->nOutBufSize = DEFAULT_PCM_BUFFER_SIZE;
    }
    if ((audec->format == ACODEC_FMT_DTS || audec->format == ACODEC_FMT_TRUEHD) && amsysfs_get_sysfs_int("/sys/class/audiodsp/digital_raw") == 2) {
        audec->adec_ops->nOutBufSize *= 2;
    }
    int ret = init_buff(audec->g_bst_raw, audec->adec_ops->nOutBufSize);
    if (ret == -1) {
        adec_print("[%s %d]raw_buf init failed !\n", __FUNCTION__, __LINE__);
        return -1;
    }
    adec_print("[%s %d]raw buffer init ok buf_size/%d\n", __FUNCTION__, __LINE__, audec->g_bst_raw->buf_length);

    audec->g_bst_raw->data_width = audec->data_width = AV_SAMPLE_FMT_S16;
    if (audec->channels > 0) {
        audec->g_bst_raw->channels = audec->channels;
    } else {
        audec->g_bst_raw->channels = audec->channels = 2;
    }

    if (audec->samplerate > 0) {
        audec->g_bst_raw->samplerate = audec->samplerate;
    } else {
        audec->g_bst_raw->samplerate = audec->samplerate = 48000;
    }

    return 0;
}
static int OutBufferRelease(aml_audio_dec_t *audec)
{
    if (audec->g_bst) {
        adec_print("[%s %d] audec->g_bst/%p", __FUNCTION__, __LINE__, audec->g_bst);
        release_buffer(audec->g_bst);
        audec->g_bst = NULL;
    }
    return 0;
}

static int OutBufferRelease_raw(aml_audio_dec_t *audec)
{
    if (audec->g_bst_raw) {
        adec_print("[%s %d] audec->g_bst_raw/%p", __FUNCTION__, __LINE__, audec->g_bst_raw);
        release_buffer(audec->g_bst_raw);
        audec->g_bst_raw = NULL;
    }
    return 0;
}

static int enable_raw_output(aml_audio_dec_t *audec)
{
    int enable = 0;
    enable = amsysfs_get_sysfs_int("/sys/class/audiodsp/digital_raw");
    if (enable) {
        if (audec->format == ACODEC_FMT_AC3 || audec->format == ACODEC_FMT_EAC3 || audec->format == ACODEC_FMT_DTS ||
            audec->format == ACODEC_FMT_TRUEHD) {
            return 1;
        }
    }
    return 0;
}
static int audio_codec_init(aml_audio_dec_t *audec)
{
    //reset static&global
    audec->exit_decode_thread = 0;
    audec->exit_decode_thread_success = 0;
    audec->decode_offset = 0;
    audec->decode_pcm_offset = 0;
    audec->nDecodeErrCount = 0;
    audec->g_bst = NULL;
    audec->g_bst_raw = NULL;
    audec->fd_uio = -1;
    audec->last_valid_pts = 0;
    audec->out_len_after_last_valid_pts = 0;
    audec->pcm_cache_size = 0;
    audec->sn_threadid = -1;
    audec->sn_getpackage_threadid = -1;
    audec->OmxFirstFrameDecoded = 0;
    audec->use_get_out_posion = am_getconfig_bool_def("media.audio.pts.use_get_posion", 0);
    package_list_init(audec);
#ifndef USE_AOUT_IN_ADEC
    ad_package_list_init(audec);
    audec->ad_sn_threadid = -1;
    audec->ad_sn_getpackage_threadid = -1;
    audec->hw_mixer.buf_size = AD_MIXER_BUF_SIZE;
    aml_hw_mixer_init(&audec->hw_mixer);
    while (0 != set_sysfs_int(DECODE_ERR_PATH, DECODE_NONE_ERR)) {
        adec_print("[%s %d]set codec fatal failed ! \n", __FUNCTION__, __LINE__);
        amthreadpool_thread_usleep(100000);
    }
#endif
    adec_print("[%s %d]param:data_width:%d samplerate:%d channel:%d \n",
               __FUNCTION__, __LINE__, audec->data_width, audec->samplerate, audec->channels);

    audec->data_width = AV_SAMPLE_FMT_S16;
    if (audec->channels > 0) {
        int NumChSave = audec->channels;
        audec->channels = (audec->channels > 2 ? 2 : audec->channels);
        audec->adec_ops->channels = audec->channels;
        if (audec->format == ACODEC_FMT_PCM_S16BE  || audec->format == ACODEC_FMT_PCM_S16LE  ||
            audec->format == ACODEC_FMT_PCM_U8     || audec->format == ACODEC_FMT_PCM_BLURAY ||
            audec->format == ACODEC_FMT_WIFIDISPLAY || audec->format == ACODEC_FMT_ALAW       ||
            audec->format == ACODEC_FMT_MULAW      || audec->format == ACODEC_FMT_ADPCM) {
            audec->adec_ops->channels = NumChSave;
        }
    }
    //for raac/cook audio pts are updated from audio decoder,so set a invalid pts default.
    else if (audec->format == ACODEC_FMT_RAAC || audec->format == ACODEC_FMT_COOK) {
        audec->decode_offset = 0xffffffff;
    }
    if (audec->samplerate > 0) {
        audec->adec_ops->samplerate = audec->samplerate;
    } else {
        //   audec->adec_ops->samplerate=audec->samplerate=48000;
    }
    switch (audec->data_width) {
    case AV_SAMPLE_FMT_U8:
        audec->adec_ops->bps = 8;
        break;
    case AV_SAMPLE_FMT_S16:
        audec->adec_ops->bps = 16;
        break;
    case AV_SAMPLE_FMT_S32:
        audec->adec_ops->bps = 32;
        break;
    default:
        audec->adec_ops->bps = 16;
    }
    adec_print("[%s %d]param_applied: bps:%d samplerate:%d channel:%d \n",
               __FUNCTION__, __LINE__, audec->adec_ops->bps, audec->adec_ops->samplerate, audec->adec_ops->channels);

    audec->adec_ops->extradata_size = audec->extradata_size;
    if (audec->extradata_size > 0) {
        memcpy(audec->adec_ops->extradata, audec->extradata, audec->extradata_size);
    }

    int ret = 0;
    if (!audec->StageFrightCodecEnableType) { //1-decoder init
#ifndef USE_AOUT_IN_ADEC
        if (audec->ad_adec_ops->init != NULL) {
            ret = audec->ad_adec_ops->init(audec->ad_adec_ops);
        }
#endif
        if (audec->adec_ops->init != NULL) {
            ret = audec->adec_ops->init(audec->adec_ops);
        }
    }
    if (ret == -1) {
        adec_print("[%s %d]adec_ops init err\n", __FUNCTION__, __LINE__);
        goto err1;
    }

    ret = OutBufferInit(audec); //2-pcm_buffer init
    if (ret == -1) {
        adec_print("[%s %d]out buffer init err\n", __FUNCTION__, __LINE__);
        goto err2;
    }
    if (enable_raw_output(audec)) {
        ret = OutBufferInit_raw(audec);
        if (ret == -1) {
            adec_print("[%s %d]out_raw buffer init err\n", __FUNCTION__, __LINE__);
            OutBufferRelease_raw(audec);
            goto err2;
        }
    }
    ret = InBufferInit(audec); //3-init uio
    if (ret == -1) {
        adec_print("====in buffer  init err \n");
        goto err3;
    }

    //4-other init
    audec->adsp_ops.dsp_on = 1;
    audec->adsp_ops.dsp_read = armdec_stream_read;
    audec->adsp_ops.get_cur_pts = armdec_get_pts;
    audec->adsp_ops.get_cur_pcrscr =  armdec_get_pcrscr;
    audec->adsp_ops.set_cur_apts    = armdec_set_pts;
    audec->adsp_ops.set_skip_bytes = armdec_set_skip_bytes;
    audec->adsp_ops.dsp_read_raw = armdec_stream_read_raw;
    audec->pcm_bytes_readed = 0;
    audec->raw_bytes_readed = 0;
    audec->raw_frame_size = 0;
    audec->pcm_frame_size = 0;
    audec->i2s_iec958_sync_flag = 1;
    audec->i2s_iec958_sync_gate = 0;
    audec->codec_type = 0;
    return 0;

err1:
    audec->adec_ops->release(audec->adec_ops);
#ifndef USE_AOUT_IN_ADEC
    audec->ad_adec_ops->release(audec->ad_adec_ops);
#endif
    return -1;
err2:
    audec->adec_ops->release(audec->adec_ops);
#ifndef USE_AOUT_IN_ADEC
    audec->ad_adec_ops->release(audec->ad_adec_ops);
#endif
    OutBufferRelease(audec);
    return -1;
err3:
    if (audec->adec_ops->release != NULL) {
        audec->adec_ops->release(audec->adec_ops);
    }
#ifndef USE_AOUT_IN_ADEC
    if (audec->ad_adec_ops->release != NULL) {
        audec->ad_adec_ops->release(audec->ad_adec_ops);
    }
#endif
    OutBufferRelease(audec);
    InBufferRelease(audec);
    OutBufferRelease_raw(audec);
    return -1;
}
int audio_codec_release(aml_audio_dec_t *audec)
{
    //1-decode thread quit
    if (!audec->StageFrightCodecEnableType) {
        stop_decode_thread(audec);//1-decode thread quit
        if (audec->adec_ops->release != NULL) {
            audec->adec_ops->release(audec->adec_ops);    //2-decoder release
        }
#ifndef USE_AOUT_IN_ADEC
        if (audec->ad_adec_ops->release != NULL) {
            audec->ad_adec_ops->release(audec->ad_adec_ops);    //2-decoder release
        }
        aml_hw_mixer_deinit(&audec->hw_mixer);
#endif

    } else {
        stop_decode_thread_omx(audec);
    }

    InBufferRelease(audec);//3-uio uninit
    OutBufferRelease(audec);//4-outbufferrelease
    OutBufferRelease_raw(audec);
    audec->adsp_ops.dsp_on = -1;//5-other release
    audec->adsp_ops.dsp_read = NULL;
    audec->adsp_ops.get_cur_pts = NULL;
    audec->adsp_ops.dsp_file_fd = -1;

    return 0;
}


static int audio_hardware_ctrl(hw_command_t cmd)
{
    int fd;
    fd = open(AUDIO_CTRL_DEVICE, O_RDONLY);
    if (fd < 0) {
        adec_print("Open Device %s Failed!", AUDIO_CTRL_DEVICE);
        return -1;
    }

    switch (cmd) {
    case HW_CHANNELS_SWAP:
        ioctl(fd, AMAUDIO_IOC_SET_CHANNEL_SWAP, 0);
        break;

    case HW_LEFT_CHANNEL_MONO:
        ioctl(fd, AMAUDIO_IOC_SET_LEFT_MONO, 0);
        break;

    case HW_RIGHT_CHANNEL_MONO:
        ioctl(fd, AMAUDIO_IOC_SET_RIGHT_MONO, 0);
        break;

    case HW_STEREO_MODE:
        ioctl(fd, AMAUDIO_IOC_SET_STEREO, 0);
        break;

    default:
        adec_print("Unknow Command %d!", cmd);
        break;

    };

    close(fd);
    return 0;
}

static int get_first_apts_flag(dsp_operations_t *dsp_ops)
{
    int val;
    if (dsp_ops->dsp_file_fd < 0) {
        adec_print("[%s %d]read error!! audiodsp have not opened\n", __FUNCTION__, __LINE__);
        return -1;
    }
    ioctl(dsp_ops->dsp_file_fd, GET_FIRST_APTS_FLAG, &val);
    return val;
}


/**
 * \brief start audio dec when receive START command.
 * \param audec pointer to audec
 */
static int start_adec(aml_audio_dec_t *audec)
{
    #ifdef USE_AOUT_IN_ADEC
    int ret;
    audio_out_operations_t *aout_ops = &audec->aout_ops;
    #endif
    dsp_operations_t *dsp_ops = &audec->adsp_ops;
    unsigned long  vpts, apts;
    int times = 0;
    char buf[32];
    apts = vpts = 0;
    audec->no_first_apts = 0;
    audec->apts_start_flag = 0;
    audec->first_apts = 0;

    char value[PROPERTY_VALUE_MAX] = {0};
    int wait_count = 100;
    if (property_get("media.amadec.wait_count", value, NULL) > 0) {
        wait_count = atoi(value);
    }
    adec_print("wait first apts count :%d \n", wait_count);

    if (audec->state == INITTED) {
        //get info from the audiodsp == can get from amstreamer
        while ((!get_first_apts_flag(dsp_ops)) && (!audec->need_stop) && (!audec->no_first_apts)) {
            adec_print("wait first pts checkin complete !");
            if (amthreadpool_on_requare_exit(pthread_self())) {
                adec_print("[%s:%d] quick interrupt \n", __FUNCTION__, __LINE__);
                break;
            }
            times++;
            if (times >= wait_count) {
                amsysfs_get_sysfs_str(TSYNC_VPTS, buf, sizeof(buf));// read vpts
                if (sscanf(buf, "0x%lx", &vpts) < 1) {
                    adec_print("unable to get vpts from: %s", buf);
                    return -1;
                }
                // save vpts to apts
                if (vpts == 0) { // vpts invalid too
                    times = 0; // loop again
                    continue;
                }
                adec_print("## can't get first apts, save vpts to apts,vpts=%lx, \n", vpts);
                sprintf(buf, "0x%lx", vpts);
                amsysfs_set_sysfs_str(TSYNC_APTS, buf);
                audec->no_first_apts = 1;
            }
            amthreadpool_thread_usleep(100000);
        }
        adec_print("get first apts ok, times:%d need_stop:%d auto_mute %d\n", times, audec->need_stop, audec->auto_mute);
        if (audec->need_stop) {
            return 0;
        }

        /*start  the  the pts scr,...*/
#ifdef USE_AOUT_IN_ADEC
        ret = adec_pts_start(audec);
#endif
        if (audec->auto_mute) {
#ifdef USE_AOUT_IN_ADEC
            avsync_en(0);
            adec_pts_pause();
#endif
            while ((!audec->need_stop) && track_switch_pts(audec)) {
                amthreadpool_thread_usleep(1000);
            }
#ifdef USE_AOUT_IN_ADEC
            avsync_en(1);
            adec_pts_resume();
#endif
            audec->auto_mute = 0;
        }
        if (audec->tsync_mode == TSYNC_MODE_PCRMASTER) {
            adec_print("[wcs-%s]-before audio track start,sleep 200ms\n", __FUNCTION__);
            amthreadpool_thread_usleep(200 * 1000); //200ms
        }
        #ifdef USE_AOUT_IN_ADEC
        aout_ops->start(audec);
        #endif
        audec->state = ACTIVE;
    } else {
        adec_print("amadec status invalid, start adec failed \n");
        return -1;
    }

    return 0;
}

/**
 * \brief pause audio dec when receive PAUSE command.
 * \param audec pointer to audec
 */
static void pause_adec(aml_audio_dec_t *audec)
{
    #ifdef USE_AOUT_IN_ADEC
    audio_out_operations_t *aout_ops = &audec->aout_ops;
    #endif
    if (audec->state == ACTIVE) {
        audec->state = PAUSED;
        adec_pts_pause();
        #ifdef USE_AOUT_IN_ADEC
        aout_ops->pause(audec);
        #endif
    }
}

/**
 * \brief resume audio dec when receive RESUME command.
 * \param audec pointer to audec
 */
static void resume_adec(aml_audio_dec_t *audec)
{
    #ifdef USE_AOUT_IN_ADEC
    audio_out_operations_t *aout_ops = &audec->aout_ops;
    #endif
    if (audec->state == PAUSED) {
        audec->state = ACTIVE;
        audec->refresh_pts_readytime_ms = gettime_ms() +
                                          am_getconfig_int_def("media.amadec.wait_fresh_ms", 200);
        #ifdef USE_AOUT_IN_ADEC
        aout_ops->resume(audec);
        #endif
        adec_pts_resume();
    }
}

/**
 * \brief stop audio dec when receive STOP command.
 * \param audec pointer to audec
 */
static void stop_adec(aml_audio_dec_t *audec)
{
    #ifdef USE_AOUT_IN_ADEC
    audio_out_operations_t *aout_ops = &audec->aout_ops;
    #endif
    adec_print("[%s %d]audec->state/%d\n", __FUNCTION__, __LINE__, audec->state);
    if (audec->state > INITING) {
        char buf[64];

        audec->state = STOPPED;
        #ifdef USE_AOUT_IN_ADEC
        aout_ops->mute(audec, 1); //mute output, some repeat sound in audioflinger after stop
        aout_ops->stop(audec);
        #endif
        audio_codec_release(audec);

        sprintf(buf, "0x%x", 0);
        amsysfs_set_sysfs_str(TSYNC_FIRSTAPTS, buf);
    }
}

/**
 * \brief release audio dec when receive RELEASE command.
 * \param audec pointer to audec
 */
static void release_adec(aml_audio_dec_t *audec)
{
    audec->state = TERMINATED;
}

/**
 * \brief mute audio dec when receive MUTE command.
 * \param audec pointer to audec
 * \param en 1 = mute, 0 = unmute
 */
static void mute_adec(aml_audio_dec_t *audec, int en)
{
    audio_out_operations_t *aout_ops = &audec->aout_ops;
    if (aout_ops->mute) {
        adec_print("%s the output !\n", (en ? "mute" : "unmute"));
        aout_ops->mute(audec, en);
        audec->muted = en;
    }
}

/**
 * \brief set volume to audio dec when receive SET_VOL command.
 * \param audec pointer to audec
 * \param vol volume value
 */
static void adec_set_volume(aml_audio_dec_t *audec, float vol)
{
    audio_out_operations_t *aout_ops = &audec->aout_ops;
    if (aout_ops->set_volume) {
        adec_print("set audio volume! vol = %f\n", vol);
        aout_ops->set_volume(audec, vol);
    }
}

/**
 * \brief set volume to audio dec when receive SET_LRVOL command.
 * \param audec pointer to audec
 * \param lvol left channel volume value
 * \param rvol right channel volume value
 */
static void adec_set_lrvolume(aml_audio_dec_t *audec, float lvol, float rvol)
{
    audio_out_operations_t *aout_ops = &audec->aout_ops;
    if (aout_ops->set_lrvolume) {
        adec_print("set audio volume! left vol = %f,right vol:%f\n", lvol, rvol);
        aout_ops->set_lrvolume(audec, lvol, rvol);
    }
}
static void adec_flag_check(aml_audio_dec_t *audec)
{
    audio_out_operations_t *aout_ops = &audec->aout_ops;
    if (audec->auto_mute && (audec->state > INITTED)) {
        aout_ops->pause(audec);
        amthreadpool_thread_usleep(10000);
        while ((!audec->need_stop) && track_switch_pts(audec)) {
            amthreadpool_thread_usleep(1000);
        }
        aout_ops->resume(audec);
        audec->auto_mute = 0;
    }
}


static void start_decode_thread(aml_audio_dec_t *audec)
{
    if (audec->state != INITTED) {
        adec_print("decode not inited quit \n");
        return;
    }

    pthread_t    tid;
    int ret;
#ifndef USE_AOUT_IN_ADEC
    if (audec->associate_dec_supported) {
        ret = amthreadpool_pthread_create_name(&tid, NULL, (void *)ad_audio_getpackage_loop, (void *)audec, "getadpackagelp");
        audec->ad_sn_getpackage_threadid = tid;
        adec_print("[%s]Create get ad package thread success! tid = %ld\n", __FUNCTION__, tid);

        ret = amthreadpool_pthread_create_name(&tid, NULL, (void *)ad_audio_decode_loop, (void *)audec, "ad_decodeloop");
        if (ret != 0) {
            adec_print("[%s]Create ffmpeg decode thread failed!\n", __FUNCTION__);
            return;
        }
        audec->ad_sn_threadid = tid;
    }
#endif
    ret = amthreadpool_pthread_create_name(&tid, NULL, (void *)audio_getpackage_loop, (void *)audec, "getpackagelp");
    audec->sn_getpackage_threadid = tid;
    adec_print("[%s]Create get package thread success! tid = %ld\n", __FUNCTION__, tid);
    ret = amthreadpool_pthread_create_name(&tid, NULL, (void *)audio_decode_loop, (void *)audec, "decodeloop");
    if (ret != 0) {
        adec_print("[%s]Create ffmpeg decode thread failed!\n", __FUNCTION__);
        return;
    }
    audec->sn_threadid = tid;
    audec->audio_decoder_enabled = 0x1;
    pthread_setname_np(tid, "AmadecDecodeLP");
    adec_print("[%s]Create ffmpeg decode thread success! tid = %ld\n", __FUNCTION__, tid);
}
static void stop_decode_thread(aml_audio_dec_t *audec)
{
    audec->exit_decode_thread = 1;
    int ret = 0;
    if (audec->sn_threadid != -1) {
        amthreadpool_thread_cancel(audec->sn_threadid);
        ret = amthreadpool_pthread_join(audec->sn_threadid, NULL);
        adec_print("[%s]decode thread exit success\n", __FUNCTION__);
    }
    if (audec->sn_getpackage_threadid != -1) {
        amthreadpool_thread_cancel(audec->sn_getpackage_threadid);
        ret = amthreadpool_pthread_join(audec->sn_getpackage_threadid, NULL);
        adec_print("[%s]get package thread exit success\n", __FUNCTION__);
    }
    adec_print("[%s] done\n", __FUNCTION__);

    audec->sn_threadid = -1;
    audec->sn_getpackage_threadid = -1;
#ifndef USE_AOUT_IN_ADEC
    if (audec->associate_dec_supported) {
        ret = amthreadpool_pthread_join(audec->ad_sn_threadid, NULL);
        adec_print("[%s]ad decode thread exit success\n", __FUNCTION__);
        ret = amthreadpool_pthread_join(audec->ad_sn_getpackage_threadid, NULL);
        adec_print("[%s]ad get package thread exit success\n", __FUNCTION__);
        audec->ad_sn_threadid = -1;
        audec->ad_sn_getpackage_threadid = -1;
    }
#endif

}

/* --------------------------------------------------------------------------*/
/**
* @brief  getNextFrameSize  Get next frame size
*
* @param[in]   format     audio format
*
* @return      -1: no frame_size  use default   0: need get again  non_zero: success
*/
/* --------------------------------------------------------------------------*/

static int get_frame_size(aml_audio_dec_t *audec)
{
    int frame_szie = 0;
    int ret = 0;
    int extra_data = 8; //?
    StartCode *start_code = &audec->start_code;

    if (start_code->status == 0 || start_code->status == 3) {
        memset(start_code, 0, sizeof(StartCode));
    }
    /*ape case*/
    if (audec->format == ACODEC_FMT_APE) {
        if (start_code->status == 0) { //have not get the sync data
            ret = read_buffer((unsigned char *)start_code->buff, 4);
            if (ret <= 0) {
                return 0;
            }
            start_code->size = 4;
            start_code->status = 1;
        }

        if (start_code->status == 1) { //start find sync word
            if (start_code->size < 4) {
                ret = read_buffer((unsigned char *)(start_code->buff + start_code->size), 4 - start_code->size);
                if (ret <= 0) {
                    return 0;
                }
                start_code->size = 4;
            }
            if (start_code->size == 4) {
                if ((start_code->buff[0] == 'A') && (start_code->buff[1] == 'P') && (start_code->buff[2] == 'T') && (start_code->buff[3] == 'S')) {
                    start_code->size = 0;
                    start_code->status = 2; //sync word found ,start find frame size
                } else {
                    start_code->size = 3;
                    start_code->buff[0] = start_code->buff[1];
                    start_code->buff[1] = start_code->buff[2];
                    start_code->buff[2] = start_code->buff[3];
                    return 0;
                }
            }

        }

        if (start_code->status == 2) {
            ret = read_buffer((unsigned char *)start_code->buff, 4);
            if (ret <= 0) {
                return 0;
            }
            start_code->size = 4;
            frame_szie  = start_code->buff[3] << 24 | start_code->buff[2] << 16 | start_code->buff[1] << 8 | start_code->buff[0] + extra_data;
            frame_szie  = (frame_szie + 3) & (~3);
            start_code->status = 3; //found frame size
            return frame_szie;
        }
    }
    return -1;
}
// check if audio format info changed,if changed, apply new parameters to audio track
static void check_audio_info_changed(aml_audio_dec_t *audec)
{
    buffer_stream_t *g_bst = audec->g_bst;
    AudioInfo   g_AudioInfo = {0};
    int BufLevelAllowDoFmtChg = 0;
    audio_decoder_operations_t *adec_ops  = audec->adec_ops;
    adec_ops->getinfo(audec->adec_ops, &g_AudioInfo);
    if (g_AudioInfo.channels != 0 && g_AudioInfo.samplerate != 0) {
        if ((g_AudioInfo.channels != g_bst->channels) || (g_AudioInfo.samplerate != g_bst->samplerate)) {
            // the first time we get sample rate/channel num info,we use that to set audio track.
            if (audec->channels == 0 || audec->samplerate == 0) {
                g_bst->channels = audec->channels = g_AudioInfo.channels;
                g_bst->samplerate = audec->samplerate = g_AudioInfo.samplerate;
            } else {
                //experienc value:0.2 Secs
                BufLevelAllowDoFmtChg = audec->samplerate * audec->channels * (audec->adec_ops->bps >> 3) / 5;
                #ifdef USE_AOUT_IN_ADEC
                while ((audec->format_changed_flag || g_bst->buf_level > BufLevelAllowDoFmtChg) && !audec->exit_decode_thread) {
                    amthreadpool_thread_usleep(20000);
                }
                #endif
                if (!audec->exit_decode_thread) {
                    adec_print("[%s]Info Changed: src:sample:%d  channel:%d dest sample:%d  channel:%d PCMBufLevel:%d\n",
                               __FUNCTION__, audec->samplerate, audec->channels, g_AudioInfo.samplerate, g_AudioInfo.channels, g_bst->buf_level);
                    g_bst->channels = g_AudioInfo.channels;
                    g_bst->samplerate = g_AudioInfo.samplerate;
                    #ifdef USE_AOUT_IN_ADEC
                    if (audec->state == ACTIVE)
                        if ((audec->aout_ops.pause != NULL)) {
                            audec->aout_ops.pause(audec);
                        }
                    #endif
                    audec->format_changed_flag = 1;
                }
            }
        }
    }
}
void *audio_getpackage_loop(void *args)
{
    int ret;
    aml_audio_dec_t *audec;
    audio_decoder_operations_t *adec_ops;
    int nNextFrameSize = 0; //next read frame size
    int inlen = 0;//real data size in in_buf
    int nInBufferSize = 0; //full buffer size
    char *inbuf = NULL;//real buffer
    int rlen = 0;//read buffer ret size
    int nAudioFormat;
    unsigned wfd = 0;
    if (am_getconfig_bool("media.libplayer.wfd"))  {
        wfd = 1;
    }

    adec_print("[%s]adec_getpackage_loop start!\n", __FUNCTION__);
    audec = (aml_audio_dec_t *)args;
    adec_ops = audec->adec_ops;
    nAudioFormat = audec->format;
    inlen = 0;
    nNextFrameSize = adec_ops->nInBufSize;
    while (1) {
        //exit_decode_loop:
        if (audec->exit_decode_thread) { /*detect quit condition*/
            if (inbuf) {
                free(inbuf);
            }
            package_list_free(audec);
            break;
        }

        nNextFrameSize = get_frame_size(audec); /*step 2  get read buffer size*/
        if (nNextFrameSize == -1) {
            nNextFrameSize = adec_ops->nInBufSize;
            if (audec->format == ACODEC_FMT_AC3 || audec->format == ACODEC_FMT_EAC3 || audec->format == ACODEC_FMT_DTS) {
                nNextFrameSize = 512;
            } else if (audec->format == ACODEC_FMT_MPEG) {
                nNextFrameSize = 1024;
            }
        } else if (nNextFrameSize == 0) {
            amthreadpool_thread_usleep(1000);
            continue;
        }

        nInBufferSize = nNextFrameSize;/*step 3  read buffer*/
        if (inbuf != NULL) {
            free(inbuf);
            inbuf = NULL;
        }
        inbuf = malloc(nInBufferSize);

        int nNextReadSize = nInBufferSize;
        int nRet = 0;
        //int nReadErrCount = 0;
        int nCurrentReadCount = 0;
        int nReadSizePerTime = 512;
        rlen = 0;
        int sleeptime = 0;
        while (nNextReadSize > 0 && !audec->exit_decode_thread) {
            if (nNextReadSize <= nReadSizePerTime) {
                nReadSizePerTime = nNextReadSize;
            }
            nRet = read_buffer((unsigned char *)(inbuf + rlen), nReadSizePerTime); //read 10K per time
            //adec_print("nRet:%d",nRet);
            if (nRet <= 0) {
                sleeptime++;
                amthreadpool_thread_usleep(1000);
                continue;
            }
            rlen += nRet;
            nNextReadSize -= nRet;
            if (wfd && nAudioFormat == ACODEC_FMT_AAC) {
                if (rlen > 300) {
                    break;
                }
            }
        }

        sleeptime = 0;
        nCurrentReadCount = rlen;
        rlen += inlen;
        ret = -1;
        while ((ret = package_add(audec, inbuf, rlen)) && !audec->exit_decode_thread) {
            amthreadpool_thread_usleep(1000);
        }
        if (ret) {
            free(inbuf);
        }
        inbuf = NULL;
    }
    //QUIT:
    adec_print("[%s]Exit adec_getpackage_loop Thread finished!", __FUNCTION__);
    pthread_exit(NULL);
    return NULL;
}
static void dump_amadec_data(void *buffer, int size, char *file_name)
{
   if (property_get_bool("vendor.media.amadec.outdump",false)) {
        FILE *fp1 = fopen(file_name, "a+");
        if (fp1) {
            int flen = fwrite((char *)buffer, 1, size, fp1);
            adec_print("%s buffer %p size %d flen %d\n", __FUNCTION__, buffer, size,flen);
            fclose(fp1);
        }
    }
}

#ifndef USE_AOUT_IN_ADEC
void *ad_audio_getpackage_loop(void *args)
{
    int ret;
    aml_audio_dec_t *audec;
    audio_decoder_operations_t *ad_adec_ops;
    int nNextFrameSize = 0; //next read frame size
    int inlen = 0;//real data size in in_buf
    int nInBufferSize = 0; //full buffer size
    char *inbuf = NULL;//real buffer
    int rlen = 0;//read buffer ret size
    int nAudioFormat;
    unsigned wfd = 0;
    if (am_getconfig_bool("vendor.media.libplayer.wfd"))  {
        wfd = 1;
    }

    adec_print("[%s]ad_adec_getadpackage_loop start!\n", __FUNCTION__);
    audec = (aml_audio_dec_t *)args;
    ad_adec_ops = audec->ad_adec_ops;
    nAudioFormat = audec->format;
    inlen = 0;
    dtv_assoc_audio_cache(2);
    nNextFrameSize = ad_adec_ops->nInBufSize;
    while (1) {
        //exit_decode_loop:
        if (audec->exit_decode_thread) { /*detect quit condition*/
            if (inbuf) {
                free(inbuf);
            }
            ad_package_list_free(audec);
            break;
        }

        nNextFrameSize = get_frame_size(audec); /*step 2  get read buffer size*/
        if (nNextFrameSize == -1) {
            //nNextFrameSize = ad_adec_ops->nInBufSize;
            //if (audec->format == ACODEC_FMT_AC3 || audec->format == ACODEC_FMT_EAC3 || audec->format == ACODEC_FMT_DTS) {
                nNextFrameSize = 512;
            //}
        } else if (nNextFrameSize == 0) {
            amthreadpool_thread_usleep(1000);
            continue;
        }

        nInBufferSize = nNextFrameSize;/*step 3  read buffer*/
        if (inbuf != NULL) {
            free(inbuf);
            inbuf = NULL;
        }
        inbuf = malloc(nInBufferSize);

        int nNextReadSize = nInBufferSize;
        int nRet = 0;
        //int nReadErrCount = 0;
        int nCurrentReadCount = 0;
        int nReadSizePerTime = 1024;
        rlen = 0;
        int sleeptime = 0;
        while (nNextReadSize > 0 && !audec->exit_decode_thread) {
            if (nNextReadSize <= nReadSizePerTime) {
                nReadSizePerTime = nNextReadSize;
            }
            nRet = dtv_assoc_read((unsigned char *)(inbuf + rlen), nReadSizePerTime); //read 10K per time
            if (nRet <= 0) {
                sleeptime++;
                amthreadpool_thread_usleep(1000);
                continue;
            }
            rlen += nRet;
            nNextReadSize -= nRet;
            if (wfd && nAudioFormat == ACODEC_FMT_AAC) {
                if (rlen > 300) {
                    break;
                }
            }
        }

        sleeptime = 0;
        nCurrentReadCount = rlen;
        rlen += inlen;
        ret = -1;
        while ((ret = ad_package_add(audec, inbuf, rlen)) && !audec->exit_decode_thread) {
            amthreadpool_thread_usleep(1000);
        }
        if (ret) {
            free(inbuf);
        }
        inbuf = NULL;
    }
    //QUIT:
    adec_print("[%s]Exit ad adec_getpackage_loop Thread finished!", __FUNCTION__);
    pthread_exit(NULL);
    return NULL;
}

static char ad_pcm_buf_tmp[AVCODEC_MAX_AUDIO_FRAME_SIZE];//max frame size out buf
void *ad_audio_decode_loop(void *args)
{
    aml_audio_dec_t *audec;
    audio_decoder_operations_t *ad_adec_ops;
    int inlen = 0;//real data size in in_buf
    char *inbuf = NULL;//real buffer
    int rlen = 0;//read buffer ret size
    char *pRestData = NULL;
    int dlen = 0;//decode size one time
    int declen = 0;//current decoded size
    int nCurrentReadCount = 0;
    int needdata = 0;
    int nAudioFormat;
    char *outbuf = ad_pcm_buf_tmp;
    int outlen = AVCODEC_MAX_AUDIO_FRAME_SIZE;
    struct package *p_Package = NULL;

    //AudioInfo g_AudioInfo;
    adec_print("[%s]ad_adec_armdec_loop start!\n", __FUNCTION__);
    audec = (aml_audio_dec_t *)args;
    ad_adec_ops = audec->ad_adec_ops;
    memset(outbuf, 0, AVCODEC_MAX_AUDIO_FRAME_SIZE);
    nAudioFormat = audec->format;
    inlen = 0;
    ad_adec_ops->nAudioDecoderType = audec->format;
    AudioInfo   g_AudioInfo = {0, 0, 0, 0, 0};
    while (1) {
        //exit_decode_loop:
        if (audec->exit_decode_thread) { //detect quit condition
            if (inbuf) {
                free(inbuf);
                inbuf = NULL;
            }
            if (pRestData) {
                free(pRestData);
                pRestData = NULL;
            }
            audec->exit_decode_thread_success = 1;
            break;
        }
        //step 2  get read buffer size
        p_Package = ad_package_get(audec);
        if (!p_Package) {
            amthreadpool_thread_usleep(1000);
            continue;
        }
        dump_amadec_data(p_Package->data,p_Package->size,"/data/audio/adec_ad.es");
        if (inbuf != NULL) {
            free(inbuf);
            inbuf = NULL;
        }

        if (inlen && pRestData) {
            rlen = p_Package->size + inlen;
            inbuf = malloc(rlen);
            memcpy(inbuf, pRestData, inlen);
            memcpy(inbuf + inlen, p_Package->data, p_Package->size);
            free(pRestData);
            free(p_Package->data);
            pRestData = NULL;
            p_Package->data = NULL;
        } else {
            rlen = p_Package->size;
            inbuf = p_Package->data;
            p_Package->data = NULL;
        }
        free(p_Package);
        p_Package = NULL;

        nCurrentReadCount = rlen;
        inlen = rlen;
        declen  = 0;
        if (nCurrentReadCount > 0) {
            while (declen < rlen && !audec->exit_decode_thread) {
                outlen = AVCODEC_MAX_AUDIO_FRAME_SIZE;
                if (nAudioFormat == ACODEC_FMT_COOK || nAudioFormat == ACODEC_FMT_RAAC || nAudioFormat == ACODEC_FMT_AMR) {
                    if (needdata > 0) {
                        pRestData = malloc(inlen);
                        if (pRestData) {
                            memcpy(pRestData, (uint8_t *)(inbuf + declen), inlen);
                        }
                        needdata = 0;
                        break;
                    }
                }
                dlen = ad_adec_ops->decode(audec->ad_adec_ops, outbuf, &outlen, inbuf + declen, inlen);
                if (outlen > AVCODEC_MAX_AUDIO_FRAME_SIZE) {
                    adec_print("!!!!!fatal error,out buffer overwriten,out len %d,actual %d", outlen, AVCODEC_MAX_AUDIO_FRAME_SIZE);
                }
                if (dlen <= 0) {
                    if (nAudioFormat == ACODEC_FMT_APE) {
                        inlen = 0;
                    } else if (inlen > 0) {
                        pRestData = malloc(inlen);
                        if (pRestData) {
                            memcpy(pRestData, (uint8_t *)(inbuf + declen), inlen);
                        }
                    }
                    needdata = 0;
                    break;
                }
                declen += dlen;
                inlen -= dlen;

                /* decoder input buffer not enough,need new data burst */
                if (nAudioFormat == ACODEC_FMT_COOK || nAudioFormat == ACODEC_FMT_RAAC || nAudioFormat == ACODEC_FMT_AMR) {
                    if (inlen <= declen) {
                        needdata ++;
                    }
                }
                // for aac decoder, if decoder cost es data but no pcm output,may need more data ,which is needed by frame resync
                else if (nAudioFormat == ACODEC_FMT_AAC_LATM || nAudioFormat == ACODEC_FMT_AAC) {
                    if (outlen == 0 && inlen) {
                        pRestData = malloc(inlen);
                        if (pRestData) {
                            memcpy(pRestData, (uint8_t *)(inbuf + declen), inlen);
                        }
                        break;
                    }
                }

                if (outlen) {
                    dump_amadec_data(outbuf,outlen,"/data/audio/adec_ad.pcm");
                    ad_adec_ops->getinfo(audec->ad_adec_ops, &g_AudioInfo);
                    float mixing_coefficient = (float)(audec->mixing_level  + 32 ) / 64;
                    apply_volume(mixing_coefficient, outbuf, sizeof(uint16_t), outlen);
                    if (g_AudioInfo.channels == 1 && audec->channels == 2) {
                        int16_t *buf = (int16_t *)outbuf;
                        int i = 0, samples_num,samples;
                        samples_num = outlen / sizeof(int16_t);
                        for (; i < samples_num; i++) {
                            samples = buf[samples_num - i - 1] ;
                            if (g_AudioInfo.channels == 1 && audec->channels == 2) {
                                buf[2 * (samples_num - i) - 1] = samples;
                                buf[2 * (samples_num - i) - 2] = samples;
                            }
                        }
                        outlen *= 2;

                    }
                    aml_hw_mixer_write(&audec->hw_mixer, outbuf, outlen);
                    //amthreadpool_thread_usleep(outlen * 1000000 / 4 / g_AudioInfo.samplerate / 2);
                    //adec_print("aml_hw_mixer_write outlen %d",outlen);
                }
            }
        } else {
            amthreadpool_thread_usleep(1000);
            continue;
        }
    }

    adec_print("[%s]exit ad_adec_armdec_loop Thread finished!", __FUNCTION__);
    pthread_exit(NULL);
    //error:
    //pthread_exit(NULL);
    return NULL;
}

#endif

static char pcm_buf_tmp[AVCODEC_MAX_AUDIO_FRAME_SIZE];//max frame size out buf
void *audio_decode_loop(void *args)
{
    //int ret;
    aml_audio_dec_t *audec;
    audio_out_operations_t *aout_ops;
    audio_decoder_operations_t *adec_ops;
    int nNextFrameSize = 0; //next read frame size
    int inlen = 0;//real data size in in_buf
    //int nRestLen = 0; //left data after last decode
    //int nInBufferSize = 0; //full buffer size
    //int nStartDecodePoint=0;//start decode point in in_buf
    char *inbuf = NULL;//real buffer
    int rlen = 0;//read buffer ret size
    char *pRestData = NULL;
    //char *inbuf2;

    int dlen = 0;//decode size one time
    int declen = 0;//current decoded size
    int nCurrentReadCount = 0;
    int needdata = 0;
    //char startcode[5];
    //int extra_data = 8;
    //int nCodecID;
    int nAudioFormat;
    char *outbuf = pcm_buf_tmp;
    int outlen = AVCODEC_MAX_AUDIO_FRAME_SIZE;
    struct package *p_Package = NULL;

    buffer_stream_t *g_bst;
    //AudioInfo g_AudioInfo;
    adec_print("[%s]adec_armdec_loop start!\n", __FUNCTION__);
    audec = (aml_audio_dec_t *)args;
    aout_ops = &audec->aout_ops;
    adec_ops = audec->adec_ops;
    memset(outbuf, 0, AVCODEC_MAX_AUDIO_FRAME_SIZE);
    g_bst = audec->g_bst;

    nAudioFormat = audec->format;
    g_bst->format = audec->format;
    inlen = 0;
    nNextFrameSize = adec_ops->nInBufSize;
    adec_ops->nAudioDecoderType = audec->format;

#ifndef USE_AOUT_IN_ADEC
    int ad_bufferms = 0;
    if (audec->associate_dec_supported) {
        if (audec->associate_audio_enable) {
            char value[PROPERTY_VALUE_MAX] = {0};
            if (property_get("vendor.media.ad.bufferms", value, NULL) > 0) {
               ad_bufferms = atoi(value);
           }
            while (aml_hw_mixer_get_content_l(&audec->hw_mixer) < (ad_bufferms * 48 * 4) && !audec->exit_decode_thread) {
                 adec_print("ad buf %d ",aml_hw_mixer_get_content_l(&audec->hw_mixer));
                 amthreadpool_thread_usleep(10000);
            }
        }
    }
#endif

    while (1) {
        //exit_decode_loop:
        if (audec->exit_decode_thread) { //detect quit condition
            if (inbuf) {
                free(inbuf);
                inbuf = NULL;
            }
            if (pRestData) {
                free(pRestData);
                pRestData = NULL;
            }
            audec->exit_decode_thread_success = 1;
            break;
        }
        //step 2  get read buffer size
        p_Package = package_get(audec);
        if (!p_Package) {
            amthreadpool_thread_usleep(1000);
            continue;
        }
        if (inbuf != NULL) {
            free(inbuf);
            inbuf = NULL;
        }
        dump_amadec_data(p_Package->data,p_Package->size,"/data/audio/amadec_abuf_read.es");
        if (inlen && pRestData) {
            rlen = p_Package->size + inlen;
            inbuf = malloc(rlen);
            memcpy(inbuf, pRestData, inlen);
            memcpy(inbuf + inlen, p_Package->data, p_Package->size);
            free(pRestData);
            free(p_Package->data);
            pRestData = NULL;
            p_Package->data = NULL;
        } else {
            rlen = p_Package->size;
            inbuf = p_Package->data;
            p_Package->data = NULL;
        }
        free(p_Package);
        p_Package = NULL;

        nCurrentReadCount = rlen;
        inlen = rlen;
        declen  = 0;
        if (nCurrentReadCount > 0) {
            while (declen < rlen && !audec->exit_decode_thread) {
                outlen = AVCODEC_MAX_AUDIO_FRAME_SIZE;
                if (nAudioFormat == ACODEC_FMT_COOK || nAudioFormat == ACODEC_FMT_RAAC || nAudioFormat == ACODEC_FMT_AMR) {
                    if (needdata > 0) {
                        pRestData = malloc(inlen);
                        if (pRestData) {
                            memcpy(pRestData, (uint8_t *)(inbuf + declen), inlen);
                        }
                        needdata = 0;
                        break;
                    }
                }
                if (nAudioFormat == ACODEC_FMT_AC3 || nAudioFormat == ACODEC_FMT_EAC3 || nAudioFormat == ACODEC_FMT_DTS) {
                    outlen = inlen;
                    dlen =  inlen;
                    memcpy(outbuf , inbuf, inlen);

                } else {
                    dlen = adec_ops->decode(audec->adec_ops, outbuf, &outlen, inbuf + declen, inlen);
                }
                if (outlen > 0) {
                    if (nAudioFormat != ACODEC_FMT_AC3 && nAudioFormat != ACODEC_FMT_EAC3 && nAudioFormat != ACODEC_FMT_DTS) {
                        check_audio_info_changed(audec);
                    }
#ifndef USE_AOUT_IN_ADEC
                if (audec->associate_dec_supported) {
                    if (audec->associate_audio_enable) {
                        //adec_print("aml_hw_mixer_mixing outlen %d",outlen);
                        float mixing_coefficient = 1.0f - (float)(audec->mixing_level  + 32 ) / 64;
                        apply_volume(mixing_coefficient, outbuf, sizeof(uint16_t), outlen);
                        aml_hw_mixer_mixing(&audec->hw_mixer, outbuf, outlen, AUDIO_FORMAT_PCM_16_BIT);
                    }
                }
#endif
                }
                if (outlen > AVCODEC_MAX_AUDIO_FRAME_SIZE) {
                    adec_print("!!!!!fatal error,out buffer overwriten,out len %d,actual %d", outlen, AVCODEC_MAX_AUDIO_FRAME_SIZE);
                }
                if (dlen <= 0) {

                    if (nAudioFormat == ACODEC_FMT_APE) {
                        inlen = 0;
                    } else if (inlen > 0) {
                        pRestData = malloc(inlen);
                        if (pRestData) {
                            memcpy(pRestData, (uint8_t *)(inbuf + declen), inlen);
                        }
                    }
                    audec->nDecodeErrCount++;//decode failed, add err_count
                    needdata = 0;
                    break;
                }
                audec->nDecodeErrCount = 0; //decode success reset to 0
                declen += dlen;
                inlen -= dlen;

                /* decoder input buffer not enough,need new data burst */
                if (nAudioFormat == ACODEC_FMT_COOK || nAudioFormat == ACODEC_FMT_RAAC || nAudioFormat == ACODEC_FMT_AMR) {
                    if (inlen <= declen) {
                        needdata ++;
                    }
                }
                // for aac decoder, if decoder cost es data but no pcm output,may need more data ,which is needed by frame resync
                else if (nAudioFormat == ACODEC_FMT_AAC_LATM || nAudioFormat == ACODEC_FMT_AAC) {
                    if (outlen == 0 && inlen) {
                        pRestData = malloc(inlen);
                        if (pRestData) {
                            memcpy(pRestData, (uint8_t *)(inbuf + declen), inlen);
                        }
                        audec->decode_offset += dlen; //update es offset for apts look up
                        break;
                    }
                }
                //write to the pcm buffer
                if (nAudioFormat == ACODEC_FMT_RAAC || nAudioFormat == ACODEC_FMT_COOK) {
                    audec->decode_offset = audec->adec_ops->pts;
                } else {
                    audec->decode_offset += dlen;
                }
                audec->decode_pcm_offset += outlen;

                if (g_bst) {
                    int wlen = 0;
                    while (outlen && !audec->exit_decode_thread) {
                        if (g_bst->buf_length - g_bst->buf_level < outlen) {
                            amthreadpool_thread_usleep(10000);
                            continue;
                        }
                        wlen = write_pcm_buffer(outbuf, g_bst, outlen);
                        outlen -= wlen;
                    }
                }
            }
        } else {
            amthreadpool_thread_usleep(1000);
            continue;
        }
    }

    adec_print("[%s]exit adec_armdec_loop Thread finished!", __FUNCTION__);
    pthread_exit(NULL);
    //error:
    //pthread_exit(NULL);
    return NULL;
}

void *adec_armdec_loop(void *args)
{
    int ret;
    aml_audio_dec_t *audec;
    audio_out_operations_t *aout_ops;
    adec_cmd_t *msg = NULL;

    audec = (aml_audio_dec_t *)args;
    aout_ops = &audec->aout_ops;

    // codec init
    audec->state = INITING;
    while (1) {
        if (audec->need_stop) {
            goto MSG_LOOP;
        }
        adec_print("audio_codec_init");
        ret = audio_codec_init(audec);
        if (ret == 0) {
            break;
        }
        usleep(10000);
    }
    audec->state = INITTED;

    //start decode thread
    while (1) {
        if (/*audec->StageFrightCodecEnableType*/0) {
            start_decode_thread_omx(audec);
            if (audec->OmxFirstFrameDecoded != 1) { // just one case, need_stop == 1
                //usleep(10000); // no need
                continue;
            }
        } else {
            start_decode_thread(audec);
        }
        //ok
        break;
    }

    //aout init
    while (1) {
        //wait the audio sr/ch ready to set audio track.
        adec_print("wait audio sr/channel begin \n");
        while (((!audec->channels) || (!audec->samplerate)) && !audec->need_stop) {
            amthreadpool_thread_usleep(10000);
        }
        adec_print("wait audio sr/channel done \n");
        #ifdef USE_AOUT_IN_ADEC
        ret = aout_ops->init(audec);
        if (ret) {
            adec_print("[%s %d]Audio out device init failed!", __FUNCTION__, __LINE__);
            amthreadpool_thread_usleep(10000);
            continue;
        }
        #endif
        //ok
        break;
    }

    //wait first pts decoded
    start_adec(audec);

MSG_LOOP:
    do {
#ifdef USE_AOUT_IN_ADEC
        adec_reset_track(audec);
#endif
        adec_flag_check(audec);
#ifdef USE_AOUT_IN_ADEC
        if (audec->state == ACTIVE) {
            adec_refresh_pts(audec);
        }
#endif
        msg = adec_get_message(audec);
        if (!msg) {
            amthreadpool_thread_usleep(10 * 1000); //if not wait,need changed to amthread usleep
            continue;
        }

        switch (msg->ctrl_cmd) {
        case CMD_START:

            adec_print("Receive START Command!\n");
            //------------------------
            if (/*!audec->StageFrightCodecEnableType*/1) {
                start_decode_thread(audec);
            }
            //------------------------
            start_adec(audec);
            break;

        case CMD_PAUSE:

            adec_print("Receive PAUSE Command!");
            pause_adec(audec);
            break;

        case CMD_RESUME:

            adec_print("Receive RESUME Command!");
            resume_adec(audec);
            break;

        case CMD_STOP:

            adec_print("Receive STOP Command!");
            stop_adec(audec);
            break;

        case CMD_MUTE:

            adec_print("Receive Mute Command!");
            if (msg->has_arg) {
                mute_adec(audec, msg->value.en);
            }
            break;

        case CMD_SET_VOL:

            adec_print("Receive Set Vol Command!");
            if (msg->has_arg) {
                adec_set_volume(audec, msg->value.volume);
            }
            break;
        case CMD_SET_LRVOL:

            adec_print("Receive Set LRVol Command!");
            if (msg->has_arg) {
                adec_set_lrvolume(audec, msg->value.volume, msg->value_ext.volume);
            }
            break;

        case CMD_CHANL_SWAP:

            adec_print("Receive Channels Swap Command!");
            audio_hardware_ctrl(HW_CHANNELS_SWAP);
            break;

        case CMD_LEFT_MONO:

            adec_print("Receive Left Mono Command!");
            audio_hardware_ctrl(HW_LEFT_CHANNEL_MONO);
            break;

        case CMD_RIGHT_MONO:

            adec_print("Receive Right Mono Command!");
            audio_hardware_ctrl(HW_RIGHT_CHANNEL_MONO);
            break;

        case CMD_STEREO:

            adec_print("Receive Stereo Command!");
            audio_hardware_ctrl(HW_STEREO_MODE);
            break;

        case CMD_RELEASE:

            adec_print("Receive RELEASE Command!");
            release_adec(audec);
            break;

        default:
            adec_print("Unknow Command!");
            break;

        }

        if (msg) {
            adec_message_free(msg);
            msg = NULL;
        }
    } while (audec->state != TERMINATED);

    adec_print("Exit Message Loop Thread!");
    pthread_exit(NULL);
    return NULL;
}


