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

#include <audio-dec.h>
#include <adec-pts-mgt.h>
#include <adec_write.h>
#include "adec_omx_brige.h"
#include <amthreadpool.h>
#include "Amsysfsutils.h"
#include "amconfigutils.h"

#define  LOG_TAG    "Adec_omx_bridge"
#define adec_print(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define SIZE_FOR_MX_BYPASS (48*1024) //about 0.25s for 48K_16bit

int find_omx_lib(aml_audio_dec_t *audec)
{
    //audio_decoder_operations_t *adec_ops = audec->adec_ops;
    audec->StageFrightCodecEnableType = 0;
    audec->parm_omx_codec_init = NULL;
    audec->parm_omx_codec_read = NULL;
    audec->parm_omx_codec_close = NULL;
    audec->parm_omx_codec_start = NULL;
    audec->parm_omx_codec_pause = NULL;
    audec->parm_omx_codec_get_declen = NULL;
    audec->parm_omx_codec_get_FS = NULL;
    audec->parm_omx_codec_get_Nch = NULL;

    if (audec->format == ACODEC_FMT_AC3) {
#ifndef USE_ARM_AUDIO_DEC
        audec->adec_ops->nOutBufSize = SIZE_FOR_MX_BYPASS;
#endif
        audec->StageFrightCodecEnableType = OMX_ENABLE_CODEC_AC3;
    } else if (audec->format == ACODEC_FMT_EAC3) {
#ifndef USE_ARM_AUDIO_DEC
        audec->adec_ops->nOutBufSize = SIZE_FOR_MX_BYPASS;
#endif
        audec->StageFrightCodecEnableType = OMX_ENABLE_CODEC_EAC3;
    } else if (audec->format == ACODEC_FMT_ALAC) {
        audec->StageFrightCodecEnableType = OMX_ENABLE_CODEC_ALAC;
    } else if (audec->format == ACODEC_FMT_MPEG || audec->format == ACODEC_FMT_MPEG1 || audec->format == ACODEC_FMT_MPEG2) {
        if (0/*!am_getconfig_bool("media.libplayer.usemad")*/) {
            audec->StageFrightCodecEnableType = OMX_ENABLE_CODEC_MPEG_LAYER_II;    //mpeg1-3, new omx libmpg123
        }
    } else if (audec->format == ACODEC_FMT_WMA) {
        audec->StageFrightCodecEnableType = OMX_ENABLE_CODEC_WMA;
    } else if (audec->format == ACODEC_FMT_WMAPRO) {
        audec->StageFrightCodecEnableType = OMX_ENABLE_CODEC_WMAPRO;
    } else if (audec->format == ACODEC_FMT_DTS) {
#ifndef USE_ARM_AUDIO_DEC
        audec->adec_ops->nOutBufSize = SIZE_FOR_MX_BYPASS;
#endif
        audec->StageFrightCodecEnableType = OMX_ENABLE_CODEC_DTSHD;
    } else if (audec->format == ACODEC_FMT_VORBIS) {
        audec->StageFrightCodecEnableType = OMX_ENABLE_CODEC_VORBIS;
    } else if (audec->format == ACODEC_FMT_TRUEHD) {
        audec->StageFrightCodecEnableType = OMX_ENABLE_CODEC_TRUEHD;
    } else if (audec->format == ACODEC_FMT_WMAVOI) {
        audec->StageFrightCodecEnableType = OMX_ENABLE_CODEC_WMAVOI;
    }

    adec_print("%s %d audec->format=%d \n", __FUNCTION__, __LINE__, audec->format);

    if (audec->StageFrightCodecEnableType) {
        int *fd = NULL;
        fd = dlopen("libamadec_omx_api.so", RTLD_NOW);
        if (fd != NULL) {
            audec->parm_omx_codec_init      = dlsym(fd, "arm_omx_codec_init");
            audec->parm_omx_codec_read      = dlsym(fd, "arm_omx_codec_read");
            audec->parm_omx_codec_close     = dlsym(fd, "arm_omx_codec_close");
            audec->parm_omx_codec_start     = dlsym(fd, "arm_omx_codec_start");
            audec->parm_omx_codec_pause     = dlsym(fd, "arm_omx_codec_pause");
            audec->parm_omx_codec_get_declen = dlsym(fd, "arm_omx_codec_get_declen");
            audec->parm_omx_codec_get_FS    = dlsym(fd, "arm_omx_codec_get_FS");
            audec->parm_omx_codec_get_Nch   = dlsym(fd, "arm_omx_codec_get_Nch");
        } else {
            adec_print("[NOTE]cant find libamadec_omx_api.so ,StageFrightCodecEnableType=0\n");
            audec->StageFrightCodecEnableType = 0;
            return 0;
        }
    }

    if (audec->parm_omx_codec_init   == NULL || audec->parm_omx_codec_read   == NULL || audec->parm_omx_codec_close == NULL ||
        audec->parm_omx_codec_start  == NULL || audec->parm_omx_codec_pause  == NULL || audec->parm_omx_codec_get_declen == NULL ||
        audec->parm_omx_codec_get_FS == NULL || audec->parm_omx_codec_get_Nch == NULL
       ) {
        adec_print("[NOTE]load func_api in libamadec_omx_api.so faided, StageFrightCodecEnableType=0\n");
        audec->StageFrightCodecEnableType = 0;
        return 0;
    }
    adec_print("%s %d StageFrightCodecEnableType=%d \n", __FUNCTION__, __LINE__, audec->StageFrightCodecEnableType);
    return audec->StageFrightCodecEnableType;
}

