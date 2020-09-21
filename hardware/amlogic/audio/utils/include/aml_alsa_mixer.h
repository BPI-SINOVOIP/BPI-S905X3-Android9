/*
 * Copyright (C) 2017 Amlogic Corporation.
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
 */

#ifndef _AML_ALSA_MIXER_H_
#define _AML_ALSA_MIXER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>
#include <tinyalsa/asoundlib.h>

/*
 *  Value of the Alsa Mixer Control Point
 **/
/* Audio i2s mute */
typedef enum MIXER_AUDIO_I2S_MUTE {
    I2S_MUTE_OFF = 0,
    I2S_MUTE_ON  = 1,
    I2S_MUTE_MAX,
} eMixerAudioI2sMute;

/* Audio spdif mute */
typedef enum MIXER_SPDIF_MUTE {
    SPDIF_MUTE_OFF = 0,
    SPDIF_MUTE_ON  = 1,
    SPDIF_MUTE_MAX,
} eMixerSpdifMute;

/* Audio In Source */
typedef enum MIXER_AUDIO_IN_SOURCE {
    AUDIOIN_SRC_LINEIN  = 0,
    AUDIOIN_SRC_ATV     = 1,
    AUDIOIN_SRC_HDMI    = 2,
    AUDIOIN_SRC_SPDIFIN = 3,
    AUDIOIN_SRC_MAX,
} eMixerAudioInSrc;

/* Audio I2SIN Audio Type */
typedef enum MIXER_I2SIN_AUDIO_TYPE {
    I2SIN_AUDIO_TYPE_LPCM      = 0,
    I2SIN_AUDIO_TYPE_NONE_LPCM = 1,
    I2SIN_AUDIO_TYPE_UN_KNOWN  = 2,
    I2SIN_AUDIO_TYPE_MAX,
} eMixerI2sInAudioType;

/* Audio SPDIFIN Audio Type */
typedef enum MIXER_SPDIFIN_AUDIO_TYPE {
    SPDIFIN_AUDIO_TYPE_LPCM   = 0,
    SPDIFIN_AUDIO_TYPE_AC3    = 1,
    SPDIFIN_AUDIO_TYPE_EAC3   = 2,
    SPDIFIN_AUDIO_TYPE_DTS    = 3,
    SPDIFIN_AUDIO_TYPE_DTSHD  = 4,
    SPDIFIN_AUDIO_TYPE_TRUEHD = 5,
    SPDIFIN_AUDIO_TYPE_PAUSE  = 6,
    SPDIFIN_AUDIO_TYPE_MAX,
} eMixerApdifinAudioType;

/* Hardware resample enable */
typedef enum MIXER_HW_RESAMPLE_ENABLE {
    HW_RESAMPLE_DISABLE = 0,
    HW_RESAMPLE_32K,
    HW_RESAMPLE_44K,
    HW_RESAMPLE_48K,
    HW_RESAMPLE_88K,
    HW_RESAMPLE_96K,
    HW_RESAMPLE_176K,
    HW_RESAMPLE_192K,
    HW_RESAMPLE_MAX,
} eMixerHwResample;

/* Output Swap */
typedef enum MIXER_OUTPUT_SWAP {
    OUTPUT_SWAP_LR = 0,
    OUTPUT_SWAP_LL = 1,
    OUTPUT_SWAP_RR = 2,
    OUTPUT_SWAP_RL = 3,
    OUTPUT_SWAP_MAX,
} eMixerOutputSwap;

/* spdifin/arcin */
typedef enum AUDIOIN_SWITCH {
    SPDIF_IN = 0,
    ARC_IN = 1,
} eMixerAudioinSwitch;

struct aml_mixer_ctrl {
    int  ctrl_idx;
    char ctrl_name[50];
};

/* the same as toddr source*/
typedef enum ResampleSource {
    RESAMPLE_FROM_SPDIFIN = 3,
    RESAMPLE_FROM_FRHDMIRX = 8,
} eMixerAudioResampleSource;

/*
 *  Alsa Mixer Control Command List
 **/
