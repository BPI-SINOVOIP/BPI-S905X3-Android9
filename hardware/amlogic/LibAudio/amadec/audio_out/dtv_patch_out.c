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
 *  DESCRIPTION:
 *      brief  Functions of Auduo output control for Linux Platform.
 *
 */

#define LOG_TAG "amadec"
#include <fcntl.h>
#include <linux/soundcard.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
//#include <config.h>
#include <adec-pts-mgt.h>
#include <audio-dec.h>
#include <cutils/properties.h>
#include <log-print.h>
#include <pthread.h>
#include <adec-external-ctrl.h>
#include <amthreadpool.h>
#include <dtv_patch_out.h>

#define AUD_ASSO_PROP "media.audio.enable_asso"
#define AUD_ASSO_MIX_PROP "media.audio.mix_asso"
#define VID_DISABLED_PROP "media.dvb.video.disabled"
#define AUD_DISABLED_PROP "media.dvb.audio.disabled"

typedef struct _dtv_patch_out {
    aml_audio_dec_t *audec;
    out_pcm_write pcmout_cb;
    out_get_wirte_buffer_info buffer_cb;
    out_audio_info   info_cb;
    int device_opened;
    int state;
    pthread_t tid;
    void *pargs;
} dtv_patch_out;

enum {
    DTV_PATCH_STATE_CLODED = 0x01,
    DTV_PATCH_STATE_OPENED,
    DTV_PATCH_STATE_RUNNING,
    DTV_PATCH_STATE_PAUSE,
    DTV_PATCH_STATE_RESUME,
    DTV_PATCH_STATE_STOPED,
};

static dtv_patch_out outparam = {
    .audec = NULL,
    .pcmout_cb = NULL,
    .buffer_cb = NULL,
    .info_cb = NULL,
    .device_opened = 0,
    .state = DTV_PATCH_STATE_CLODED,
    .tid = -1,
    .pargs = NULL,
};

static pthread_mutex_t patch_out_mutex = PTHREAD_MUTEX_INITIALIZER;

//static char output_buffer[64 * 1024];
static unsigned char decode_buffer[OUTPUT_BUFFER_SIZE + 64];
#define PERIOD_SIZE 1024
#define PERIOD_NUM 4
#define AV_SYNC_THRESHOLD 60

/*
static int _get_vid_disabled()
{
    return property_get_int32(VID_DISABLED_PROP, 0);
}
static int _get_aud_disabled()
{
    return property_get_int32(AUD_DISABLED_PROP, 0);
}
*/
static dtv_patch_out *get_patchout(void)
{
    return &outparam;
}

static int out_patch_initd = 0;

unsigned long dtv_patch_get_pts(void)
{
    unsigned long pts;
    dtv_patch_out *param = get_patchout();

    pthread_mutex_lock(&patch_out_mutex);
    aml_audio_dec_t *audec = (aml_audio_dec_t *)param->audec;
    if (param->state != DTV_PATCH_STATE_STOPED && audec != NULL
        && audec->adsp_ops.get_cur_pts != NULL) {
        pts = audec->adsp_ops.get_cur_pts(&audec->adsp_ops);
        pthread_mutex_unlock(&patch_out_mutex);
        return pts;
    } else {
        pthread_mutex_unlock(&patch_out_mutex);
        return -1;
    }
}

