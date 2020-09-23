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
 *      This file implements a DBX_TV effect.
 *
 */

#define LOG_TAG "DBX_Effect"
//#define LOG_NDEBUG 0

#include <cutils/log.h>
#include <cutils/properties.h>
#include <utils/Log.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <hardware/audio_effect.h>
#include <cutils/properties.h>
#include <stdio.h>
#include <unistd.h>

#include "IniParser.h"
#include "dbx.h"

extern "C" {

#define MODEL_SUM_DEFAULT_PATH "/vendor/etc/tvconfig/model/model_sum.ini"
#define AUDIO_EFFECT_DEFAULT_PATH "/vendor/etc/tvconfig/audio/AMLOGIC_AUDIO_EFFECT_DEFAULT.ini"
#define BUFFSIZE    (1024)

#if defined(__LP64__)
#define LIBVX_PATH_A "/vendor/lib64/soundfx/libdbx_tv.so"
#else
#define LIBVX_PATH_A "/vendor/lib/soundfx/libdbx_tv.so"
#endif

const char *DBXStatusstr[] = {"Disable", "Enable"};

// effect_handle_t interface implementation for DBX effectextern
extern const struct effect_interface_s DBXInterface;

//DBXTV effect TYPE: a41cedc0-578e-11e5-9cb0-0002a5d5c51b
//DBXTV effect UUID: 07210842-7432-4624-8b97-35ac8782efa3

const effect_descriptor_t DBXDescriptor = {
    {0xa41cedc0, 0x578e, 0x11e5, 0x9cb0, {0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b}}, // type
    {0x07210842, 0x7432, 0x4624, 0x8b97, {0x35, 0xac, 0x87, 0x82, 0xef, 0xa3}}, // uuid
    EFFECT_CONTROL_API_VERSION,
    EFFECT_FLAG_TYPE_POST_PROC | EFFECT_FLAG_DEVICE_IND | EFFECT_FLAG_NO_PROCESS | EFFECT_FLAG_OFFLOAD_SUPPORTED,
    DBX_CUP_LOAD_ARM9E,
    DBX_MEM_USAGE,
    "DBX",
    "THAT Corporation",
};

enum DBX_state_e {
    DBX_STATE_UNINITIALIZED,
    DBX_STATE_INITIALIZED,
    DBX_STATE_ACTIVE,
};

typedef struct DBXapi_s {
    int (*DBX_init)(void*);
    int (*DBX_release)(void);
    int (*DBX_process)(int32_t *leftin, int32_t *rightin, int32_t *leftout, int32_t *rightout, int framecount);
    int (*DBX_setParameter)(int son_mode, int vol_mode, int sur_mode);
} DBxapi;

typedef enum {
    DBX_PARAM_ENABLE = 0,
    DBX_SET_MODE     = 1,
} DBXparams;

typedef struct DBXmode_8bit_s {
    signed char    son_mode;
    signed char    vol_mode;
    signed char    sur_mode;
} DBXmode_8bit;

typedef struct DBXmode_s {
    int         son_mode;
    int         vol_mode;
    int         sur_mode;
} DBXmode;

typedef struct DBXdata_s {
    int          enable;
    DBXmode      mode;
} DBXdata;

typedef struct DBXContext_s {
    const struct effect_interface_s *itfe;
    effect_config_t          config;
    DBX_state_e              state;
    void                     *gDBXLibHandler;
    DBxapi                    gDBXapi;
    DBXdata                   gDBXdata;
    int32_t                  *aiLeftA;
    int32_t                  *aiRightA;
    int32_t                  *aiLeftB;
    int32_t                  *aiRightB;
} DBXContext;

int DBX_get_model_name(char *model_name, int size)
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

int DBX_get_ini_file(char *ini_name, int size)
{
    int result = -1;
    char model_name[50] = {0};
    IniParser* pIniParser = NULL;
    const char *ini_value = NULL;
    const char *filename = MODEL_SUM_DEFAULT_PATH;

    DBX_get_model_name(model_name, sizeof(model_name));
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

int DBX_load_ini_file(DBXContext *pContext)
{
    int result = -1;
    char ini_name[100] = {0};
    const char *ini_value = NULL;
    char *Rch = NULL;
    DBXdata *data = &pContext->gDBXdata;
    IniParser* pIniParser = NULL;

    if (DBX_get_ini_file(ini_name, sizeof(ini_name)) < 0)
        goto error;

    pIniParser = new IniParser();
    if (pIniParser->parse((const char *)ini_name) < 0) {
        ALOGD("%s: %s load failed", __FUNCTION__, ini_name);
        goto error;
    }

    ini_value = pIniParser->GetString("DBX", "enable", "1");
    if (ini_value == NULL)
        goto error;
    ALOGD("%s: enable -> %s", __FUNCTION__, ini_value);
    data->enable = atoi(ini_value);

    ini_value = pIniParser->GetString("DBX", "mode", "NULL");
    if (ini_value == NULL)
        goto error;
    Rch = (char *)ini_value;
    Rch = strtok(Rch, ",");
    if (Rch == NULL) {
         goto error;
    }
    data->mode.son_mode=  atoi(Rch);
    ALOGD("sonics mode is %d",data->mode.son_mode);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
        goto error;
    }
    data->mode.vol_mode = atoi(Rch);
    ALOGD("volume mode is %d",data->mode.vol_mode);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
        goto error;
    }
    data->mode.sur_mode = atoi(Rch);
    ALOGD("surround mode is %d",data->mode.sur_mode);
    result = 0;
error:
    ALOGD("%s: %s", __FUNCTION__, result == 0 ? "sucessful" : "failed");
    delete pIniParser;
    pIniParser = NULL;
    return result;
}

