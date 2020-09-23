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
 *      This file implements a special EQ  from Amlogic.
 *
 */

#define LOG_TAG "TruSurround_Effect"
//#define LOG_NDEBUG 0

#include <cutils/log.h>
#include <utils/Log.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <dlfcn.h>
#include <hardware/audio_effect.h>
#include <stdio.h>
#include <unistd.h>
#include "IniParser.h"
#include "tshd_wrapper.h"

extern "C" {

#define DEFAULT_INI_FILE_PATH "/tvconfig/audio/amlogic_audio_effect_default.ini"
#if defined(__LP64__)
#define LIBSRS_PATH "/system/lib64/soundfx/libsrs.so"
#else
#define LIBSRS_PATH "/system/lib/soundfx/libsrs.so"
#endif

// effect_handle_t interface implementation for SRS effect
extern const struct effect_interface_s SRSInterface;

// SRS effect TYPE: 1424f5a0-2457-11e6-9fe0-0002a5d5c51b
// SRS effect UUID: 8a857720-0209-11e2-a9d8-0002a5d5c51b
const effect_descriptor_t SRSDescriptor = {
        {0x1424f5a0, 0x2457, 0x11e6, 0x9fe0, {0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b}}, // type
        {0x8a857720, 0x0209, 0x11e2, 0xa9d8, {0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b}}, // uuid
        EFFECT_CONTROL_API_VERSION,
        EFFECT_FLAG_TYPE_POST_PROC | EFFECT_FLAG_DEVICE_IND | EFFECT_FLAG_NO_PROCESS | EFFECT_FLAG_OFFLOAD_SUPPORTED,
        SRS_CUP_LOAD_ARM9E,
        SRS_MEM_USAGE,
        "True Surround HD",
        "SRS Labs",
};

enum SRS_state_e {
    SRS_STATE_UNINITIALIZED,
    SRS_STATE_INITIALIZED,
    SRS_STATE_ACTIVE,
};

typedef enum {
    SRS_MODE_STANDARD = 0,
    SRS_MODE_MUSIC,
    SRS_MODE_MOVIE,
    SRS_MODE_CLEAR_VOICE,
    SRS_MODE_ENHANCED_BASS,
    SRS_MODE_CUSTOM,
    SRS_MODE_NUM,
} SRSmode;

typedef enum {
    SRS_PARAM_MODE = 0,
    SRS_PARAM_DIALOGCLARTY_MODE,
    SRS_PARAM_SURROUND_MODE,
    SRS_PARAM_VOLUME_MODE,
    SRS_PARAM_ENABLE,
    SRS_PARAM_TRUEBASS_ENABLE,
    SRS_PARAM_TRUEBASS_SPKER_SIZE,
    SRS_PARAM_TRUEBASS_GAIN,
    SRS_PARAM_DIALOG_CLARITY_ENABLE,
    SRS_PARAM_DIALOGCLARTY_GAIN,
    SRS_PARAM_DEFINITION_ENABLE,
    SRS_PARAM_DEFINITION_GAIN,
    SRS_PARAM_SURROUND_ENABLE,
    SRS_PARAM_SURROUND_GAIN,
    SRS_PARAM_INPUT_GAIN,
    SRS_PARAM_OUTPUT_GAIN,
    SRS_PARAM_OUTPUT_GAIN_COMP,
} SRSparams;

typedef struct SRSapi_s {
    int (*SRS_init)(void *data);
    int (*SRS_release)(void);
    int (*SRS_reset)(int sample_rate);
    int (*SRS_TRUEBASS_ENABLE)(int value);
    int (*SRS_DIALOGCLARITY_ENABLE)(int value);
    int (*SRS_DEFINITION_ENABLE)(int value);
    int (*SRS_SURROUND_ENABLE)(int value);
    int (*SRS_process)(short *in, short *out, int framecount);
    int (*SRS_setTruebass_spkersize)(int truebass_spker_size);
    int (*SRS_getTruebass_spkersize)(int *truebass_spker_size);
    int (*SRS_setTruebass_gain)(float truebass_gain);
    int (*SRS_getTruebass_gain)(float *truebass_gain);
    int (*SRS_setDialogclarity_gain)(float dialogclarity_gain);
    int (*SRS_getDialogclarity_gain)(float *dialogclarity_gain);
    int (*SRS_setDefinition_gain)(float definition_gain);
    int (*SRS_getDefinition_gain)(float *definition_gain);
    int (*SRS_setSurround_gain)(float surround_gain);
    int (*SRS_getSurround_gain)(float *surround_gain);
    int (*SRS_setInput_gain)(float input_gain);
    int (*SRS_getInput_gain)(float *input_gain);
    int (*SRS_setOutput_gain)(float output_gain);
    int (*SRS_getOutput_gain)(float *output_gain);
    int (*SRS_getTruebass_status)(int *truebass_enable);
    int (*SRS_getDialogclarity_status)(int *dialogclarity_enable);
    int (*SRS_getDefinition_status)(int *dialogclarity_enable);
    int (*SRS_getSurround_status)(int *surround_enable);
} SRSapi;

typedef struct SRStruebass_param_s {
    int     enable;
    int     spkersize;
    float   gain;
} SRStruebass_param;

typedef struct SRSdialogclarity_param_s {
    int     enable;
    float   gain;
} SRSdialogclarity_param;

typedef struct SRSdefinition_param_s {
    int     enable;
    float   gain;
} SRSdefinition_param;

typedef struct SRSsurround_param_s {
    int     enable;
    float   gain;
} SRSsurround_param;

typedef struct SRSgain_param_s {
    float   input_gain;
    float   output_gain;
    float   comp_gain; //output gain compensation
} SRSgain_param;

typedef struct TS_SRScfg_s {
    SRStruebass_param           truebass;
    SRSdefinition_param         definition;
    SRSsurround_param           surround;
    SRSgain_param               gain;
} TS_SRScfg;

typedef struct DC_SRScfg_s {
    SRSdialogclarity_param      dialogclarity;
} DC_SRScfg;

#define TS_MODE_MUM     2
#define DC_MODE_MUM     3

typedef struct SRSdata_s {
    /* This struct is used to initialize SRS default config*/
    struct {
        int         truebass_enable;
        int         truebass_spker_size;
        float       truebass_gain;

        int         dialogclarity_enable;
        float       dialogclarity_gain;

        int         definition_enable;
        float       definition_gain;

        int         surround_enable;
        float       surround_gain;

        float       input_gain;
        float       output_gain;
        float       comp_gain;
    };
    int                     enable;
    int                     sound_mode;
    int                     dialogclarity_mode;
    int                     surround_mode;
    TS_SRScfg               TS_usr_cfg[TS_MODE_MUM];
    DC_SRScfg               DC_usr_cfg[DC_MODE_MUM];
} SRSdata;

typedef struct SRSContext_s {
    const struct effect_interface_s *itfe;
    effect_config_t                 config;
    SRS_state_e                     state;
    void                            *gSRSLibHandler;
    SRSapi                          gSRSapi;
    SRSdata                         gSRSdata;
} SRSContext;

static TS_SRScfg TS_default_usr_cfg[TS_MODE_MUM] = {
    {{1, 2, 0.27}, {1, 0.03}, {1, 0.08}, {0.2, 1.0, 14.25}}, //ON
    {{1, 2, 0.25}, {1, 0.03}, {0, 0.0},  {0.2, 1.0, 14.25}}, //OFF
};

static DC_SRScfg DC_default_usr_cfg[DC_MODE_MUM] = {
    {{1, 0.0}},  //OFF
    {{1, 0.15}}, //LOW
    {{1, 0.3}},  //HIGH
};

const char *SRSModestr[] = {"Standard", "Music", "Movie", "Clear Voice", "Enhanced Bass", "Custom"};

const char *SRSStatusstr[] = {"Disable", "Enable"};
const char *SRSTrueBassstr[] = {"Disable", "Enable"};
const char *SRSDialogClaritystr[] = {"Disable", "Enable"};
const char *SRSDefinitionstr[] = {"Disable", "Enable"};
const char *SRSSurroundstr[] = {"Disable", "Enable"};

const char *SRSDialogClarityModestr[DC_MODE_MUM] = {"OFF", "LOW", "HIGH"};
const char *SRSSurroundModestr[TS_MODE_MUM] = {"ON", "OFF"};

float db_to_ampl(float decibels)
{
    /*exp( dB * ln(10) / 20 )*/
    return exp( decibels * 0.115129f);
}

int16_t clamp16(int32_t sample)
{
    if ((sample >> 15) ^ (sample >> 31))
        sample = 0x7FFF ^ (sample >> 31);
    return sample;
}

int SRS_get_model_name(char *model_name, int size)
{
    int fd;
    int ret = -1;
    char node[50] = {0};
    const char *filename = "/proc/idme/model_name";

    fd = open(filename, O_RDONLY);
    if (fd < 0) {
        ALOGE("%s: open %s failed", __FUNCTION__, filename);
        goto exit;
    }
    if (read (fd, node, 50) < 0) {
        ALOGE("%s: read Model Name failed", __FUNCTION__);
        goto exit;
    }

    ret = 0;
exit:
    if (ret < 0)
        snprintf(model_name, size, "DEFAULT");
    else
        snprintf(model_name, size, "%s", node);
    ALOGD("%s: Model Name -> %s", __FUNCTION__, model_name);
    close(fd);
    return ret;
}

int SRS_get_ini_file(char *ini_name, int size)
{
    int result = -1;
    char model_name[50] = {0};
    IniParser* pIniParser = NULL;
    const char *ini_value = NULL;
    const char *filename = "/tvconfig/model/model_sum.ini";

    SRS_get_model_name(model_name, sizeof(model_name));
    pIniParser = new IniParser();
    if (pIniParser->parse(filename) < 0) {
        ALOGW("%s: Load INI file -> %s Failed", __FUNCTION__, filename);
        goto exit;
    }

    ini_value = pIniParser->GetString(model_name, "AMLOGIC_AUDIO_EFFECT_INI_PATH", "/tvconfig/audio/AMLOGIC_AUDIO_EFFECT_DEFAULT.ini");
    if (ini_value == NULL || access(ini_value, F_OK) == -1) {
        ALOGD("%s: INI File is not exist", __FUNCTION__);
        goto exit;
    }
    ALOGD("%s: INI File -> %s", __FUNCTION__, ini_value);
    strncpy(ini_name, ini_value, size);

    result = 0;
exit:
    delete pIniParser;
    pIniParser = NULL;
    return result;
}

int SRS_parse_dialogclarity(DC_SRScfg *DC_parm, const char *buffer)
{
    char *Rch = (char *)buffer;

    Rch = strtok(Rch, ",");
    if (Rch == NULL)
        goto error;
    DC_parm->dialogclarity.enable = atoi(Rch);
    //ALOGD("%s: Dialog Clarity enable = %d", __FUNCTION__, DC_parm->dialogclarity.enable);

    Rch = strtok(NULL, ",");
    if (Rch == NULL)
        goto error;
    DC_parm->dialogclarity.gain = atof(Rch);
    //ALOGD("%s: Dialog Clarity Gain = %f", __FUNCTION__, DC_parm->dialogclarity.gain);

    return 0;
error:
    ALOGE("%s: Dialog Clarity Parse failed", __FUNCTION__);
    return -1;
}

int SRS_parse_surround(TS_SRScfg *TS_param, const char *buffer)
{
    char *Rch = (char *)buffer;

    /*True Bass param*/
    Rch = strtok(Rch, ",");
    if (Rch == NULL)
        goto error;
    TS_param->truebass.enable = atoi(Rch);
    //ALOGD("%s: True Bass enable = %d", __FUNCTION__, TS_param->truebass.enable);
    Rch = strtok(NULL, ",");
    if (Rch == NULL)
        goto error;
    TS_param->truebass.spkersize = atoi(Rch);
    //ALOGD("%s: True Bass spkersize = %d", __FUNCTION__, TS_param->truebass.spkersize);
    Rch = strtok(NULL, ",");
    if (Rch == NULL)
        goto error;
    TS_param->truebass.gain = atof(Rch);
    //ALOGD("%s: True Bass gain = %f", __FUNCTION__, TS_param->truebass.gain);

    /*Definition param*/
    Rch = strtok(NULL, ",");
    if (Rch == NULL)
        goto error;
    TS_param->definition.enable = atoi(Rch);
    //ALOGD("%s: Definition enable = %d", __FUNCTION__, TS_param->definition.enable);
    Rch = strtok(NULL, ",");
    if (Rch == NULL)
        goto error;
    TS_param->definition.gain = atof(Rch);
    //ALOGD("%s: Definition gain = %f", __FUNCTION__, TS_param->definition.gain);

    /*Surround param*/
    Rch = strtok(NULL, ",");
    if (Rch == NULL)
        goto error;
    TS_param->surround.enable = atoi(Rch);
    //ALOGD("%s: Surround enable = %d", __FUNCTION__, TS_param->surround.enable);
    Rch = strtok(NULL, ",");
    if (Rch == NULL)
        goto error;
    TS_param->surround.gain = atof(Rch);
    //ALOGD("%s: Surround gain = %f", __FUNCTION__, TS_param->surround.gain);

    /*gain param*/
    Rch = strtok(NULL, ",");
    if (Rch == NULL)
        goto error;
    TS_param->gain.input_gain = atof(Rch);
    //ALOGD("%s: Input gain = %f", __FUNCTION__, TS_param->gain.input_gain);

    Rch = strtok(NULL, ",");
    if (Rch == NULL)
        goto error;
    TS_param->gain.output_gain = atof(Rch);
    //ALOGD("%s: Output gain = %f", __FUNCTION__, TS_param->gain.output_gain);

    Rch = strtok(NULL, ",");
    if (Rch == NULL)
        goto error;
    TS_param->gain.comp_gain = db_to_ampl(atof(Rch));
    //ALOGD("%s: Output gain compensation = %s -> %f", __FUNCTION__, Rch, TS_param->gain.comp_gain);

    return 0;
error:
    ALOGE("%s: TruSurround Parse failed", __FUNCTION__);
    return -1;
}

int SRS_load_ini_file(SRSContext *pContext)
{
    int result = -1;
    char ini_name[100] = {0};
    const char *ini_value = NULL;
    SRSdata *data = &pContext->gSRSdata;
    IniParser* pIniParser = NULL;

    if (SRS_get_ini_file(ini_name, sizeof(ini_name)) < 0)
        goto error;

    pIniParser = new IniParser();
    if (pIniParser->parse((const char *)ini_name) < 0) {
        ALOGD("%s: %s load failed", __FUNCTION__, ini_name);
        goto error;
    }

    ini_value = pIniParser->GetString("TruSurround", "enable", "1");
    if (ini_value == NULL)
        goto error;
    ALOGD("%s: enable -> %s", __FUNCTION__, ini_value);
    data->enable = atoi(ini_value);

    // dialog clarity parse
    ini_value = pIniParser->GetString("TruSurround", "dialogclarity_mode_off", "NULL");
    if (ini_value == NULL)
        goto error;
    //ALOGD("%s: DialogClarity Mode -> %s", __FUNCTION__, "dialogclarity_mode_off");
    if (SRS_parse_dialogclarity(&data->DC_usr_cfg[0], ini_value) < 0)
        goto error;

    ini_value = pIniParser->GetString("TruSurround", "dialogclarity_mode_low", "NULL");
    if (ini_value == NULL)
        goto error;
    //ALOGD("%s: DialogClarity Mode -> %s", __FUNCTION__, "dialogclarity_mode_low");
    if (SRS_parse_dialogclarity(&data->DC_usr_cfg[1], ini_value) < 0)
        goto error;

    ini_value = pIniParser->GetString("TruSurround", "dialogclarity_mode_high", "NULL");
    if (ini_value == NULL)
        goto error;
    //ALOGD("%s: DialogClarity Mode -> %s", __FUNCTION__, "dialogclarity_mode_high");
    if (SRS_parse_dialogclarity(&data->DC_usr_cfg[2], ini_value) < 0)
        goto error;

    // surround parse
    ini_value = pIniParser->GetString("TruSurround", "surround_mode_on", "NULL");
    if (ini_value == NULL)
        goto error;
    //ALOGD("%s: surround_gain -> %s", __FUNCTION__, "surround_mode_on");
    if (SRS_parse_surround(&data->TS_usr_cfg[0], ini_value) < 0)
        goto error;

    ini_value = pIniParser->GetString("TruSurround", "surround_mode_off", "NULL");
    if (ini_value == NULL)
        goto error;
    //ALOGD("%s: surround_gain -> %s", __FUNCTION__, "surround_mode_off");
    if (SRS_parse_surround(&data->TS_usr_cfg[1], ini_value) < 0)
        goto error;

    result = 0;
error:
    ALOGD("%s: %s", __FUNCTION__, result == 0 ? "sucessful" : "failed");
    delete pIniParser;
    pIniParser = NULL;
    return result;
}

int SRS_load_lib(SRSContext *pContext)
{
    pContext->gSRSLibHandler = dlopen(LIBSRS_PATH, RTLD_NOW);
    if (!pContext->gSRSLibHandler) {
        ALOGE("%s: failed", __FUNCTION__);
        return -EINVAL;
    }
    pContext->gSRSapi.SRS_init = (int (*)(void*))dlsym(pContext->gSRSLibHandler, "SRS_init_api");
    if (!pContext->gSRSapi.SRS_init) {
        ALOGE("%s: find func SRS_init_api() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gSRSapi.SRS_release = (int (*)(void))dlsym(pContext->gSRSLibHandler, "SRS_release_api");
    if (!pContext->gSRSapi.SRS_release) {
        ALOGE("%s: find func SRS_release_api() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gSRSapi.SRS_reset = (int (*)(int))dlsym(pContext->gSRSLibHandler, "SRS_reset_api");
    if (!pContext->gSRSapi.SRS_reset) {
        ALOGE("%s: find func SRS_reset_api() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gSRSapi.SRS_TRUEBASS_ENABLE = (int (*)(int))dlsym(pContext->gSRSLibHandler, "SRS_TRUEBASS_ENABLE_api");
    if (!pContext->gSRSapi.SRS_TRUEBASS_ENABLE) {
        ALOGE("%s: find func SRS_TRUEBASS_ENABLE_api() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gSRSapi.SRS_DIALOGCLARITY_ENABLE = (int (*)(int))dlsym(pContext->gSRSLibHandler, "SRS_DIALOGCLARITY_ENABLE_api");
    if (!pContext->gSRSapi.SRS_DIALOGCLARITY_ENABLE) {
        ALOGE("%s: find func SRS_DIALOGCLARITY_ENABLE_api() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gSRSapi.SRS_DEFINITION_ENABLE = (int (*)(int))dlsym(pContext->gSRSLibHandler, "SRS_DEFINITION_ENABLE_api");
    if (!pContext->gSRSapi.SRS_DEFINITION_ENABLE) {
        ALOGE("%s: find func SRS_DEFINITION_ENABLE_api() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gSRSapi.SRS_SURROUND_ENABLE = (int (*)(int))dlsym(pContext->gSRSLibHandler, "SRS_SURROUND_ENABLE_api");
    if (!pContext->gSRSapi.SRS_SURROUND_ENABLE) {
        ALOGE("%s: find func SRS_SURROUND_ENABLE_api() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gSRSapi.SRS_process = (int (*)(short*, short*, int))dlsym(pContext->gSRSLibHandler, "SRS_process_api");
    if (!pContext->gSRSapi.SRS_process) {
        ALOGE("%s: find func SRS_process_api() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gSRSapi.SRS_setTruebass_spkersize = (int (*)(int))dlsym(pContext->gSRSLibHandler, "SRS_setTruebass_spkersize_api");
    if (!pContext->gSRSapi.SRS_setTruebass_spkersize) {
        ALOGE("%s: find func SRS_setTruebass_spkersize_api() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gSRSapi.SRS_getTruebass_spkersize = (int (*)(int*))dlsym(pContext->gSRSLibHandler, "SRS_getTruebass_spkersize_api");
    if (!pContext->gSRSapi.SRS_getTruebass_spkersize) {
        ALOGE("%s: find func SRS_getTruebass_spkersize_api() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gSRSapi.SRS_setTruebass_gain = (int (*)(float))dlsym(pContext->gSRSLibHandler, "SRS_setTruebass_gain_api");
    if (!pContext->gSRSapi.SRS_setTruebass_gain) {
        ALOGE("%s: find func SRS_setTruebass_gain_api() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gSRSapi.SRS_getTruebass_gain = (int (*)(float*))dlsym(pContext->gSRSLibHandler, "SRS_getTruebass_gain_api");
    if (!pContext->gSRSapi.SRS_getTruebass_gain) {
        ALOGE("%s: find func SRS_getTruebass_gain_api() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gSRSapi.SRS_setDialogclarity_gain = (int (*)(float))dlsym(pContext->gSRSLibHandler, "SRS_setDialogclarity_gain_api");
    if (!pContext->gSRSapi.SRS_setDialogclarity_gain) {
        ALOGE("%s: find func SRS_setDialogclarity_gain_api() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gSRSapi.SRS_getDialogclarity_gain = (int (*)(float*))dlsym(pContext->gSRSLibHandler, "SRS_getDialogclarity_gain_api");
    if (!pContext->gSRSapi.SRS_getDialogclarity_gain) {
        ALOGE("%s: find func SRS_getDialogclarity_gain_api() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gSRSapi.SRS_setDefinition_gain = (int (*)(float))dlsym(pContext->gSRSLibHandler, "SRS_setDefinition_gain_api");
    if (!pContext->gSRSapi.SRS_setDefinition_gain) {
        ALOGE("%s: find func SRS_setDefinition_gain_api() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gSRSapi.SRS_getDefinition_gain = (int (*)(float*))dlsym(pContext->gSRSLibHandler, "SRS_getDefinition_gain_api");
    if (!pContext->gSRSapi.SRS_getDefinition_gain) {
        ALOGE("%s: find func SRS_getDefinition_gain_api() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gSRSapi.SRS_setSurround_gain = (int (*)(float))dlsym(pContext->gSRSLibHandler, "SRS_setSurround_gain_api");
    if (!pContext->gSRSapi.SRS_setSurround_gain) {
        ALOGE("%s: find func SRS_setSurround_gain_api() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gSRSapi.SRS_getSurround_gain = (int (*)(float*))dlsym(pContext->gSRSLibHandler, "SRS_getSurround_gain_api");
    if (!pContext->gSRSapi.SRS_getSurround_gain) {
        ALOGE("%s: find func SRS_getSurround_gain_api() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gSRSapi.SRS_setInput_gain = (int (*)(float))dlsym(pContext->gSRSLibHandler, "SRS_setInput_gain_api");
    if (!pContext->gSRSapi.SRS_setInput_gain) {
        ALOGE("%s: find func SRS_setInput_gain_api() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gSRSapi.SRS_getInput_gain = (int (*)(float*))dlsym(pContext->gSRSLibHandler, "SRS_getInput_gain_api");
    if (!pContext->gSRSapi.SRS_getInput_gain) {
        ALOGE("%s: find func SRS_getInput_gain_api() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gSRSapi.SRS_setOutput_gain = (int (*)(float))dlsym(pContext->gSRSLibHandler, "SRS_setOutput_gain_api");
    if (!pContext->gSRSapi.SRS_setOutput_gain) {
        ALOGE("%s: find func SRS_setOutput_gain_api() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gSRSapi.SRS_getOutput_gain = (int (*)(float*))dlsym(pContext->gSRSLibHandler, "SRS_getOutput_gain_api");
    if (!pContext->gSRSapi.SRS_getOutput_gain) {
        ALOGE("%s: find func SRS_getOutput_gain_api() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gSRSapi.SRS_getTruebass_status = (int (*)(int*))dlsym(pContext->gSRSLibHandler, "SRS_getTruebass_status_api");
    if (!pContext->gSRSapi.SRS_getTruebass_status) {
        ALOGE("%s: find func SRS_getTruebass_status_api() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gSRSapi.SRS_getDialogclarity_status = (int (*)(int*))dlsym(pContext->gSRSLibHandler, "SRS_getDialogclarity_status_api");
    if (!pContext->gSRSapi.SRS_getDialogclarity_status) {
        ALOGE("%s: find func SRS_getDialogclarity_status_api() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gSRSapi.SRS_getDefinition_status = (int (*)(int*))dlsym(pContext->gSRSLibHandler, "SRS_getDefinition_status_api");
    if (!pContext->gSRSapi.SRS_getDefinition_status) {
        ALOGE("%s: find func SRS_getDefinition_status_api() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gSRSapi.SRS_getSurround_status = (int (*)(int*))dlsym(pContext->gSRSLibHandler, "SRS_getSurround_status_api");
    if (!pContext->gSRSapi.SRS_getSurround_status) {
        ALOGE("%s: find func SRS_getSurround_status_api() failed\n", __FUNCTION__);
        goto Error;
    }

    ALOGD("%s: sucessful", __FUNCTION__);

    return 0;
Error:
    memset(&pContext->gSRSapi, 0, sizeof(pContext->gSRSapi));
    dlclose(pContext->gSRSLibHandler);
    pContext->gSRSLibHandler = NULL;
    return -EINVAL;
}

int unload_SRS_lib(SRSContext *pContext)
{
    memset(&pContext->gSRSapi, 0, sizeof(pContext->gSRSapi));
    if (pContext->gSRSLibHandler) {
        dlclose(pContext->gSRSLibHandler);
        pContext->gSRSLibHandler = NULL;
    }

    return 0;
}

int SRS_init(SRSContext *pContext)
{
    SRSdata *data = &pContext->gSRSdata;

    pContext->config.inputCfg.accessMode = EFFECT_BUFFER_ACCESS_READ;
    pContext->config.inputCfg.channels = AUDIO_CHANNEL_OUT_STEREO;
    pContext->config.inputCfg.format = AUDIO_FORMAT_PCM_16_BIT;
    pContext->config.inputCfg.samplingRate = 48000;
    pContext->config.inputCfg.bufferProvider.getBuffer = NULL;
    pContext->config.inputCfg.bufferProvider.releaseBuffer = NULL;
    pContext->config.inputCfg.bufferProvider.cookie = NULL;
    pContext->config.inputCfg.mask = EFFECT_CONFIG_ALL;
    pContext->config.outputCfg.accessMode = EFFECT_BUFFER_ACCESS_ACCUMULATE;
    pContext->config.outputCfg.channels = AUDIO_CHANNEL_OUT_STEREO;
    pContext->config.outputCfg.format = AUDIO_FORMAT_PCM_16_BIT;
    pContext->config.outputCfg.samplingRate = 48000;
    pContext->config.outputCfg.bufferProvider.getBuffer = NULL;
    pContext->config.outputCfg.bufferProvider.releaseBuffer = NULL;
    pContext->config.outputCfg.bufferProvider.cookie = NULL;
    pContext->config.outputCfg.mask = EFFECT_CONFIG_ALL;

    /* default mode is Standard */
    data->sound_mode = 0;

    data->truebass_enable       = data->TS_usr_cfg[0].truebass.enable;
    data->truebass_spker_size   = data->TS_usr_cfg[0].truebass.spkersize;
    data->truebass_gain         = data->TS_usr_cfg[0].truebass.gain;

    data->dialogclarity_enable  = data->DC_usr_cfg[0].dialogclarity.enable;
    data->dialogclarity_gain    = data->DC_usr_cfg[0].dialogclarity.gain;

    data->definition_enable     = data->TS_usr_cfg[0].definition.enable;
    data->definition_gain       = data->TS_usr_cfg[0].definition.gain;

    data->surround_enable       = data->TS_usr_cfg[0].surround.enable;
    data->surround_gain         = data->TS_usr_cfg[0].surround.gain;

    data->input_gain            = data->TS_usr_cfg[0].gain.input_gain;
    data->output_gain           = data->TS_usr_cfg[0].gain.output_gain;
    data->comp_gain             = data->TS_usr_cfg[0].gain.comp_gain;

    if (pContext->gSRSLibHandler)
        (*pContext->gSRSapi.SRS_init)((void*)data);

    ALOGD("%s: sucessful", __FUNCTION__);

    return 0;
}

int SRS_reset(SRSContext *pContext)
{
    if (pContext->gSRSLibHandler)
        (*pContext->gSRSapi.SRS_reset)(pContext->config.inputCfg.samplingRate);

    return 0;
}

int SRS_configure(SRSContext *pContext, effect_config_t *pConfig)
{
    if (pConfig->inputCfg.samplingRate != pConfig->outputCfg.samplingRate)
        return -EINVAL;
    if (pConfig->inputCfg.channels != pConfig->outputCfg.channels)
        return -EINVAL;
    if (pConfig->inputCfg.format != pConfig->outputCfg.format)
        return -EINVAL;

    if (pConfig->inputCfg.channels != AUDIO_CHANNEL_OUT_STEREO) {
        ALOGW("%s: channels in = 0x%x channels out = 0x%x", __FUNCTION__, pConfig->inputCfg.channels, pConfig->outputCfg.channels);
        pConfig->inputCfg.channels = pConfig->outputCfg.channels = AUDIO_CHANNEL_OUT_STEREO;
    }
    if (pConfig->outputCfg.accessMode != EFFECT_BUFFER_ACCESS_WRITE &&
            pConfig->outputCfg.accessMode != EFFECT_BUFFER_ACCESS_ACCUMULATE)
        return -EINVAL;
    if (pConfig->inputCfg.format != AUDIO_FORMAT_PCM_16_BIT) {
        ALOGW("%s: format in = 0x%x format out = 0x%x", __FUNCTION__, pConfig->inputCfg.format, pConfig->outputCfg.format);
        pConfig->inputCfg.format = pConfig->outputCfg.format = AUDIO_FORMAT_PCM_16_BIT;
    }

    memcpy(&pContext->config, pConfig, sizeof(effect_config_t));

    return SRS_reset(pContext);
}

int SRS_getParameter(SRSContext *pContext, void *pParam, size_t *pValueSize, void *pValue)
{
    uint32_t param = *(uint32_t *)pParam;
    int32_t value;
    float scale;
    SRSdata *data = &pContext->gSRSdata;

    switch (param) {
    case SRS_PARAM_MODE:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gSRSLibHandler) {
             return 0;
        }
        value = data->sound_mode;
        *(uint32_t *) pValue = value;
        ALOGD("%s: Get Mode -> %s", __FUNCTION__, SRSModestr[value]);
        break;
    case SRS_PARAM_DIALOGCLARTY_MODE:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gSRSLibHandler) {
             return 0;
        }
        if (data->enable) {
            (*pContext->gSRSapi.SRS_getDialogclarity_gain)(&scale);
            *(uint32_t *) pValue = data->dialogclarity_mode;
            ALOGD("%s: Get Dialog Clarity Mode %s -> %f", __FUNCTION__,
                SRSDialogClarityModestr[data->dialogclarity_mode], scale);
        } else {
            ALOGD("%s: Get Dialog Clarity Mode failed", __FUNCTION__);
            return 0;
        }
        break;
    case SRS_PARAM_SURROUND_MODE:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gSRSLibHandler) {
             return 0;
        }
        if (data->enable) {
            (*pContext->gSRSapi.SRS_getSurround_gain)(&scale);
            *(uint32_t *) pValue = data->surround_mode;
            ALOGD("%s: Get Surround Mode %s -> %f", __FUNCTION__,
                SRSSurroundModestr[data->surround_mode], scale);
        } else {
            ALOGD("%s: Get Surround Mode failed", __FUNCTION__);
            return 0;
        }
        break;
    case SRS_PARAM_VOLUME_MODE:
        if (*pValueSize < sizeof(float)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gSRSLibHandler) {
             return 0;
        }
        break;
    case SRS_PARAM_ENABLE:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gSRSLibHandler) {
            return 0;
        }
        value = data->enable;
        *(uint32_t *) pValue = value;
        ALOGD("%s: Get status -> %s", __FUNCTION__, SRSStatusstr[value]);
        break;
    case SRS_PARAM_TRUEBASS_ENABLE:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gSRSLibHandler) {
            return 0;
        }
        (*pContext->gSRSapi.SRS_getTruebass_status)(&value);
        *(uint32_t *) pValue = value;
        ALOGD("%s: Get True Bass -> %s", __FUNCTION__, SRSTrueBassstr[value]);
        break;
    case SRS_PARAM_TRUEBASS_SPKER_SIZE:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gSRSLibHandler) {
             return 0;
        }
        (*pContext->gSRSapi.SRS_getTruebass_status)(&value);
        if (value) {
            (*pContext->gSRSapi.SRS_getTruebass_spkersize)(&value);
            *(uint32_t *) pValue = value;
            ALOGD("%s: Get True Bass Speaker Size -> %u", __FUNCTION__, value);
        } else {
            ALOGD("%s: Get True Bass Speaker Size failed", __FUNCTION__);
            return 0;
        }
        break;
    case SRS_PARAM_TRUEBASS_GAIN:
        if (*pValueSize < sizeof(float)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gSRSLibHandler) {
             return 0;
        }
        (*pContext->gSRSapi.SRS_getTruebass_status)(&value);
        if (value) {
            (*pContext->gSRSapi.SRS_getTruebass_gain)(&scale);
            *(float *) pValue = scale;
            ALOGD("%s: Get True Bass Gain -> %f", __FUNCTION__, scale);
        } else {
            ALOGD("%s: Get True Bass Gain failed", __FUNCTION__);
            return 0;
        }
        break;
    case SRS_PARAM_DIALOG_CLARITY_ENABLE:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gSRSLibHandler) {
             return 0;
        }
        (*pContext->gSRSapi.SRS_getDialogclarity_status)(&value);
        *(uint32_t *) pValue = value;
        ALOGD("%s: Get Dialog Clarity -> %s", __FUNCTION__, SRSDialogClaritystr[value]);
        break;
    case SRS_PARAM_DIALOGCLARTY_GAIN:
        if (*pValueSize < sizeof(float)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gSRSLibHandler) {
             return 0;
        }
        (*pContext->gSRSapi.SRS_getDialogclarity_status)(&value);
        if (value) {
            (*pContext->gSRSapi.SRS_getDialogclarity_gain)(&scale);
            *(float *) pValue = scale;
            ALOGD("%s: Get Dialog Clarity Gain -> %f", __FUNCTION__, scale);
        } else {
            ALOGD("%s: Get Dialog Clarity Gain failed", __FUNCTION__);
            return 0;
        }
        break;
    case SRS_PARAM_DEFINITION_ENABLE:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gSRSLibHandler) {
             return 0;
        }
        (*pContext->gSRSapi.SRS_getDefinition_status)(&value);
        *(uint32_t *) pValue = value;
        ALOGD("%s: Get Definition -> %s", __FUNCTION__, SRSDefinitionstr[value]);
        break;
    case SRS_PARAM_DEFINITION_GAIN:
        if (*pValueSize < sizeof(float)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gSRSLibHandler) {
             return 0;
        }
        (*pContext->gSRSapi.SRS_getDefinition_status)(&value);
        if (value) {
            (*pContext->gSRSapi.SRS_getDefinition_gain)(&scale);
            *(float *) pValue = scale;
            ALOGD("%s: Get Definition Gain -> %f", __FUNCTION__, scale);
        } else {
            ALOGD("%s: Get Definition Gain failed", __FUNCTION__);
            return 0;
        }
        break;
    case SRS_PARAM_SURROUND_ENABLE:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gSRSLibHandler) {
             return 0;
        }
        (*pContext->gSRSapi.SRS_getSurround_status)(&value);
        *(uint32_t *) pValue = value;
        ALOGD("%s: Get Surround -> %s", __FUNCTION__, SRSSurroundstr[value]);
        break;
    case SRS_PARAM_SURROUND_GAIN:
        if (*pValueSize < sizeof(float)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gSRSLibHandler) {
             return 0;
        }
        (*pContext->gSRSapi.SRS_getSurround_status)(&value);
        if (value) {
            (*pContext->gSRSapi.SRS_getSurround_gain)(&scale);
            *(float *) pValue = scale;
            ALOGD("%s: Get Surround Gain -> %f", __FUNCTION__, scale);
        } else {
            ALOGD("%s: Get Surround Gain failed", __FUNCTION__);
            return 0;
        }
        break;
    case SRS_PARAM_INPUT_GAIN:
        if (*pValueSize < sizeof(float)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gSRSLibHandler) {
             return 0;
        }
        (*pContext->gSRSapi.SRS_getInput_gain)(&scale);
        *(float *) pValue = scale;
        ALOGD("%s: Get Input Gain -> %f", __FUNCTION__, scale);
        break;
    case SRS_PARAM_OUTPUT_GAIN:
        if (*pValueSize < sizeof(float)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gSRSLibHandler) {
             return 0;
        }
        (*pContext->gSRSapi.SRS_getOutput_gain)(&scale);
        *(float *) pValue = scale;
        ALOGD("%s: Get Output Gain -> %f", __FUNCTION__, scale);
        break;
    case SRS_PARAM_OUTPUT_GAIN_COMP:
        if (*pValueSize < sizeof(float)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gSRSLibHandler) {
             return 0;
        }

        scale = data->comp_gain;
        *(float *) pValue = scale;
        ALOGD("%s: Get Output Gain Compensation -> %f", __FUNCTION__, scale);
        break;
    default:
        ALOGE("%s: unknown param %d", __FUNCTION__, param);
        return -EINVAL;
    }

    return 0;
}

int SRS_setParameter(SRSContext *pContext, void *pParam, void *pValue)
{
    uint32_t param = *(uint32_t *)pParam;
    int32_t value;
    float scale;
    SRSdata *data = &pContext->gSRSdata;

    switch (param) {
    case SRS_PARAM_MODE:
        if (!pContext->gSRSLibHandler) {
            return 0;
        }
        //only show
        value = *(uint32_t *)pValue;
        if (value > 5)
            value = 5;
        data->sound_mode = value;
        ALOGD("%s: Set Mode -> %s", __FUNCTION__, SRSModestr[value]);
        break;
    case SRS_PARAM_DIALOGCLARTY_MODE:
        if (!pContext->gSRSLibHandler) {
            return 0;
        }

        value = *(uint32_t *)pValue;
        if (value > 2)
            value = 2;
        data->dialogclarity_mode = value;
        data->dialogclarity_enable = data->DC_usr_cfg[value].dialogclarity.enable;
        data->dialogclarity_gain = data->DC_usr_cfg[value].dialogclarity.gain;

        if (data->enable) {
            (*pContext->gSRSapi.SRS_DIALOGCLARITY_ENABLE)(data->dialogclarity_enable);
            (*pContext->gSRSapi.SRS_setDialogclarity_gain)(data->dialogclarity_gain);
            ALOGD("%s: Set Dialog Clarity Mode %s -> %f", __FUNCTION__, SRSDialogClarityModestr[value], data->dialogclarity_gain);
        } else {
            ALOGD("%s: Set Dialog Clarity Mode failed", __FUNCTION__);
            return 0;
        }
        break;
    case SRS_PARAM_SURROUND_MODE:
        if (!pContext->gSRSLibHandler) {
            return 0;
        }

        value = *(uint32_t *)pValue;
        if (value > 1)
            value = 1;
        data->surround_mode = value;
        data->truebass_enable = data->TS_usr_cfg[value].truebass.enable;
        data->truebass_spker_size = data->TS_usr_cfg[value].truebass.spkersize;
        data->truebass_gain = data->TS_usr_cfg[value].truebass.gain;
        data->definition_enable = data->TS_usr_cfg[value].definition.enable;
        data->definition_gain = data->TS_usr_cfg[value].definition.gain;
        data->surround_enable = data->TS_usr_cfg[value].surround.enable;
        data->surround_gain = data->TS_usr_cfg[value].surround.gain;
        data->input_gain = data->TS_usr_cfg[value].gain.input_gain;
        data->output_gain = data->TS_usr_cfg[value].gain.output_gain;
        data->comp_gain = data->TS_usr_cfg[value].gain.comp_gain;

        if (data->enable) {
            (*pContext->gSRSapi.SRS_setInput_gain)(data->input_gain);
            (*pContext->gSRSapi.SRS_setOutput_gain)(data->output_gain);

            (*pContext->gSRSapi.SRS_DEFINITION_ENABLE)(data->definition_enable);
            (*pContext->gSRSapi.SRS_setDefinition_gain)(data->definition_gain);

            (*pContext->gSRSapi.SRS_TRUEBASS_ENABLE)(data->truebass_enable);
            (*pContext->gSRSapi.SRS_setTruebass_gain)(data->truebass_gain);
            (*pContext->gSRSapi.SRS_setTruebass_spkersize)(data->truebass_spker_size);

            (*pContext->gSRSapi.SRS_SURROUND_ENABLE)(data->surround_enable);
            (*pContext->gSRSapi.SRS_setSurround_gain)(data->surround_gain);

            ALOGD("%s: Set Surround Mode %s -> %f", __FUNCTION__, SRSSurroundModestr[value], data->surround_gain);
        } else {
            ALOGD("%s: Set Surround Mode failed", __FUNCTION__);
            return 0;
        }
        break;
    case SRS_PARAM_VOLUME_MODE:
        if (!pContext->gSRSLibHandler) {
            return 0;
        }
        break;
    case SRS_PARAM_ENABLE:
        if (!pContext->gSRSLibHandler) {
            return 0;
        }
        value = *(uint32_t *)pValue;
        data->enable = value;

        ALOGD("%s: Set status -> %s", __FUNCTION__, SRSStatusstr[value]);
        break;
    case SRS_PARAM_TRUEBASS_ENABLE:
        if (!pContext->gSRSLibHandler) {
            return 0;
        }
        value = *(uint32_t *)pValue;
        data->truebass_enable = value;

        (*pContext->gSRSapi.SRS_TRUEBASS_ENABLE)(data->truebass_enable);
        if (data->truebass_enable) {
            (*pContext->gSRSapi.SRS_setTruebass_spkersize)(data->truebass_spker_size);
            (*pContext->gSRSapi.SRS_setTruebass_gain)(data->truebass_gain);
        }

        ALOGD("%s: Set True Bass -> %s", __FUNCTION__, SRSTrueBassstr[value]);
        break;
    case SRS_PARAM_TRUEBASS_SPKER_SIZE:
        if (!pContext->gSRSLibHandler) {
            return 0;
        }
        if (data->truebass_enable) {
            value = *(uint32_t *)pValue;

            (*pContext->gSRSapi.SRS_setTruebass_spkersize)(value);
            data->truebass_spker_size = value;
            ALOGD("%s: Set True Bass Speaker Size -> %d", __FUNCTION__, value);
        } else {
            ALOGD("%s: Set True Bass Speaker Size failed", __FUNCTION__);
            return 0;
        }
        break;
    case SRS_PARAM_TRUEBASS_GAIN:
        if (!pContext->gSRSLibHandler) {
            return 0;
        }
        if (data->truebass_enable) {
            scale = *(float *)pValue;
            if (scale > 1.0)
                scale = 1.0;
            else if (scale < 0.0)
                scale = 0.0;

            (*pContext->gSRSapi.SRS_setTruebass_gain)(scale);
            data->truebass_gain = scale;
            ALOGD("%s: Set True Bass Gain -> %f", __FUNCTION__, scale);
        } else {
            ALOGD("%s: Set True Bass Gain failed", __FUNCTION__);
            return 0;
        }
        break;
    case SRS_PARAM_DIALOG_CLARITY_ENABLE:
        if (!pContext->gSRSLibHandler) {
            return 0;
        }
        value = *(uint32_t *)pValue;
        data->dialogclarity_enable = value;

        (*pContext->gSRSapi.SRS_DIALOGCLARITY_ENABLE)(data->dialogclarity_enable);
        if (data->dialogclarity_enable) {
            (*pContext->gSRSapi.SRS_setDialogclarity_gain)(data->dialogclarity_gain);
        }

        ALOGD("%s: Set Dialog Clarity -> %s", __FUNCTION__, SRSDialogClaritystr[value]);
        break;
     case SRS_PARAM_DIALOGCLARTY_GAIN:
        if (!pContext->gSRSLibHandler) {
            return 0;
        }
        if (data->dialogclarity_enable) {
            scale = *(float *)pValue;
            if (scale > 1.0)
                scale = 1.0;
            else if (scale < 0.0)
                scale = 0.0;

            (*pContext->gSRSapi.SRS_setDialogclarity_gain)(scale);
            data->dialogclarity_gain = scale;
            ALOGD("%s: Set Dialog Clarity Gain -> %f", __FUNCTION__, scale);
        } else {
            ALOGD("%s: Set Dialog Clarity Gain failed", __FUNCTION__);
            return 0;
        }
        break;
    case SRS_PARAM_DEFINITION_ENABLE:
        if (!pContext->gSRSLibHandler) {
            return 0;
        }
        value = *(uint32_t *)pValue;
        data->definition_enable = value;

        (*pContext->gSRSapi.SRS_DEFINITION_ENABLE)(data->definition_enable);
        if (data->definition_enable) {
            (*pContext->gSRSapi.SRS_setDefinition_gain)(data->definition_gain);
        }

        ALOGD("%s: Set Definition -> %s", __FUNCTION__, SRSDefinitionstr[value]);
        break;
    case SRS_PARAM_DEFINITION_GAIN:
        if (!pContext->gSRSLibHandler) {
            return 0;
        }
        if (data->definition_enable) {
            scale = *(float *)pValue;
            if (scale > 1.0)
                scale = 1.0;
            else if (scale < 0.0)
                scale = 0.0;

            (*pContext->gSRSapi.SRS_setDefinition_gain)(scale);
            data->definition_gain = scale;
            ALOGD("%s: Set Definition Gain -> %f", __FUNCTION__, scale);
        } else {
            ALOGD("%s: Set Definition Gain failed", __FUNCTION__);
            return 0;
        }
        break;
    case SRS_PARAM_SURROUND_ENABLE:
        if (!pContext->gSRSLibHandler) {
            return 0;
        }
        value = *(uint32_t *)pValue;
        data->surround_enable = value;

        (*pContext->gSRSapi.SRS_SURROUND_ENABLE)(data->surround_enable);
        if (data->surround_enable)
            (*pContext->gSRSapi.SRS_setSurround_gain)(data->surround_gain);

        ALOGD("%s: Set Surround -> %s", __FUNCTION__, SRSSurroundstr[value]);
        break;
    case SRS_PARAM_SURROUND_GAIN:
        if (!pContext->gSRSLibHandler) {
            return 0;
        }
        if (data->surround_enable) {
            scale = *(float *)pValue;
            if (scale > 1.0)
                scale = 1.0;
            else if (scale < 0.0)
                scale = 0.0;

            (*pContext->gSRSapi.SRS_setSurround_gain)(scale);
            data->surround_gain = scale;
            ALOGD("%s: Set Surround Gain -> %f", __FUNCTION__, scale);
        } else {
            ALOGD("%s: Set Surround Gain failed", __FUNCTION__);
            return 0;
        }
        break;
    case SRS_PARAM_INPUT_GAIN:
        if (!pContext->gSRSLibHandler) {
            return 0;
        }
        scale = *(float *)pValue;
        if (scale > 1.0)
            scale = 1.0;
        else if (scale < 0.0)
            scale = 0.0;

        (*pContext->gSRSapi.SRS_setInput_gain)(scale);
        data->input_gain = scale;
        ALOGD("%s: Set Input Gain -> %f", __FUNCTION__, scale);
        break;
    case SRS_PARAM_OUTPUT_GAIN:
        if (!pContext->gSRSLibHandler) {
            return 0;
        }
        scale = *(float *)pValue;
        if (scale > 1.0)
            scale = 1.0;
        else if (scale < 0.0)
            scale = 0.0;

        (*pContext->gSRSapi.SRS_setOutput_gain)(scale);
        data->output_gain = scale;
        ALOGD("%s: Set Output Gain -> %f", __FUNCTION__, scale);
        break;
    case SRS_PARAM_OUTPUT_GAIN_COMP:
        if (!pContext->gSRSLibHandler) {
            return 0;
        }
        scale = *(float *)pValue;
        if (scale > 20.0)
            scale = 20.0;
        else if (scale < -20.0)
            scale = -20.0;

        data->comp_gain = db_to_ampl(scale);
        ALOGD("%s: Set Output Gain Compensation %fdB-> %f", __FUNCTION__, scale, data->comp_gain);
        break;
    default:
        ALOGE("%s: unknown param %08x", __FUNCTION__, param);
        return -EINVAL;
    }

    return 0;
}

int SRS_release(SRSContext *pContext)
{
    if (pContext->gSRSLibHandler)
        (*pContext->gSRSapi.SRS_release)();

    return 0;
}

//-------------------Effect Control Interface Implementation--------------------------

int SRS_process(effect_handle_t self, audio_buffer_t *inBuffer, audio_buffer_t *outBuffer)
{
    SRSContext *pContext = (SRSContext *)self;

    if (pContext == NULL)
        return -EINVAL;

    if (inBuffer == NULL || inBuffer->raw == NULL ||
        outBuffer == NULL || outBuffer->raw == NULL ||
        inBuffer->frameCount != outBuffer->frameCount ||
        inBuffer->frameCount == 0)
        return -EINVAL;

    if (pContext->state != SRS_STATE_ACTIVE)
        return -ENODATA;

    int16_t   *in  = (int16_t *)inBuffer->raw;
    int16_t   *out = (int16_t *)outBuffer->raw;
    SRSdata *data = &pContext->gSRSdata;

    if (!data->enable || !pContext->gSRSLibHandler) {
        for (size_t i = 0; i < inBuffer->frameCount; i++) {
            *out++ = *in++;
            *out++ = *in++;
        }
    } else {
        (*pContext->gSRSapi.SRS_process)(in, out, inBuffer->frameCount);
        for (size_t i = 0; i < inBuffer->frameCount; i++) {
            /*Input left process*/
            *out++ = clamp16((int32_t)(*in++ * data->comp_gain));
            /*Input right process*/
            *out++ = clamp16((int32_t)(*in++ * data->comp_gain));
        }
    }

    return 0;
}

int SRS_command(effect_handle_t self, uint32_t cmdCode, uint32_t cmdSize,
        void *pCmdData, uint32_t *replySize, void *pReplyData)
{
    SRSContext * pContext = (SRSContext *)self;
    effect_param_t *p;
    int voffset;

    if (pContext == NULL || pContext->state == SRS_STATE_UNINITIALIZED)
        return -EINVAL;

    //ALOGD("%s: cmd = %u", __FUNCTION__, cmdCode);
    switch (cmdCode) {
    case EFFECT_CMD_INIT:
        if (pReplyData == NULL || replySize == NULL || *replySize != sizeof(int))
            return -EINVAL;
        *(int *) pReplyData = SRS_init(pContext);
        break;
    case EFFECT_CMD_SET_CONFIG:
        if (pCmdData == NULL || cmdSize != sizeof(effect_config_t) || pReplyData == NULL || replySize == NULL || *replySize != sizeof(int))
            return -EINVAL;
        *(int *) pReplyData = SRS_configure(pContext,(effect_config_t *) pCmdData);
        break;
    case EFFECT_CMD_RESET:
        SRS_reset(pContext);
        break;
    case EFFECT_CMD_ENABLE:
        if (pReplyData == NULL || replySize == NULL || *replySize != sizeof(int))
            return -EINVAL;
        if (pContext->state != SRS_STATE_INITIALIZED)
            return -ENOSYS;
        pContext->state = SRS_STATE_ACTIVE;
        *(int *)pReplyData = 0;
        break;
    case EFFECT_CMD_DISABLE:
        if (pReplyData == NULL || replySize == NULL || *replySize != sizeof(int))
            return -EINVAL;
        if (pContext->state != SRS_STATE_ACTIVE)
            return -ENOSYS;
        pContext->state = SRS_STATE_INITIALIZED;;
        *(int *)pReplyData = 0;
        break;
    case EFFECT_CMD_GET_PARAM:
        if (pCmdData == NULL ||
            cmdSize != (int)(sizeof(effect_param_t) + sizeof(uint32_t)) ||
            pReplyData == NULL || replySize == NULL ||
            *replySize < (int)(sizeof(effect_param_t) + sizeof(uint32_t) + sizeof(uint32_t)))
            return -EINVAL;
        p = (effect_param_t *)pCmdData;
        memcpy(pReplyData, pCmdData, sizeof(effect_param_t) + p->psize);
        p = (effect_param_t *)pReplyData;

        voffset = ((p->psize - 1) / sizeof(int32_t) + 1) * sizeof(int32_t);

        p->status = SRS_getParameter(pContext, p->data, (size_t  *)&p->vsize, p->data + voffset);
        *replySize = sizeof(effect_param_t) + voffset + p->vsize;
        break;
    case EFFECT_CMD_SET_PARAM:
        if (pCmdData == NULL ||
            cmdSize != (int)(sizeof(effect_param_t) + sizeof(uint32_t) + sizeof(uint32_t)) ||
            pReplyData == NULL || replySize == NULL || *replySize != sizeof(int32_t))
            return -EINVAL;
        p = (effect_param_t *)pCmdData;
        if (p->psize != sizeof(uint32_t) || p->vsize != sizeof(uint32_t)) {
            *(int32_t *)pReplyData = -EINVAL;
            break;
        }
        *(int *)pReplyData = SRS_setParameter(pContext, (void *)p->data, p->data + p->psize);
        break;
    case EFFECT_CMD_OFFLOAD:
        *(int *)pReplyData = 0;
        break;
    case EFFECT_CMD_SET_DEVICE:
    case EFFECT_CMD_SET_VOLUME:
    case EFFECT_CMD_SET_AUDIO_MODE:
        break;
    default:
        ALOGE("%s: invalid command %d", __FUNCTION__, cmdCode);
        return -EINVAL;
    }

    return 0;
}

int SRS_getDescriptor(effect_handle_t self, effect_descriptor_t *pDescriptor)
{
    SRSContext * pContext = (SRSContext *) self;

    if (pContext == NULL || pDescriptor == NULL) {
        ALOGE("%s: invalid param", __FUNCTION__);
        return -EINVAL;
    }

    *pDescriptor = SRSDescriptor;

    return 0;
}

//-------------------- Effect Library Interface Implementation------------------------

int SRSLib_Create(const effect_uuid_t *uuid, int32_t sessionId __unused, int32_t ioId __unused, effect_handle_t *pHandle)
{
    //int ret;

    if (pHandle == NULL || uuid == NULL)
        return -EINVAL;

    if (memcmp(uuid, &SRSDescriptor.uuid, sizeof(effect_uuid_t)) != 0)
        return -EINVAL;

    SRSContext *pContext = new SRSContext;
    if (!pContext) {
        ALOGE("%s: alloc SRSContext failed", __FUNCTION__);
        return -EINVAL;
    }
    memset(pContext, 0, sizeof(SRSContext));

    if (SRS_load_ini_file(pContext) < 0) {
        ALOGE("%s: Load INI File faied, use default param", __FUNCTION__);
        pContext->gSRSdata.enable = 1;
        memcpy(pContext->gSRSdata.DC_usr_cfg, DC_default_usr_cfg, sizeof(pContext->gSRSdata.DC_usr_cfg));
        memcpy(pContext->gSRSdata.TS_usr_cfg, TS_default_usr_cfg, sizeof(pContext->gSRSdata.TS_usr_cfg));
    }

    if (SRS_load_lib(pContext) < 0) {
        ALOGE("%s: Load Library File faied", __FUNCTION__);
    }

    pContext->itfe = &SRSInterface;
    pContext->state = SRS_STATE_UNINITIALIZED;

    *pHandle = (effect_handle_t)pContext;

    pContext->state = SRS_STATE_INITIALIZED;

    ALOGD("%s: %p", __FUNCTION__, pContext);

    return 0;
}

int SRSLib_Release(effect_handle_t handle)
{
    SRSContext * pContext = (SRSContext *)handle;

    if (pContext == NULL)
        return -EINVAL;

    SRS_release(pContext);
    unload_SRS_lib(pContext);
    pContext->state = SRS_STATE_UNINITIALIZED;

    delete pContext;

    return 0;
}

int SRSLib_GetDescriptor(const effect_uuid_t *uuid, effect_descriptor_t *pDescriptor)
{
    if (pDescriptor == NULL || uuid == NULL) {
        ALOGE("%s: called with NULL pointer", __FUNCTION__);
        return -EINVAL;
    }

    if (memcmp(uuid, &SRSDescriptor.uuid, sizeof(effect_uuid_t)) == 0) {
        *pDescriptor = SRSDescriptor;
        return 0;
    }

    return  -EINVAL;
}

// effect_handle_t interface implementation for SRS effect
const struct effect_interface_s SRSInterface = {
        SRS_process,
        SRS_command,
        SRS_getDescriptor,
        NULL,
};

audio_effect_library_t AUDIO_EFFECT_LIBRARY_INFO_SYM = {
    .tag = AUDIO_EFFECT_LIBRARY_TAG,
    .version = EFFECT_LIBRARY_API_VERSION,
    .name = "True Surround HD",
    .implementor = "SRS Labs",
    .create_effect = SRSLib_Create,
    .release_effect = SRSLib_Release,
    .get_descriptor = SRSLib_GetDescriptor,
};

}; // extern "C"