static void *dtv_patch_out_loop(void *args)
{

    int len = 0;
    int len2 = 0;
    int offset = 0;
    int readcount = 0;
    //unsigned space_size = 0;
    //unsigned long pts;
    char *buffer =
        (char *)(((unsigned long)decode_buffer + 32) & (~0x1f));
    dtv_patch_out *patchparm = (dtv_patch_out *)args;
    int  channels, samplerate;
    aml_audio_dec_t *audec = (aml_audio_dec_t *)patchparm->audec;

    while (!(patchparm->state == DTV_PATCH_STATE_STOPED)) {
        // pthread_mutex_lock(&patch_out_mutex);
        {

            if (patchparm->state == DTV_PATCH_STATE_PAUSE) {
                // pthread_mutex_unlock(&patch_out_mutex);
                usleep(10000);
                continue;
            }
            if (offset > 0) {
                memmove(buffer, buffer + offset, len);
            }
            if (audec == NULL) {
                adec_print("the audec is NULL\n");
                // pthread_mutex_unlock(&patch_out_mutex);
                usleep(10000);
                if (patchparm->state == DTV_PATCH_STATE_STOPED) {
                    goto exit;
                }
                continue;
            }
            if (audec->adsp_ops.dsp_read == NULL) {
                adec_print("the audec dsp_read is NULL\n");
                // pthread_mutex_unlock(&patch_out_mutex);
                usleep(10000);
                if (patchparm->state == DTV_PATCH_STATE_STOPED) {
                    goto exit;
                }
                continue;
            }

            /*[SE][BUG][SWPL-14813][chengshun.wang] decrease buf_level avoid
             *dolby stream underrun
             *XXX 768 is some audio format frame size*/
            if (audec->g_bst->buf_level < 768) {
                usleep(1000);
                if (patchparm->state == DTV_PATCH_STATE_STOPED) {
                    goto exit;
                }
                continue;
            }

            if (audec->adsp_ops.get_cur_pts == NULL) {
                adec_print("the audec get_cur_pts is NULL\n");
                // pthread_mutex_unlock(&patch_out_mutex);
                usleep(10000);
                if (patchparm->state == DTV_PATCH_STATE_STOPED) {
                    goto exit;
                }

                continue;
            }
#ifndef USE_AOUT_IN_ADEC
                if (audec->associate_dec_supported) {
                    if (audec->associate_audio_enable) {
                      audec->mixing_level =  patchparm->buffer_cb(patchparm->pargs, AD_MIXING_LEVLE);
                    }
                }
#endif
            if (patchparm->buffer_cb(patchparm->pargs, BUFFER_SPACE) < 4096) {
                if (patchparm->state == DTV_PATCH_STATE_STOPED) {
                    goto exit;
                }
                adec_print("the amadec no space to write\n");
                usleep(1000);
                if (patchparm->state == DTV_PATCH_STATE_STOPED) {
                    goto exit;
                }

                continue;
            }
            // pts = audec->adsp_ops.get_cur_pts(&audec->adsp_ops);

            int audio_raw_format_type = (audec->format == ACODEC_FMT_AC3 ||
                audec->format == ACODEC_FMT_EAC3 ||
                audec->format == ACODEC_FMT_DTS);
            int dtv_buffer_level = patchparm->buffer_cb(patchparm->pargs, BUFFER_LEVEL);
            /* XXX 2304 = 3*768, maybe need modify */
            if (dtv_buffer_level > 2304 &&
                readcount < 25 &&
                audio_raw_format_type) {
                len2 = 0;
                readcount++;
            } else {
                len2 = audec->adsp_ops.dsp_read(&audec->adsp_ops, (buffer + len),
                                                (OUTPUT_BUFFER_SIZE - len));
                readcount = 0;
            }
            //adec_print("len2 %d", len2);
            len = len + len2;
            offset = 0;
        }

        if (len == 0) {
            usleep(1000);
            if (patchparm->state == DTV_PATCH_STATE_STOPED) {
                goto exit;
            }
            continue;
        }
        audec->pcm_bytes_readed += len;
        {
           if (audec->format == ACODEC_FMT_DRA) {
                patchparm->info_cb(patchparm->pargs,audec->adec_ops->NchOriginal,
                 audec->adec_ops->lfepresent);
           }
            channels = audec->g_bst->channels;
            samplerate = audec->g_bst->samplerate;
            len2 = patchparm->pcmout_cb((unsigned char *)(buffer + offset), len, samplerate,
                                        channels, patchparm->pargs);
            if (len2 == 0) {
                if (patchparm->state == DTV_PATCH_STATE_STOPED) {
                    goto exit;
                }
            }
        }

        if (len2 >= 0) {
            len -= len2;
            offset += len2;
        } else {
            len = 0;
            offset = 0;
        }
        // pthread_mutex_unlock(&patch_out_mutex);
    }
exit:
    adec_print("Exit alsa playback loop !\n");
    pthread_exit(NULL);
    return NULL;
}