#define AVCODEC_MAX_AUDIO_FRAME_SIZE 500*1024
static char pcm_buf_tmp[AVCODEC_MAX_AUDIO_FRAME_SIZE];//max frame size out buf
void omx_codec_Release();
extern int read_buffer(unsigned char *buffer, int size);

void *audio_decode_loop_omx(void *args)
{
    //int ret;
    aml_audio_dec_t *audec;
    audio_out_operations_t *aout_ops;
    audio_decoder_operations_t *adec_ops;

    int nNextFrameSize = 0; //next read frame size
    int nAudioFormat;
    //char *inbuf = NULL;//real buffer
    int dlen = 0;//decode size one time
    int outlen = 0;
    char *outbuf = pcm_buf_tmp, *outbuf_raw;
    int outlen_raw = 0;
    int rawoutput_enable;
    buffer_stream_t *g_bst, *g_bst_raw;
    AudioInfo  g_AudioInfo = {0};
    adec_print("\n\naudio_decode_loop_omx start!\n");

    audec = (aml_audio_dec_t *)args;
    aout_ops = &audec->aout_ops;
    adec_ops = audec->adec_ops;
    memset(outbuf, 0, AVCODEC_MAX_AUDIO_FRAME_SIZE);
    nAudioFormat = audec->format;
    nNextFrameSize = adec_ops->nInBufSize;
    g_bst = audec->g_bst;
    g_bst_raw = audec->g_bst_raw;

    rawoutput_enable = amsysfs_get_sysfs_int("/sys/class/audiodsp/digital_raw");
    adec_print("rawoutput_enable/%d", rawoutput_enable);
    if (rawoutput_enable == 1 && audec->StageFrightCodecEnableType == OMX_ENABLE_CODEC_TRUEHD) {
        adec_print("truehd passthrough enable only when hdmi passthr\n");
        rawoutput_enable = 0;
    }
    if (audec->parm_omx_codec_init && audec->parm_omx_codec_start) {
        (*audec->parm_omx_codec_init)(audec, audec->StageFrightCodecEnableType, (void*)read_buffer, &audec->exit_decode_thread);
        (*audec->parm_omx_codec_start)(audec);
    } else {
        audec->exit_decode_thread = 1;
        adec_print("audio_decode_loop_omx start failed!");
    }
    audec->OmxFirstFrameDecoded = 0;
    while (1) {
//exit_decode_loop:
        if (audec->exit_decode_thread) { //detect quit condition
            break;
        }
        outbuf = pcm_buf_tmp;
        outlen = AVCODEC_MAX_AUDIO_FRAME_SIZE;
        (*audec->parm_omx_codec_read)(audec, (unsigned char *)outbuf, (unsigned int *)&outlen, &audec->exit_decode_thread);

        outlen_raw = 0;
        if (audec->StageFrightCodecEnableType == OMX_ENABLE_CODEC_DTSHD ||
            audec->StageFrightCodecEnableType == OMX_ENABLE_CODEC_TRUEHD) {
            int outlen0 = outlen;
            if (outlen > 8) {
                memcpy(&outlen, outbuf, 4);
                outbuf += 4;
                if (outlen + 8 < outlen0) {
                    memcpy(&outlen_raw, outbuf + outlen, 4);
                    outbuf_raw = outbuf + outlen + 4;
                }
            } else {
                outlen = 0;
            }
        } else if ((audec->StageFrightCodecEnableType == OMX_ENABLE_CODEC_AC3)   ||
                   (audec->StageFrightCodecEnableType == OMX_ENABLE_CODEC_EAC3)) {
            if (outlen > 8) {
                memcpy(&outlen, outbuf, 4);
                outbuf += 4;
                memcpy(&outlen_raw, outbuf + outlen, 4);
                outbuf_raw = outbuf + outlen + 4;
                memset(outbuf + outlen, 0, 4);
            } else {
                outlen = 0;
            }
        }
        if (outlen > 0) {
            memset(&g_AudioInfo, 0, sizeof(AudioInfo));
            g_AudioInfo.channels = (*audec->parm_omx_codec_get_Nch)(audec);
            g_AudioInfo.samplerate = (*audec->parm_omx_codec_get_FS)(audec);
            if (g_AudioInfo.channels != 0 && g_AudioInfo.samplerate != 0) {
                if (!audec->OmxFirstFrameDecoded) {
                    g_bst->channels = audec->channels = g_AudioInfo.channels;
                    g_bst->samplerate = audec->samplerate = g_AudioInfo.samplerate;
                    audec->OmxFirstFrameDecoded = 1;
                } else if (audec->OmxFirstFrameDecoded == 1) {
                    if ((g_AudioInfo.channels != g_bst->channels) || (g_AudioInfo.samplerate != g_bst->samplerate)) {
                        while (audec->format_changed_flag && !audec->exit_decode_thread) {
                            amthreadpool_thread_usleep(20000);
                        }
                        if (!audec->exit_decode_thread) {
                            adec_print("Info Changed: src:sample:%d  channel:%d dest sample:%d  channel:%d \n",
                                       g_bst->samplerate, g_bst->channels, g_AudioInfo.samplerate, g_AudioInfo.channels);
                            g_bst->channels = g_AudioInfo.channels;
                            g_bst->samplerate = g_AudioInfo.samplerate;
                            aout_ops->pause(audec);
                            audec->format_changed_flag = 1;
                        }
                    }
                }
            }
        }
        if (outlen > 0) {
            dlen = (*audec->parm_omx_codec_get_declen)(audec);
        } else {
            dlen = 0;
        }
        //write to the pcm buffer
        audec->decode_offset += dlen;
        audec->pcm_cache_size = outlen;

        if (g_bst) {
            int wlen = 0;
            while (outlen && !audec->exit_decode_thread) {
                if ((g_bst->buf_length - g_bst->buf_level) < outlen) {
                    amthreadpool_thread_usleep(20000);
                    continue;
                }
                wlen = write_pcm_buffer(outbuf, g_bst, outlen);
                outlen -= wlen;
                audec->pcm_cache_size -= wlen;
            }

            while (rawoutput_enable && !audec->exit_decode_thread && outlen_raw && aout_ops->audio_out_raw_enable) {
                if (g_bst_raw->buf_length - g_bst_raw->buf_level < outlen_raw) {
                    amthreadpool_thread_usleep(20000);
                    continue;
                }
                wlen = write_pcm_buffer(outbuf_raw, g_bst_raw, outlen_raw);
                outlen_raw -= wlen;
            }
        }
    }

    adec_print("[%s %d] has stepped out decodeloop \n", __FUNCTION__, __LINE__);
    if (audec->StageFrightCodecEnableType && audec->parm_omx_codec_close) {
        (*audec->parm_omx_codec_close)(audec);
    }
    adec_print("Exit audio_decode_loop_omx Thread finished!");
    pthread_exit(NULL);
    return NULL;
}


