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

#ifndef __ADEC_OMXDDPDEC_BRIGE_H__
#define __ADEC_OMXDDPDEC_BRIGE_H__

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



enum OMX_CodecID {
    OMX_ENABLE_CODEC_NULL = 0,
    OMX_ENABLE_CODEC_AC3,
    OMX_ENABLE_CODEC_EAC3,
    OMX_ENABLE_CODEC_AMR_NB,
    OMX_ENABLE_CODEC_AMR_WB,
    OMX_ENABLE_CODEC_MPEG,
    OMX_ENABLE_CODEC_MPEG_LAYER_I,
    OMX_ENABLE_CODEC_MPEG_LAYER_II,
    OMX_ENABLE_CODEC_AAC,
    OMX_ENABLE_CODEC_QCELP,
    OMX_ENABLE_CODEC_VORBIS,
    OMX_ENABLE_CODEC_G711_ALAW,
    OMX_ENABLE_CODEC_G711_MLAW,
    OMX_ENABLE_CODEC_RAW,
    OMX_ENABLE_CODEC_ADPCM_IMA,
    OMX_ENABLE_CODEC_ADPCM_MS,
    OMX_ENABLE_CODEC_FLAC,
    OMX_ENABLE_CODEC_AAC_ADTS,
    OMX_ENABLE_CODEC_ALAC,
    OMX_ENABLE_CODEC_AAC_ADIF,
    OMX_ENABLE_CODEC_AAC_LATM,
    OMX_ENABLE_CODEC_ADTS_PROFILE,
    OMX_ENABLE_CODEC_WMA,
    OMX_ENABLE_CODEC_WMAPRO,
    OMX_ENABLE_CONTAINER_WAV,
    OMX_ENABLE_CONTAINER_AIFF,
    OMX_ENABLE_CODEC_DTSHD,
    OMX_ENABLE_CODEC_TRUEHD,
    OMX_ENABLE_CODEC_WMAVOI,
};


void stop_decode_thread_omx(aml_audio_dec_t *audec);
void start_decode_thread_omx(aml_audio_dec_t *audec);
int find_omx_lib(aml_audio_dec_t *audec);
#endif

