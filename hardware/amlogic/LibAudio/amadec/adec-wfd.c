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
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <dlfcn.h>

#include <adec-pts-mgt.h>
#include <adec_write.h>
#include <adec_omx_brige.h>
#include <Amsysfsutils.h>
#include <audio-dec.h>
#include <cutils/properties.h>
#include <amthreadpool.h>
#include "audiodsp_update_format.h"

#define LOG_TAG "adec-wfd"

typedef struct {
    int codec_id;
    char    name[64];
} audio_lib_t;

typedef int (*fn_pcm_output_init)(int sr, int ch);
typedef int (*fn_pcm_output_write)(char *buf, unsigned size);
typedef int (*fn_pcm_output_uninit)();
typedef int (*fn_pcm_output_latency)();


typedef void (*fn_audio_set_exit_flag)();


static audio_lib_t wfd_audio_lib_list[] = {
    {ACODEC_FMT_WIFIDISPLAY, "libpcm_wfd.so"},
    {ACODEC_FMT_AAC, "libaac_helix.so"}
} ;
static audio_decoder_operations_t WFDAudioDecoder = {
    "WFDDecoder",
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

static fn_pcm_output_init  wfd_out_init = NULL;
static  fn_pcm_output_write wfd_out_write = NULL;
static fn_pcm_output_uninit  wfd_out_uninit = NULL;
static fn_pcm_output_latency wfd_out_latency = NULL;


static fn_audio_set_exit_flag  wfd_adec_exit = NULL;
static void *dec_mLibHandle = NULL;
static void *out_mLibHandle  = NULL;
static void *audio_wfd_decode_loop(void *args);
static void stop_wfd_decode_thread(aml_audio_dec_t *audec);
static int wfd_register_audio_lib(aml_audio_dec_t *audec)
{
    int i;
    int num;
    audio_lib_t *f;
    adec_print("wfd audec->format  %d,audec->codec_id %x", audec->format, audec->codec_id);
    num = ARRAY_SIZE(wfd_audio_lib_list);
    audio_decoder_operations_t *adec_ops = audec->adec_ops;
    for (i = 0; i < num; i++) {
        f = &wfd_audio_lib_list[i];
        if (f->codec_id == audec->format) {
            dec_mLibHandle = dlopen(wfd_audio_lib_list[i].name, RTLD_NOW);
            if (dec_mLibHandle) {
                adec_ops->init    = dlsym(dec_mLibHandle, "audio_dec_init");
                adec_ops->decode  = dlsym(dec_mLibHandle, "audio_dec_decode");
                adec_ops->release = dlsym(dec_mLibHandle, "audio_dec_release");
                adec_ops->getinfo = dlsym(dec_mLibHandle, "audio_dec_getinfo");
                wfd_adec_exit =   dlsym(dec_mLibHandle, "audio_set_exit_flag");
                if (!adec_ops->init || !adec_ops->decode || !adec_ops->release || !adec_ops->getinfo || !wfd_adec_exit) {
                    adec_print("in %s,decode func not implemented %p %p %p %p \n", wfd_audio_lib_list[i].name, \
                               adec_ops->init, adec_ops->decode, adec_ops->release, adec_ops->getinfo);
                    goto error;
                }
            } else {
                adec_print("wfd cant find decoder lib\n");
                goto error;
            }
            return 0;
        }
    }
error:
    if (dec_mLibHandle) {
        dlclose(dec_mLibHandle);
    }
    dec_mLibHandle = NULL;
    return -1;
}
static int wfd_register_output_lib(aml_audio_dec_t *audec __unused)
{
    out_mLibHandle = dlopen("libamadec_wfd_out.so", RTLD_NOW);
    if (out_mLibHandle) {
        wfd_out_init =  dlsym(out_mLibHandle, "pcm_output_init");
        wfd_out_write =  dlsym(out_mLibHandle, "pcm_output_write");
        wfd_out_uninit =  dlsym(out_mLibHandle, "pcm_output_uninit");
        wfd_out_latency =  dlsym(out_mLibHandle, "pcm_output_latency");

        if (!wfd_out_init || !wfd_out_write || !wfd_out_uninit || !wfd_out_latency) {
            adec_print("wfd load output lib api failed\n");
            goto error;
        }
    } else {
        goto error;
    }
    return 0;
error:
    if (out_mLibHandle) {
        dlclose(out_mLibHandle);
    }
    out_mLibHandle = NULL;
    return -1;


}
static void wfd_unregister_lib()
{
    if (out_mLibHandle) {
        dlclose(out_mLibHandle);
    }
    out_mLibHandle = NULL;
    if (dec_mLibHandle) {
        dlclose(dec_mLibHandle);
    }
    dec_mLibHandle = NULL;
}
extern int read_buffer(unsigned char *buffer, int size);


static int   wfd_dec_pcm_read(dsp_operations_t *dsp_ops __unused, char *buffer __unused, int len __unused)
{
    return 0;
}
static unsigned long  wfd_dec_get_pts(dsp_operations_t *dsp_ops)
{
    unsigned long offset;
    unsigned long pts;
    int samplerate;
    unsigned long long frame_nums ;
    //unsigned long delay_pts;
    samplerate = 48000;
    aml_audio_dec_t *audec = (aml_audio_dec_t *)dsp_ops->audec;
    offset = audec->decode_offset;

    if (dsp_ops->dsp_file_fd >= 0) {
        ioctl(dsp_ops->dsp_file_fd, AMSTREAM_IOC_APTS_LOOKUP, &offset);
    } else {
        adec_print("====abuf have not open!\n");
    }
    pts = offset;
    if (pts == 0) {
        if (audec->last_valid_pts) {
            pts = audec->last_valid_pts;
        }
        frame_nums = (audec->out_len_after_last_valid_pts * 8 / (16 * 2));
        adec_print("decode_offset:%ld out_pcm:%d   pts:%ld \n", audec->decode_offset, audec->out_len_after_last_valid_pts, pts);
        pts += (frame_nums * 90000 / samplerate);
        return pts;
    }
    audec->last_valid_pts = pts;
    audec->out_len_after_last_valid_pts = 0;
    return pts;
}

static  unsigned long  wfd_dec_get_pcrscr(dsp_operations_t *dsp_ops)
{
    unsigned long val;
    if (dsp_ops->dsp_file_fd < 0) {
        adec_print("read error!! audiodsp have not opened\n");
        return -1;
    }
    ioctl(dsp_ops->dsp_file_fd, AMSTREAM_IOC_PCRSCR, &val);
    return val;
}
int wfd_dec_set_pts(dsp_operations_t *dsp_ops, unsigned long apts)
{
    if (dsp_ops->dsp_file_fd < 0) {
        adec_print("armdec_set_apts err!\n");
        return -1;
    }
    ioctl(dsp_ops->dsp_file_fd, AMSTREAM_IOC_SET_APTS, &apts);
    return 0;
}
static int wfd_dec_set_skip_bytes(dsp_operations_t* dsp_ops __unused, unsigned int bytes __unused)
{
    return  0;
}
static int set_sysfs_int(const char *path, int val)
{
    return amsysfs_set_sysfs_int(path, val);
}


static int wfd_audio_codec_release(aml_audio_dec_t *audec)
{
    if (wfd_adec_exit) {
        wfd_adec_exit();
    }
    stop_wfd_decode_thread(audec);
    if (wfd_out_uninit) {
        wfd_out_uninit();
    }
    audec->adec_ops->release(audec->adec_ops);
    wfd_unregister_lib();
    adec_print("wfd audio_codec_release done\n");
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
static void start_adec(aml_audio_dec_t *audec)
{
    int ret;
    dsp_operations_t *dsp_ops = &audec->adsp_ops;
    unsigned long  vpts, apts;
    int times = 0;
    char buf[32];
    apts = vpts = 0;
    audec->no_first_apts = 0;

    if (audec->state == INITTED) {
        audec->state = ACTIVE;
        //get info from the audiodsp == can get from amstreamer
        while ((!get_first_apts_flag(dsp_ops)) && (!audec->need_stop) && (!audec->no_first_apts)) {
            adec_print("wait first pts checkin complete !");
            times++;
            if (times >= 5) {
                amsysfs_get_sysfs_str(TSYNC_VPTS, buf, sizeof(buf));// read vpts
                if (sscanf(buf, "0x%lx", &vpts) < 1) {
                    adec_print("unable to get vpts from: %s", buf);
                    return;
                }
                // save vpts to apts
                adec_print("## can't get first apts, save vpts to apts,vpts=%lx, \n", vpts);
                sprintf(buf, "0x%lx", vpts);
                amsysfs_set_sysfs_str(TSYNC_APTS, buf);
                audec->no_first_apts = 1;
            }
            amthreadpool_thread_usleep(100000);
        }

        /*start  the  the pts scr,...*/
        ret = adec_pts_start(audec);
        if (audec->auto_mute) {
            avsync_en(0);
            adec_pts_pause();
            while ((!audec->need_stop) && track_switch_pts(audec)) {
                amthreadpool_thread_usleep(1000);
            }
            avsync_en(1);
            adec_pts_resume();
            audec->auto_mute = 0;
        }
#ifdef OUT_USE_AUDIOTRACK
        audio_out_operations_t *aout_ops = &audec->aout_ops;
        aout_ops->start(audec);
#endif
    }
}

/**
 * \brief pause audio dec when receive PAUSE command.
 * \param audec pointer to audec
 */
static void pause_adec(aml_audio_dec_t *audec)
{
    if (audec->state == ACTIVE) {
        audec->state = PAUSED;
        adec_pts_pause();
#ifdef OUT_USE_AUDIOTRACK
        audio_out_operations_t *aout_ops = &audec->aout_ops;
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
    if (audec->state == PAUSED) {
        audec->state = ACTIVE;
#ifdef OUT_USE_AUDIOTRACK
        audio_out_operations_t *aout_ops = &audec->aout_ops;
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
    adec_print("[%s %d]audec->state/%d\n", __FUNCTION__, __LINE__, audec->state);
    if (audec->state > INITING) {
        audec->state = STOPPED;
#ifdef OUT_USE_AUDIOTRACK
        audio_out_operations_t *aout_ops = &audec->aout_ops;
        aout_ops->mute(audec, 1); //mute output, some repeat sound in audioflinger after stop
        aout_ops->stop(audec);
#endif
        wfd_audio_codec_release(audec);
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
static void mute_adec(aml_audio_dec_t *audec __unused, int en __unused)
{
#ifdef OUT_USE_AUDIOTRACK
    audio_out_operations_t *aout_ops = &audec->aout_ops;
    if (aout_ops->mute) {
        adec_print("%s the output !\n", (en ? "mute" : "unmute"));
        aout_ops->mute(audec, en);
        audec->muted = en;
    }
#endif
}

/**
 * \brief set volume to audio dec when receive SET_VOL command.
 * \param audec pointer to audec
 * \param vol volume value
 */
static void adec_set_volume(aml_audio_dec_t *audec __unused, float vol __unused)
{
#ifdef OUT_USE_AUDIOTRACK
    audio_out_operations_t *aout_ops = &audec->aout_ops;
    if (aout_ops->set_volume) {
        adec_print("set audio volume! vol = %f\n", vol);
        aout_ops->set_volume(audec, vol);
    }
#endif
}

/**
 * \brief set volume to audio dec when receive SET_LRVOL command.
 * \param audec pointer to audec
 * \param lvol left channel volume value
 * \param rvol right channel volume value
 */
static void adec_set_lrvolume(aml_audio_dec_t *audec __unused, float lvol __unused, float rvol __unused)
{
#ifdef OUT_USE_AUDIOTRACK
    audio_out_operations_t *aout_ops = &audec->aout_ops;
    if (aout_ops->set_lrvolume) {
        adec_print("set audio volume! left vol = %f,right vol:%f\n", lvol, rvol);
        aout_ops->set_lrvolume(audec, lvol, rvol);
    }
#endif
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
#ifdef OUT_USE_AUDIOTRACK
        aout_ops->resume(audec);
#endif
        audec->auto_mute = 0;
    }
}


static void start_wfd_decode_thread(aml_audio_dec_t *audec)
{
    int ret = -1;
    if (audec->state != INITTED) {
        adec_print("decode not inited quit \n");
        return;
    }
    pthread_t    tid;
    ret = amthreadpool_pthread_create(&tid, NULL, (void *)audio_wfd_decode_loop, (void *)audec);
    if (ret != 0) {
        adec_print("[%s]Create ffmpeg decode thread failed!\n", __FUNCTION__);
        return;
    }
    audec->sn_threadid = tid;
    pthread_setname_np(tid, "AmadecDecodeWFD");
    adec_print("[%s]Create WFD audio decode thread success! tid = %ld\n", __FUNCTION__, tid);
}
static void stop_wfd_decode_thread(aml_audio_dec_t *audec)
{
    adec_print("enter stop_wfd_decode_thread \n");
    audec->exit_decode_thread = 1;
    amthreadpool_pthread_join(audec->sn_threadid, NULL);
    adec_print("[%s]wfd decode thread exit success\n", __FUNCTION__);
    audec->exit_decode_thread = 0;
    audec->sn_threadid = -1;

}

static int wfd_audio_codec_init(aml_audio_dec_t *audec)
{
    int ret = 0;
    audec->exit_decode_thread = 0;
    audec->fd_uio = -1;
    audec->sn_threadid = -1;
    audec->adsp_ops.dsp_on = 1;
    //TODO : get the actual sample rate /channel num
    audec->data_width = AV_SAMPLE_FMT_S16;
    audec->channels  = 2;
    audec->samplerate = 48000;

    audec->adsp_ops.dsp_read = wfd_dec_pcm_read;
    audec->adsp_ops.get_cur_pts = wfd_dec_get_pts;
    audec->adsp_ops.get_cur_pcrscr =  wfd_dec_get_pcrscr;
    audec->adsp_ops.set_cur_apts    = wfd_dec_set_pts;
    audec->adsp_ops.set_skip_bytes = wfd_dec_set_skip_bytes;
    audec->adsp_ops.dsp_read_raw =  NULL;
    while (0 != set_sysfs_int(DECODE_ERR_PATH, DECODE_NONE_ERR)) {
        adec_print("[%s %d]set codec fatal failed ! \n", __FUNCTION__, __LINE__);
        amthreadpool_thread_usleep(100000);
    }
    /*
          //enable uio interface
        ret = uio_init(audec);
        if (ret < 0) {
            adec_print("wfd uio init error! \n");
            return -1;
        }
    */
    // register decoder lib
    memset(&WFDAudioDecoder, 0, sizeof(audio_decoder_operations_t));
    audec->adec_ops = &WFDAudioDecoder;
    audec->adec_ops->priv_data = audec;
    if (wfd_register_audio_lib(audec)) {
        adec_print("wfd register decoder failed \n");
        return -1;
    }
    if (wfd_register_output_lib(audec)) {
        adec_print("wfd register  output failed \n");
        return -1;
    }
    // output init,not init here as we should get the sr/ch info when decode one frame
#if 0
    if (wfd_out_init()) {
        adec_print("wfd init pcm output device failed\n");
        return -1;
    }
#endif
    ret = audec->adec_ops->init(audec->adec_ops);
    if (ret) {
        adec_print("wfd audio decoder init failed \n");
        goto error1;
    }


    adec_print("wfd audio_codec_init ok \n");
    return 0;
error1:
    wfd_out_uninit();
    return -1;

}
#define  RESAMPLE_FRAMES  128
static short  date_temp[1024 * 4 * 2];

static void  audio_resample_api(char* buffer, unsigned int *size, int Chnum, int enable, int delta)
{
    short *pbuf;
    int resample_enable;
    //int resample_type;
    int resample_delta;
    int frame_read;
    int num_sample = 0;
    int j, k;
    //int request = *size;
    int dsp_read = 0;
    static int last_resample_enable = 0;
    pbuf = (short*)date_temp;
    unsigned index = 0;
    float mPhaseFraction;
    float  phaseIncrement ;
    float mPhaseFraction1;
    unsigned in_sr;
    unsigned out_sr;
    short  *input;
    short  *output;
    unsigned frames = 0;
    in_sr = (RESAMPLE_FRAMES - 1);
    out_sr = (RESAMPLE_FRAMES - delta - 1);
    phaseIncrement = (float)in_sr / out_sr;
    resample_enable = enable;
    resample_delta = delta;

    if (last_resample_enable != resample_enable) {
        adec_print("resample changed: %s\n", resample_enable ? "Enabled" : "Disabled");
        last_resample_enable = resample_enable;
    }

    if (resample_enable && resample_delta  && *size >= RESAMPLE_FRAMES * sizeof(short)*Chnum) {
        //adec_print("resample start ... %d, step=%d\n", *size, resample_delta);
        if (resample_delta < 0) {
            //  *size = *size*RESAMPLE_FRAMES/(RESAMPLE_FRAMES-resample_delta);
        }
        memcpy(pbuf,    buffer, *size);
        frame_read = *size / (sizeof(short) * Chnum); //dsp_pcm_read(audec, pbuf, *size); // return mono sample number
        dsp_read += frame_read;
        k = 0;
        while (frame_read >= RESAMPLE_FRAMES) {
            mPhaseFraction = 0;
            mPhaseFraction1 = 0;
            index = 0;
            input = (short*)pbuf + frames * Chnum;
            output = (short*)buffer + k * Chnum;
            for (j = 0; j < RESAMPLE_FRAMES - resample_delta; j++) {
                output[2 * j] =   input[index * 2] + (short)((input[(index + 1) * 2] - input[index * 2]) * mPhaseFraction1);
                output[2 * j + 1] = input[index * 2 + 1] + (short)((input[(index + 1) * 2 + 1] - input[index * 2 + 1]) * mPhaseFraction1);
                mPhaseFraction += phaseIncrement;
                index =  mPhaseFraction;
                mPhaseFraction1 = mPhaseFraction - index;
                k++;
            }
            frames += RESAMPLE_FRAMES;
            frame_read -= RESAMPLE_FRAMES;
        }
        if (frame_read > 0) {
            memcpy((short*)buffer + k * Chnum, (short*)pbuf + frames * Chnum, frame_read * sizeof(short)* Chnum);
            k += frame_read;
        }


        num_sample  = k * sizeof(short) * Chnum;
        *size = k * sizeof(short) * Chnum;
    }
    //  adec_print("resample size from %d to %d, original %d\n", request, *size, dsp_read);
}

static  int skip_thred = 400;
static  int up_thred =  100;
static   int dn_thred = 200;
static int   dn_resample_delta = 2;
static int   up_resample_delta = -4;
static void set_wfd_pcm_thredhold()
{

    char value[PROPERTY_VALUE_MAX];
    if (property_get("media.wfd.skip", value, NULL) > 0) {
        skip_thred = atoi(value);
    }
    if (property_get("media.wfd.up", value, NULL) > 0) {
        up_thred = atoi(value);
    }
    if (property_get("media.wfd.dn", value, NULL) > 0) {
        dn_thred = atoi(value);
    }
    if (property_get("media.wfd.dn_delta", value, NULL) > 0) {
        dn_resample_delta = atoi(value);
    }
    if (property_get("media.wfd.up_delta", value, NULL) > 0) {
        up_resample_delta = atoi(value);
    }
}
void *audio_wfd_decode_loop(void *args)
{
    //t ret;
    aml_audio_dec_t *audec;
    audio_out_operations_t *aout_ops;
    audio_decoder_operations_t *adec_ops;
    short  outbuf[2048];
    int outlen = 0;
    int write_size  = 0;
    int  in_latency = 0;
    int out_latency = 0;
    int total_latency = 0;
    int out_init_flag = 0;
    char value[PROPERTY_VALUE_MAX];
    unsigned char debug_latency = 0;
    adec_print("audio_wfd_decode_loop start!\n");
    audec = (aml_audio_dec_t *)args;
    aout_ops = &audec->aout_ops;
    adec_ops = audec->adec_ops;
    if (property_get("media.wfd.debug_latency", value, NULL) > 0) {
        debug_latency = atoi(value);
    }
    while (!audec->exit_decode_thread) {
        outlen = 0;
        //  adec_refresh_pts(audec);
        set_wfd_pcm_thredhold();
        audec->decode_offset = adec_ops->decode(adec_ops, (char *)outbuf, &outlen, (char*)&in_latency, 0);

        if (outlen > 0) {
            if (!out_init_flag) {
                if (wfd_out_init(adec_ops->samplerate, adec_ops->channels)) {
                    adec_print("wfd init pcm output device failed\n");
                    continue;
                }
                out_init_flag = 1;
            }
            out_latency = wfd_out_latency();
            if (out_latency < 0) {
                out_latency = 0;
            }
            total_latency = in_latency + out_latency;
            audec->out_len_after_last_valid_pts += outlen;
            if (debug_latency) {
                adec_print("latency in %d ms,out %d ms.total %d ms\n", in_latency, out_latency, total_latency);
                if (total_latency < 150) {
                    if (property_get("sys.pkginfo", value, NULL) > 0) {
                        adec_print("latency pkg info %s \n", value);
                    }
                }
            }
            if (total_latency > skip_thred) {
                if (debug_latency) {
                    adec_print(" total latency  %d ms ,skip bytes %d \n", total_latency, outlen);
                }
                outlen = 0;
            } else if (total_latency > dn_thred) {
                audio_resample_api((char *)outbuf, (unsigned int *)&outlen, 2, 1, dn_resample_delta);
            } else if (total_latency < up_thred) {
                audio_resample_api((char *)outbuf, (unsigned int *)&outlen, 2, 1, up_resample_delta);
            }
            if (debug_latency) {
                adec_print("wfd dec out %d byts pcm \n", outlen);
            }
            write_size = wfd_out_write((char *)outbuf, outlen);
        }
        //TODO decode function
    }
    adec_print("exit audio_wfd_decode_loop Thread finished!\n");
    pthread_exit(NULL);
//error:
//  pthread_exit(NULL);
    return NULL;
}


/*
    WFD audio decoder control thread
*/
void *adec_wfddec_msg_loop(void *args)
{
    int ret;
    aml_audio_dec_t *audec;
    audio_out_operations_t *aout_ops;
    adec_cmd_t *msg = NULL;
    audec = (aml_audio_dec_t *)args;
    aout_ops = &audec->aout_ops;
    adec_print("adec_wfddec_loop in \n");
    while (!audec->need_stop) {
        audec->state = INITING;
        ret = wfd_audio_codec_init(audec);
        if (ret == 0) {
            {
#ifdef OUT_USE_AUDIOTRACK
                ret = aout_ops->init(audec);
                if (ret) {
                    adec_print("WFD Audio out device init failed!\n");
                    wfd_audio_codec_release(audec);
                    continue;
                }
#endif
                audec->state = INITTED;
                start_wfd_decode_thread(audec);
                start_adec(audec);
            }
            break;
        }

        if (!audec->need_stop) {
            amthreadpool_thread_usleep(100000);
        }
    }

    do {

        adec_reset_track(audec);
        adec_flag_check(audec);
        msg = adec_get_message(audec);
        if (!msg) {
            amthreadpool_thread_usleep(100000);
            continue;
        }

        switch (msg->ctrl_cmd) {
        case CMD_START:

            adec_print("Receive START Command!\n");
            start_wfd_decode_thread(audec);
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
#if 0
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
#endif
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

    adec_print("Exit adec_wfddec_msg_loop!");
    pthread_exit(NULL);
    return NULL;
}



