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

#define LOG_TAG "TrebleBass_Effect"
//#define LOG_NDEBUG 0

#include <cutils/log.h>
#include <utils/Log.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <hardware/audio_effect.h>
#include <cutils/properties.h>
#include <stdio.h>
#include <unistd.h>

#include "IniParser.h"
#include "TrebleBass.h"

extern "C" {

#include "aml_treble_bass.h"

#define MODEL_SUM_DEFAULT_PATH "/vendor/etc/tvconfig/model/model_sum.ini"
#define AUDIO_EFFECT_DEFAULT_PATH "/vendor/etc/tvconfig/audio/AMLOGIC_AUDIO_EFFECT_DEFAULT.ini"

// effect_handle_t interface implementation for treblebass effect
extern const struct effect_interface_s TrebleBassInterface;

//HPEQ effect TYPE: 7e282240-242e-11e6-bb63-0002a5d5c51b
//HPEQ effect UUID: 76733af0-2889-11e2-81c1-0800200c9a66
const effect_descriptor_t TrebleBassDescriptor = {
        {0x7e282240, 0x242e, 0x11e6, 0xbb63, {0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b}},
        {0x76733af0, 0x2889, 0x11e2, 0x81c1, {0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66}},
        EFFECT_CONTROL_API_VERSION,
        EFFECT_FLAG_TYPE_POST_PROC | EFFECT_FLAG_DEVICE_IND | EFFECT_FLAG_NO_PROCESS | EFFECT_FLAG_OFFLOAD_SUPPORTED,
        HPEQ_CUP_LOAD_ARM9E,
        HPEQ_MEM_USAGE,
        "TrebleBass",
        "Amlogic",
};

enum treblebass_state_e {
    TREBASS_STATE_UNINITIALIZED,
    TREBASS_STATE_INITIALIZED,
    TREBASS_STATE_ACTIVE,
};

typedef enum {
    TREBASS_PARAM_INVALID = -1,
    TREBASS_PARAM_BASS_LEVEL,
    TREBASS_PARAM_TREBLE_LEVEL,
    TREBASS_PARAM_ENABLE,
} TREBASSparams;

typedef struct TrebleBasscfg_s {
    int32_t     bass_level;
    int32_t     treble_level;
    float       bass_gain;
    float       treble_gain;
} TrebleBasscfg;

typedef struct TreBassdata_s {
    int32_t       count;
    TrebleBasscfg tbcfg;
    int32_t       enable;
} TreBassdata;

typedef struct TREBASSContext_s {
    const struct effect_interface_s *itfe;
    effect_config_t                 config;
    treblebass_state_e                    state;
    TreBassdata                        gTreBassdata;
} TREBASSContext;

const char *TREBASSStatusstr[] = {"Disable", "Enable"};

int TrebleBass_get_model_name(char *model_name, int size)
{
     int ret = -1;
    char node[PROPERTY_VALUE_MAX];

    ret = property_get("tv.model_name", node, NULL);

    if (ret < 0)
        snprintf(model_name, size, "DEFAULT");
    else
        snprintf(model_name, size, "%s", node);
    ALOGD("%s: Model Name -> %s", __FUNCTION__, model_name);
    return ret;
}

int TrebleBass_get_ini_file(char *ini_name, int size)
{
    int result = -1;
    char model_name[50] = {0};
    IniParser* pIniParser = NULL;
    const char *ini_value = NULL;
    const char *filename = MODEL_SUM_DEFAULT_PATH;

    TrebleBass_get_model_name(model_name, sizeof(model_name));
    pIniParser = new IniParser();
    if (pIniParser->parse(filename) < 0) {
        ALOGW("%s: Load INI file -> %s Failed", __FUNCTION__, filename);
        goto exit;
    }

    ini_value = pIniParser->GetString(model_name, "AMLOGIC_AUDIO_EFFECT_INI_PATH", AUDIO_EFFECT_DEFAULT_PATH);
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


int TrebleBass_load_ini_file(TREBASSContext *pContext)
{
    int result = -1;
    char ini_name[100] = {0};
    const char *ini_value = NULL;
    TreBassdata *data = &pContext->gTreBassdata;
    IniParser* pIniParser = NULL;

    if (TrebleBass_get_ini_file(ini_name, sizeof(ini_name)) < 0)
        goto error;

    pIniParser = new IniParser();
    if (pIniParser->parse((const char *)ini_name) < 0) {
        ALOGD("%s: %s load failed", __FUNCTION__, ini_name);
        goto error;
    }
    ini_value = pIniParser->GetString("TrebleBass", "treble_bass_enable", "1");
    if (ini_value == NULL)
        goto error;
    ALOGD("%s: enable -> %s", __FUNCTION__, ini_value);
    data->enable = atoi(ini_value);

    result = 0;
error:
    ALOGD("%s: %s", __FUNCTION__, result == 0 ? "sucessful" : "failed");
    delete pIniParser;
    pIniParser = NULL;
    return result;
}

int TrebleBass_init(TREBASSContext *pContext)
{
    //TreBassdata *data = &pContext->gTreBassdata;
    //int32_t count = data->count;

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
    ALOGD("%s: sucessful", __FUNCTION__);

    return 0;
}

int TrebleBass_configure(TREBASSContext *pContext, effect_config_t *pConfig)
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

    return 0;
}

int TrebleBass_getParameter(TREBASSContext *pContext, void *pParam, size_t *pValueSize, void *pValue)
{
    int32_t param = *(int32_t *)pParam;
    int32_t value;
    TreBassdata *data = &pContext->gTreBassdata;
    TrebleBasscfg *tbcfg = &data->tbcfg;

    switch (param) {
    case TREBASS_PARAM_BASS_LEVEL:
        if (*pValueSize < sizeof(int32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        *(int32_t *) pValue = tbcfg->bass_level;
        ALOGD("%s: Get Bass -> %d level", __FUNCTION__, tbcfg->bass_level);
        break;
    case TREBASS_PARAM_TREBLE_LEVEL:
        if (*pValueSize < sizeof(int32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        *(int32_t *) pValue = tbcfg->treble_level;
        ALOGD("%s: Get Treble -> %d level", __FUNCTION__, tbcfg->treble_level);
        break;
    case TREBASS_PARAM_ENABLE:
        if (*pValueSize < sizeof(int32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        value = data->enable;
        *(int32_t *) pValue = value;
        ALOGD("%s: Get status -> %s", __FUNCTION__, TREBASSStatusstr[value]);
        break;
    default:
        ALOGE("%s: unknown param %d", __FUNCTION__, param);
        return -EINVAL;
    }

    return 0;
}

int TrebleBass_setParameter(TREBASSContext *pContext, void *pParam, void *pValue)
{
    int32_t param = *(int32_t *)pParam;
    int32_t value;
    TreBassdata *data = &pContext->gTreBassdata;
    TrebleBasscfg *tbcfg = &data->tbcfg;

    switch (param) {
    case TREBASS_PARAM_BASS_LEVEL:
        value = *(int32_t *)pValue;
        if (value < 0 || value > 100) {
            ALOGE("wrong parameter %d\n", value);
            return -EINVAL;
        }
        tbcfg->bass_level = value;
        tbcfg->bass_gain = ((float)value - 50)/5; //-10dB~10dB
        audio_Treble_Bass_init(tbcfg->bass_gain, tbcfg->treble_gain);
        ALOGD("%s: Set Bass -> %fdB, Treble -> %fdB, value %d",
                __FUNCTION__, tbcfg->bass_gain, tbcfg->treble_gain, value);
        break;
    case TREBASS_PARAM_TREBLE_LEVEL:
        value = *(int32_t *)pValue;
        if (value < 0 || value > 100) {
            ALOGE("wrong parameter %d\n", value);
            return -EINVAL;
        }
        tbcfg->treble_level = value;
        tbcfg->treble_gain = ((float)value - 50)/5; //-10dB~10dB
        audio_Treble_Bass_init(tbcfg->bass_gain, tbcfg->treble_gain);
        ALOGD("%s: Set Bass -> %fdB, Treble -> %fdB, value %d",
                __FUNCTION__, tbcfg->bass_gain, tbcfg->treble_gain, value);
        break;
    case TREBASS_PARAM_ENABLE:
        value = *(int32_t *)pValue;
        data->enable = value;

        ALOGD("%s: Set status -> %s", __FUNCTION__, TREBASSStatusstr[value]);
        break;
    default:
        ALOGE("%s: unknown param %08x", __FUNCTION__, param);
        return -EINVAL;
    }

    return 0;
}

int TrebleBass_release(TREBASSContext *pContext __unused)
{
    return 0;
}

//-------------------Effect Control Interface Implementation--------------------------

int TrebleBass_process(effect_handle_t self, audio_buffer_t *inBuffer, audio_buffer_t *outBuffer)
{
    TREBASSContext * pContext = (TREBASSContext *)self;

    if (pContext == NULL) {
        return -EINVAL;
    }

   if (inBuffer == NULL || inBuffer->raw == NULL ||
        outBuffer == NULL || outBuffer->raw == NULL ||
        inBuffer->frameCount != outBuffer->frameCount ||
        inBuffer->frameCount == 0) {
        return -EINVAL;
    }

    if (pContext->state != TREBASS_STATE_ACTIVE) {
        return -ENODATA;
    }

    int16_t *in  = (int16_t *)inBuffer->raw;
    int16_t *out = (int16_t *)outBuffer->raw;
    TreBassdata *data = &pContext->gTreBassdata;
    if (!data->enable) {
        for (size_t i = 0; i < inBuffer->frameCount; i++) {
            *out++ = *in++;
            *out++ = *in++;
        }
    } else {
        audio_Treble_Bass_process(in, in, inBuffer->frameCount);
    }
    return 0;
}

int TrebleBass_command(effect_handle_t self, uint32_t cmdCode, uint32_t cmdSize,
        void *pCmdData, uint32_t *replySize, void *pReplyData)
{
    TREBASSContext * pContext = (TREBASSContext *)self;
    effect_param_t *p;
    int voffset;

    if (pContext == NULL || pContext->state == TREBASS_STATE_UNINITIALIZED) {
        return -EINVAL;
    }

    ALOGD("%s: cmd = %u", __FUNCTION__, cmdCode);
    switch (cmdCode) {
    case EFFECT_CMD_INIT:
        if (pReplyData == NULL || replySize == NULL || *replySize != sizeof(int)) {
            return -EINVAL;
        }
        *(int *) pReplyData = TrebleBass_init(pContext);
        break;
    case EFFECT_CMD_SET_CONFIG:
        if (pCmdData == NULL || cmdSize != sizeof(effect_config_t) || pReplyData == NULL || replySize == NULL || *replySize != sizeof(int))
            return -EINVAL;
        *(int *) pReplyData = TrebleBass_configure(pContext, (effect_config_t *) pCmdData);
        break;
    case EFFECT_CMD_RESET:
        //HPEQ_reset(pContext);
        break;
    case EFFECT_CMD_ENABLE:
        if (pReplyData == NULL || replySize == NULL || *replySize != sizeof(int))
            return -EINVAL;
        if (pContext->state != TREBASS_STATE_INITIALIZED)
            return -ENOSYS;
        pContext->state = TREBASS_STATE_ACTIVE;
        *(int *)pReplyData = 0;
        break;
    case EFFECT_CMD_DISABLE:
        if (pReplyData == NULL || replySize == NULL || *replySize != sizeof(int))
            return -EINVAL;
        if (pContext->state != TREBASS_STATE_ACTIVE)
            return -ENOSYS;
        pContext->state = TREBASS_STATE_INITIALIZED;
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

        p->status = TrebleBass_getParameter(pContext, p->data, (size_t  *)&p->vsize, p->data + voffset);
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
        *(int *)pReplyData = TrebleBass_setParameter(pContext, (void *)p->data, p->data + p->psize);
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

int TrebleBass_getDescriptor(effect_handle_t self, effect_descriptor_t *pDescriptor)
{
    TREBASSContext * pContext = (TREBASSContext *) self;

    if (pContext == NULL || pDescriptor == NULL) {
        ALOGE("%s: invalid param", __FUNCTION__);
        return -EINVAL;
    }

    *pDescriptor = TrebleBassDescriptor;

    return 0;
}

//-------------------- Effect Library Interface Implementation------------------------

int TrebleBassLib_Create(const effect_uuid_t * uuid, int32_t sessionId __unused, int32_t ioId __unused, effect_handle_t * pHandle)
{
    //int ret;

    if (pHandle == NULL || uuid == NULL) {
        return -EINVAL;
    }

    if (memcmp(uuid, &TrebleBassDescriptor.uuid, sizeof(effect_uuid_t)) != 0) {
        return -EINVAL;
    }

    TREBASSContext *pContext = new TREBASSContext;
    if (!pContext) {
        ALOGE("%s: alloc HPEQContext failed", __FUNCTION__);
        return -EINVAL;
    }
    memset(pContext, 0, sizeof(TREBASSContext));
    if (TrebleBass_load_ini_file(pContext) < 0) {
        ALOGE("%s: Load INI File faied, use default param", __FUNCTION__);
        pContext->gTreBassdata.enable = 1;
    }

    pContext->itfe = &TrebleBassInterface;
    pContext->state = TREBASS_STATE_UNINITIALIZED;

    *pHandle = (effect_handle_t)pContext;

    pContext->state = TREBASS_STATE_INITIALIZED;

    ALOGD("%s: %p", __FUNCTION__, pContext);

    return 0;
}

int TrebleBassLib_Release(effect_handle_t handle)
{
    TREBASSContext * pContext = (TREBASSContext *)handle;

    if (pContext == NULL) {
        return -EINVAL;
    }

    TrebleBass_release(pContext);
    pContext->state = TREBASS_STATE_UNINITIALIZED;
    delete pContext;

    return 0;
}

int TrebleBassLib_GetDescriptor(const effect_uuid_t *uuid, effect_descriptor_t *pDescriptor)
{
    if (pDescriptor == NULL || uuid == NULL) {
        ALOGE("%s: called with NULL pointer", __FUNCTION__);
        return -EINVAL;
    }

    if (memcmp(uuid, &TrebleBassDescriptor.uuid, sizeof(effect_uuid_t)) == 0) {
        *pDescriptor = TrebleBassDescriptor;
        return 0;
    }

    return  -EINVAL;
}

// effect_handle_t interface implementation for TrebleBassInterface effect
const struct effect_interface_s TrebleBassInterface = {
        TrebleBass_process,
        TrebleBass_command,
        TrebleBass_getDescriptor,
        NULL,
};

audio_effect_library_t AUDIO_EFFECT_LIBRARY_INFO_SYM = {
    .tag = AUDIO_EFFECT_LIBRARY_TAG,
    .version = EFFECT_LIBRARY_API_VERSION,
    .name = "TrebleBass",
    .implementor = "Amlogic",
    .create_effect = TrebleBassLib_Create,
    .release_effect = TrebleBassLib_Release,
    .get_descriptor = TrebleBassLib_GetDescriptor,
};

}; // extern "C"