typedef enum AML_MIXER_CTRL_ID {
    AML_MIXER_ID_I2S_MUTE = 0,
    AML_MIXER_ID_SPDIF_MUTE,
    AML_MIXER_ID_HDMI_OUT_AUDIO_MUTE,
    AML_MIXER_ID_HDMI_ARC_AUDIO_ENABLE,
	/* eARC latency and CDS */
    AML_MIXER_ID_HDMI_EARC_AUDIO_ENABLE,
    AML_MIXER_ID_EARCRX_LATENCY,
    AML_MIXER_ID_EARCTX_LATENCY,
    AML_MIXER_ID_EARCRX_CDS, /* Capability Data Structure */
    AML_MIXER_ID_EARCTX_CDS,
    AML_MIXER_ID_AUDIO_IN_SRC,
    AML_MIXER_ID_I2SIN_AUDIO_TYPE,
    AML_MIXER_ID_SPDIFIN_AUDIO_TYPE,
    AML_MIXER_ID_HW_RESAMPLE_ENABLE,
    AML_MIXER_ID_OUTPUT_SWAP,
    AML_MIXER_ID_HDMI_IN_AUDIO_STABLE,
    AML_MIXER_ID_HDMI_IN_SAMPLERATE,
    AML_MIXER_ID_HDMI_IN_CHANNELS,
    AML_MIXER_ID_HDMI_IN_FORMATS,
    AML_MIXER_ID_HDMI_ATMOS_EDID,
    AML_MIXER_ID_ATV_IN_AUDIO_STABLE,
    AML_MIXER_ID_SPDIF_FORMAT,
    AML_MIXER_ID_AV_IN_AUDIO_STABLE,
    AML_MIXER_ID_EQ_MASTER_VOLUME,
    AML_MIXER_ID_SPDIFIN_ARCIN_SWITCH,
    AML_MIXER_ID_SPDIFIN_PAO,
    AML_MIXER_ID_HDMIIN_AUDIO_TYPE,
    AML_MIXER_ID_SPDIFIN_SRC,
    AML_MIXER_ID_HDMIIN_AUDIO_PACKET,
    AML_MIXER_ID_CHANGE_SPIDIF_PLL,
    AML_MIXER_ID_CHANGE_I2S_PLL,
    AML_MIXER_ID_AED_EQ_ENABLE,
    AML_MIXER_ID_AED_MULTI_DRC_ENABLE,
    AML_MIXER_ID_AED_FULL_DRC_ENABLE,
    AML_MIXER_ID_SPDIF_IN_SAMPLERATE,
    AML_MIXER_ID_HW_RESAMPLE_SOURCE,
    AML_MIXER_ID_MAX,
} eMixerCtrlID;

/*
 *tinymix "Audio spdif format" list
 */
enum AML_SPDIF_FORMAT {
    AML_STEREO_PCM = 0,
    AML_DTS_RAW_MODE = 1,
    AML_DOLBY_DIGITAL = 2,
    AML_DTS = 3,
    AML_DOLBY_DIGITAL_PLUS = 4,
    AML_DTS_HD = 5,
    AML_MULTI_CH_LPCM = 6,
    AML_TRUE_HD = 7,
    AML_DTS_HD_MA = 8,
    AML_HIGH_SR_STEREO_LPCM = 9,
};

struct aml_mixer_list {
    int  id;
    char mixer_name[50];
};

struct aml_mixer_handle {
    struct mixer *pMixer;
    pthread_mutex_t lock;
};

int open_mixer_handle(struct aml_mixer_handle *mixer_handle);
int close_mixer_handle(struct aml_mixer_handle *mixer_handle);

/*
 * get interface
 **/
int aml_mixer_ctrl_get_int(struct aml_mixer_handle *mixer_handle, int mixer_id);
int aml_mixer_ctrl_get_enum_str_to_int(struct aml_mixer_handle *mixer_handle, int mixer_id, int *ret);

// or
#if 0
int aml_mixer_get_audioin_src(int mixer_id);
int aml_mixer_get_i2sin_type(int mixer_id);
int aml_mixer_get_spdifin_type(int mixer_id);
#endif

/*
 * set interface
 **/
int aml_mixer_ctrl_set_int(struct aml_mixer_handle *mixer_handle, int mixer_id, int value);
int aml_mixer_ctrl_set_str(struct aml_mixer_handle *mixer_handle, int mixer_id, char *value);

int mixer_get_int(struct mixer *pMixer, int mixer_id);
int mixer_set_int(struct mixer *pMixer, int mixer_id, int value);
int mixer_get_array(struct mixer *pMixer, int mixer_id, void *array, int count);
int mixer_set_array(struct mixer *pMixer, int mixer_id, void *array, int count);

#ifdef __cplusplus
}
#endif

#endif