void start_decode_thread_omx(aml_audio_dec_t *audec)
{
    int ret;
    pthread_t    tid;
    int wait_aout_ops_start_time = 0;

    ret = amthreadpool_pthread_create(&tid, NULL, (void *)audio_decode_loop_omx, (void *)audec);
    if (ret != 0) {
        adec_print("Create <audio_decode_loop_omx> thread failed!\n");
        return;
    }
    audec->sn_threadid = tid;
    pthread_setname_np(tid, "AmadecDecodeLP");
    adec_print("Create <audio_decode_loop_omx> thread success! tid = %ld\n", tid);

    while ((!audec->need_stop) && (!audec->OmxFirstFrameDecoded)) {
        amthreadpool_thread_usleep(50);
        wait_aout_ops_start_time++;
    }
    adec_print("[%s] start thread finished: <audec->OmxFirstFrameDecoded=%d> used time: %d*50(us)\n", __FUNCTION__, audec->OmxFirstFrameDecoded, wait_aout_ops_start_time);

}


void stop_decode_thread_omx(aml_audio_dec_t *audec)
{
    audec->exit_decode_thread = 1;
    amthreadpool_pthread_join(audec->sn_threadid, NULL);
    //audec->exit_decode_thread = 0;
    audec->sn_threadid = -1;
    audec->sn_getpackage_threadid = -1;
}