int dtv_patch_input_open(unsigned int *handle, out_pcm_write pcmcb,
                         out_get_wirte_buffer_info buffercb, out_audio_info info_cb,void *args)
{
    //int ret;

    if (handle == NULL || pcmcb == NULL || buffercb == NULL) {
        return -1;
    }

    dtv_patch_out *param = get_patchout();

    pthread_mutex_lock(&patch_out_mutex);
    param->pcmout_cb = pcmcb;
    param->buffer_cb = buffercb;
    param->info_cb = info_cb;
    param->pargs = args;
    param->device_opened = 1;
    amthreadpool_system_init();
    pthread_mutex_unlock(&patch_out_mutex);
    adec_print("now the audio decoder open now!\n");
    *handle = (int)param;

    return 0;
}

int dtv_patch_input_start(unsigned int handle, int aformat, int has_video,
           bool associate_dec_supported,bool associate_audio_mixing_enable,int dual_decoder_mixing_level)
{
    int ret;
    adec_print("now enter the dtv_patch_input_start function handle %d "
               "out_patch_initd %d  \n ",
               handle, out_patch_initd);

    if (handle == 0) {
        return -1;
    }

    pthread_mutex_lock(&patch_out_mutex);
    if (out_patch_initd == 1) {
        pthread_mutex_unlock(&patch_out_mutex);
        return -1;
    }

    arm_audio_info param;
    dtv_patch_out *paramout = get_patchout();

    if (paramout->state == DTV_PATCH_STATE_RUNNING) {
        pthread_mutex_unlock(&patch_out_mutex);
        return -1;
    }

    adec_print("now the audio decoder start  now, aformat %d  has_video %d!\n",
               aformat, has_video);
    memset(&param, 0, sizeof(param));
    param.handle = -1;
    param.format = aformat;
    param.has_video = has_video;
    paramout->audec = NULL;
    if (aformat == ACODEC_FMT_MPEG || aformat == ACODEC_FMT_AAC) {
        param.associate_dec_supported = associate_dec_supported;
        param.associate_mixing_enable = associate_audio_mixing_enable;
        param.mixing_level = dual_decoder_mixing_level;
    } else {
        param.associate_dec_supported = 0;
        param.associate_mixing_enable = 0;
        param.mixing_level = 0;
    }

    //param.mixing_level = _get_asso_mix();
    paramout->state = DTV_PATCH_STATE_RUNNING;
    audio_decode_init((void **)(&(paramout->audec)), &param);
    ret = pthread_create(&(paramout->tid), NULL, (void *)dtv_patch_out_loop, paramout);
    out_patch_initd = 1;
    pthread_mutex_unlock(&patch_out_mutex);
    adec_print("now leave the dtv_patch_input_start function \n ");

    return 0;
}

int dtv_patch_input_stop(unsigned int handle)
{

    if (handle == 0) {
        return -1;
    }

    dtv_patch_out *paramout = get_patchout();
    pthread_mutex_lock(&patch_out_mutex);
    if (out_patch_initd == 0) {
        pthread_mutex_unlock(&patch_out_mutex);
        return -1;
    }

    adec_print("now enter the audio decoder stop now!\n");

    paramout->state = DTV_PATCH_STATE_STOPED;
    pthread_join(paramout->tid, NULL);
    adec_print("now enter the audio decoder stop now111111!\n");
    audio_decode_stop(paramout->audec);
    audio_decode_release((void **) & (paramout->audec));

    adec_print("now enter the audio decoder stop now222222!\n");
    paramout->audec = NULL;
    adec_print("now enter the audio decoder stop now!\n");
    paramout->tid = -1;
    out_patch_initd = 0;
    pthread_mutex_unlock(&patch_out_mutex);

    adec_print("now leave the audio decoder stop now!\n");
    return 0;
}

int dtv_patch_input_pause(unsigned int handle)
{
    if (handle == 0) {
        return -1;
    }
    dtv_patch_out *paramout = get_patchout();
    pthread_mutex_lock(&patch_out_mutex);
    audio_decode_pause(paramout->audec);
    paramout->state = DTV_PATCH_STATE_PAUSE;
    pthread_mutex_unlock(&patch_out_mutex);
    return 0;
}

int dtv_patch_input_resume(unsigned int handle)
{
    if (handle == 0) {
        return -1;
    }
    dtv_patch_out *paramout = get_patchout();
    pthread_mutex_lock(&patch_out_mutex);
    audio_decode_resume(paramout->audec);
    paramout->state = DTV_PATCH_STATE_RUNNING;
    pthread_mutex_unlock(&patch_out_mutex);
    return 0;
}
