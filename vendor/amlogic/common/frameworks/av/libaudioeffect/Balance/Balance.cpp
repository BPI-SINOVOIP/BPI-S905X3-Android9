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

#define LOG_TAG "Balance_Effect"
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
#include <hardware/audio_effect.h>
#include <cutils/properties.h>
#include <stdio.h>
#include <unistd.h>

#include "IniParser.h"
#include "Balance.h"

extern "C" {

#define MODEL_SUM_DEFAULT_PATH "/vendor/etc/tvconfig/model/model_sum.ini"
#define AUDIO_EFFECT_DEFAULT_PATH "/vendor/etc/tvconfig/audio/AMLOGIC_AUDIO_EFFECT_DEFAULT.ini"

// effect_handle_t interface implementation for Balance effect
extern const struct effect_interface_s BalanceInterface;

//Balance effect TYPE: 7cb34dc0-242e-11e6-bb63-0002a5d5c51b
//Balance effect UUID: 6f33b3a0-578e-11e5-892f-0002a5d5c51b
const effect_descriptor_t BalanceDescriptor = {
        {0x7cb34dc0, 0x242e, 0x11e6, 0xbb63, {0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b}}, // type
        {0x6f33b3a0, 0x578e, 0x11e5, 0x892f, {0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b}}, // uuid
        EFFECT_CONTROL_API_VERSION,
        EFFECT_FLAG_TYPE_POST_PROC | EFFECT_FLAG_DEVICE_IND | EFFECT_FLAG_NO_PROCESS | EFFECT_FLAG_OFFLOAD_SUPPORTED,
        BALANCE_CUP_LOAD_ARM9E,
        BALANCE_MEM_USAGE,
        "Balance",
        "Amlogic",
};

enum balance_state_e {
    BALANCE_STATE_UNINITIALIZED,
    BALANCE_STATE_INITIALIZED,
    BALANCE_STATE_ACTIVE,
};

typedef enum {
    BALANCE_PARAM_LEVEL = 0,
    BALANCE_PARAM_ENABLE,
    BALANCE_PARAM_LEVEL_NUM,
    BALANCE_PARAM_INDEX,
} Balanceparams;

typedef struct Balancecfg_s {
    int32_t num;
    float   *level;
} Balancecfg;

typedef struct Balancedata_s {
    Balancecfg  usr_cfg;
    int32_t     enable;
    int32_t     index;
} Balancedata;

typedef struct BalanceContext_s{
    const struct effect_interface_s *itfe;
    effect_config_t                 config;
    balance_state_e                 state;
    Balancedata                     gBalancedata;
} BalanceContext;

#define LSR (1)
/*Absolute min volume in dB (can be represented in single precision normal float value)*/
float default_level[] = {
    -50.0, -48.0, -46.0, -44.0, -42.0,  /*0-4*/
    -40.0, -38.0, -36.0, -34.0, -32.0,  /*5-9*/
    -30.0, -28.0, -26.0, -24.0, -22.0,  /*10-14*/
    -20.0, -18.0, -16.0, -14.0, -12.0,  /*15-19*/
    -10.0, -8.0,  -6.0,  -4.0,  -2.0,   /*20-24*/
    0.0,                                /*25*/
    -2.0,  -4.0,  -6.0,  -8.0,  -10.0,  /*26-30*/
    -12.0, -14.0, -16.0, -18.0, -20.0,  /*31-35*/
    -22.0, -24.0, -26.0, -28.0, -30.0,  /*36-40*/
    -32.0, -34.0, -36.0, -38.0, -40.0,  /*41-45*/
    -42.0, -44.0, -46.0, -48.0, -50.0   /*46-50*/
};

const char *BalanceStatusstr[] = {"Disable", "Enable"};

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

int Balance_get_model_name(char *model_name, int size)
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

int Balance_get_ini_file(char *ini_name, int size)
{
    int result = -1;
    char model_name[50] = {0};
    IniParser* pIniParser = NULL;
    const char *ini_value = NULL;
    const char *filename = MODEL_SUM_DEFAULT_PATH;

    Balance_get_model_name(model_name, sizeof(model_name));
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

int Balance_parse_level(BalanceContext *pContext, int num, const char *buffer)
{
    int i;
    char *Rch = (char *)buffer;
    Balancedata *data = &pContext->gBalancedata;

    if (data->usr_cfg.level == NULL) {
        data->usr_cfg.level = (float *)calloc(num, sizeof(float));
        if (!data->usr_cfg.level) {
            ALOGE("%s: alloc failed", __FUNCTION__);
            return -EINVAL;
        }
    }

    for (i = 0; i < num; i++) {
        if (i == 0)
            Rch = strtok(Rch, ",");
        else
            Rch = strtok(NULL, ",");
        if (Rch == NULL) {
            ALOGE("%s: Level Parse failed", __FUNCTION__);
            return -1;
        }
        data->usr_cfg.level[i] = db_to_ampl(atof(Rch));
        //ALOGD("%s: Leve[%d] = %s -> %f", __FUNCTION__, i, Rch, data->usr_cfg.level[i]);
    }

    return 0;
}

int Balance_load_ini_file(BalanceContext *pContext)
{
    int result = -1;
    char ini_name[100] = {0};
    const char *ini_value = NULL;
    Balancedata *data = &pContext->gBalancedata;
    IniParser* pIniParser = NULL;

    if (Balance_get_ini_file(ini_name, sizeof(ini_name)) < 0)
        goto error;

    pIniParser = new IniParser();
    if (pIniParser->parse((const char *)ini_name) < 0) {
        ALOGD("%s: %s load failed", __FUNCTION__, ini_name);
        goto error;
    }
    ini_value = pIniParser->GetString("Balance", "balance_enable", "1");
    if (ini_value == NULL)
        goto error;
    ALOGD("%s: enable -> %s", __FUNCTION__, ini_value);
    data->enable = atoi(ini_value);

    ini_value = pIniParser->GetString("Balance", "balance_num", "51");
    if (ini_value == NULL)
        goto error;
    ALOGD("%s: num -> %s", __FUNCTION__, ini_value);
    data->usr_cfg.num = atoi(ini_value);

    // level parse
    ini_value = pIniParser->GetString("Balance", "balance_level", "NULL");
    if (ini_value == NULL)
        goto error;
    ALOGD("%s: level -> %s", __FUNCTION__, ini_value);
    result = Balance_parse_level(pContext, data->usr_cfg.num, ini_value);
    if (result < 0)
        goto error;

    result = 0;
error:
    ALOGD("%s: %s", __FUNCTION__, result == 0 ? "sucessful" : "failed");
    delete pIniParser;
    pIniParser = NULL;
    return result;
}

int Balance_init(BalanceContext *pContext)
{
    Balancedata *data = &pContext->gBalancedata;
    int32_t num = data->usr_cfg.num;

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

    data->index = (num>>LSR);

    ALOGD("%s: sucessful", __FUNCTION__);

    return 0;
}

int Balance_reset(BalanceContext *pContext __unused)
{
    return 0;
}

int Balance_configure(BalanceContext *pContext, effect_config_t *pConfig)
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

int Balance_getParameter(BalanceContext *pContext, void *pParam, size_t *pValueSize, void *pValue)
{
    int32_t param = *(int32_t *)pParam;
    int32_t value;
    float scale;
    Balancedata *data = &pContext->gBalancedata;

    switch (param) {
    case BALANCE_PARAM_LEVEL:
        if (*pValueSize < sizeof(int32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        value = data->index;
        scale = data->usr_cfg.level[value];
        *(int32_t *) pValue = value << LSR;
        ALOGD("%s: Get Balance Gain %d -> %f", __FUNCTION__, value, scale);
        break;
    case BALANCE_PARAM_ENABLE:
        if (*pValueSize < sizeof(int32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        value = data->enable;
        *(int32_t *) pValue = value;
        ALOGD("%s: Get status -> %s", __FUNCTION__, BalanceStatusstr[value]);
        break;
    case BALANCE_PARAM_LEVEL_NUM:
        if (*pValueSize < sizeof(int32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        value = data->usr_cfg.num;
        *(int32_t *) pValue = value;
        break;
    case BALANCE_PARAM_INDEX:
        float *pTmp;
        pTmp = (float *)pValue;
        for (int i = 0; i < data->usr_cfg.num ; i++) {
            *pTmp++ = data->usr_cfg.level[i];
            //ALOGE(" Leve[%f]",data->usr_cfg.level[i]);
         }
        break;
    default:
        ALOGE("%s: unknown param %d", __FUNCTION__, param);
        return -EINVAL;
    }
    return 0;
}

int Balance_setParameter(BalanceContext *pContext, void *pParam, void *pValue)
{
    int32_t param = *(int32_t *)pParam;
    int32_t value;
    Balancedata *data = &pContext->gBalancedata;

    switch (param) {
    case BALANCE_PARAM_LEVEL:
        value = (*(int32_t *)pValue >> LSR);
        if (value >= data->usr_cfg.num)
            value = data->usr_cfg.num -1;
        else if (value < 0)
            value = 0;
        data->index = value;
        ALOGD("%s: Set Balance Gain %d -> %f", __FUNCTION__, value, data->usr_cfg.level[value]);
        break;
    case BALANCE_PARAM_ENABLE:
        value = *(int32_t *)pValue;
        data->enable = value;

        ALOGD("%s: Set status -> %s", __FUNCTION__, BalanceStatusstr[value]);
        break;
    default:
        ALOGE("%s: unknown param %08x", __FUNCTION__, param);
        return -EINVAL;
    }

    return 0;
}

int Balance_release(BalanceContext *pContext)
{
    Balancedata *data = &pContext->gBalancedata;

    if (data->usr_cfg.level != NULL) {
        free(data->usr_cfg.level);
        data->usr_cfg.level = NULL;
    }

    return 0;
}

//-------------------Effect Control Interface Implementation--------------------------

int Balance_process(effect_handle_t self, audio_buffer_t *inBuffer, audio_buffer_t *outBuffer)
{
    BalanceContext * pContext = (BalanceContext *)self;

    if (pContext == NULL)
        return -EINVAL;

    if (inBuffer == NULL || inBuffer->raw == NULL ||
        outBuffer == NULL || outBuffer->raw == NULL ||
        inBuffer->frameCount != outBuffer->frameCount ||
        inBuffer->frameCount == 0)
        return -EINVAL;

    if (pContext->state != BALANCE_STATE_ACTIVE)
        return -ENODATA;

    int16_t *in  = (int16_t *)inBuffer->raw;
    int16_t *out = (int16_t *)outBuffer->raw;
    Balancedata *data = &pContext->gBalancedata;
    int32_t val = data->index;

    for (size_t i = 0; i < inBuffer->frameCount; i++) {
        if (!data->enable) {
            *out++ = *in++;
            *out++ = *in++;
        } else if (val < (data->usr_cfg.num >> LSR)) {
            *out++ = *in++;
            /*Input right process*/
            *out++ = clamp16((int32_t)(*in++ * data->usr_cfg.level[val]));
        } else {
            /*Input left process*/
            *out++ = clamp16((int32_t)(*in++ * data->usr_cfg.level[val]));
            *out++ = *in++;
        }
    }

    return 0;
}

int Balance_command(effect_handle_t self, uint32_t cmdCode, uint32_t cmdSize,
        void *pCmdData, uint32_t *replySize, void *pReplyData)
{
    BalanceContext * pContext = (BalanceContext *)self;
    effect_param_t *p;
    int voffset;

    if (pContext == NULL || pContext->state == BALANCE_STATE_UNINITIALIZED)
        return -EINVAL;

    ALOGD("%s: cmd = %u", __FUNCTION__, cmdCode);
    switch (cmdCode) {
    case EFFECT_CMD_INIT:
        if (pReplyData == NULL || replySize == NULL || *replySize != sizeof(int))
            return -EINVAL;
        *(int *) pReplyData = Balance_init(pContext);
        break;
    case EFFECT_CMD_SET_CONFIG:
        if (pCmdData == NULL || cmdSize != sizeof(effect_config_t) || pReplyData == NULL || replySize == NULL || *replySize != sizeof(int))
            return -EINVAL;
        *(int *) pReplyData = Balance_configure(pContext,(effect_config_t *) pCmdData);
        break;
    case EFFECT_CMD_RESET:
        Balance_reset(pContext);
        break;
    case EFFECT_CMD_ENABLE:
        if (pReplyData == NULL || replySize == NULL || *replySize != sizeof(int))
            return -EINVAL;
        if (pContext->state != BALANCE_STATE_INITIALIZED)
            return -ENOSYS;
        pContext->state = BALANCE_STATE_ACTIVE;
        *(int *)pReplyData = 0;
        break;
    case EFFECT_CMD_DISABLE:
        if (pReplyData == NULL || replySize == NULL || *replySize != sizeof(int))
            return -EINVAL;
        if (pContext->state != BALANCE_STATE_ACTIVE)
            return -ENOSYS;
        pContext->state = BALANCE_STATE_INITIALIZED;
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

        p->status = Balance_getParameter(pContext, p->data, (size_t  *)&p->vsize, p->data + voffset);
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
        *(int *)pReplyData = Balance_setParameter(pContext, (void *)p->data, p->data + p->psize);
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

int Balance_getDescriptor(effect_handle_t self, effect_descriptor_t *pDescriptor)
{
    BalanceContext * pContext = (BalanceContext *) self;

    if (pContext == NULL || pDescriptor == NULL) {
        ALOGE("%s: invalid param", __FUNCTION__);
        return -EINVAL;
    }

    *pDescriptor = BalanceDescriptor;

    return 0;
}

//-------------------- Effect Library Interface Implementation------------------------

int BalanceLib_Create(const effect_uuid_t *uuid, int32_t sessionId __unused, int32_t ioId __unused, effect_handle_t *pHandle)
{
    int i;

    if (pHandle == NULL || uuid == NULL)
        return -EINVAL;

    if (memcmp(uuid, &BalanceDescriptor.uuid, sizeof(effect_uuid_t)) != 0)
        return -EINVAL;

    BalanceContext *pContext = new BalanceContext;
    if (!pContext) {
        ALOGE("%s: alloc BalanceContext failed", __FUNCTION__);
        return -EINVAL;
    }
    memset(pContext, 0, sizeof(BalanceContext));
    if (Balance_load_ini_file(pContext) < 0) {
        ALOGE("%s: Load INI File faied, use default param", __FUNCTION__);
        pContext->gBalancedata.enable = 1;
        pContext->gBalancedata.usr_cfg.num = sizeof(default_level)/sizeof(float);
        pContext->gBalancedata.usr_cfg.level = (float *)calloc(pContext->gBalancedata.usr_cfg.num, sizeof(float));
        if (!pContext->gBalancedata.usr_cfg.level) {
            ALOGE("%s: alloc failed", __FUNCTION__);
            delete pContext;
            return -EINVAL;
        }
        for (i = 0; i < pContext->gBalancedata.usr_cfg.num; i++)
            pContext->gBalancedata.usr_cfg.level[i] = db_to_ampl(default_level[i]);
    }

    pContext->itfe = &BalanceInterface;
    pContext->state = BALANCE_STATE_UNINITIALIZED;

    *pHandle = (effect_handle_t)pContext;

    pContext->state = BALANCE_STATE_INITIALIZED;

    ALOGD("%s: %p", __FUNCTION__, pContext);

    return 0;
}

int BalanceLib_Release(effect_handle_t handle)
{
    BalanceContext * pContext = (BalanceContext *)handle;

    if (pContext == NULL)
        return -EINVAL;

    Balance_release(pContext);
    pContext->state = BALANCE_STATE_UNINITIALIZED;
    delete pContext;

    return 0;
}

int BalanceLib_GetDescriptor(const effect_uuid_t *uuid, effect_descriptor_t *pDescriptor)
{
    if (pDescriptor == NULL || uuid == NULL) {
        ALOGE("%s: called with NULL pointer", __FUNCTION__);
        return -EINVAL;
    }

    if (memcmp(uuid, &BalanceDescriptor.uuid, sizeof(effect_uuid_t)) == 0) {
        *pDescriptor = BalanceDescriptor;
        return 0;
    }

    return  -EINVAL;
}

// effect_handle_t interface implementation for Balance effect
const struct effect_interface_s BalanceInterface = {
        Balance_process,
        Balance_command,
        Balance_getDescriptor,
        NULL,
};

audio_effect_library_t AUDIO_EFFECT_LIBRARY_INFO_SYM = {
    .tag = AUDIO_EFFECT_LIBRARY_TAG,
    .version = EFFECT_LIBRARY_API_VERSION,
    .name = "Balance",
    .implementor = "Amlogic",
    .create_effect = BalanceLib_Create,
    .release_effect = BalanceLib_Release,
    .get_descriptor = BalanceLib_GetDescriptor,
};

}; // extern "C"