int DBX_load_lib(DBXContext *pContext)
{
    pContext->gDBXLibHandler = dlopen(LIBVX_PATH_A, RTLD_NOW);
    if (!pContext->gDBXLibHandler) {
        ALOGE("%s: failed", __FUNCTION__);
        return -EINVAL;
    }
    pContext->gDBXapi.DBX_init = (int (*)(void*))dlsym(pContext->gDBXLibHandler, "DBXTV_init_api");
    if (!pContext->gDBXapi.DBX_init) {
        ALOGE("%s: find func DBX_init() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gDBXapi.DBX_process = (int (*)(int32_t*,int32_t*,int32_t*,int32_t*,int))dlsym(pContext->gDBXLibHandler, "DBXTV_process_api");
    if (!pContext->gDBXapi.DBX_process) {
        ALOGE("%s: find func DBX_process() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gDBXapi.DBX_setParameter = (int (*)(int,int,int))dlsym(pContext->gDBXLibHandler, "DBXTV_setParameter_api");
    if (!pContext->gDBXapi.DBX_setParameter) {
        ALOGE("%s: find func DBX_setParameter() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gDBXapi.DBX_release = (int (*)(void))dlsym(pContext->gDBXLibHandler, "DBXTV_release_api");
    if (!pContext->gDBXapi.DBX_release) {
        ALOGE("%s: find func DBX_release() failed\n", __FUNCTION__);
        goto Error;
    }
    ALOGD("%s: sucessful", __FUNCTION__);
    return 0;
Error:
    memset(&pContext->gDBXapi, 0, sizeof(pContext->gDBXapi));
    dlclose(pContext->gDBXLibHandler);
    pContext->gDBXLibHandler = NULL;
    return -EINVAL;
}

int unload_DBX_lib(DBXContext *pContext)
{
    memset(&pContext->gDBXapi, 0, sizeof(pContext->gDBXapi));
    if (pContext->gDBXLibHandler) {
        dlclose(pContext->gDBXLibHandler);
        pContext->gDBXLibHandler = NULL;
    }
    return 0;
}

int DBX_init(DBXContext *pContext)
{
    DBXdata *data = &pContext->gDBXdata;

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

    pContext->aiLeftA  = (int32_t *)malloc(sizeof(int32_t) * BUFFSIZE * 4);
    pContext->aiLeftB  = (int32_t *)malloc(sizeof(int32_t) * BUFFSIZE * 4);
    pContext->aiRightA = (int32_t *)malloc(sizeof(int32_t) * BUFFSIZE * 4);
    pContext->aiRightB = (int32_t *)malloc(sizeof(int32_t) * BUFFSIZE * 4);

    if (pContext->aiLeftA == NULL || pContext->aiLeftB == NULL ||
            pContext->aiRightA == NULL || pContext->aiRightB == NULL) {
        ALOGE("Malloc temp buffer failed !\n");
        return -EINVAL;
    }

    if (pContext->gDBXLibHandler) {
        (*pContext->gDBXapi.DBX_init)((void*) data);
    }
    ALOGD("%s: sucessful", __FUNCTION__);

    return 0;
}

int DBX_configure(DBXContext *pContext, effect_config_t *pConfig)
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

int DBX_getParameter(DBXContext *pContext, void *pParam, size_t *pValueSize,void *pValue) {
    int32_t param = *(int32_t *)pParam;
    int32_t value;
    DBXdata *data = &pContext->gDBXdata;
    DBXmode_8bit cfg_8bit;
    switch (param) {
        case DBX_PARAM_ENABLE:
            if (*pValueSize < sizeof(int32_t)) {
                *pValueSize = 0;
                return -EINVAL;
            }
            value = data->enable;
            *(int32_t *) pValue = value;
            ALOGD("%s: Get status -> %s", __FUNCTION__, DBXStatusstr[value]);
            break;
        case DBX_SET_MODE:
            if (*pValueSize < sizeof(DBXmode)) {
                *pValueSize = 0;
                return -EINVAL;
            }
            cfg_8bit.son_mode = (signed char)data->mode.son_mode;
            cfg_8bit.sur_mode = (signed char)data->mode.sur_mode;
            cfg_8bit.vol_mode = (signed char)data->mode.vol_mode;
            *(DBXmode_8bit *) pValue = cfg_8bit;
            break;
        default:
            ALOGE("%s: unknown param %d", __FUNCTION__, param);
            return -EINVAL;
    }
    return 0;
}

int DBX_setParameter(DBXContext *pContext, void *pParam, void *pValue) {
    int32_t param = *(int32_t *)pParam;
    int32_t value;
    DBXmode_8bit modecfg_8bit;
    DBXdata *data = &pContext->gDBXdata;

    switch (param) {
        case DBX_PARAM_ENABLE:
            if (!pContext->gDBXLibHandler) {
                return 0;
            }
            value = *(int32_t *)pValue;
            data->enable = value;
            ALOGD("%s: Set status -> %s", __FUNCTION__, DBXStatusstr[value]);
            break;
        case DBX_SET_MODE:
            if (!pContext->gDBXLibHandler) {
                return 0;
            }
            modecfg_8bit = *(DBXmode_8bit *)pValue;
            data->mode.son_mode = (signed int)modecfg_8bit.son_mode;
            data->mode.vol_mode = (signed int)modecfg_8bit.vol_mode;
            data->mode.sur_mode = (signed int)modecfg_8bit.sur_mode;
            ALOGD("son_mode is %d,vol_mode is %d,sur_mode is %d",data->mode.son_mode,data->mode.vol_mode,data->mode.sur_mode);
            if (data->mode.son_mode > 4 || data->mode.vol_mode > 2 || data->mode.sur_mode > 2) {
                ALOGE("parameter exceed the range\n");
                return -EINVAL;
            }
            (*pContext->gDBXapi.DBX_setParameter)(data->mode.son_mode, data->mode.vol_mode, data->mode.sur_mode);
            break;
        default:
            ALOGE("%s: unknown param %08x", __FUNCTION__, param);
            return -EINVAL;
    }
    return 0;
}

int DBX_release(DBXContext *pContext)
{
    if (pContext->aiLeftA != NULL || pContext->aiLeftB != NULL ||
            pContext->aiRightA != NULL || pContext->aiRightB != NULL){
        free(pContext->aiLeftA);
        pContext->aiLeftA = NULL;
        free(pContext->aiLeftB);
        pContext->aiLeftB = NULL;
        free(pContext->aiRightA);
        pContext->aiRightA = NULL;
        free(pContext->aiRightB);
        pContext->aiRightB = NULL;
    }
    if (pContext->gDBXLibHandler) {
       (*pContext->gDBXapi.DBX_release)();
    }
    return 0;
}

int DBX_process(effect_handle_t self, audio_buffer_t *inBuffer, audio_buffer_t *outBuffer)
{
    DBXContext *pContext = (DBXContext *)self;
    if (pContext == NULL)
        return -EINVAL;

    if (inBuffer == NULL || inBuffer->raw == NULL ||
        outBuffer == NULL || outBuffer->raw == NULL ||
        inBuffer->frameCount != outBuffer->frameCount ||
        inBuffer->frameCount == 0)
        return -EINVAL;

    if (pContext->state != DBX_STATE_ACTIVE)
        return -ENODATA;

    int16_t   *in  = (int16_t *)inBuffer->raw;
    int16_t   *out = (int16_t *)outBuffer->raw;
    DBXdata *data = &pContext->gDBXdata;
    if (!data->enable || !pContext->gDBXLibHandler) {
        for (size_t i = 0; i < inBuffer->frameCount; i++) {
            *out++ = *in++;
            *out++ = *in++;
        }
    } else {
        for (size_t i = 0; i < inBuffer->frameCount; i++) {
            pContext->aiLeftA[i] = ((int32_t)in[i*2]) << 16;
            pContext->aiRightA[i] = ((int32_t)in[i*2+1]) << 16;
        }
        (*pContext->gDBXapi.DBX_process)(pContext->aiLeftA, pContext->aiRightA, pContext->aiLeftB,
            pContext->aiRightB, inBuffer->frameCount);
        for (size_t i = 0; i < inBuffer->frameCount; i++) {
            out[i*2] = (int16_t)(pContext->aiLeftB[i] >> 16);
            out[i*2+1] = (int16_t)(pContext->aiRightB[i] >> 16);
        }
   }
    return 0;
}

int DBX_command(effect_handle_t self, uint32_t cmdCode, uint32_t cmdSize,
        void *pCmdData, uint32_t *replySize, void *pReplyData)
{
    DBXContext * pContext = (DBXContext *)self;
    effect_param_t *p;
    int voffset;
    ALOGD("%s: cmd = %u", __FUNCTION__, cmdCode);
    if (pContext == NULL || pContext->state == DBX_STATE_UNINITIALIZED)
        return -EINVAL;
    switch (cmdCode) {
    case EFFECT_CMD_INIT:
        if (pReplyData == NULL || replySize == NULL || *replySize != sizeof(int))
            return -EINVAL;
        *(int *) pReplyData = DBX_init(pContext);
        break;
    case EFFECT_CMD_SET_CONFIG:
        if (pCmdData == NULL || cmdSize != sizeof(effect_config_t) || pReplyData == NULL || replySize == NULL || *replySize != sizeof(int))
            return -EINVAL;
        *(int *) pReplyData = DBX_configure(pContext,(effect_config_t *) pCmdData);
        break;
    case EFFECT_CMD_RESET:
        //SRS_reset(pContext);
        break;
    case EFFECT_CMD_ENABLE:
        if (pReplyData == NULL || replySize == NULL || *replySize != sizeof(int))
            return -EINVAL;
        if (pContext->state != DBX_STATE_INITIALIZED)
            return -ENOSYS;
        pContext->state = DBX_STATE_ACTIVE;
        *(int *)pReplyData = 0;
        break;
    case EFFECT_CMD_DISABLE:
        if (pReplyData == NULL || replySize == NULL || *replySize != sizeof(int))
            return -EINVAL;
        if (pContext->state != DBX_STATE_ACTIVE)
            return -ENOSYS;
        pContext->state = DBX_STATE_INITIALIZED;
        *(int *)pReplyData = 0;
        break;
    case EFFECT_CMD_GET_PARAM:
        if (pCmdData == NULL ||
            cmdSize != (int)(sizeof(effect_param_t) + sizeof(uint32_t)+sizeof(uint32_t)) ||
            pReplyData == NULL || replySize == NULL ||
            (*replySize < (int)(sizeof(effect_param_t) + sizeof(uint32_t) + sizeof(uint32_t)) &&
            *replySize < (int)(sizeof(effect_param_t) + sizeof(uint32_t) + sizeof(DBXmode_8bit_s))))
            return -EINVAL;
        p = (effect_param_t *)pCmdData;
        memcpy(pReplyData, pCmdData, sizeof(effect_param_t) + p->psize);
        p = (effect_param_t *)pReplyData;

        voffset = ((p->psize - 1) / sizeof(int32_t) + 1) * sizeof(int32_t);

        p->status = DBX_getParameter(pContext, p->data, (size_t  *)&p->vsize, p->data + voffset);
        *replySize = sizeof(effect_param_t) + voffset + p->vsize;
        break;
    case EFFECT_CMD_SET_PARAM:
        if (pCmdData == NULL ||
            (cmdSize != (int)(sizeof(effect_param_t) + sizeof(uint32_t) + sizeof(uint32_t)) &&
            cmdSize != (int)(sizeof(effect_param_t) + sizeof(uint32_t) + sizeof(DBXmode_8bit_s)))||
            pReplyData == NULL || replySize == NULL || *replySize != sizeof(int32_t))
            return -EINVAL;
        p = (effect_param_t *)pCmdData;
        if (p->psize != sizeof(uint32_t) && p->vsize != sizeof(DBXmode_8bit_s)) {
            *(int32_t *)pReplyData = -EINVAL;
            break;
        }
        *(int *)pReplyData = DBX_setParameter(pContext, (void *)p->data, p->data + p->psize);
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

int DBX_getDescriptor(effect_handle_t self, effect_descriptor_t *pDescriptor)
{
    DBXContext * pContext = (DBXContext *) self;

    if (pContext == NULL || pDescriptor == NULL) {
        ALOGE("%s: invalid param", __FUNCTION__);
        return -EINVAL;
    }

    *pDescriptor = DBXDescriptor;

    return 0;
}
//-------------------- Effect Library Interface Implementation------------------------

int DBXLib_Create(const effect_uuid_t *uuid, int32_t sessionId __unused, int32_t ioId __unused, effect_handle_t *pHandle)
{
    if (pHandle == NULL || uuid == NULL)
        return -EINVAL;

    if (memcmp(uuid, &DBXDescriptor.uuid, sizeof(effect_uuid_t)) != 0)
        return -EINVAL;

    DBXContext *pContext = new DBXContext;
    if (!pContext) {
        ALOGE("%s: alloc DBXContext failed", __FUNCTION__);
        return -EINVAL;
    }
    memset(pContext, 0, sizeof(DBXContext));

    if (DBX_load_ini_file(pContext) < 0) {
        ALOGE("%s: Load INI File faied, use default param", __FUNCTION__);
        pContext->gDBXdata.enable = 1;
    }

    if (DBX_load_lib(pContext) < 0) {
        ALOGE("%s: Load Library File faied", __FUNCTION__);
    }

    pContext->itfe = &DBXInterface;
    pContext->state = DBX_STATE_UNINITIALIZED;

    *pHandle = (effect_handle_t)pContext;

    pContext->state = DBX_STATE_INITIALIZED;

    ALOGD("%s: %p", __FUNCTION__, pContext);

    return 0;
}

int DBXLib_Release(effect_handle_t handle)
{
    DBXContext * pContext = (DBXContext *)handle;
    if (pContext == NULL)
        return -EINVAL;

    DBX_release(pContext);
    unload_DBX_lib(pContext);
    pContext->state = DBX_STATE_UNINITIALIZED;

    delete pContext;
    ALOGD("DBXLib_Release");
    return 0;
}

int DBXLib_GetDescriptor(const effect_uuid_t *uuid, effect_descriptor_t *pDescriptor)
{
    if (pDescriptor == NULL || uuid == NULL) {
        ALOGE("%s: called with NULL pointer", __FUNCTION__);
        return -EINVAL;
    }
    if (memcmp(uuid, &DBXDescriptor.uuid, sizeof(effect_uuid_t)) == 0) {
        *pDescriptor = DBXDescriptor;
        return 0;
    }
    return  -EINVAL;
}

// effect_handle_t interface implementation for DBX effect
const struct effect_interface_s DBXInterface = {
        DBX_process,
        DBX_command,
        DBX_getDescriptor,
        NULL,
};

audio_effect_library_t AUDIO_EFFECT_LIBRARY_INFO_SYM = {
    .tag = AUDIO_EFFECT_LIBRARY_TAG,
    .version = EFFECT_LIBRARY_API_VERSION,
    .name = "DBX",
    .implementor = "THAT Corporation",
    .create_effect = DBXLib_Create,
    .release_effect = DBXLib_Release,
    .get_descriptor = DBXLib_GetDescriptor,
};

};//extern c