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
 *
 *  DESCRIPTION
 *      This file implements a special EQ  from Amlogic.
 *
 */
#define LOG_TAG "Virtualsurround_Effect"
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
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#include <hardware/audio_effect.h>
#include <cutils/properties.h>
#include "Virtualsurround.h"


#include "IniParser.h"

extern "C"{

#include "LVCS.h"
#include "InstAlloc.h"
#include "LVCS_Private.h"

LVCS_Handle_t           hCSInstance = LVM_NULL; /* Concert Sound instance handle */
LVCS_Instance_t         CS_Instance;        /* Concert Sound instance */
LVCS_MemTab_t           CS_MemTab;          /* Memory table */
LVCS_Capabilities_t     CS_Capabilities;    /* Initial capabilities */
static pthread_mutex_t audio_vir_mutex = PTHREAD_MUTEX_INITIALIZER;

#define MODEL_SUM_DEFAULT_PATH "/vendor/etc/tvconfig/model/model_sum.ini"
#define AUDIO_EFFECT_DEFAULT_PATH "/vendor/etc/tvconfig/audio/AMLOGIC_AUDIO_EFFECT_DEFAULT.ini"

// effect_handle_t interface implementation for Virtualsurround effect
extern const struct effect_interface_s VirtualsurroundInterface;

//Virtualsurround effect TYPE: c656ec6f-d6be-4e7f-854b-1218077f3915
//Virtualsurround effect UUID: c8459cd3-4400-4859-b76b-e12cc2aa67ce
const effect_descriptor_t VirtualsurroundDescriptor = {
       {0xc656ec6f,0xd6be,0x4e7f,0x854b,{0x12,0x18,0x07,0x7f,0x39,0x15}},
       {0xc8459cd3,0x4400,0x4859,0xb76b,{0xe1,0x2c,0xc2,0xaa,0x67,0xce}},
        EFFECT_CONTROL_API_VERSION,
        EFFECT_FLAG_TYPE_POST_PROC | EFFECT_FLAG_DEVICE_IND | EFFECT_FLAG_NO_PROCESS | EFFECT_FLAG_OFFLOAD_SUPPORTED,
        VIRTUALSURROUND_CUP_LOAD_ARM9E,
        VIRTUALSURROUND_MEM_USAGE,
        "Virtualsurround",
        "Amlogic",
};

enum Virtualsurround_state_e {
    VIRTUALSURROUND_STATE_UNINITIALIZED,
    VIRTUALSURROUND_STATE_INITIALIZED,
    VIRTUALSURROUND_STATE_ACTIVE,
};

/*parameter number passed from the Java layer corresponds to this*/
typedef enum {
    VIRTUALSURROUND_PARAM_ENABLE,
    VIRTUALSURROUND_PARAM_EFFECTLEVEL,
} Virtualsurroundparams;

typedef struct Virtualsurroundcfg_s {
    int  enable;
    int  effectlevel;
} Virtualsurroundcfg;

typedef struct Virtualsurrounddata_s {
    /* Virtualsurround API needed data */
    Virtualsurroundcfg      tbcfg;
} Virtualsurrounddata;

typedef struct VirtualsurroundContext_s {
    const struct effect_interface_s *itfe;
    effect_config_t                 config;
    Virtualsurround_state_e         state;
    Virtualsurrounddata             gVirtualsurrounddata;
} VirtualsurroundContext;

const char *VirtualsurroundStatusstr[] = {"Disable", "Enable"};

int Virtualsurround_get_model_name(char *model_name, int size) {
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

int Virtualsurround_get_ini_file(char *ini_name, int size) {
    int result = -1;
    char model_name[50] = {0};
    IniParser* pIniParser = NULL;
    const char *ini_value = NULL;
    const char *filename = MODEL_SUM_DEFAULT_PATH;

    Virtualsurround_get_model_name(model_name, sizeof(model_name));
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

int Virtualsurround_load_ini_file(VirtualsurroundContext *pContext)
{
    int result = -1;
    char ini_name[100] = {0};
    const char *ini_value = NULL;
    Virtualsurrounddata *data = &pContext->gVirtualsurrounddata;
    IniParser* pIniParser = NULL;
    if (Virtualsurround_get_ini_file(ini_name, sizeof(ini_name)) < 0)
        goto error;

    pIniParser = new IniParser();
    if (pIniParser->parse((const char *)ini_name) < 0) {
        ALOGD("%s: %s load failed", __FUNCTION__, ini_name);
        goto error;
    }
    /*
    ini_value = pIniParser->GetString("Virtualsurround", "Virtualsurround_enable", "1");
    if (ini_value == NULL)
        goto error;
    ALOGD("%s: enable -> %s", __FUNCTION__, ini_value);
    data->tbcfg.enable = atoi(ini_value);
    */
error:
    ALOGD("%s: %s", __FUNCTION__, result == 0 ? "sucessful" : "failed");
    delete pIniParser;
    pIniParser = NULL;
    return result;
}

int Virtualsurround_init(VirtualsurroundContext *pContext) {
    LVCS_ReturnStatus_en    LVCS_Status;
    LVCS_Params_t *CS_Params = &CS_Instance.Params;
    int i = 0;

    pthread_mutex_lock(&audio_vir_mutex);
    CS_Capabilities.MaxBlockSize = 2048;
    CS_Capabilities.pBundleInstance = (void*)hCSInstance;
    LVCS_Status = LVCS_Memory(LVM_NULL,
                              &CS_MemTab,
                              &CS_Capabilities);
    CS_MemTab.Region[LVCS_MEMREGION_PERSISTENT_SLOW_DATA].pBaseAddress = &CS_Instance;
    /* Allocate memory */
    for (i = 0; i < LVM_NR_MEMORY_REGIONS; i++) {
        if (CS_MemTab.Region[i].Size != 0) {
            CS_MemTab.Region[i].pBaseAddress = malloc(CS_MemTab.Region[i].Size);
            if (CS_MemTab.Region[i].pBaseAddress == LVM_NULL) {
                ALOGV("\tLVM_ERROR :LvmBundle_init CreateInstance Failed to allocate %"
                    " bytes for region %u\n", CS_MemTab.Region[i].Size, i );
                return LVCS_NULLADDRESS;
            } else {
                ALOGV("\tLvmBundle_init CreateInstance allocated %"
                    " bytes for region %u at %p\n",
                    CS_MemTab.Region[i].Size, i, CS_MemTab.Region[i].pBaseAddress);
            }
        }
    }
    hCSInstance = LVM_NULL;
    LVCS_Status = LVCS_Init(&hCSInstance,
                              &CS_MemTab,
                              &CS_Capabilities);
    pContext->gVirtualsurrounddata.tbcfg.effectlevel = 0;
    pContext->gVirtualsurrounddata.tbcfg.enable = 0;
    CS_Params->OperatingMode = LVCS_OFF;
    CS_Params->CompressorMode = LVM_MODE_ON;
    CS_Params->SourceFormat = LVCS_MONOINSTEREO;//LVCS_STEREO;
    CS_Params->SpeakerType = LVCS_HEADPHONES;
    CS_Params->SampleRate  = LVM_FS_48000;
    CS_Params->ReverbLevel = 512;
    CS_Params->EffectLevel = 32700; /* 0~32767 */
    pthread_mutex_unlock(&audio_vir_mutex);

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

/*Set I/O configuration: sample rate, channel, format, etc.*/
int Virtualsurround_configure(VirtualsurroundContext *pContext, effect_config_t *pConfig)
{
    if (pConfig->inputCfg.samplingRate != pConfig->outputCfg.samplingRate)
        return -EINVAL;
    if (pConfig->inputCfg.channels != pConfig->outputCfg.channels)
        return -EINVAL;
    if (pConfig->inputCfg.format != pConfig->outputCfg.format)
        return -EINVAL;
    if (pConfig->inputCfg.channels != AUDIO_CHANNEL_OUT_STEREO) {
        ALOGW("%s: channels in = 0x%x channels out = 0x%x", __FUNCTION__,
            pConfig->inputCfg.channels, pConfig->outputCfg.channels);
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

int Virtualsurround_setParameter(VirtualsurroundContext *pContext, void *pParam, void *pValue)
{
    int32_t param = *(int32_t *)pParam;
    int32_t value;
    Virtualsurrounddata *data=&pContext->gVirtualsurrounddata;
    Virtualsurroundcfg *tbcfg=&data->tbcfg;
    LVCS_ReturnStatus_en  CS_Status;
    LVCS_Params_t *CS_Params = &CS_Instance.Params;
    if (hCSInstance == LVM_NULL)
        return LVCS_NULLADDRESS;
    pthread_mutex_lock(&audio_vir_mutex);
    switch (param) {
        case VIRTUALSURROUND_PARAM_ENABLE:
            value = *(int32_t *)pValue;
            tbcfg->enable = value;
            if (tbcfg->enable == 1)
               CS_Params->OperatingMode = LVCS_ON;
            else
               CS_Params->OperatingMode = LVCS_OFF;
            LVCS_Control(hCSInstance,CS_Params);
            pthread_mutex_unlock(&audio_vir_mutex);
            break;
        case VIRTUALSURROUND_PARAM_EFFECTLEVEL:
            value = *(int32_t *)pValue;
            tbcfg->effectlevel = value;
            if (tbcfg->effectlevel > 100)
                CS_Params->EffectLevel = 32700;
            else if (tbcfg->effectlevel < 0)
                CS_Params->EffectLevel  = 0;
            else
                CS_Params->EffectLevel = tbcfg->effectlevel * 327;
            LVCS_Control(hCSInstance,CS_Params);
            pthread_mutex_unlock(&audio_vir_mutex);
            break;
        default:
            ALOGE("%s: unknown param %08x", __FUNCTION__, param);
            return -EINVAL;
    }
    return 0;
}

int Virtualsurround_getParameter(VirtualsurroundContext*pContext, void *pParam, size_t *pValueSize, void *pValue)
{
    int32_t param = *(int32_t *)pParam;
    Virtualsurrounddata *data=&pContext->gVirtualsurrounddata;
    Virtualsurroundcfg *tbcfg=&data->tbcfg;
    switch (param) {
    case VIRTUALSURROUND_PARAM_ENABLE:
        if (*pValueSize < sizeof(int32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        *(int*)pValue = (int)tbcfg->enable;
        ALOGD("%s: Get enable -> %d ", __FUNCTION__, tbcfg->enable);
        break;
    case VIRTUALSURROUND_PARAM_EFFECTLEVEL:
        if (*pValueSize < sizeof(int32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        *(int*)pValue = tbcfg->effectlevel;
        ALOGD("%s: Get effctlevel -> %d ", __FUNCTION__, tbcfg->effectlevel);
        break;
    default:
        ALOGE("%s: unknown param %d", __FUNCTION__, param);
        return -EINVAL;
    }
    return 0;
}

int Virtualsurround_release(VirtualsurroundContext *pContext __unused) {
    int i;
    pthread_mutex_lock(&audio_vir_mutex);
    for (i = 0; i < LVM_NR_MEMORY_REGIONS; i++) {
        if (CS_MemTab.Region[i].pBaseAddress != 0) {
            free(CS_MemTab.Region[i].pBaseAddress);
            CS_MemTab.Region[i].pBaseAddress = NULL;
        }
    }
    hCSInstance = LVM_NULL;
    pthread_mutex_unlock(&audio_vir_mutex);
    return 0;
}

//-------------------Effect Control Interface Implementation--------------------------

int Virtualsurround_process(effect_handle_t self, audio_buffer_t *inBuffer, audio_buffer_t *outBuffer)
{
    LVCS_ReturnStatus_en        CS_Status;
    VirtualsurroundContext * pContext = (VirtualsurroundContext *)self;
    if (pContext == NULL) {
        return -EINVAL;
    }

    if (inBuffer == NULL || inBuffer->raw == NULL ||
        outBuffer == NULL || outBuffer->raw == NULL ||
        inBuffer->frameCount != outBuffer->frameCount ||
        inBuffer->frameCount == 0) {
        return -EINVAL;
    }
    if (pContext->state != VIRTUALSURROUND_STATE_ACTIVE) {
        return -ENODATA;
    }
    int16_t *in  = (int16_t *)inBuffer->raw;
    int16_t *out = (int16_t *)outBuffer->raw;
    Virtualsurrounddata *data = &pContext->gVirtualsurrounddata;
    if (!data->tbcfg.enable) {
        for (size_t i = 0; i < inBuffer->frameCount; i++) {
            *out++ = *in++;
            *out++ = *in++;
        }
    } else {
        if (hCSInstance == LVM_NULL)
            return LVCS_NULLADDRESS;
        pthread_mutex_lock(&audio_vir_mutex);
        LVCS_Process(hCSInstance,in,out,inBuffer->frameCount);

        pthread_mutex_unlock(&audio_vir_mutex);
    }
    return 0;
}

int Virtualsurround_command(effect_handle_t self, uint32_t cmdCode, uint32_t cmdSize,
        void *pCmdData, uint32_t *replySize, void *pReplyData)
{
    VirtualsurroundContext* pContext = (VirtualsurroundContext *)self;
    effect_param_t *p;
    int voffset;
    if (pContext == NULL || pContext->state == VIRTUALSURROUND_STATE_UNINITIALIZED) {
        return -EINVAL;
    }
    ALOGD("%s: cmd = %u", __FUNCTION__, cmdCode);
    switch (cmdCode) {
    case EFFECT_CMD_INIT:
        if (pReplyData == NULL || replySize == NULL || *replySize != sizeof(int)) {
            return -EINVAL;
        }
        *(int *) pReplyData = Virtualsurround_init(pContext);
        break;
    case EFFECT_CMD_SET_CONFIG:
        if (pCmdData == NULL || cmdSize != sizeof(effect_config_t) || pReplyData == NULL || replySize == NULL || *replySize != sizeof(int))
            return -EINVAL;
        *(int *) pReplyData = Virtualsurround_configure(pContext, (effect_config_t *) pCmdData);
        break;
    case EFFECT_CMD_RESET:
        break;
    case EFFECT_CMD_ENABLE:
        if (pReplyData == NULL || replySize == NULL || *replySize != sizeof(int))
            return -EINVAL;
        if (pContext->state != VIRTUALSURROUND_STATE_INITIALIZED)
            return -ENOSYS;
        pContext->state = VIRTUALSURROUND_STATE_ACTIVE;
        *(int *)pReplyData = 0;
        break;
    case EFFECT_CMD_DISABLE:
        if (pReplyData == NULL || replySize == NULL || *replySize != sizeof(int))
            return -EINVAL;
        if (pContext->state != VIRTUALSURROUND_STATE_ACTIVE)
            return -ENOSYS;
        pContext->state = VIRTUALSURROUND_STATE_INITIALIZED;
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

        p->status = Virtualsurround_getParameter(pContext, p->data, (size_t  *)&p->vsize, p->data + voffset);
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
        *(int *)pReplyData = Virtualsurround_setParameter(pContext, (void *)p->data, p->data + p->psize);
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

int Virtualsurround_getDescriptor(effect_handle_t self, effect_descriptor_t *pDescriptor)
{
    VirtualsurroundContext * pContext = ( VirtualsurroundContext *) self;
    if (pContext == NULL || pDescriptor == NULL) {
        ALOGE("%s: invalid param", __FUNCTION__);
        return -EINVAL;
    }
    *pDescriptor = VirtualsurroundDescriptor;
    return 0;
}

//-------------------- Effect Library Interface Implementation------------------------

int VirtualsurroundLib_Create(const effect_uuid_t * uuid, int32_t sessionId __unused, int32_t ioId __unused, effect_handle_t * pHandle)
{
    if (pHandle == NULL || uuid == NULL) {
        return -EINVAL;
    }
    if (memcmp(uuid, &VirtualsurroundDescriptor.uuid, sizeof(effect_uuid_t)) != 0) {
        return -EINVAL;
    }
    VirtualsurroundContext *pContext = new VirtualsurroundContext;
    if (!pContext) {
        ALOGE("%s: alloc VirtualsurroundContext failed", __FUNCTION__);
        return -EINVAL;
    }
    memset(pContext, 0, sizeof(VirtualsurroundContext));
    if (Virtualsurround_load_ini_file(pContext) < 0) {
        ALOGE("%s: Load INI File faied, use default param", __FUNCTION__);
        pContext->gVirtualsurrounddata.tbcfg.enable = 0;
    }
    pContext->itfe = &VirtualsurroundInterface;
    pContext->state = VIRTUALSURROUND_STATE_UNINITIALIZED;
    *pHandle = (effect_handle_t)pContext;
    pContext->state = VIRTUALSURROUND_STATE_INITIALIZED;
    ALOGD("%s: %p", __FUNCTION__, pContext);
    return 0;
}

int VirtualsurroundLib_Release(effect_handle_t handle)
{
    VirtualsurroundContext * pContext = (VirtualsurroundContext *)handle;
    if (pContext == NULL) {
        return -EINVAL;
    }
    Virtualsurround_release(pContext);
    pContext->state = VIRTUALSURROUND_STATE_UNINITIALIZED;
    delete pContext;
    return 0;
}

int VirtualsurroundLib_GetDescriptor(const effect_uuid_t *uuid, effect_descriptor_t *pDescriptor)
{
    if (pDescriptor == NULL || uuid == NULL) {
        ALOGE("%s: called with NULL pointer", __FUNCTION__);
        return -EINVAL;
    }
    if (memcmp(uuid, &VirtualsurroundDescriptor.uuid, sizeof(effect_uuid_t)) == 0) {
        *pDescriptor = VirtualsurroundDescriptor;
        return 0;
    }
    return  -EINVAL;
}

// effect_handle_t interface implementation for VirtualsurroundInterface effect

const struct effect_interface_s VirtualsurroundInterface = {
        Virtualsurround_process,
        Virtualsurround_command,
        Virtualsurround_getDescriptor,
        NULL,
};

audio_effect_library_t AUDIO_EFFECT_LIBRARY_INFO_SYM = {
    .tag = AUDIO_EFFECT_LIBRARY_TAG,
    .version = EFFECT_LIBRARY_API_VERSION,
    .name = "Virtualsurround",
    .implementor = "Amlogic",
    .create_effect = VirtualsurroundLib_Create,
    .release_effect = VirtualsurroundLib_Release,
    .get_descriptor = VirtualsurroundLib_GetDescriptor,
};

};//extern "C"


