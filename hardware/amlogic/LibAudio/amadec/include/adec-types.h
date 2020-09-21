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
 *     brief  Definitiond Of Audio Dec Types.
 *
 */

#ifndef ADEC_TYPES_H
#define ADEC_TYPES_H
typedef enum {
    ADEC_AUDIO_FORMAT_UNKNOWN = -1,
    ADEC_AUDIO_FORMAT_MPEG = 0,
    ADEC_AUDIO_FORMAT_PCM_S16LE,
    ADEC_AUDIO_FORMAT_AAC,
    ADEC_AUDIO_FORMAT_AC3,
    ADEC_AUDIO_FORMAT_ALAW,
    ADEC_AUDIO_FORMAT_MULAW,
    ADEC_AUDIO_FORMAT_DTS,
    ADEC_AUDIO_FORMAT_PCM_S16BE,
    ADEC_AUDIO_FORMAT_FLAC,
    ADEC_AUDIO_FORMAT_COOK,
    ADEC_AUDIO_FORMAT_PCM_U8,
    ADEC_AUDIO_FORMAT_ADPCM,
    ADEC_AUDIO_FORMAT_AMR,
    ADEC_AUDIO_FORMAT_RAAC,
    ADEC_AUDIO_FORMAT_WMA,
    ADEC_AUDIO_FORMAT_WMAPRO,
    ADEC_AUDIO_AFORMAT_PCM_BLURAY,
    ADEC_AUDIO_AFORMAT_ALAC,
    ADEC_AUDIO_AFORMAT_VORBIS,
    ADEC_AUDIO_FORMAT_AAC_LATM,
    ADEC_AUDIO_FORMAT_APE,
    ADEC_AUDIO_FORMAT_EAC3,
    ADEC_AUDIO_FORMAT_PCM_WIFIDISPLAY,
    ADEC_AUDIO_FORMAT_MAX,
} adec_audio_format_t;

#define VALID_FMT(f)    ((f>ADEC_AUDIO_FORMAT_UNKNOWN)&& (f<ADEC_AUDIO_FORMAT_MAX))

typedef enum {
    IDLE,
    TERMINATED,
    STOPPED,
    INITING,
    INITTED,
    ACTIVE,
    PAUSED,
} adec_state_t;

#endif
