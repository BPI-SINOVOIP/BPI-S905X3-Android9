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

#define LOG_TAG "Virtualx_Effect"
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
#include <cutils/properties.h>
#include <unistd.h>
#include "IniParser.h"
#include "Virtualx.h"
#include <pthread.h>

extern "C" {

#define MODEL_SUM_DEFAULT_PATH "/vendor/etc/tvconfig/model/model_sum.ini"
#define AUDIO_EFFECT_DEFAULT_PATH "/vendor/etc/tvconfig/audio/AMLOGIC_AUDIO_EFFECT_DEFAULT.ini"
#define DTS_VIRTUALX_FRAME_SIZE 256
#define FXP32(val, x) (int32_t)(val * ((int64_t)1L << (32 - x)))
#define FXP16( val, x ) ( int32_t )( val * ( 1L << ( 16 - x ) ) )

#if defined(__LP64__)
#define LIBVX_PATH_A "/vendor/lib64/soundfx/libvx.so"
#else
#define LIBVX_PATH_A "/vendor/lib/soundfx/libvx.so"
#endif

// effect_handle_t interface implementation for virtualx effect
extern const struct effect_interface_s VirtualxInterface;

// VX effect TYPE: 5112a99e-b8b9-4c5e-91fd-a804d29c36b2
// VX effect UUID: 61821587-ce3c-4aac-9122-86d874ea1fb1

const effect_descriptor_t VirtualxDescriptor = {
        {0x5112a99e, 0xb8b9, 0x4c5e, 0x91fd, {0xa8, 0x04, 0xd2, 0x9c, 0x36, 0xb2}}, // type
        {0x61821587, 0xce3c, 0x4aac, 0x9122, {0x86, 0xd8, 0x74, 0xea, 0x1f, 0xb1}}, // uuid
        EFFECT_CONTROL_API_VERSION,
        EFFECT_FLAG_TYPE_POST_PROC | EFFECT_FLAG_DEVICE_IND | EFFECT_FLAG_NO_PROCESS | EFFECT_FLAG_OFFLOAD_SUPPORTED,
        VX_CUP_LOAD_ARM9E,
        VX_MEM_USAGE,
        "VirtualX",
        "DTS Labs",
};

enum Virtualx_state_e {
    VIRTUALX_STATE_UNINITIALIZED,
    VIRTUALX_STATE_INITIALIZED,
    VIRTUALX_STATE_ACTIVE,
};

typedef struct Virtualxapi_s {
    int (*VX_init)(void *);
    int (*VX_release)(void);
    int (*VX_process)(int32_t **in,int32_t **out);
    int (*Truvolume_process)(int32_t **in,int32_t **out);
    int (*MBHL_process)(int32_t **in,int32_t **out);
    int (*VX_reset)(void);
    int (*setvxlib1_enable)(int32_t value);
    int (*getvxlib1_enable)(int32_t *value);
    int (*setvxlib1_inmode)(int32_t value);
    int (*getvxlib1_inmode)(int32_t *value);
    int (*setvxlib1_outmode)(int32_t value);
    int (*getvxlib1_outmode)(int32_t *value);
    int (*setvxlib1_heardroomgain)(int32_t value);
    int (*getvxlib1_heardroomgain)(int32_t *value);
    int (*setvxlib1_procoutgain)(int32_t value);
    int (*getvxlib1_procoutgain)(int32_t *value);
    int (*settsx_enable)(int32_t value);
    int (*gettsx_enable)(int32_t *value);
    int (*settsx_pssvmtrxenable)(int32_t value);
    int (*gettsx_pssvmtrxenable)(int32_t *value);
    int (*settsx_horizontctl)(int32_t value);
    int (*gettsx_horizontctl)(int32_t *value);
    int (*settsx_frntctrl)(int32_t value);
    int (*gettsx_frntctrl)(int32_t *value);
    int (*settsx_surroundctrl)(int32_t value);
    int (*gettsx_surroundctrl)(int32_t *value);
    int (*settsx_lprgain)(int32_t value);
    int (*gettsx_lprgain)(int32_t *value);
    int (*settsx_heightmixcoeff)(int32_t value);
    int (*gettsx_heightmixcoeff)(int32_t *value);
    int (*settsx_centergain)(int32_t value);
    int (*gettsx_centergain)(int32_t *value);
    int (*settsx_heightdiscards)(int32_t value);
    int (*gettsx_heightdiscards)(int32_t *value);
    int (*settsx_discard)(int32_t value);
    int (*gettsx_discard)(int32_t *value);
    int (*settsx_hghtupmixenable)(int32_t value);
    int (*gettsx_hghtupmixenable)(int32_t *value);
    int (*settsx_dcenable)(int32_t value);
    int (*gettsx_dcenable)(int32_t *value);
    int (*settsx_dccontrol)(int32_t value);
    int (*gettsx_dccontrol)(int32_t *value);
    int (*settsx_defenable)(int32_t value);
    int (*gettsx_defenable)(int32_t *value);
    int (*settsx_defcontrol)(int32_t value);
    int (*gettsx_defcontrol)(int32_t *value);
    int (*settbhdx_enable)(int32_t value);
    int (*gettbhdx_enable)(int32_t *value);
    int (*settbhdx_monomode)(int32_t value);
    int (*gettbhdx_monomode)(int32_t *value);
    int (*settbhdx_spksize)(int32_t value);
    int (*gettbhdx_spksize)(int32_t *value);
    int (*settbhdx_tempgain)(int32_t value);
    int (*gettbhdx_tempgain)(int32_t *value);
    int (*settbhdx_maxgain)(int32_t value);
    int (*gettbhdx_maxgain)(int32_t *value);
    int (*settbhdx_hporder)(int32_t value);
    int (*gettbhdx_hporder)(int32_t *value);
    int (*settbhdx_hpenable)(int32_t value);
    int (*gettbhdx_hpenable)(int32_t *value);
    int (*settbhdx_processdiscard)(int32_t value);
    int (*getthbdx_processdiscard)(int32_t *value);
    int (*setmbhl_processdiscard)(int32_t value);
    int (*getmbhl_processdiscard)(int32_t *value);
    int (*setmbhl_enable)(int32_t value);
    int (*getmbhl_enable)(int32_t *value);
    int (*setmbhl_bypassgain)(int32_t value);
    int (*getmbhl_bypassgain)(int32_t *value);
    int (*setmbhl_reflevel)(int32_t value);
    int (*getmbhl_reflevel)(int32_t *value);
    int (*setmbhl_volume)(int32_t value);
    int (*getmbhl_volume)(int32_t *value);
    int (*setmbhl_volumestep)(int32_t value);
    int (*getmbhl_volumestep)(int32_t *value);
    int (*setmbhl_balancestep)(int32_t value);
    int (*getmbhl_balancestep)(int32_t *value);
    int (*setmbhl_outputgain)(int32_t value);
    int (*getmbhl_outputgain)(int32_t *value);
    int (*setmbhl_boost)(int32_t value);
    int (*getmbhl_boost)(int32_t *value);
    int (*setmbhl_threshold)(int32_t value);
    int (*getmbhl_threshold)(int32_t *value);
    int (*setmbhl_slowoffset)(int32_t value);
    int (*getmbhl_slowoffset)(int32_t *value);
    int (*setmbhl_fastattack)(int32_t value);
    int (*getmbhl_fastattack)(int32_t *value);
    int (*setmbhl_fastrelease)(int32_t value);
    int (*getmbhl_fastrelease)(int32_t *value);
    int (*setmbhl_slowattrack)(int32_t value);
    int (*getmbhl_slowattrack)(int32_t *value);
    int (*setmbhl_slowrelease)(int32_t value);
    int (*getmbhl_slowrelease)(int32_t *value);
    int (*setmbhl_delay)(int32_t value);
    int (*getmbhl_delay)(int32_t *value);
    int (*setmbhl_envelopefre)(int32_t value);
    int (*getmbhl_envelopefre)(int32_t *value);
    int (*setmbhl_mode)(int32_t value);
    int (*getmbhl_mode)(int32_t *value);
    int (*setmbhl_lowcross)(int32_t value);
    int (*getmbhl_lowcross)(int32_t *value);
    int (*setmbhl_crossmid)(int32_t value);
    int (*getmbhl_crossmid)(int32_t *value);
    int (*setmbhl_compattrack)(int32_t value);
    int (*getmbhl_compattrack)(int32_t *value);
    int (*setmbhl_lowrelease)(int32_t value);
    int (*getmbhl_lowrelease)(int32_t *value);
    int (*setmbhl_complowratio)(int32_t value);
    int (*getmbhl_complowratio)(int32_t *value);
    int (*setmbhl_complowthresh)(int32_t value);
    int (*getmbhl_complowthresh)(int32_t *value);
    int (*setmbhl_lowmakeup)(int32_t value);
    int (*getmbhl_lowmakeup)(int32_t *value);
    int (*setmbhl_compmidrelease)(int32_t value);
    int (*getmbhl_compmidrelease)(int32_t *value);
    int (*setmbhl_midratio)(int32_t value);
    int (*getmbhl_midratio)(int32_t *value);
    int (*setmbhl_compmidthresh)(int32_t value);
    int (*getmbhl_compmidthresh)(int32_t *value);
    int (*setmbhl_compmidmakeup)(int32_t value);
    int (*getmbhl_compmidmakeup)(int32_t *value);
    int (*setmbhl_comphighrelease)(int32_t value);
    int (*getmbhl_comphighrelease)(int32_t *value);
    int (*setmbhl_comphighratio)(int32_t value);
    int (*getmbhl_comphighratio)(int32_t *value);
    int (*setmbhl_comphighthresh)(int32_t value);
    int (*getmbhl_comphighthresh)(int32_t *value);
    int (*setmbhl_comphighmakeup)(int32_t value);
    int (*getmbhl_comphighmakeup)(int32_t *value);
    int (*settruvolume_ctren)(int32_t value);
    int (*gettruvolume_ctren)(int32_t *value);
    int (*settruvolume_ctrtarget)(int32_t value);
    int (*gettruvolume_ctrtarget)(int32_t *value);
    int (*settruvolume_ctrpreset)(int32_t value);
    int (*gettruvolume_ctrpreset)(int32_t *value);
    int (*setlc_inmode)(int32_t value);
    int (*settbhd_FilterDesign)(int32_t spksize,float basslvl, float hpratio,float extbass);
    int (*setmbhl_FilterDesign)(float lowCrossFreq,float midCrossFreq);
    int (*seteq_enable)(int32_t value);
    int (*geteq_enable)(int32_t *value);
    int (*seteq_discard)(int32_t value);
    int (*seteq_inputG)(int32_t value);
    int (*geteq_inputG)(int32_t *value);
    int (*seteq_outputG)(int32_t value);
    int (*geteq_outputG)(int32_t *value);
    int (*seteq_bypassG)(int32_t value);
    int (*geteq_bypassG)(int32_t *value);
    int (*seteq_band)(int32_t value,void *pValue);
} Virtualxapi;

typedef struct vxlib_param_s {
    int32_t    enable;
    int32_t    input_mode;
    int32_t    output_mode;
    float      headroom_gain;
    float      output_gain;
} vxlib_param;

typedef struct vxtrusurround_param_s {
    int32_t    enable;
    int32_t    upmixer_enable;//passive matrix upmixer enable
    int32_t    horizontaleffect_control;
    float      frontwidening_control;
    float      surroundwidening_control;
    float      phantomcenter_mixlevel;
    float      heightmix_coeff;
    float      center_gain;
    int32_t    height_discard;
    int32_t    discard;//process discard
    int32_t    heightupmix_enable;
} vxtrusurround_param;

typedef struct vxdialogclarity_param_s {
    int32_t    enable;
    float      level;
} vxdialogclarity_param;

typedef struct vxdefinition_param_s {
    int32_t    enable;
    float      level;
} vxdefinition_param;

typedef struct aeq_param_s {
    int32_t    aeq_enable;
    int32_t    aeq_discard;
    float      aeq_inputgain;
    float      aeq_outputgain;
    float      aeq_bypassgain;
    int32_t    aeq_bandnum;
    int32_t    aeq_fc[12];
} aeq_param;

typedef struct tbhdx_param_s {
    int32_t    enable;
    int32_t    monomode_enable;
    int32_t    speaker_size;
    float      tmporal_gain;
    float      max_gain;
    int32_t    hpfo;//high pass filter order
    int32_t    highpass_enble;
} tbhdx_param;

typedef struct mbhl_param_s {
    int32_t    mbhl_enable;
    float      mbhl_bypassgain;
    float      mbhl_reflevel;
    float      mbhl_volume;
    int32_t    mbhl_volumesttep;
    int32_t    mbhl_balancestep;
    float      mbhl_outputgain;
    float      mbhl_boost;
    float      mbhl_threshold;
    float      mbhl_slowoffset;
    float      mbhl_fastattack;
    int32_t    mbhl_fastrelease;
    int32_t    mbhl_slowattack;
    int32_t    mbhl_slowrelease;
    int32_t    mbhl_delay;
    int32_t    mbhl_envelopefre;
    int32_t    mbhl_mode;
    int32_t    mbhl_lowcross;
    int32_t    mbhl_midcross;
    int32_t    mbhl_compressorat;//Compressor Attack Time
    int32_t    mbhl_compressorlrt;// Compressorlow Release Time
    float      mbhl_compressolr;//Compressor low ratio
    float      mbhl_compressolt;//Compressor low Threshold
    float      mbhl_makeupgain;//Compressor Low Makeup Gain
    int32_t    mbhl_midreleasetime;//Compressor Mid release
    float      mbhl_midratio;//Compressor mid ratio
    float      mbhl_midthreshold;//Compressor mid threshold
    float      mbhl_midmakeupgain;//Compressor mid makeupgain
    int32_t    mbhl_highreleasetime;//Compressor high release time
    float      mbhl_highratio;//Compressor high ratio
    float      mbhl_highthreshold;//Compressor high threshold
    float      mbhl_highmakegian;//Compressor high makeupgain
} mbhl_param;

typedef struct Truvolume_param_s {
    int32_t    enable;
    int32_t    targetlevel;
    int32_t    preset;
} Truvolume_param;

typedef struct TS_cfg_s {
    vxtrusurround_param       vxtrusurround;
    vxdefinition_param        vxdefinition;
    tbhdx_param               tbhdx;
} TS_cfg;

static TS_cfg default_TS_cfg {
    {1,1,0,1.0,1.0,1.0,1.0,1.0,0,0,1},{0,0.4},{0,0,2,0.3339,0.3,4,1},
};
typedef struct DC_cfg_s {
    vxdialogclarity_param     vxdialogclarity;
} DC_cfg;

static DC_cfg default_DC_cfg {
    0,0.4,
};

#define TS_MODE_MUM     2
#define DC_MODE_MUM     3

typedef struct Virtualxcfg_s {
    vxlib_param               vxlib;
    mbhl_param                mbhl;
    Truvolume_param           Truvolume;
    aeq_param                 eqparam;
} Virtualxcfg;

static Virtualxcfg default_Virtualxcfg {
    {1,0,0,1.0,1.0},{1,1.0,1.0,1.0,100,0,1.0,1.0,1.0,1.0,5,50,500,500,8,20,0,7,15,5,250,4.0,0.501,1.0,250,4.0,0.501,1.0,250,4.0,0.501,1.0,},
    {1,-24,0},{1,0,1.0,1.0,1.0,5,{100,300,1000,3000,10000}},
};

typedef struct vxdata_s {
    struct {
        int32_t    vxlib_enable;
        int32_t    input_mode;
        int32_t    output_mode;
        int32_t    headroom_gain;
        int32_t    output_gain;
        int32_t    vxtrusurround_enable;
        int32_t    upmixer_enable;
        int32_t    horizontaleffect_control;
        int32_t    frontwidening_control;
        int32_t    surroundwidening_control;
        int32_t    phantomcenter_mixlevel;
        int32_t    heightmix_coeff;
        int32_t    center_gain;
        int32_t    height_discard;
        int32_t    discard;
        int32_t    heightupmix_enable;
        int32_t    vxdialogclarity_enable;
        int32_t    vxdialogclarity_level;
        int32_t    vxdefinition_enable;
        int32_t    vxdefinition_level;
        int32_t    tbhdx_enable;
        int32_t    monomode_enable;
        int32_t    speaker_size;
        int32_t    tmporal_gain;
        int32_t    max_gain;
        int32_t    hpfo;//high pass filter order
        int32_t    highpass_enble;
        int32_t    tbhdx_processdiscard;
        int32_t    mbhl_enable;
        int32_t    mbhl_bypassgain;
        int32_t    mbhl_reflevel;
        int32_t    mbhl_volume;
        int32_t    mbhl_volumesttep;
        int32_t    mbhl_balancestep;
        int32_t    mbhl_outputgain;
        int32_t    mbhl_boost;
        int32_t    mbhl_threshold;
        int32_t    mbhl_slowoffset;
        int32_t    mbhl_fastattack;
        int32_t    mbhl_fastrelease;
        int32_t    mbhl_slowattack;
        int32_t    mbhl_slowrelease;
        int32_t    mbhl_delay;
        int32_t    mbhl_envelopefre;
        int32_t    mbhl_mode;
        int32_t    mbhl_lowcross;
        int32_t    mbhl_midcross;
        int32_t    mbhl_compressorat;//Compressor Attack Time
        int32_t    mbhl_compressorlrt;// Compressorlow Release Time
        int32_t    mbhl_compressolr;//Compressor low ratio
        int32_t    mbhl_compressolt;//Compressor low Threshold
        int32_t    mbhl_makeupgain;//Compressor Low Makeup Gain
        int32_t    mbhl_midreleasetime;//Compressor Mid release
        int32_t    mbhl_midratio;//Compressor mid ratio
        int32_t    mbhl_midthreshold;//Compressor mid threshold
        int32_t    mbhl_midmakeupgain;//Compressor mid makeupgain
        int32_t    mbhl_highreleasetime;//Compressor high release time
        int32_t    mbhl_highratio;//Compressor high ratio
        int32_t    mbhl_highthreshold;//Compressor high threshold
        int32_t    mbhl_highmakegian;//Compressor high makeupgain
        int32_t    mbhl_processdiscard;
        int32_t    Truvolume_enable;
        int32_t    targetlevel;
        int32_t    preset;
        int32_t    aeq_enable;
        int32_t    aeq_discard;
        int32_t    aeq_inputgain;
        int32_t    aeq_outputgain;
        int32_t    aeq_bypassgain;
        int32_t    aeq_bandnum;
        int32_t    aeq_fc[12];
    };
    int            enable;
    Virtualxcfg    vxcfg;
    TS_cfg         TS_usr_cfg[TS_MODE_MUM];
    DC_cfg         DC_usr_cfg[DC_MODE_MUM];
    int            dialogclarity_mode;
    int            surround_mode;
} vxdata;

typedef struct vxContext_s {
    const struct effect_interface_s *itfe;
    effect_config_t                 config;
    Virtualx_state_e                state;
    void                            *gVXLibHandler;
    vxdata                          gvxdata;
    Virtualxapi                     gVirtualxapi;
    int32_t                         sTempBuffer[12][256];
    int32_t                         TempBuffer[12][256];
    int32_t                         *ppMappedInCh[12];
    int32_t                         *ppMappedOutCh[12];
    float                           lowcrossfreq;
    float                           midcrossfreq;
    int32_t                         spksize;
    float                           hpratio;
    float                           extbass;
    int32_t                         ch_num;
} vxContext;

const char *VXStatusstr[] = {"Disable", "Enable"};
const char *VXDialogClarityModestr[DC_MODE_MUM] = {"OFF", "LOW", "HIGH"};
const char *VXSurroundModestr[TS_MODE_MUM] = {"ON", "OFF"};

int Virtualx_get_model_name(char *model_name, int size)
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
/*
static int getprop_bool(const char *path)
{
    char buf[PROPERTY_VALUE_MAX];
    int ret = -1;

    ret = property_get(path, buf, NULL);
    if (ret > 0) {
        if (strcasecmp(buf, "true") == 0 || strcmp(buf, "1") == 0) {
            return 1;
        }
    }
    return 0;
}
*/
int Virtualx_get_ini_file(char *ini_name, int size)
{
    int result = -1;
    char model_name[50] = {0};
    IniParser* pIniParser = NULL;
    const char *ini_value = NULL;
    const char *filename = MODEL_SUM_DEFAULT_PATH;

    Virtualx_get_model_name(model_name, sizeof(model_name));
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

int Virtualx_parse_dialogclarity(DC_cfg *DC_parm, const char *buffer)
{
    char *Rch = (char *)buffer;

    Rch = strtok(Rch, ",");
    if (Rch == NULL)
        goto error;
    DC_parm->vxdialogclarity.enable = atoi(Rch);
    //ALOGD("%s: Dialog Clarity enable = %d", __FUNCTION__, DC_parm->vxdialogclarity.enable);
    Rch = strtok(NULL, ",");
    if (Rch == NULL)
        goto error;
    DC_parm->vxdialogclarity.level = atof(Rch);
    //ALOGD("%s: Dialog Clarity Gain = %f", __FUNCTION__, DC_parm->vxdialogclarity.level);
    return 0;
error:
    ALOGE("%s: Dialog Clarity Parse failed", __FUNCTION__);
    return -1;
}

int Virtualx_parse_surround(TS_cfg *TS_param, const char *buffer)
{
    char *Rch = (char *)buffer;
    Rch = strtok(Rch, ",");
    if (Rch == NULL) {
        goto error;
    }
    TS_param->vxtrusurround.enable = atoi(Rch);
    //ALOGD("vxtrusurround.enable is %d",TS_param->vxtrusurround.enable);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
        goto error;
    }
    TS_param->vxtrusurround.upmixer_enable = atoi(Rch);
    //ALOGD("vxtrusurround.upmixer_enable is %d",TS_param->vxtrusurround.upmixer_enable);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
        goto error;
    }
    TS_param->vxtrusurround.horizontaleffect_control = atoi(Rch);
    //ALOGD("horizontaleffect_control is %d",data->vxcfg.vxtrusurround.horizontaleffect_control);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
        goto error;
    }
    TS_param->vxtrusurround.frontwidening_control = atof(Rch);
    //ALOGD("frontwidening_control is %d",data->vxcfg.vxtrusurround.frontwidening_control);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
        goto error;
    }
    TS_param->vxtrusurround.surroundwidening_control = atof(Rch);
    //ALOGD("surroundwidening_control is %d",data->vxcfg.vxtrusurround.surroundwidening_control);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
        goto error;
    }
    TS_param->vxtrusurround.phantomcenter_mixlevel = atof(Rch);
    //ALOGD("phantomcenter_mixlevel is %d",data->vxcfg.vxtrusurround.phantomcenter_mixlevel);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
        goto error;
    }
    TS_param->vxtrusurround.heightmix_coeff = atof(Rch);
    //ALOGD("heightmix_coeff is %d",data->vxcfg.vxtrusurround.heightmix_coeff);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
        goto error;
    }
    TS_param->vxtrusurround.center_gain = atof(Rch);
    //ALOGD("center_gain is %f",TS_param->vxtrusurround.center_gain);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
        goto error;
    }
    TS_param->vxtrusurround.height_discard = atoi(Rch);
    //ALOGD("height_discard is %d",data->vxcfg.vxtrusurround.height_discard);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
        goto error;
    }
    TS_param->vxtrusurround.discard = atoi(Rch);
    //ALOGD("discard is %d",data->vxcfg.vxtrusurround.discard);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
        goto error;
    }
    TS_param->vxtrusurround.heightupmix_enable = atoi(Rch);
    //ALOGD("heightupmix_enable is %d",data->vxcfg.vxtrusurround.heightupmix_enable);
    //definition parse
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
        goto error;
    }
    TS_param->vxdefinition.enable = atoi(Rch);
    //ALOGD("vxdefinition.enable is %d",TS_param->vxdefinition.enable);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
        goto error;
    }
    TS_param->vxdefinition.level = atof(Rch);
    //ALOGD("vxdefinition.level is %d",data->vxcfg.vxdefinition.level);
    //Trubass HDX parse
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
        goto error;
    }
    TS_param->tbhdx.enable = atoi(Rch);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
        goto error;
    }
    TS_param->tbhdx.monomode_enable = atoi(Rch);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
        goto error;
    }
    TS_param->tbhdx.speaker_size = atoi(Rch);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
        goto error;
    }
    TS_param->tbhdx.tmporal_gain = atof(Rch);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
        goto error;
    }
    TS_param->tbhdx.max_gain = atof(Rch);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
        goto error;
    }
    TS_param->tbhdx.hpfo = atoi(Rch);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
        goto error;
    }
    TS_param->tbhdx.highpass_enble = atoi(Rch);
    return 0;
error:
    ALOGE("%s: TruSurround Parse failed", __FUNCTION__);
    return -1;
}

int AEQ_parse_fc_config(vxContext *pContext, int bandnum, const char *buffer)
{
    int i;
    char *Rch = (char *)buffer;
    vxdata *data = &pContext->gvxdata;

    for (i = 0; i < bandnum; i++) {
        if (i == 0)
            Rch = strtok(Rch, ",");
        else
            Rch = strtok(NULL, ",");
        if (Rch == NULL) {
            ALOGE("%s: Config Parse failed, using default config", __FUNCTION__);
            return -1;
        }
        data->vxcfg.eqparam.aeq_fc[i] = atoi(Rch);
        ALOGD("data->vxcfg.eqparam.aeq_fc[%d] is %d",i,data->vxcfg.eqparam.aeq_fc[i]);
    }
    return 0;
}

int Virtualx_load_ini_file(vxContext *pContext)
{
    int result = -1;
    char *Rch = NULL;
    char ini_name[100] = {0};
    const char *ini_value = NULL;
    vxdata *data = &pContext->gvxdata;
    IniParser* pIniParser = NULL;
    if (Virtualx_get_ini_file(ini_name, sizeof(ini_name)) < 0)
        goto error;
    pIniParser = new IniParser();
    if (pIniParser->parse((const char *)ini_name) < 0) {
        ALOGD("%s: %s load failed", __FUNCTION__, ini_name);
        goto error;
    }
    ini_value = pIniParser->GetString("Virtualx", "enable", "1");
    if (ini_value == NULL)
        goto error;
    //ALOGD("%s: enable -> %s", __FUNCTION__, ini_value);
    data->enable = atoi(ini_value);
    if (data->enable == 0) {
        property_set("media.libplayer.dtsMulChPcm","false");
    }
     //virtuallibx parse
    ini_value =  pIniParser->GetString("Virtualx", "virtuallibx", "NULL");
    if (ini_value == NULL)
        goto error;
    Rch = (char *)ini_value;
    Rch = strtok(Rch, ",");
    if (Rch == NULL) {
         goto error;
    }
    data->vxcfg.vxlib.enable =  atoi(Rch);
    //ALOGD("vxlib.enable is %d",data->vxcfg.vxlib.enable);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
        goto error;
    }
    data->vxcfg.vxlib.input_mode = atoi(Rch);
    //ALOGD("input mode is %d",data->vxcfg.vxlib.input_mode);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
        goto error;
    }
    data->vxcfg.vxlib.output_mode = atoi(Rch);
    // ALOGD("vxlib.output_mode is %d",data->vxcfg.vxlib.output_mode);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
        goto error;
    }
    data->vxcfg.vxlib.headroom_gain = atof(Rch);
    //ALOGD("vxlib.headroom_gain is %f",data->vxcfg.vxlib.headroom_gain);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
        goto error;
    }
    data->vxcfg.vxlib.output_gain = atof(Rch);
    //ALOGD("vxlib.output_gain is %d",data->vxcfg.vxlib.output_gain);

    //trusurroundx parse dialogClarity parse Trubass HDX parse
    ini_value =  pIniParser->GetString("Virtualx", "surround_mode_on", "NULL");
    if (ini_value == NULL)
        goto error;
    if (Virtualx_parse_surround(&data->TS_usr_cfg[0],ini_value) < 0)
        goto error;

    ini_value =  pIniParser->GetString("Virtualx", "surround_mode_off", "NULL");
    if (ini_value == NULL)
        goto error;
    if (Virtualx_parse_surround(&data->TS_usr_cfg[1],ini_value) < 0)
        goto error;
    //dialog clarity parse
    ini_value =  pIniParser->GetString("Virtualx", "dialogclarity_mode_off", "NULL");
    if (ini_value == NULL)
        goto error;
    if (Virtualx_parse_dialogclarity(&data->DC_usr_cfg[0],ini_value) < 0)
        goto error;

    ini_value =  pIniParser->GetString("Virtualx", "dialogclarity_mode_low", "NULL");
    if (ini_value == NULL)
        goto error;
    if (Virtualx_parse_dialogclarity(&data->DC_usr_cfg[1],ini_value) < 0)
        goto error;

    ini_value =  pIniParser->GetString("Virtualx", "dialogclarity_mode_high", "NULL");
    if (ini_value == NULL)
        goto error;
    if (Virtualx_parse_dialogclarity(&data->DC_usr_cfg[2],ini_value) < 0)
        goto error;

    //mbhl parse
    ini_value =  pIniParser->GetString("Virtualx", "mbhl", "NULL");
    if (ini_value == NULL)
        goto error;
    Rch = (char *)ini_value;
    Rch = strtok(Rch, ",");
    if (Rch == NULL) {
        goto error;
    }
    data->vxcfg.mbhl.mbhl_enable = atoi(Rch);
    //ALOGD("mbhl_enable is %d",data->vxcfg.mbhl.mbhl_enable);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
         goto error;
    }
    data->vxcfg.mbhl.mbhl_bypassgain = atof(Rch);
    //ALOGD("mbhl_bypassgain is %d",data->vxcfg.mbhl.mbhl_bypassgain);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
         goto error;
    }
    data->vxcfg.mbhl.mbhl_reflevel = atof(Rch);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
         goto error;
    }
    data->vxcfg.mbhl.mbhl_volume = atof(Rch);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
         goto error;
    }
    data->vxcfg.mbhl.mbhl_volumesttep = atoi(Rch);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
         goto error;
    }
    data->vxcfg.mbhl.mbhl_balancestep = atoi(Rch);
    //ALOGD("mbhl_balancestep is %d",data->vxcfg.mbhl.mbhl_balancestep);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
         goto error;
    }
    data->vxcfg.mbhl.mbhl_outputgain = atof(Rch);
    //ALOGD("mbhl_outputgain is %d",data->vxcfg.mbhl.mbhl_outputgain);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
         goto error;
    }
    data->vxcfg.mbhl.mbhl_boost = atof(Rch);
    //ALOGD("mbhl_boost is %d",data->vxcfg.mbhl.mbhl_boost);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
         goto error;
    }
    data->vxcfg.mbhl.mbhl_threshold = atof(Rch);
    //ALOGD("mbhl_threshold is %d",data->vxcfg.mbhl.mbhl_threshold);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
         goto error;
    }
    data->vxcfg.mbhl.mbhl_slowoffset = atof(Rch);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
         goto error;
    }
    data->vxcfg.mbhl.mbhl_fastattack = atof(Rch);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
         goto error;
    }
    data->vxcfg.mbhl.mbhl_fastrelease = atoi(Rch);
    //ALOGD("mbhl_fastrelease is %d",data->vxcfg.mbhl.mbhl_fastrelease);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
         goto error;
    }
    data->vxcfg.mbhl.mbhl_slowattack = atoi(Rch);
    //ALOGD("mbhl_slowattack is %d",data->vxcfg.mbhl.mbhl_slowattack);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
         goto error;
    }
    data->vxcfg.mbhl.mbhl_slowrelease = atoi(Rch);
    //ALOGD("mbhl_slowrelease is %d",data->vxcfg.mbhl.mbhl_slowrelease);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
         goto error;
    }
    data->vxcfg.mbhl.mbhl_delay = atoi(Rch);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
         goto error;
    }
    data->vxcfg.mbhl.mbhl_envelopefre = atoi(Rch);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
         goto error;
    }
    data->vxcfg.mbhl.mbhl_mode = atoi(Rch);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
         goto error;
    }
    data->vxcfg.mbhl.mbhl_lowcross = atoi(Rch);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
         goto error;
    }
    data->vxcfg.mbhl.mbhl_midcross = atoi(Rch);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
         goto error;
    }
    data->vxcfg.mbhl.mbhl_compressorat = atoi(Rch);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
         goto error;
    }
    data->vxcfg.mbhl.mbhl_compressorlrt = atoi(Rch);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
         goto error;
    }
    data->vxcfg.mbhl.mbhl_compressolr = atof(Rch);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
         goto error;
    }
    data->vxcfg.mbhl.mbhl_compressolt = atof(Rch);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
         goto error;
    }
    data->vxcfg.mbhl.mbhl_makeupgain = atof(Rch);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
         goto error;
    }
    data->vxcfg.mbhl.mbhl_midreleasetime = atoi(Rch);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
         goto error;
    }
    data->vxcfg.mbhl.mbhl_midratio = atof(Rch);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
         goto error;
    }
    data->vxcfg.mbhl.mbhl_midthreshold = atof(Rch);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
         goto error;
    }
    data->vxcfg.mbhl.mbhl_midmakeupgain = atof(Rch);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
         goto error;
    }
    data->vxcfg.mbhl.mbhl_highreleasetime = atoi(Rch);
    //ALOGD("mbhl_highreleasetime is %d",data->vxcfg.mbhl.mbhl_highreleasetime);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
         goto error;
    }
    data->vxcfg.mbhl.mbhl_highratio = atof(Rch);
    //ALOGD("mbhl_highratio is %d",data->vxcfg.mbhl.mbhl_highratio);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
         goto error;
    }
    data->vxcfg.mbhl.mbhl_highthreshold = atof(Rch);
    //ALOGD("mbhl_highthreshold is %d",data->vxcfg.mbhl.mbhl_highthreshold);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
         goto error;
    }
    data->vxcfg.mbhl.mbhl_highmakegian = atof(Rch);
    //ALOGD("mbhl_highmakegian is %d",data->vxcfg.mbhl.mbhl_highmakegian);

    // truvolume parse
    ini_value =  pIniParser->GetString("Virtualx", "truvolume", "NULL");
    if (ini_value == NULL)
         goto error;
    Rch = (char *)ini_value;
    Rch = strtok(Rch, ",");
    if (Rch == NULL) {
        goto error;
    }
    data->vxcfg.Truvolume.enable = atoi(Rch);
    //ALOGD("Truvolume.enable is %d",data->vxcfg.Truvolume.enable);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
        goto error;
    }
    data->vxcfg.Truvolume.targetlevel = atoi(Rch);
    //ALOGD("Truvolume.targetlevel is %d",data->vxcfg.Truvolume.targetlevel);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
        goto error;
    }
    data->vxcfg.Truvolume.preset = atoi(Rch);
   // ALOGD("Truvolume.preset is %d",data->vxcfg.Truvolume.preset);
    ini_value =  pIniParser->GetString("Virtualx", "aeq", "NULL");
    if (ini_value == NULL)
         goto error;
    Rch = (char *)ini_value;
    Rch = strtok(Rch, ",");
    if (Rch == NULL) {
        goto error;
    }
    data->vxcfg.eqparam.aeq_enable = atoi(Rch);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
        goto error;
    }
    data->vxcfg.eqparam.aeq_discard = atoi(Rch);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
        goto error;
    }
    data->vxcfg.eqparam.aeq_inputgain = atof(Rch);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
        goto error;
    }
    data->vxcfg.eqparam.aeq_outputgain = atof(Rch);
    Rch = strtok(NULL, ",");
    if (Rch == NULL) {
        goto error;
    }
    data->vxcfg.eqparam.aeq_bypassgain = atof(Rch);
    ini_value =  pIniParser->GetString("Virtualx", "aeq_bandnum", "5");
    if (ini_value == NULL)
     goto error;
    data->vxcfg.eqparam.aeq_bandnum = atoi(ini_value);
    //ALOGD("aeq_bandnum  is %d",data->vxcfg.eqparam.aeq_bandnum);
    ini_value =  pIniParser->GetString("Virtualx", "fc", "NULL");
    if (ini_value == NULL)
        goto error;
    result = AEQ_parse_fc_config(pContext, data->vxcfg.eqparam.aeq_bandnum, ini_value);
    if (result != 0)
        goto error;

    result = 0;

error:
    ALOGD("%s: %s", __FUNCTION__, result == 0 ? "sucessful" : "failed");
    delete pIniParser;
    pIniParser = NULL;
    return result;
}
int Virtualx_load_lib(vxContext *pContext)
{
    pContext->gVXLibHandler = dlopen(LIBVX_PATH_A, RTLD_NOW);
    if (!pContext->gVXLibHandler) {
        ALOGE("%s: failed", __FUNCTION__);
        return -EINVAL;
    }
    pContext->gVirtualxapi.VX_init = (int (*)(void*))dlsym(pContext->gVXLibHandler, "VX_init_api");
    if (!pContext->gVirtualxapi.VX_init) {
        ALOGE("%s: find func VX_init() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.VX_release = (int (*)(void))dlsym(pContext->gVXLibHandler, "VX_release_api");
    if (!pContext->gVirtualxapi.VX_release) {
        ALOGE("%s: find func VX_release() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.VX_process = (int (*)(int32_t **,int32_t**))dlsym(pContext->gVXLibHandler, "VX_process_api");
    if (!pContext->gVirtualxapi.VX_process) {
        ALOGE("%s: find func VX_process() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.Truvolume_process = (int (*)(int32_t **,int32_t**))dlsym(pContext->gVXLibHandler, "Truvolume_process_api");
    if (!pContext->gVirtualxapi.Truvolume_process) {
        ALOGE("%s: find func Truvolume_process() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.MBHL_process = (int (*)(int32_t **,int32_t**))dlsym(pContext->gVXLibHandler, "MBHL_process_api");
    if (!pContext->gVirtualxapi.MBHL_process) {
        ALOGE("%s: find func MBHL_process() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.VX_reset = (int (*)(void))dlsym(pContext->gVXLibHandler, "VX_reset_api");
    if (!pContext->gVirtualxapi.VX_reset) {
        ALOGE("%s: find func VX_reset() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.setvxlib1_enable = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "setvxlib1_enable");
    if (!pContext->gVirtualxapi.setvxlib1_enable) {
        ALOGE("%s: find func setvxlib1_enable() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.getvxlib1_enable = (int (*)(int32_t *))dlsym(pContext->gVXLibHandler, "getvxlib1_enable");
    if (!pContext->gVirtualxapi.getvxlib1_enable) {
        ALOGE("%s: find func getvxlib1_enable() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.setvxlib1_inmode = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "setvxlib1_inmode");
    if (!pContext->gVirtualxapi.setvxlib1_inmode) {
        ALOGE("%s: find func setvxlib1_inmode() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.getvxlib1_inmode = (int (*)(int32_t *))dlsym(pContext->gVXLibHandler, "getvxlib1_inmode");
    if (!pContext->gVirtualxapi.getvxlib1_inmode) {
        ALOGE("%s: find func getvxlib1_inmode failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.setvxlib1_outmode = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "setvxlib1_outmode");
    if (!pContext->gVirtualxapi.setvxlib1_outmode) {
        ALOGE("%s: find func setvxlib1_outmode failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.getvxlib1_outmode = (int (*)(int32_t *))dlsym(pContext->gVXLibHandler, "getvxlib1_outmode");
    if (!pContext->gVirtualxapi.getvxlib1_outmode) {
        ALOGE("%s: find func getvxlib1_outmode failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.setvxlib1_heardroomgain= (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "setvxlib1_heardroomgain");
    if (!pContext->gVirtualxapi.setvxlib1_heardroomgain) {
        ALOGE("%s: find func setvxlib1_heardroomgain() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.getvxlib1_heardroomgain = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "getvxlib1_heardroomgain");
    if (!pContext->gVirtualxapi.getvxlib1_heardroomgain) {
        ALOGE("%s: find func getvxlib1_heardroomgain() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.setvxlib1_procoutgain = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "setvxlib1_procoutgain");
    if (!pContext->gVirtualxapi.setvxlib1_procoutgain) {
        ALOGE("%s: find func setvxlib1_procoutgain() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.getvxlib1_procoutgain = (int (*)(int32_t *))dlsym(pContext->gVXLibHandler, "getvxlib1_procoutgain");
    if (!pContext->gVirtualxapi.getvxlib1_procoutgain) {
        ALOGE("%s: find func getvxlib1_procoutgain() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.settsx_enable = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "settsx_enable");
    if (!pContext->gVirtualxapi.settsx_enable) {
        ALOGE("%s: find func settsx_enable() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.gettsx_enable = (int (*)(int32_t *))dlsym(pContext->gVXLibHandler, "gettsx_enable");
    if (!pContext->gVirtualxapi.gettsx_enable) {
        ALOGE("%s: find func gettsx_enable() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.settsx_pssvmtrxenable = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "settsx_pssvmtrxenable");
    if (!pContext->gVirtualxapi.settsx_pssvmtrxenable) {
        ALOGE("%s: find func settsx_pssvmtrxenable() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.gettsx_pssvmtrxenable = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "gettsx_pssvmtrxenable");
    if (!pContext->gVirtualxapi.gettsx_pssvmtrxenable) {
        ALOGE("%s: find func gettsx_pssvmtrxenable() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.settsx_horizontctl = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "settsx_horizontctl");
    if (!pContext->gVirtualxapi.settsx_horizontctl) {
        ALOGE("%s: find func settsx_horizontctl() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.gettsx_horizontctl = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "gettsx_horizontctl");
    if (!pContext->gVirtualxapi.gettsx_horizontctl) {
        ALOGE("%s: find func gettsx_horizontctl() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.settsx_frntctrl = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "settsx_frntctrl");
    if (!pContext->gVirtualxapi.settsx_frntctrl) {
        ALOGE("%s: find func settsx_frntctrl() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.gettsx_frntctrl = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "gettsx_frntctrl");
    if (!pContext->gVirtualxapi.gettsx_frntctrl) {
        ALOGE("%s: find func gettsx_frntctrl() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.settsx_surroundctrl = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "settsx_surroundctrl");
    if (!pContext->gVirtualxapi.settsx_surroundctrl) {
        ALOGE("%s: find func settsx_surroundctrl() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.gettsx_surroundctrl = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "gettsx_surroundctrl");
    if (!pContext->gVirtualxapi.gettsx_surroundctrl) {
        ALOGE("%s: find func gettsx_surroundctrl() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.settsx_lprgain = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "settsx_lprgain");
    if (!pContext->gVirtualxapi.settsx_lprgain) {
        ALOGE("%s: find func settsx_lprgain() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.gettsx_lprgain = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "gettsx_lprgain");
    if (!pContext->gVirtualxapi.gettsx_lprgain) {
        ALOGE("%s: find func gettsx_lprgain() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.settsx_heightmixcoeff = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "settsx_heightmixcoeff");
    if (!pContext->gVirtualxapi.settsx_heightmixcoeff) {
        ALOGE("%s: find func settsx_heightmixcoeff() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.gettsx_heightmixcoeff = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "gettsx_heightmixcoeff");
    if (!pContext->gVirtualxapi.gettsx_heightmixcoeff) {
        ALOGE("%s: find func gettsx_heightmixcoeff() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.settsx_centergain = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "settsx_centergain");
    if (!pContext->gVirtualxapi.settsx_centergain) {
        ALOGE("%s: find func settsx_centergain() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.gettsx_centergain = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "gettsx_centergain");
    if (!pContext->gVirtualxapi.gettsx_centergain) {
        ALOGE("%s: find func gettsx_centergain() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.settsx_heightdiscards = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "settsx_heightdiscards");
    if (!pContext->gVirtualxapi.settsx_heightdiscards) {
        ALOGE("%s: find func settsx_heightdiscards() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.gettsx_heightdiscards = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "gettsx_heightdiscards");
    if (!pContext->gVirtualxapi.gettsx_heightdiscards) {
        ALOGE("%s: find func gettsx_heightdiscards() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.settsx_discard = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "settsx_discard");
    if (!pContext->gVirtualxapi.settsx_discard) {
        ALOGE("%s: find func settsx_discard() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.gettsx_discard = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "gettsx_discard");
    if (!pContext->gVirtualxapi.gettsx_discard) {
        ALOGE("%s: find func gettsx_discard() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.settsx_hghtupmixenable = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "settsx_hghtupmixenable");
    if (!pContext->gVirtualxapi.settsx_hghtupmixenable) {
        ALOGE("%s: find func settsx_hghtupmixenable() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.gettsx_hghtupmixenable = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "gettsx_hghtupmixenable");
    if (!pContext->gVirtualxapi.gettsx_hghtupmixenable) {
        ALOGE("%s: find func gettsx_hghtupmixenable() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.settsx_dcenable = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "settsx_dcenable");
    if (!pContext->gVirtualxapi.settsx_dcenable) {
        ALOGE("%s: find func settsx_dcenable() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.gettsx_dcenable = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "gettsx_dcenable");
    if (!pContext->gVirtualxapi.gettsx_dcenable) {
        ALOGE("%s: find func gettsx_dcenable() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.settsx_dccontrol = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "settsx_dccontrol");
    if (!pContext->gVirtualxapi.settsx_dccontrol) {
        ALOGE("%s: find func settsx_dccontrol() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.gettsx_dccontrol = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "gettsx_dccontrol");
    if (!pContext->gVirtualxapi.gettsx_dccontrol) {
        ALOGE("%s: find func gettsx_dccontrol() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.settsx_defenable = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "settsx_defenable");
    if (!pContext->gVirtualxapi.settsx_defenable) {
        ALOGE("%s: find func settsx_defenable() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.gettsx_defenable = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "gettsx_defenable");
    if (!pContext->gVirtualxapi.gettsx_defenable) {
        ALOGE("%s: find func gettsx_defenable() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.settsx_defcontrol = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "settsx_defcontrol");
    if (!pContext->gVirtualxapi.settsx_defcontrol) {
        ALOGE("%s: find func settsx_defcontrol() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.gettsx_defcontrol = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "gettsx_defcontrol");
    if (!pContext->gVirtualxapi.gettsx_defcontrol) {
        ALOGE("%s: find func gettsx_defcontrol() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.settbhdx_enable = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "settbhdx_enable");
    if (!pContext->gVirtualxapi.settbhdx_enable) {
        ALOGE("%s: find func settbhdx_enable() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.gettbhdx_enable = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "gettbhdx_enable");
    if (!pContext->gVirtualxapi.gettbhdx_enable) {
        ALOGE("%s: find func gettbhdx_enable() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.settbhdx_monomode = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "settbhdx_monomode");
    if (!pContext->gVirtualxapi.settbhdx_monomode) {
        ALOGE("%s: find func settbhdx_monomode() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.gettbhdx_monomode = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "gettbhdx_monomode");
    if (!pContext->gVirtualxapi.gettbhdx_monomode) {
        ALOGE("%s: find func gettbhdx_monomode() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.settbhdx_spksize = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "settbhdx_spksize");
    if (!pContext->gVirtualxapi.settbhdx_spksize) {
        ALOGE("%s: find func settbhdx_spksize() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.gettbhdx_spksize = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "gettbhdx_spksize");
    if (!pContext->gVirtualxapi.gettbhdx_spksize) {
        ALOGE("%s: find func gettbhdx_spksize() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.settbhdx_tempgain = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "settbhdx_tempgain");
    if (!pContext->gVirtualxapi.settbhdx_tempgain) {
        ALOGE("%s: find func settbhdx_tempgain() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.gettbhdx_tempgain = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "gettbhdx_tempgain");
    if (!pContext->gVirtualxapi.gettbhdx_tempgain) {
        ALOGE("%s: find func gettbhdx_tempgain() failed\n", __FUNCTION__);
        goto Error;
    }

    pContext->gVirtualxapi.settbhdx_maxgain = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "settbhdx_maxgain");
    if (!pContext->gVirtualxapi.settbhdx_maxgain) {
        ALOGE("%s: find func settbhdx_maxgain() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.gettbhdx_maxgain = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "gettbhdx_maxgain");
    if (!pContext->gVirtualxapi.gettbhdx_maxgain) {
        ALOGE("%s: find func gettbhdx_maxgain() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.settbhdx_hporder = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "settbhdx_hporder");
    if (!pContext->gVirtualxapi.settbhdx_hporder) {
        ALOGE("%s: find func settbhdx_hporder() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.gettbhdx_hporder = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "gettbhdx_hporder");
    if (!pContext->gVirtualxapi.gettbhdx_hporder) {
        ALOGE("%s: find func gettbhdx_hporder() failed\n", __FUNCTION__);
        goto Error;
    }

    pContext->gVirtualxapi.settbhdx_hpenable = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "settbhdx_hpenable");
    if (!pContext->gVirtualxapi.settbhdx_hpenable) {
        ALOGE("%s: find func settbhdx_hpenable() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.gettbhdx_hpenable = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "gettbhdx_hpenable");
    if (!pContext->gVirtualxapi.gettbhdx_hpenable) {
        ALOGE("%s: find func gettbhdx_hpenable() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.settbhdx_processdiscard = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "settbhdx_processdiscard");
    if (!pContext->gVirtualxapi.settbhdx_processdiscard) {
        ALOGE("%s: find func settbhdx_processdiscard() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.getthbdx_processdiscard = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "getthbdx_processdiscard");
    if (!pContext->gVirtualxapi.getthbdx_processdiscard) {
        ALOGE("%s: find func getthbdx_processdiscard() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.setmbhl_processdiscard = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "setmbhl_processdiscard");
    if (!pContext->gVirtualxapi.setmbhl_processdiscard) {
        ALOGE("%s: find func setmbhl_processdiscard() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.getmbhl_processdiscard = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "getmbhl_processdiscard");
    if (!pContext->gVirtualxapi.getmbhl_processdiscard) {
        ALOGE("%s: find func getmbhl_processdiscard() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.setmbhl_enable = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "setmbhl_enable");
    if (!pContext->gVirtualxapi.setmbhl_enable) {
        ALOGE("%s: find func setmbhl_enable() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.getmbhl_enable = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "getmbhl_enable");
    if (!pContext->gVirtualxapi.getmbhl_enable) {
        ALOGE("%s: find func getmbhl_enable() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.setmbhl_bypassgain = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "setmbhl_bypassgain");
    if (!pContext->gVirtualxapi.setmbhl_bypassgain) {
        ALOGE("%s: find func setmbhl_bypassgain() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.getmbhl_bypassgain = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "getmbhl_bypassgain");
    if (!pContext->gVirtualxapi.getmbhl_bypassgain) {
        ALOGE("%s: find func getmbhl_bypassgain() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.setmbhl_reflevel = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "setmbhl_reflevel");
    if (!pContext->gVirtualxapi.setmbhl_reflevel) {
        ALOGE("%s: find func setmbhl_reflevel() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.getmbhl_reflevel = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "getmbhl_reflevel");
    if (!pContext->gVirtualxapi.getmbhl_reflevel) {
        ALOGE("%s: find func getmbhl_reflevel() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.setmbhl_volume = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "setmbhl_volume");
    if (!pContext->gVirtualxapi.setmbhl_volume) {
        ALOGE("%s: find func setmbhl_volume() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.getmbhl_volume = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "getmbhl_volume");
    if (!pContext->gVirtualxapi.getmbhl_volume) {
        ALOGE("%s: find func getmbhl_volume() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.setmbhl_volumestep = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "setmbhl_volumestep");
    if (!pContext->gVirtualxapi.setmbhl_volumestep) {
        ALOGE("%s: find func setmbhl_volumestep() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.getmbhl_volumestep = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "getmbhl_volumestep");
    if (!pContext->gVirtualxapi.getmbhl_volumestep) {
        ALOGE("%s: find func getmbhl_volumestep() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.setmbhl_balancestep = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "setmbhl_balancestep");
    if (!pContext->gVirtualxapi.setmbhl_balancestep) {
        ALOGE("%s: find func setmbhl_balancestep() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.getmbhl_balancestep = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "getmbhl_balancestep");
    if (!pContext->gVirtualxapi.getmbhl_balancestep) {
        ALOGE("%s: find func getmbhl_balancestep() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.setmbhl_outputgain = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "setmbhl_outputgain");
    if (!pContext->gVirtualxapi.setmbhl_outputgain) {
        ALOGE("%s: find func setmbhl_outputgain() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.getmbhl_outputgain = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "getmbhl_outputgain");
    if (!pContext->gVirtualxapi.getmbhl_outputgain) {
        ALOGE("%s: find func getmbhl_outputgain() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.setmbhl_boost = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "setmbhl_boost");
    if (!pContext->gVirtualxapi.setmbhl_boost) {
        ALOGE("%s: find func setmbhl_boost() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.getmbhl_boost = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "getmbhl_boost");
    if (!pContext->gVirtualxapi.getmbhl_boost) {
        ALOGE("%s: find func getmbhl_boost() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.setmbhl_threshold = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "setmbhl_threshold");
    if (!pContext->gVirtualxapi.setmbhl_threshold) {
        ALOGE("%s: find func setmbhl_threshold() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.getmbhl_threshold = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "getmbhl_threshold");
    if (!pContext->gVirtualxapi.getmbhl_threshold) {
        ALOGE("%s: find func getmbhl_threshold() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.setmbhl_slowoffset = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "setmbhl_slowoffset");
    if (!pContext->gVirtualxapi.setmbhl_slowoffset) {
        ALOGE("%s: find func setmbhl_slowoffset() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.getmbhl_slowoffset = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "getmbhl_slowoffset");
    if (!pContext->gVirtualxapi.getmbhl_slowoffset) {
        ALOGE("%s: find func getmbhl_slowoffset() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.setmbhl_fastattack = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "setmbhl_fastattack");
    if (!pContext->gVirtualxapi.setmbhl_fastattack) {
        ALOGE("%s: find func setmbhl_fastattack() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.getmbhl_fastattack = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "getmbhl_fastattack");
    if (!pContext->gVirtualxapi.getmbhl_fastattack) {
        ALOGE("%s: find func getmbhl_fastattack() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.setmbhl_fastrelease = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "setmbhl_fastrelease");
    if (!pContext->gVirtualxapi.setmbhl_fastrelease) {
        ALOGE("%s: find func setmbhl_fastrelease() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.getmbhl_fastrelease = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "getmbhl_fastrelease");
    if (!pContext->gVirtualxapi.getmbhl_fastrelease) {
        ALOGE("%s: find func getmbhl_fastrelease() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.setmbhl_slowattrack = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "setmbhl_slowattrack");
    if (!pContext->gVirtualxapi.setmbhl_slowattrack) {
        ALOGE("%s: find func setmbhl_slowattrack() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.getmbhl_slowattrack = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "getmbhl_slowattrack");
    if (!pContext->gVirtualxapi.getmbhl_slowattrack) {
        ALOGE("%s: find func getmbhl_slowattrack() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.setmbhl_slowrelease = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "setmbhl_slowrelease");
    if (!pContext->gVirtualxapi.setmbhl_slowrelease) {
        ALOGE("%s: find func setmbhl_slowrelease() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.getmbhl_slowrelease = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "getmbhl_slowrelease");
    if (!pContext->gVirtualxapi.getmbhl_slowrelease) {
        ALOGE("%s: find func getmbhl_slowrelease() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.setmbhl_delay = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "setmbhl_delay");
    if (!pContext->gVirtualxapi.setmbhl_delay) {
        ALOGE("%s: find func setmbhl_delay() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.getmbhl_delay = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "getmbhl_delay");
    if (!pContext->gVirtualxapi.getmbhl_delay) {
        ALOGE("%s: find func getmbhl_delay() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.setmbhl_envelopefre = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "setmbhl_envelopefre");
    if (!pContext->gVirtualxapi.setmbhl_envelopefre) {
        ALOGE("%s: find func setmbhl_envelopefre() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.getmbhl_envelopefre = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "getmbhl_envelopefre");
    if (!pContext->gVirtualxapi.getmbhl_envelopefre) {
        ALOGE("%s: find func getmbhl_envelopefre() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.setmbhl_mode = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "setmbhl_mode");
    if (!pContext->gVirtualxapi.setmbhl_mode) {
        ALOGE("%s: find func setmbhl_mode() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.getmbhl_mode = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "getmbhl_mode");
    if (!pContext->gVirtualxapi.getmbhl_mode) {
        ALOGE("%s: find func getmbhl_mode() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.setmbhl_lowcross = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "setmbhl_lowcross");
    if (!pContext->gVirtualxapi.setmbhl_lowcross) {
        ALOGE("%s: find func setmbhl_lowcross() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.getmbhl_lowcross = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "getmbhl_lowcross");
    if (!pContext->gVirtualxapi.getmbhl_lowcross) {
        ALOGE("%s: find func getmbhl_lowcross() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.setmbhl_crossmid = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "setmbhl_crossmid");
    if (!pContext->gVirtualxapi.setmbhl_crossmid) {
        ALOGE("%s: find func setmbhl_crossmid() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.getmbhl_crossmid = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "getmbhl_crossmid");
    if (!pContext->gVirtualxapi.getmbhl_crossmid) {
        ALOGE("%s: find func getmbhl_crossmid() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.setmbhl_compattrack = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "setmbhl_compattrack");
    if (!pContext->gVirtualxapi.setmbhl_compattrack) {
        ALOGE("%s: find func setmbhl_compattrack() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.getmbhl_compattrack = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "getmbhl_compattrack");
    if (!pContext->gVirtualxapi.getmbhl_compattrack) {
        ALOGE("%s: find func getmbhl_compattrack() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.setmbhl_lowrelease = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "setmbhl_lowrelease");
    if (!pContext->gVirtualxapi.setmbhl_lowrelease) {
        ALOGE("%s: find func setmbhl_lowrelease() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.getmbhl_lowrelease = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "getmbhl_lowrelease");
    if (!pContext->gVirtualxapi.getmbhl_lowrelease) {
        ALOGE("%s: find func getmbhl_lowrelease() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.setmbhl_complowratio = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "setmbhl_complowratio");
    if (!pContext->gVirtualxapi.setmbhl_complowratio) {
        ALOGE("%s: find func setmbhl_complowratio() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.getmbhl_complowratio = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "getmbhl_complowratio");
    if (!pContext->gVirtualxapi.getmbhl_complowratio) {
        ALOGE("%s: find func getmbhl_complowratio() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.setmbhl_complowthresh = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "setmbhl_complowthresh");
    if (!pContext->gVirtualxapi.setmbhl_complowthresh) {
        ALOGE("%s: find func setmbhl_complowthresh() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.getmbhl_complowthresh = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "getmbhl_complowthresh");
    if (!pContext->gVirtualxapi.getmbhl_complowthresh) {
        ALOGE("%s: find func getmbhl_complowthresh() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.setmbhl_lowmakeup = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "setmbhl_lowmakeup");
    if (!pContext->gVirtualxapi.setmbhl_lowmakeup) {
        ALOGE("%s: find func setmbhl_lowmakeup() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.getmbhl_lowmakeup = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "getmbhl_lowmakeup");
    if (!pContext->gVirtualxapi.getmbhl_lowmakeup) {
        ALOGE("%s: find func getmbhl_lowmakeup() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.setmbhl_compmidrelease = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "setmbhl_compmidrelease");
    if (!pContext->gVirtualxapi.setmbhl_compmidrelease) {
        ALOGE("%s: find func setmbhl_compmidrelease() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.getmbhl_compmidrelease = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "getmbhl_compmidrelease");
    if (!pContext->gVirtualxapi.getmbhl_compmidrelease) {
        ALOGE("%s: find func getmbhl_compmidrelease() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.setmbhl_midratio = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "setmbhl_midratio");
    if (!pContext->gVirtualxapi.setmbhl_midratio) {
        ALOGE("%s: find func setmbhl_midratio() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.getmbhl_midratio = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "getmbhl_midratio");
    if (!pContext->gVirtualxapi.getmbhl_midratio) {
        ALOGE("%s: find func getmbhl_midratio() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.setmbhl_compmidthresh = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "setmbhl_compmidthresh");
    if (!pContext->gVirtualxapi.setmbhl_compmidthresh) {
        ALOGE("%s: find func setmbhl_compmidthresh() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.getmbhl_compmidthresh = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "getmbhl_compmidthresh");
    if (!pContext->gVirtualxapi.getmbhl_compmidthresh) {
        ALOGE("%s: find func getmbhl_compmidthresh() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.setmbhl_compmidmakeup = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "setmbhl_compmidmakeup");
    if (!pContext->gVirtualxapi.setmbhl_compmidmakeup) {
        ALOGE("%s: find func setmbhl_compmidmakeup() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.getmbhl_compmidmakeup = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "getmbhl_compmidmakeup");
    if (!pContext->gVirtualxapi.getmbhl_compmidmakeup) {
        ALOGE("%s: find func getmbhl_compmidmakeup() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.setmbhl_comphighrelease = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "setmbhl_comphighrelease");
    if (!pContext->gVirtualxapi.setmbhl_comphighrelease) {
        ALOGE("%s: find func setmbhl_comphighrelease() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.getmbhl_comphighrelease = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "getmbhl_comphighrelease");
    if (!pContext->gVirtualxapi.getmbhl_comphighrelease) {
        ALOGE("%s: find func getmbhl_comphighrelease() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.setmbhl_comphighratio = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "setmbhl_comphighratio");
    if (!pContext->gVirtualxapi.setmbhl_comphighratio) {
        ALOGE("%s: find func setmbhl_comphighratio() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.getmbhl_comphighratio = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "getmbhl_comphighratio");
    if (!pContext->gVirtualxapi.getmbhl_comphighratio) {
        ALOGE("%s: find func getmbhl_comphighratio() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.setmbhl_comphighthresh = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "setmbhl_comphighthresh");
    if (!pContext->gVirtualxapi.setmbhl_comphighthresh) {
        ALOGE("%s: find func setmbhl_comphighthresh() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.getmbhl_comphighthresh = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "getmbhl_comphighthresh");
    if (!pContext->gVirtualxapi.getmbhl_comphighthresh) {
        ALOGE("%s: find func getmbhl_comphighthresh() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.setmbhl_comphighmakeup = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "setmbhl_comphighmakeup");
    if (!pContext->gVirtualxapi.setmbhl_comphighmakeup) {
        ALOGE("%s: find func setmbhl_comphighmakeup() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.getmbhl_comphighmakeup = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "getmbhl_comphighmakeup");
    if (!pContext->gVirtualxapi.getmbhl_comphighmakeup) {
        ALOGE("%s: find func getmbhl_comphighmakeup() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.settruvolume_ctren = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "settruvolume_ctren");
    if (!pContext->gVirtualxapi.settruvolume_ctren) {
        ALOGE("%s: find func settruvolume_ctren() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.gettruvolume_ctren = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "gettruvolume_ctren");
    if (!pContext->gVirtualxapi.gettruvolume_ctren) {
        ALOGE("%s: find func gettruvolume_ctren() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.settruvolume_ctrtarget = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "settruvolume_ctrtarget");
    if (!pContext->gVirtualxapi.settruvolume_ctrtarget) {
        ALOGE("%s: find func settruvolume_ctrtarget() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.gettruvolume_ctrtarget = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "gettruvolume_ctrtarget");
    if (!pContext->gVirtualxapi.gettruvolume_ctrtarget) {
        ALOGE("%s: find func gettruvolume_ctrtarget() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.settruvolume_ctrpreset = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "settruvolume_ctrpreset");
    if (!pContext->gVirtualxapi.settruvolume_ctrpreset) {
        ALOGE("%s: find func settruvolume_ctrpreset() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.gettruvolume_ctrpreset = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "gettruvolume_ctrpreset");
    if (!pContext->gVirtualxapi.gettruvolume_ctrpreset) {
        ALOGE("%s: find func gettruvolume_ctrpreset() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.setlc_inmode = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "setlc_inmode");
    if (!pContext->gVirtualxapi.setlc_inmode) {
        ALOGE("%s: find func setlc_inmode() failed\n", __FUNCTION__);
        goto Error;
    }
    pContext->gVirtualxapi.settbhd_FilterDesign = (int (*)(int32_t,float,float,float))dlsym(pContext->gVXLibHandler, "settbhd_FilterDesign");
    if (!pContext->gVirtualxapi.settbhd_FilterDesign) {
        ALOGE("%s: find func settbhd_FilterDesign() failed\n", __FUNCTION__);
        return -1;
    }
    pContext->gVirtualxapi.setmbhl_FilterDesign = (int (*)(float,float))dlsym(pContext->gVXLibHandler, "setmbhl_FilterDesign");
    if (!pContext->gVirtualxapi.setmbhl_FilterDesign) {
        ALOGE("%s: find func setmbhl_FilterDesign() failed\n", __FUNCTION__);
        return -1;
    }
    pContext->gVirtualxapi.seteq_enable = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "seteq_enable");
    if (!pContext->gVirtualxapi.seteq_enable) {
        ALOGE("%s: find func seteq_enable() failed\n", __FUNCTION__);
        return -1;
    }
    pContext->gVirtualxapi.geteq_enable = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "geteq_enable");
    if (!pContext->gVirtualxapi.geteq_enable) {
        ALOGE("%s: find func geteq_enable() failed\n", __FUNCTION__);
        return -1;
    }
    pContext->gVirtualxapi.seteq_discard = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "seteq_discard");
    if (!pContext->gVirtualxapi.seteq_discard) {
        ALOGE("%s: find func seteq_discard() failed\n", __FUNCTION__);
        return -1;
    }
    pContext->gVirtualxapi.seteq_inputG = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "seteq_inputG");
    if (!pContext->gVirtualxapi.seteq_inputG) {
        ALOGE("%s: find func seteq_inputG() failed\n", __FUNCTION__);
        return -1;
    }
    pContext->gVirtualxapi.geteq_inputG = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "geteq_inputG");
    if (!pContext->gVirtualxapi.geteq_inputG) {
        ALOGE("%s: find func geteq_inputG() failed\n", __FUNCTION__);
        return -1;
    }
    pContext->gVirtualxapi.seteq_outputG = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "seteq_outputG");
    if (!pContext->gVirtualxapi.seteq_outputG) {
        ALOGE("%s: find func seteq_outputG() failed\n", __FUNCTION__);
        return -1;
    }
    pContext->gVirtualxapi.geteq_outputG = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "geteq_outputG");
    if (!pContext->gVirtualxapi.geteq_outputG) {
        ALOGE("%s: find func geteq_outputG() failed\n", __FUNCTION__);
        return -1;
    }
    pContext->gVirtualxapi.seteq_bypassG = (int (*)(int32_t))dlsym(pContext->gVXLibHandler, "seteq_bypassG");
    if (!pContext->gVirtualxapi.seteq_bypassG) {
        ALOGE("%s: find func seteq_bypassG() failed\n", __FUNCTION__);
        return -1;
    }
    pContext->gVirtualxapi.geteq_bypassG = (int (*)(int32_t*))dlsym(pContext->gVXLibHandler, "geteq_bypassG");
    if (!pContext->gVirtualxapi.geteq_bypassG) {
        ALOGE("%s: find func geteq_bypassG() failed\n", __FUNCTION__);
        return -1;
    }
    pContext->gVirtualxapi.seteq_band = (int (*)(int32_t,void*))dlsym(pContext->gVXLibHandler, "seteq_band");
    if (!pContext->gVirtualxapi.seteq_band) {
        ALOGE("%s: find func seteq_band() failed\n", __FUNCTION__);
        return -1;
    }
    ALOGD("%s: sucessful", __FUNCTION__);
    return 0;
Error:
    memset(&pContext->gVirtualxapi, 0, sizeof(pContext->gVirtualxapi));
    dlclose(pContext->gVXLibHandler);
    pContext->gVXLibHandler = NULL;
    return -EINVAL;
}

int unload_Virtualx_lib(vxContext *pContext)
{
    memset(&pContext->gVirtualxapi, 0, sizeof(pContext->gVirtualxapi));
    if (pContext->gVXLibHandler) {
        dlclose(pContext->gVXLibHandler);
        pContext->gVXLibHandler = NULL;
    }
    return 0;
}

int Virtualx_init(vxContext *pContext)
{
    vxdata *data = &pContext->gvxdata;
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

    //init counter and value
    pContext->ch_num = 0;
    pContext->lowcrossfreq = 300;
    pContext->midcrossfreq = 5000;
    pContext->spksize = 80;
    pContext->hpratio = 0.5f;
    pContext->extbass = 0.8f;
    data->vxlib_enable = data->vxcfg.vxlib.enable;
    data->input_mode = data->vxcfg.vxlib.input_mode;
    data->output_mode = data->vxcfg.vxlib.output_mode;
    data->headroom_gain = FXP32(data->vxcfg.vxlib.headroom_gain,2);
    data->output_gain = FXP32(data->vxcfg.vxlib.output_gain,4);
    data->vxtrusurround_enable = data->TS_usr_cfg[0].vxtrusurround.enable;
    data->upmixer_enable = data->TS_usr_cfg[0].vxtrusurround.upmixer_enable;
    data->horizontaleffect_control =data->TS_usr_cfg[0].vxtrusurround.horizontaleffect_control;
    data->frontwidening_control = FXP32(data->TS_usr_cfg[0].vxtrusurround.frontwidening_control,3);
    data->surroundwidening_control = FXP32(data->TS_usr_cfg[0].vxtrusurround.surroundwidening_control,3);
    data->phantomcenter_mixlevel = FXP32(data->TS_usr_cfg[0].vxtrusurround.phantomcenter_mixlevel,3);
    data->heightmix_coeff = FXP32(data->TS_usr_cfg[0].vxtrusurround.heightmix_coeff,3);
    data->center_gain = FXP32(data->TS_usr_cfg[0].vxtrusurround.center_gain,3);
    data->height_discard = data->TS_usr_cfg[0].vxtrusurround.height_discard;
    data->discard = data->TS_usr_cfg[0].vxtrusurround.discard;
    data->heightupmix_enable = data->TS_usr_cfg[0].vxtrusurround.heightupmix_enable;
    data->vxdialogclarity_enable = data->DC_usr_cfg[0].vxdialogclarity.enable;
    data->vxdialogclarity_level = FXP32(data->DC_usr_cfg[0].vxdialogclarity.level,2);
    data->vxdefinition_enable = data->TS_usr_cfg[0].vxdefinition.enable;
    data->vxdefinition_level = FXP32(data->TS_usr_cfg[0].vxdefinition.level,2);
    data->tbhdx_enable = data->TS_usr_cfg[0].tbhdx.enable;
    data->monomode_enable = data->TS_usr_cfg[0].tbhdx.monomode_enable;
    data->speaker_size = data->TS_usr_cfg[0].tbhdx.speaker_size;
    data->tmporal_gain = FXP32(data->TS_usr_cfg[0].tbhdx.tmporal_gain,2);
    data->max_gain = FXP32(data->TS_usr_cfg[0].tbhdx.max_gain,2);
    data->hpfo = data->TS_usr_cfg[0].tbhdx.hpfo;
    data->highpass_enble = data->TS_usr_cfg[0].tbhdx.highpass_enble;
    data->tbhdx_processdiscard = 0;
    data->mbhl_enable = data->vxcfg.mbhl.mbhl_enable;
    data->mbhl_bypassgain = FXP32(data->vxcfg.mbhl.mbhl_bypassgain,2);
    data->mbhl_reflevel = FXP32(data->vxcfg.mbhl.mbhl_reflevel,2);
    data->mbhl_volume = FXP32(data->vxcfg.mbhl.mbhl_volume,2);
    data->mbhl_volumesttep = data->vxcfg.mbhl.mbhl_volumesttep;
    data->mbhl_balancestep = data->vxcfg.mbhl.mbhl_balancestep;
    data->mbhl_outputgain = FXP32(data->vxcfg.mbhl.mbhl_outputgain,2);
    data->mbhl_boost = FXP32(data->vxcfg.mbhl.mbhl_boost,11);
    data->mbhl_threshold = FXP32(data->vxcfg.mbhl.mbhl_threshold,2);
    data->mbhl_slowoffset = FXP32(data->vxcfg.mbhl.mbhl_slowoffset,3);
    data->mbhl_fastattack = FXP32(data->vxcfg.mbhl.mbhl_fastattack,5);
    data->mbhl_fastrelease = data->vxcfg.mbhl.mbhl_fastrelease;
    data->mbhl_slowattack = data->vxcfg.mbhl.mbhl_slowattack;
    data->mbhl_slowrelease = data->vxcfg.mbhl.mbhl_slowrelease;
    data->mbhl_delay = data->vxcfg.mbhl.mbhl_delay;
    data->mbhl_envelopefre = data->vxcfg.mbhl.mbhl_envelopefre;
    data->mbhl_mode = data->vxcfg.mbhl.mbhl_mode;
    data->mbhl_lowcross = data->vxcfg.mbhl.mbhl_lowcross;
    data->mbhl_midcross = data->vxcfg.mbhl.mbhl_midcross;
    data->mbhl_compressorat = data->vxcfg.mbhl.mbhl_compressorat;
    data->mbhl_compressorlrt = data->vxcfg.mbhl.mbhl_compressorlrt;
    data->mbhl_compressolr = FXP32(data->vxcfg.mbhl.mbhl_compressolr,6);
    data->mbhl_compressolt = FXP32(data->vxcfg.mbhl.mbhl_compressolt,5);
    data->mbhl_makeupgain = FXP32(data->vxcfg.mbhl.mbhl_makeupgain,5);
    data->mbhl_midreleasetime = data->vxcfg.mbhl.mbhl_midreleasetime;
    data->mbhl_midratio = FXP32(data->vxcfg.mbhl.mbhl_midratio,6);
    data->mbhl_midthreshold = FXP32(data->vxcfg.mbhl.mbhl_midthreshold,5);
    data->mbhl_midmakeupgain = FXP32(data->vxcfg.mbhl.mbhl_midmakeupgain,5);
    data->mbhl_highreleasetime = data->vxcfg.mbhl.mbhl_highreleasetime;
    data->mbhl_highratio = FXP32(data->vxcfg.mbhl.mbhl_highratio,6);
    data->mbhl_highthreshold = FXP32(data->vxcfg.mbhl.mbhl_highthreshold,5);
    data->mbhl_highmakegian = FXP32(data->vxcfg.mbhl.mbhl_highmakegian,5);
    data->mbhl_processdiscard = 0;
    data->Truvolume_enable = data->vxcfg.Truvolume.enable;
    data->targetlevel = data->vxcfg.Truvolume.targetlevel;
    data->preset = data->vxcfg.Truvolume.preset;
    data->aeq_enable = data->vxcfg.eqparam.aeq_enable;
    data->aeq_discard = data->vxcfg.eqparam.aeq_discard;
    data->aeq_inputgain = FXP16(data->vxcfg.eqparam.aeq_inputgain,1);
    data->aeq_outputgain = FXP16(data->vxcfg.eqparam.aeq_outputgain,1);
    data->aeq_bypassgain = FXP16(data->vxcfg.eqparam.aeq_bypassgain,1);
    data->aeq_bandnum = data->vxcfg.eqparam.aeq_bandnum;
    for (int i = 0; i < data->aeq_bandnum; i++)
        data->aeq_fc[i] = data->vxcfg.eqparam.aeq_fc[i];
    //malloc memory for ppMappedInCh[3]~ppMappedInCh[11] to fix crash (null pointer dereference)
    for (int i = 0; i < 12; i++) {
        pContext->ppMappedInCh[i]  = pContext->sTempBuffer[i];
        pContext->ppMappedOutCh[i] = pContext->TempBuffer[i];
    }
    if (pContext->gVXLibHandler) {
        (*pContext->gVirtualxapi.VX_init)((void*) data);
    }
    ALOGD("%s: sucessful", __FUNCTION__);
    return 0;
}

int Virtualx_reset(vxContext *pContext)
{
    if (pContext->gVXLibHandler)
        (*pContext->gVirtualxapi.VX_reset)();
    memset(pContext->sTempBuffer,0,sizeof(int32_t) * 12 * 256);
    memset(pContext->TempBuffer,0,sizeof(int32_t) * 12 * 256);
    for (int i = 0; i < 12; i++) {
        memset((void*)pContext->ppMappedInCh[i],0,256 * sizeof(int32_t));
        memset((void*)pContext->ppMappedOutCh[i],0,256 * sizeof(int32_t));
    }
    return 0;
}

int Virtualx_configure(vxContext *pContext, effect_config_t *pConfig)
{
    if (pConfig->inputCfg.samplingRate != pConfig->outputCfg.samplingRate)
        return -EINVAL;
    if (pConfig->inputCfg.format != pConfig->outputCfg.format)
       return -EINVAL;
    /*
    if (pConfig->inputCfg.channels != AUDIO_CHANNEL_OUT_STEREO) {
        ALOGW("%s: channels in = 0x%x channels out = 0x%x", __FUNCTION__, pConfig->inputCfg.channels, pConfig->outputCfg.channels);
        pConfig->inputCfg.channels = pConfig->outputCfg.channels = AUDIO_CHANNEL_OUT_STEREO;
    }
    */
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

int Virtualx_setParameter(vxContext *pContext, void *pParam, void *pValue)
{
    int32_t param = *(int32_t *)pParam;
    int32_t value;
    float scale;
    int32_t basslvl;
    float * p;
    vxdata *data = &pContext->gvxdata;
    switch (param) {
    case VIRTUALX_PARAM_ENABLE:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        value = *(int32_t *)pValue;
        data->enable = value;
        if (data->enable) {
            property_set("media.libplayer.dtsMulChPcm","true");
        } else {
            property_set("media.libplayer.dtsMulChPcm","false");
        }
        ALOGD("%s: Set status -> %s", __FUNCTION__, VXStatusstr[value]);
        break;
    case VIRTUALX_PARAM_DIALOGCLARTY_MODE:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        value = *(int32_t *)pValue;
        if (value > 2)
            value = 2;
        data->dialogclarity_mode = value;
        data->vxdialogclarity_enable = data->DC_usr_cfg[value].vxdialogclarity.enable;
        data->vxdialogclarity_level = FXP32(data->DC_usr_cfg[value].vxdialogclarity.level,2);
        if (data->enable) {
            (*pContext->gVirtualxapi.settsx_dcenable)(data->vxdialogclarity_enable);
            (*pContext->gVirtualxapi.settsx_dccontrol)(data->vxdialogclarity_level);
            ALOGD("%s: Set Dialog Clarity Mode %s", __FUNCTION__, VXDialogClarityModestr[value]);
        } else {
            ALOGD("%s: Set Dialog Clarity Mode failed", __FUNCTION__);
            return 0;
        }
        break;
    case VIRTUALX_PARAM_SURROUND_MODE:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        value = *(int32_t *)pValue;
        if (value > 1)
            value = 1;
        data->surround_mode = value;
        data->vxtrusurround_enable = data->TS_usr_cfg[value].vxtrusurround.enable;
        data->upmixer_enable = data->TS_usr_cfg[value].vxtrusurround.upmixer_enable;
        data->horizontaleffect_control =data->TS_usr_cfg[value].vxtrusurround.horizontaleffect_control;
        data->frontwidening_control = FXP32(data->TS_usr_cfg[value].vxtrusurround.frontwidening_control,3);
        data->surroundwidening_control = FXP32(data->TS_usr_cfg[value].vxtrusurround.surroundwidening_control,3);
        data->phantomcenter_mixlevel = FXP32(data->TS_usr_cfg[value].vxtrusurround.phantomcenter_mixlevel,3);
        data->heightmix_coeff = FXP32(data->TS_usr_cfg[value].vxtrusurround.heightmix_coeff,3);
        data->center_gain = FXP32(data->TS_usr_cfg[value].vxtrusurround.center_gain,3);
        data->height_discard = data->TS_usr_cfg[value].vxtrusurround.height_discard;
        data->discard = data->TS_usr_cfg[value].vxtrusurround.discard;
        data->heightupmix_enable = data->TS_usr_cfg[value].vxtrusurround.heightupmix_enable;
        data->vxdefinition_enable = data->TS_usr_cfg[value].vxdefinition.enable;
        data->vxdefinition_level = FXP32(data->TS_usr_cfg[value].vxdefinition.level,2);
        data->tbhdx_enable = data->TS_usr_cfg[value].tbhdx.enable;
        data->monomode_enable = data->TS_usr_cfg[value].tbhdx.monomode_enable;
        data->speaker_size = data->TS_usr_cfg[value].tbhdx.speaker_size;
        data->tmporal_gain = FXP32(data->TS_usr_cfg[value].tbhdx.tmporal_gain,2);
        data->max_gain = FXP32(data->TS_usr_cfg[value].tbhdx.max_gain,2);
        data->hpfo = data->TS_usr_cfg[value].tbhdx.hpfo;
        data->highpass_enble = data->TS_usr_cfg[value].tbhdx.highpass_enble;
        if (data->enable) {
            (*pContext->gVirtualxapi.settsx_enable)(data->vxtrusurround_enable);
            (*pContext->gVirtualxapi.settsx_pssvmtrxenable)(data->upmixer_enable);
            (*pContext->gVirtualxapi.settsx_horizontctl)(data->horizontaleffect_control);
            (*pContext->gVirtualxapi.settsx_frntctrl)(data->frontwidening_control);
            (*pContext->gVirtualxapi.settsx_surroundctrl)(data->surroundwidening_control);
            (*pContext->gVirtualxapi.settsx_lprgain)(data->phantomcenter_mixlevel);
            (*pContext->gVirtualxapi.settsx_heightmixcoeff)(data->heightmix_coeff);
            (*pContext->gVirtualxapi.settsx_centergain)(data->center_gain);
            (*pContext->gVirtualxapi.settsx_heightdiscards)(data->height_discard);
            (*pContext->gVirtualxapi.settsx_discard)(data->discard);
            (*pContext->gVirtualxapi.settsx_hghtupmixenable)(data->heightupmix_enable);
            (*pContext->gVirtualxapi.settsx_defenable)(data->vxdefinition_enable);
            (*pContext->gVirtualxapi.settsx_defcontrol)(data->vxdefinition_level);
            (*pContext->gVirtualxapi.settbhdx_enable)(data->tbhdx_enable);
            (*pContext->gVirtualxapi.settbhdx_monomode)(data->monomode_enable);
            (*pContext->gVirtualxapi.settbhdx_spksize)(data->speaker_size);
            (*pContext->gVirtualxapi.settbhdx_tempgain)(data->tmporal_gain);
            (*pContext->gVirtualxapi.settbhdx_maxgain)(data->max_gain);
            (*pContext->gVirtualxapi.settbhdx_hporder)(data->hpfo);
            (*pContext->gVirtualxapi.settbhdx_hpenable)(data->highpass_enble);
             ALOGD("%s: Set Surround Mode %s", __FUNCTION__, VXSurroundModestr[value]);
        } else {
            ALOGD("%s: Set Surround Mode failed", __FUNCTION__);
            return 0;
        }
        break;
    case DTS_PARAM_MBHL_ENABLE_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        value = *(int32_t *)pValue;
        (*pContext->gVirtualxapi.setmbhl_enable)(value);
        ALOGD("%s set mbhl enable is %d", __FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_BYPASS_GAIN_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        scale = *(float *)pValue;
        value = FXP32(scale,2);
        (*pContext->gVirtualxapi.setmbhl_bypassgain)(value);
        ALOGD("%s set mbhl baypss gain is %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_REFERENCE_LEVEL_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        scale = *(float *)pValue;
        value = FXP32(scale,2);
        (*pContext->gVirtualxapi.setmbhl_reflevel)(value);
        ALOGD("%s set mbhl reflevel is %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_VOLUME_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        scale = *(float *)pValue;
        value = FXP32(scale,2);
        (*pContext->gVirtualxapi.setmbhl_volume)(value);
        ALOGD("%s set mbhl volume is %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_VOLUME_STEP_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        value = *(int32_t *)pValue;
        (*pContext->gVirtualxapi.setmbhl_volumestep)(value);
        ALOGD("%s set mbhl volumestep is %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_BALANCE_STEP_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        value = *(int32_t *)pValue;
        (*pContext->gVirtualxapi.setmbhl_balancestep)(value);
        ALOGD("%s set mbhl balancestep is %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_OUTPUT_GAIN_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        scale = *(float *)pValue;
        value = FXP32(scale,2);
        (*pContext->gVirtualxapi.setmbhl_outputgain)(value);
        ALOGD("%s set mbhl outputgain is %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_MODE_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        value = *(int32_t *)pValue;
        (*pContext->gVirtualxapi.setmbhl_mode)(value);
        ALOGD("%s set mbhl mode is %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_PROCESS_DISCARD_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        value = *(int32_t *)pValue;
        (*pContext->gVirtualxapi.setmbhl_processdiscard)(value);
        ALOGD("%s set mbhl process discard is %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_CROSS_LOW_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        value = *(int32_t *)pValue;
        (*pContext->gVirtualxapi.setmbhl_lowcross)(value);
        ALOGD("%s set mbhl lowcross is %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_CROSS_MID_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        value = *(int32_t *)pValue;
        (*pContext->gVirtualxapi.setmbhl_crossmid)(value);
        ALOGD("%s set mbhl crossmid is %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_COMP_ATTACK_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        value = *(int32_t *)pValue;
        (*pContext->gVirtualxapi.setmbhl_compattrack)(value);
        ALOGD("%s set mbhl compattrack is %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_COMP_LOW_RELEASE_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        value = *(int32_t *)pValue;
        (*pContext->gVirtualxapi.setmbhl_lowrelease)(value);
        ALOGD("%s set mbhl lowrelease is %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_COMP_LOW_RATIO_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        scale = *(float *)pValue;
        value = FXP32(scale,6);
        (*pContext->gVirtualxapi.setmbhl_complowratio)(value);
        ALOGD("%s set mbhl complowratio is %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_COMP_LOW_THRESH_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        scale = *(float *)pValue;
        value = FXP32(scale,5);
        (*pContext->gVirtualxapi.setmbhl_complowthresh)(value);
        ALOGD("%s set mbhl complowthresh is %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_COMP_LOW_MAKEUP_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        scale = *(float *)pValue;
        value = FXP32(scale,5);
        (*pContext->gVirtualxapi.setmbhl_lowmakeup)(value);
        ALOGD("%s set mbhl lowmakeup is %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_COMP_MID_RELEASE_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        value = *(int32_t *)pValue;
        (*pContext->gVirtualxapi.setmbhl_compmidrelease)(value);
        ALOGD("%s set mbhl compmidrelease is %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_COMP_MID_RATIO_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        scale = *(float *)pValue;
        value = FXP32(scale,6);
        (*pContext->gVirtualxapi.setmbhl_midratio)(value);
        ALOGD("%s set mbhl midratio is %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_COMP_MID_THRESH_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        scale = *(float *)pValue;
        value = FXP32(scale,5);
        (*pContext->gVirtualxapi.setmbhl_compmidthresh)(value);
        ALOGD("%s set mbhl compmidthresh is %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_COMP_MID_MAKEUP_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        scale = *(float *)pValue;
        value = FXP32(scale,5);
        (*pContext->gVirtualxapi.setmbhl_compmidmakeup)(value);
        ALOGD("%s set mbhl compmidmakeup is %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_COMP_HIGH_RELEASE_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        value = *(int32_t *)pValue;
        (*pContext->gVirtualxapi.setmbhl_comphighrelease)(value);
        ALOGD("%s set mbhl comphighrelease is %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_COMP_HIGH_RATIO_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        scale = *(float *)pValue;
        value = FXP32(scale,6);
        (*pContext->gVirtualxapi.setmbhl_comphighratio)(value);
        ALOGD("%s set mbhl comphighratio is %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_COMP_HIGH_THRESH_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        scale = *(float *)pValue;
        value = FXP32(scale,5);
        (*pContext->gVirtualxapi.setmbhl_comphighthresh)(value);
        ALOGD("%s set mbhl comphighthresh is %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_COMP_HIGH_MAKEUP_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        scale = *(float *)pValue;
        value = FXP32(scale,5);
        (*pContext->gVirtualxapi.setmbhl_comphighmakeup)(value);
        ALOGD("%s set mbhl comphighmakeup is %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_BOOST_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        scale = *(float *)pValue;
        value = FXP32(scale,11);
        (*pContext->gVirtualxapi.setmbhl_boost)(value);
        ALOGD("%s set mbhl boost is %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_THRESHOLD_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        scale = *(float *)pValue;
        value = FXP32(scale,2);
        (*pContext->gVirtualxapi.setmbhl_threshold)(value);
        ALOGD("%s set mbhl threshold is %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_SLOW_OFFSET_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        scale = *(float *)pValue;
        value = FXP32(scale,3);
        (*pContext->gVirtualxapi.setmbhl_slowoffset)(value);
        ALOGD("%s set mbhl slowoffset %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_FAST_ATTACK_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        scale = *(float *)pValue;
        value = FXP32(scale,5);
        (*pContext->gVirtualxapi.setmbhl_fastattack)(value);
        ALOGD("%s set mbhl fastattackt %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_FAST_RELEASE_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        value = *(int32_t *)pValue;
        (*pContext->gVirtualxapi.setmbhl_fastrelease)(value);
        ALOGD("%s set mbhl fastrelease %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_SLOW_ATTACK_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        value = *(int32_t *)pValue;
        (*pContext->gVirtualxapi.setmbhl_slowattrack)(value);
        ALOGD("%s set mbhl slowattrack %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_SLOW_RELEASE_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        value = *(int32_t *)pValue;
        (*pContext->gVirtualxapi.setmbhl_slowrelease)(value);
        ALOGD("%s set mbhl slowrelease %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_DELAY_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        value = *(int32_t *)pValue;
        (*pContext->gVirtualxapi.setmbhl_delay)(value);
        ALOGD("%s set mbhl delay %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_ENVELOPE_FREQUENCY_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        value = *(int32_t *)pValue;
        (*pContext->gVirtualxapi.setmbhl_envelopefre)(value);
        ALOGD("%s set mbhl envelopefre %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_TBHDX_ENABLE_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        value = *(int32_t *)pValue;
        (*pContext->gVirtualxapi.settbhdx_enable)(value);
        ALOGD("%s set tbhdx enable %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_TBHDX_MONO_MODE_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        value = *(int32_t *)pValue;
        (*pContext->gVirtualxapi.settbhdx_monomode)(value);
        ALOGD("%s set tbhdx monomode %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_TBHDX_MAXGAIN_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        scale = *(float *)pValue;
        value = FXP32(scale,2);
        (*pContext->gVirtualxapi.settbhdx_maxgain)(value);
        ALOGD("%s set tbhdx maxgain %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_TBHDX_SPKSIZE_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        value = *(int32_t *)pValue;
        (*pContext->gVirtualxapi.settbhdx_spksize)(value);
        ALOGD("%s set tbhdx spksize %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_TBHDX_HP_ENABLE_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        value = *(int32_t *)pValue;
        (*pContext->gVirtualxapi.settbhdx_hpenable)(value);
        ALOGD("%s set tbhdx hpenable %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_TBHDX_TEMP_GAIN_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        scale = *(float *)pValue;
        value = FXP32(scale,2);
        (*pContext->gVirtualxapi.settbhdx_tempgain)(value);
        ALOGD("%s set tbhdx tempgain %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_TBHDX_PROCESS_DISCARD_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        value = *(int32_t *)pValue;
        (*pContext->gVirtualxapi.settbhdx_processdiscard)(value);
        ALOGD("%s set tbhdx process discard %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_TBHDX_HPORDER_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        value = *(int32_t *)pValue;
        (*pContext->gVirtualxapi.settbhdx_hporder)(value);
        ALOGD("%s set tbhdx hporder %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_VX_ENABLE_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        value = *(int32_t *)pValue;
        (*pContext->gVirtualxapi.setvxlib1_enable)(value);
        ALOGD("%s set vxlib1 enable %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_VX_INPUT_MODE_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        value = *(int32_t *)pValue;
        if (value > 1)
            value = 1;
        pContext->ch_num = value; // here to set input ch num
        (*pContext->gVirtualxapi.setvxlib1_inmode)(value);
        ALOGD("%s set vxlib1 inmode %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_VX_OUTPUT_MODE_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        value = *(int32_t *)pValue;
        (*pContext->gVirtualxapi.setvxlib1_outmode)(value);
        ALOGD("%s set vxlib1 outmode %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_VX_HEADROOM_GAIN_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        scale = *(float *)pValue;
        value = FXP32(scale,2);
        (pContext->gVirtualxapi.setvxlib1_heardroomgain)(value);
        ALOGD("%s set vxlib1 heardroomgain %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_VX_PROC_OUTPUT_GAIN_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        scale = *(float *)pValue;
        value = FXP32(scale,4);
        (*pContext->gVirtualxapi.setvxlib1_procoutgain)(value);
        ALOGD("%s set vxlib1 procoutgain %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_TSX_ENABLE_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        value = *(int32_t *)pValue;
        (*pContext->gVirtualxapi.settsx_enable)(value);
        ALOGD("%s set tsx enable %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_TSX_PASSIVEMATRIXUPMIX_ENABLE_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        value = *(int32_t *)pValue;
        (*pContext->gVirtualxapi.settsx_pssvmtrxenable)(value);
        ALOGD("%s set tsx pssvmtrxenable %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_TSX_HEIGHT_UPMIX_ENABLE_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        value = *(int32_t *)pValue;
        (*pContext->gVirtualxapi.settsx_hghtupmixenable)(value);
        ALOGD("%s set tsx hghtupmixenable %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_TSX_LPR_GAIN_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        scale = *(float *)pValue;
        value = FXP32(scale,3);
        (*pContext->gVirtualxapi.settsx_lprgain)(value);
        ALOGD("%s set tsx lprgain %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_TSX_CENTER_GAIN_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        scale = *(float *)pValue;
        value = FXP32(scale,3);
        (pContext->gVirtualxapi.settsx_centergain)(value);
        ALOGD("%s set tsx centergain %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_TSX_HORIZ_VIR_EFF_CTRL_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        value = *(int32_t *)pValue;
        (*pContext->gVirtualxapi.settsx_horizontctl)(value);
        ALOGD("%s set tsx horizontctl %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_TSX_HEIGHTMIX_COEFF_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        scale = *(float *)pValue;
        value = FXP32(scale,3);
        (*pContext->gVirtualxapi.settsx_heightmixcoeff)(value);
        ALOGD("%s set tsx heightmixcoeff %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_TSX_PROCESS_DISCARD_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        value = *(int32_t *)pValue;
        (*pContext->gVirtualxapi.settsx_discard)(value);
        ALOGD("%s set tsx discard %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_TSX_HEIGHT_DISCARD_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        value = *(int32_t *)pValue;
        (*pContext->gVirtualxapi.settsx_heightdiscards)(value);
        ALOGD("%s set tsx heightdiscards %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_TSX_FRNT_CTRL_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        scale = *(float *)pValue;
        value = FXP32(scale,3);
        (*pContext->gVirtualxapi.settsx_frntctrl)(value);
        ALOGD("%s set tsx frntctrl %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_TSX_SRND_CTRL_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        scale = *(float *)pValue;
        value = FXP32(scale,3);
        (*pContext->gVirtualxapi.settsx_surroundctrl)(value);
        ALOGD("%s set tsx surroundctrl %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_VX_DC_ENABLE_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        value = *(int32_t *)pValue;
        (*pContext->gVirtualxapi.settsx_dcenable)(value);
        ALOGD("%s set tsx dcenable %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_VX_DC_CONTROL_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        scale = *(float *)pValue;
        value = FXP32(scale,2);
        (*pContext->gVirtualxapi.settsx_dccontrol)(value);
        ALOGD("%s set tsx dccontrol %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_VX_DEF_ENABLE_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        value = *(int32_t *)pValue;
        (*pContext->gVirtualxapi.settsx_defenable)(value);
        ALOGD("%s set tsx defenable %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_VX_DEF_CONTROL_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        scale = *(float *)pValue;
        value = FXP32(scale,2);
        (*pContext->gVirtualxapi.settsx_defcontrol)(value);
        ALOGD("%s set tsx defcontrol %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_LOUDNESS_CONTROL_ENABLE_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        value = *(int32_t *)pValue;
        (*pContext->gVirtualxapi.settruvolume_ctren)(value);
        ALOGD("%s set truvolume ctren %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_LOUDNESS_CONTROL_TARGET_LOUDNESS_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        value = *(int32_t *)pValue;
        (*pContext->gVirtualxapi.settruvolume_ctrtarget)(value);
        ALOGD("%s set truvolume ctrtarget %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_LOUDNESS_CONTROL_PRESET_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        value = *(int32_t *)pValue;
        (*pContext->gVirtualxapi.settruvolume_ctrpreset)(value);
        ALOGD("%s set truvolume ctrpreset %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_LOUDNESS_CONTROL_IO_MODE_I32:
         if (!pContext->gVXLibHandler) {
            return 0;
        }
        value = *(int32_t *)pValue;
        (*pContext->gVirtualxapi.setlc_inmode)(value);
        ALOGD("%s set lc inmode %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_APP_FRT_LOWCROSS_F32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        pContext->lowcrossfreq = *(float*)pValue;
        (*pContext->gVirtualxapi.setmbhl_FilterDesign)(pContext->lowcrossfreq,pContext->midcrossfreq);
        ALOGD("%s set mbhl filter design low freq %f and mid freq %f",__FUNCTION__,pContext->lowcrossfreq,pContext->midcrossfreq);
        break;
    case DTS_PARAM_MBHL_APP_FRT_MIDCROSS_F32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        pContext->midcrossfreq = *(float*)pValue;
        (*pContext->gVirtualxapi.setmbhl_FilterDesign)(pContext->lowcrossfreq,pContext->midcrossfreq);
        ALOGD("%s set mbhl filter design low freq %f and mid freq %f",__FUNCTION__,pContext->lowcrossfreq,pContext->midcrossfreq);
        break;
    case DTS_PARAM_TBHDX_APP_SPKSIZE_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        pContext->spksize = *(int32_t *)pValue;
        (*pContext->gVirtualxapi.gettbhdx_tempgain)(&basslvl);
        (*pContext->gVirtualxapi.settbhd_FilterDesign)(pContext->spksize,((float)basslvl)/1073741824.0f,pContext->hpratio,pContext->extbass);
        ALOGD("%s set tbhd filter design spksize %d  hpratio %f extbass %f",__FUNCTION__,pContext->spksize,pContext->hpratio,pContext->extbass);
        break;
    case DTS_PARAM_TBHDX_APP_HPRATIO_F32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        pContext->hpratio = *(float *)pValue;
        (*pContext->gVirtualxapi.gettbhdx_tempgain)(&basslvl);
        (*pContext->gVirtualxapi.settbhd_FilterDesign)(pContext->spksize,((float)basslvl)/1073741824.0f,pContext->hpratio,pContext->extbass);
        ALOGD("%s set tbhd filter design spksize %d  hpratio %f extbass %f",__FUNCTION__,pContext->spksize,pContext->hpratio,pContext->extbass);
        break;
    case DTS_PARAM_TBHDX_APP_EXTBASS_F32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        pContext->extbass = *(float *)pValue;
        (*pContext->gVirtualxapi.gettbhdx_tempgain)(&basslvl);
        (*pContext->gVirtualxapi.settbhd_FilterDesign)(pContext->spksize,((float)basslvl)/1073741824.0f,pContext->hpratio,pContext->extbass);
        ALOGD("%s set tbhd filter design spksize %d  hpratio %f extbass %f",__FUNCTION__,pContext->spksize,pContext->hpratio,pContext->extbass);
        break;
    case DTS_PARAM_AEQ_ENABLE_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        value = *(int32_t *)pValue;
        (*pContext->gVirtualxapi.seteq_enable)(value);
        ALOGD("%s set eq enable %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_AEQ_DISCARD_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        value = *(int32_t *)pValue;
        (*pContext->gVirtualxapi.seteq_discard)(value);
        ALOGD("%s set eq discard %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_AEQ_INPUT_GAIN_I16:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        scale = *(float *)pValue;
        value = FXP16(scale,1);
        if (value > 32767)
            value = 32767;
        (*pContext->gVirtualxapi.seteq_inputG)(value);
        ALOGD("%s set eq input gain %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_AEQ_OUTPUT_GAIN_I16:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        scale = *(float *)pValue;
        value = FXP16(scale,1);
        if (value > 32767)
            value = 32767;
        (*pContext->gVirtualxapi.seteq_outputG)(value);
        ALOGD("%s set eq output gain %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_AEQ_BYPASS_GAIN_I16:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        scale = *(float *)pValue;
        value = FXP16(scale,1);
        if (value > 32767)
            value = 32767;
        (*pContext->gVirtualxapi.seteq_bypassG)(value);
        ALOGD("%s set eq bypass gain %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_AEQ_LR_LINK_I32:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        value = *(int32_t*)pValue;
        (*pContext->gVirtualxapi.seteq_band)(0,(void*)&value);
        ALOGD("%s set eq link flag is %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_AEQ_BAND_Fre:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        p = (float *)pValue;
        (*pContext->gVirtualxapi.seteq_band)(1,(void*)p);
        break;
    case DTS_PARAM_AEQ_BAND_Gain:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        p = (float *)pValue;
        (*pContext->gVirtualxapi.seteq_band)(2,(void*)p);
        break;
    case DTS_PARAM_AEQ_BAND_Q:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        p = (float *)pValue;
        (*pContext->gVirtualxapi.seteq_band)(3,(void*)p);
        break;
    case DTS_PARAM_AEQ_BAND_type:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        p = (float *)pValue;
        (*pContext->gVirtualxapi.seteq_band)(4,(void*)p);
        break;
    case DTS_PARAM_CHANNEL_NUM:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        value = *(int32_t *)pValue;
        if (value == 6)
            property_set("media.libplayer.dtsMulChPcm","true");
        else
            property_set("media.libplayer.dtsMulChPcm","false");
        break;
    case AUDIO_DTS_PARAM_TYPE_NONE:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        p = (float *)pValue;
        (*pContext->gVirtualxapi.setvxlib1_enable)((int32_t)*p++);
        (*pContext->gVirtualxapi.settsx_dcenable)((int32_t)*p);
        break;
    case AUDIO_DTS_PARAM_TYPE_TRU_SURROUND:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        p = (float *)pValue;
        value = (int32_t)*p++;
        ALOGV("value1 is %d",value);
        if (value > 1)
            value = 1;
        else if (value < 0)
            value = 0;
        (*pContext->gVirtualxapi.setvxlib1_enable)(value);
        scale = *p++;
        if (scale < 0.125)
            scale = 0.125;
        else if (scale > 1.0)
            scale = 1.0;
        ALOGV("scale2 is %f",scale);
        value = FXP32(scale,2);
        (pContext->gVirtualxapi.setvxlib1_heardroomgain)(value);
        scale = *p++;
        if (scale < 0.5)
            scale = 0.5;
        else if (scale > 4.0)
            scale = 4.0;
        ALOGV("scale3 is %f",scale);
        value = FXP32(scale,4);
        //(*pContext->gVirtualxapi.setvxlib1_procoutgain)(value);
        value = (int32_t)*p++;
        if (value < 0)
            value = 0;
        else if (value > 1)
            value = 1;
        ALOGV("value4 is %d",value);
        (*pContext->gVirtualxapi.settsx_enable)(value);
        value = (int32_t)*p++;
        if (value < 0)
            value = 0;
        else if (value > 1)
            value = 1;
        ALOGV("value5 is %d",value);
        (*pContext->gVirtualxapi.settsx_pssvmtrxenable)(value);
        value = (int32_t)*p++;
        if (value < 0)
            value = 0;
        else if (value > 1)
            value = 1;
        ALOGV("value6 is %d",value);
        (*pContext->gVirtualxapi.settsx_horizontctl)(value);
        scale = *p++;
        if (scale < 0.5)
            scale = 0.5;
        else if (scale > 2.0)
            scale = 2.0;
        ALOGV("scale7 is %f",scale);
        value = FXP32(scale,3);
        (*pContext->gVirtualxapi.settsx_frntctrl)(value);
        scale = *p;
        if (scale < 0.5)
            scale = 0.5;
        else if (scale > 2.0)
            scale = 2.0;
        ALOGV("scale8 is %f",scale);
        value = FXP32(scale,3);
        (*pContext->gVirtualxapi.settsx_surroundctrl)(value);
        break;
    case AUDIO_DTS_PARAM_TYPE_CC3D:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        p = (float *)pValue;
        value = (int32_t)*p++;
        if (value < 0)
            value = 0;
        else if (value > 1)
            value = 1;
        ALOGV("value1 is %d",value);
        (*pContext->gVirtualxapi.setvxlib1_enable)(value);
        scale = *p++;
        if (scale < 0.125)
            scale = 0.125;
        else if (scale > 1.0)
            scale = 1.0;
        ALOGV("scale2 is %f",scale);
        value = FXP32(scale,2);
        (pContext->gVirtualxapi.setvxlib1_heardroomgain)(value);
        scale = *p++;
        if (scale < 0.5)
            scale = 0.5;
        else if (scale > 4.0)
            scale = 4.0;
        ALOGV("scale3 is %f",scale);
        value = FXP32(scale,4);
        //(*pContext->gVirtualxapi.setvxlib1_procoutgain)(value);
        value = (int32_t)*p++;
        if (value < 0)
            value = 0;
        else if (value > 1)
            value = 1;
        ALOGV("value4 is %d",value);
        //(*pContext->gVirtualxapi.settsx_enable)(value);
        value = (int32_t)*p++;
        if (value < 0)
            value = 0;
        else if (value > 1)
            value = 1;
        ALOGV("value5 is %d",value);
        (*pContext->gVirtualxapi.settsx_pssvmtrxenable)(value);
        value = (int32_t)*p++;
        if (value < 0)
            value = 0;
        else if (value > 1)
            value = 1;
        ALOGV("value6 is %d",value);
        (*pContext->gVirtualxapi.settsx_hghtupmixenable)(value);
        value = (int32_t)*p++;
        if (value < 0)
            value = 0;
        else if (value > 1)
            value = 1;
        ALOGV("value7 is %d",value);
        (*pContext->gVirtualxapi.settsx_heightdiscards)(value);
        scale = *p;
        if (scale < 0.5)
            scale = 0.5;
        else if (scale > 2.0)
            scale = 2.0;
        ALOGV("scale8 is %f",scale);
         value = FXP32(scale,3);
        (*pContext->gVirtualxapi.settsx_heightmixcoeff)(value);
        break;
    case AUDIO_DTS_PARAM_TYPE_TRU_BASS:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        p = (float *)pValue;
        value = (int32_t)*p++;
        if (value < 0)
            value = 0;
        else if (value > 1)
            value = 1;
        ALOGV("value1 is %d",value);
        (*pContext->gVirtualxapi.setvxlib1_enable)(value);
        scale = *p++;
        if (scale < 0.125)
            scale = 0.125;
        else if (scale > 1.0)
            scale = 1.0;
        ALOGV("scale2 is %f",scale);
        value = FXP32(scale,2);
        (pContext->gVirtualxapi.setvxlib1_heardroomgain)(value);
        scale = *p++;
        if (scale < 0.5)
            scale = 0.5;
        else if (scale > 4.0)
            scale = 4.0;
        ALOGV("scale3 is %f",scale);
        value = FXP32(scale,4);
        //(*pContext->gVirtualxapi.setvxlib1_procoutgain)(value);
        value = (int32_t)*p++;
        if (value < 0)
            value = 0;
        else if (value > 1)
            value = 1;
        ALOGV("value4 is %d",value);
        (*pContext->gVirtualxapi.settbhdx_enable)(value);
        value = (int32_t)*p++;
        if (value < 0)
            value = 0;
        else if (value > 1)
            value = 1;
        ALOGV("value5 is %d",value);
        (*pContext->gVirtualxapi.settbhdx_monomode)(value);
        value = (int32_t)*p++;
        if (value < 0)
            value = 0;
        else if (value > 12)
            value = 12;
        ALOGV("value6 is %d",value);
        (*pContext->gVirtualxapi.settbhdx_spksize)(value);
        scale = *p++;
        if (scale < 0.0)
            scale = 0.0;
        else if (scale > 1.0)
            scale = 1.0;
        ALOGV("scale7 is %f",scale);
        value = FXP32(scale,2);
        (*pContext->gVirtualxapi.settbhdx_tempgain)(value);
        scale = *p++;
        if (scale < 0.0)
            scale = 0.0;
        else if (scale > 1.0)
            scale = 1.0;
        ALOGV("scale8 is %f",scale);
        value = FXP32(scale,2);
        (*pContext->gVirtualxapi.settbhdx_maxgain)(value);
        value = (int32_t)*p++;
        if (value < 1)
            value = 1;
        else if (value > 8)
            value = 8;
        ALOGV("value9 is %d",value);
        (*pContext->gVirtualxapi.settbhdx_hporder)(value);
        value = (int32_t)*p;
        if (value < 0)
            value = 0;
        else if (value > 1)
            value = 1;
        ALOGV("value10 is %d",value);
        (*pContext->gVirtualxapi.settbhdx_hpenable)(value);
        break;
    case AUDIO_DTS_PARAM_TYPE_TRU_DIALOG:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        p = (float *)pValue;
        value = (int32_t)*p++;
        ALOGV("value1 is %d",value);
        (*pContext->gVirtualxapi.setvxlib1_enable)(value);
        scale = *p++;
        if (scale < 0.125)
            scale = 0.125;
        else if (scale > 1.0)
            scale = 1.0;
        ALOGV("scale2 is %f",scale);
        value = FXP32(scale,2);
        (pContext->gVirtualxapi.setvxlib1_heardroomgain)(value);
        scale = *p++;
        if (scale < 0.5)
            scale = 0.5;
        else if (scale > 4.0)
            scale = 4.0;
        ALOGV("scale3 is %f",scale);
        value = FXP32(scale,4);
        //(*pContext->gVirtualxapi.setvxlib1_procoutgain)(value);
        value = (int32_t)*p++;
        if (value < 0)
            value = 0;
        else if (value > 1)
            value = 1;
        ALOGV("value4 is %d",value);
        //(*pContext->gVirtualxapi.settsx_enable)(value);
        scale = *p++;
        if (scale < 0.0)
            scale = 0.0;
        else if (scale > 2.0)
            scale = 2.0;
        ALOGV("scale5 is %f",scale);
        value = FXP32(scale,3);
        (*pContext->gVirtualxapi.settsx_lprgain)(value);
        scale = *p++;
        if (scale < 1.0)
            scale = 1.0;
        else if (scale > 2.0)
            scale = 2.0;
        ALOGV("scale6 is %f",scale);
        value = FXP32(scale,3);
        (pContext->gVirtualxapi.settsx_centergain)(value);
        value = (int32_t)*p++;
        if (value < 0)
            value = 0;
        else if (value > 1)
            value = 1;
        ALOGV("value7 is %d",value);
        (*pContext->gVirtualxapi.settsx_dcenable)(value);
        scale = *p;
        if (scale < 0.0)
            scale = 0.0;
        else if (scale > 1.0)
            scale = 1.0;
        ALOGV("scale8 is %f",scale);
        value = FXP32(scale,2);
        (*pContext->gVirtualxapi.settsx_dccontrol)(value);
        break;
    case AUDIO_DTS_PARAM_TYPE_DEFINATION:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        p = (float *)pValue;
        value = (int32_t)*p++;
        if (value < 0)
            value = 0;
        else if (value > 1)
            value = 1;
        ALOGV("value1 is %d",value);
        (*pContext->gVirtualxapi.setvxlib1_enable)(value);
        scale = *p++;
        if (scale < 0.125)
            scale = 0.125;
        else if (scale > 1.0)
            scale = 1.0;
        ALOGV("scale2 is %f",scale);
        value = FXP32(scale,2);
        (pContext->gVirtualxapi.setvxlib1_heardroomgain)(value);
        scale = *p++;
        if (scale < 0.5)
            scale = 0.5;
        else if (scale > 4.0)
            scale = 4.0;
        ALOGV("scale3 is %f",scale);
        value = FXP32(scale,4);
        (*pContext->gVirtualxapi.setvxlib1_procoutgain)(value);
        value = (int32_t)*p++;
        if (value < 0)
            value = 0;
        else if (value > 1)
            value = 1;
        ALOGV("value4 is %d",value);
        //(*pContext->gVirtualxapi.settsx_enable)(value);
        value = (int32_t)*p++;
        if (value < 0)
            value = 0;
        else if (value > 1)
            value = 1;
        ALOGV("value5 is %d",value);
        (*pContext->gVirtualxapi.settsx_defenable)(value);
        scale = *p;
        if (scale < 0.0)
            scale = 0.0;
        else if (scale > 1.0)
            scale = 1.0;
        ALOGV("scale6 is %f",scale);
        value = FXP32(scale,2);
        (*pContext->gVirtualxapi.settsx_defcontrol)(value);
        break;
    case AUDIO_DTS_PARAM_TYPE_TRU_VOLUME:
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        p = (float *)pValue;
        value = (int32_t)*p++;
        if (value < 0)
            value = 0;
        else if (value > 1)
            value = 1;
        ALOGV("value1 is %d",value);
        (*pContext->gVirtualxapi.settruvolume_ctren)(value);
        value = (int32_t)*p++;
        if (value < -40)
            value = -40;
        else if (value > 0)
            value = 0;
        ALOGV("value2 is %d",value);
        (*pContext->gVirtualxapi.settruvolume_ctrtarget)(value);
        value = (int32_t)*p;
        if (value < 0)
            value = 0;
        else if (value > 2)
            value = 2;
        ALOGV("value3 is %d",value);
        (*pContext->gVirtualxapi.settruvolume_ctrpreset)(value);
        break;
    default:
        ALOGE("%s: unknown param %08x", __FUNCTION__, param);
        return -EINVAL;
    }
    return 0;
}

int Virtualx_getParameter(vxContext *pContext, void *pParam, size_t *pValueSize,void *pValue)
{
    uint32_t param = *(uint32_t *)pParam;
    int32_t value;
    vxdata *data = &pContext->gvxdata;
    switch (param) {
    case VIRTUALX_PARAM_ENABLE:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        value = data->enable;
        *(uint32_t *) pValue = value;
        ALOGD("%s: Get status -> %s", __FUNCTION__, VXStatusstr[value]);
        break;
    case VIRTUALX_PARAM_DIALOGCLARTY_MODE:
         if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        value = data->dialogclarity_mode;
        *(uint32_t *) pValue = value;
        ALOGD("%s:dialogclarity mode is %s",__FUNCTION__,VXDialogClarityModestr[value]);
        break;
    case VIRTUALX_PARAM_SURROUND_MODE:
         if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        value = data->surround_mode;
        *(uint32_t *) pValue = value;
        ALOGD("%s: Get Surround Mode %s", __FUNCTION__,VXSurroundModestr[data->surround_mode]);
        break;
    case DTS_PARAM_MBHL_ENABLE_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.getmbhl_enable)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get mbhl enable is %d", __FUNCTION__, value);
        break;
     case DTS_PARAM_MBHL_BYPASS_GAIN_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.getmbhl_bypassgain)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get mbhlbypassgain %d", __FUNCTION__, value);
        break;
     case DTS_PARAM_MBHL_REFERENCE_LEVEL_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.getmbhl_reflevel)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get mbhl reflevel %d", __FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_VOLUME_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.getmbhl_volume)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get mbhl volume is %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_VOLUME_STEP_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.getmbhl_volumestep)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get mbhl volumestep is %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_BALANCE_STEP_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.getmbhl_balancestep)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get mbhl balancestep is %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_OUTPUT_GAIN_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.getmbhl_outputgain)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get mbhl outputgain is %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_MODE_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.getmbhl_mode)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get mbhl mode is %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_PROCESS_DISCARD_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.getmbhl_processdiscard)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get mbhl process discard is %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_CROSS_LOW_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.getmbhl_lowcross)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get mbhl lowcross is %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_CROSS_MID_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.getmbhl_crossmid)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get mbhl crossmid is %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_COMP_ATTACK_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.getmbhl_compattrack)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get mbhl compattrack is %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_COMP_LOW_RELEASE_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.getmbhl_lowrelease)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get mbhl lowrelease is %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_COMP_LOW_RATIO_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.getmbhl_complowratio)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get mbhl complowratio is %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_COMP_LOW_THRESH_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.getmbhl_complowthresh)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get mbhl complowthresh is %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_COMP_LOW_MAKEUP_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.getmbhl_lowmakeup)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get mbhl lowmakeup is %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_COMP_MID_RELEASE_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.getmbhl_compmidrelease)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get mbhl compmidrelease is %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_COMP_MID_RATIO_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.getmbhl_midratio)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get mbhl midratio is %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_COMP_MID_THRESH_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.getmbhl_compmidthresh)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get mbhl compmidthresh is %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_COMP_MID_MAKEUP_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.getmbhl_compmidmakeup)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get mbhl compmidmakeup is %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_COMP_HIGH_RELEASE_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }

        value = *(int32_t *)pValue;
        (*pContext->gVirtualxapi.getmbhl_comphighrelease)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get mbhl comphighrelease is %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_COMP_HIGH_RATIO_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.getmbhl_comphighratio)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get mbhl comphighratio is %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_COMP_HIGH_THRESH_I32:
         if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.getmbhl_comphighthresh)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get mbhl comphighthresh is %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_COMP_HIGH_MAKEUP_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.getmbhl_comphighmakeup)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get mbhl comphighmakeup is %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_BOOST_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.getmbhl_boost)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get mbhl boost is %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_THRESHOLD_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.getmbhl_threshold)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get mbhl threshold is %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_SLOW_OFFSET_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.getmbhl_slowoffset)(&value);
        ALOGD("%s get mbhl slowoffset %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_FAST_ATTACK_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.getmbhl_fastattack)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get mbhl fastattackt %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_FAST_RELEASE_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.getmbhl_fastrelease)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get mbhl fastrelease %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_SLOW_ATTACK_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.getmbhl_slowattrack)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get mbhl slowattrack %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_SLOW_RELEASE_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }

        value = *(int32_t *)pValue;
        (*pContext->gVirtualxapi.setmbhl_slowrelease)(value);
        ALOGD("%s set mbhl slowrelease %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_DELAY_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.getmbhl_delay)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get mbhl delay %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_MBHL_ENVELOPE_FREQUENCY_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }

        value = *(int32_t *)pValue;
        (*pContext->gVirtualxapi.getmbhl_envelopefre)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get mbhl envelopefre %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_TBHDX_ENABLE_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.gettbhdx_enable)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get tbhdx enable %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_TBHDX_MONO_MODE_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.gettbhdx_monomode)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get tbhdx monomode %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_TBHDX_MAXGAIN_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.gettbhdx_maxgain)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get tbhdx maxgain %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_TBHDX_SPKSIZE_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.gettbhdx_spksize)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get tbhdx spksize %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_TBHDX_HP_ENABLE_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.gettbhdx_hpenable)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get tbhdx hpenable %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_TBHDX_TEMP_GAIN_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.gettbhdx_tempgain)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get tbhdx tempgain %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_TBHDX_PROCESS_DISCARD_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.getthbdx_processdiscard)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get tbhdx process discard %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_TBHDX_HPORDER_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.gettbhdx_hporder)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get tbhdx hporder %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_VX_ENABLE_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.getvxlib1_enable)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get vxlib1 enable %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_VX_INPUT_MODE_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.getvxlib1_inmode)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get vxlib1 inmode %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_VX_OUTPUT_MODE_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.getvxlib1_outmode)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get vxlib1 outmode %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_VX_HEADROOM_GAIN_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.getvxlib1_heardroomgain)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get vxlib1 heardroomgain %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_VX_PROC_OUTPUT_GAIN_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.getvxlib1_procoutgain)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get vxlib1 procoutgain %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_TSX_ENABLE_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.gettsx_enable)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get tsx enable %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_TSX_PASSIVEMATRIXUPMIX_ENABLE_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.gettsx_pssvmtrxenable)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get tsx pssvmtrxenable %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_TSX_HEIGHT_UPMIX_ENABLE_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.gettsx_hghtupmixenable)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get tsx hghtupmixenable %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_TSX_LPR_GAIN_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.gettsx_lprgain)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get tsx lprgain %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_TSX_CENTER_GAIN_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (pContext->gVirtualxapi.gettsx_centergain)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get tsx centergain %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_TSX_HORIZ_VIR_EFF_CTRL_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.gettsx_horizontctl)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get tsx horizontctl %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_TSX_HEIGHTMIX_COEFF_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.gettsx_heightmixcoeff)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get tsx heightmixcoeff %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_TSX_PROCESS_DISCARD_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.gettsx_discard)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get tsx discard %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_TSX_HEIGHT_DISCARD_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.gettsx_heightdiscards)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get tsx heightdiscards %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_TSX_FRNT_CTRL_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.gettsx_frntctrl)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get tsx frntctrl %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_TSX_SRND_CTRL_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.gettsx_surroundctrl)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get tsx surroundctrl %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_VX_DC_ENABLE_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.gettsx_dcenable)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get tsx dcenable %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_VX_DC_CONTROL_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.gettsx_dccontrol)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get tsx dccontrol %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_VX_DEF_ENABLE_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.gettsx_defenable)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get tsx defenable %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_VX_DEF_CONTROL_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.gettsx_defcontrol)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get tsx defcontrol %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_LOUDNESS_CONTROL_ENABLE_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.gettruvolume_ctren)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get truvolume ctren %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_LOUDNESS_CONTROL_TARGET_LOUDNESS_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.gettruvolume_ctrtarget)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get truvolume ctrtarget %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_LOUDNESS_CONTROL_PRESET_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.gettruvolume_ctrpreset)(&value);
        *(int32_t *) pValue = value;
        ALOGD("%s get truvolume ctrpreset %d",__FUNCTION__, value);
        break;
    case DTS_PARAM_AEQ_ENABLE_I32:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.geteq_enable)(&value);
        *(int32_t *) pValue = value;
        break;
    case DTS_PARAM_AEQ_INPUT_GAIN_I16:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.geteq_inputG)(&value);
        *(int32_t *) pValue = value;
        break;
    case DTS_PARAM_AEQ_OUTPUT_GAIN_I16:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.geteq_outputG)(&value);
        *(int32_t *) pValue = value;
        break;
    case DTS_PARAM_AEQ_BYPASS_GAIN_I16:
        if (*pValueSize < sizeof(uint32_t)) {
            *pValueSize = 0;
            return -EINVAL;
        }
        if (!pContext->gVXLibHandler) {
            return 0;
        }
        (*pContext->gVirtualxapi.geteq_bypassG)(&value);
        *(int32_t *) pValue = value;
        break;
    default:
       ALOGE("%s: unknown param %d", __FUNCTION__, param);
       return -EINVAL;
    }
    return 0;
}

int Virtualx_release(vxContext *pContext)
{
    if (pContext->gVXLibHandler) {
       (*pContext->gVirtualxapi.VX_release)();
    }
    return 0;
}

//-------------------Effect Control Interface Implementation--------------------------

int Virtualx_process(effect_handle_t self, audio_buffer_t *inBuffer, audio_buffer_t *outBuffer)
{
    vxContext *pContext = (vxContext *)self;
    if (pContext == NULL)
        return -EINVAL;

    if (inBuffer == NULL || inBuffer->raw == NULL ||
        outBuffer == NULL || outBuffer->raw == NULL ||
        inBuffer->frameCount != outBuffer->frameCount ||
        inBuffer->frameCount == 0)
        return -EINVAL;

    if (pContext->state != VIRTUALX_STATE_ACTIVE)
        return -ENODATA;

    int16_t  *in   = (int16_t *)inBuffer->raw;
    int16_t  *out  = (int16_t *)outBuffer->raw;
    vxdata   *data = &pContext->gvxdata;
    if (!data->enable || !pContext->gVXLibHandler) {
        for (size_t i = 0; i < inBuffer->frameCount; i++) {
            *out++ = *in++;
            *out++ = *in++;
        }
    } else {
        int32_t blockSize = DTS_VIRTUALX_FRAME_SIZE;
        int32_t blockCount = inBuffer->frameCount / blockSize;
        for (int i = 0; i < blockCount; i++) {
            for (int sampleCount = 0; sampleCount < 256; sampleCount++) {
                if (pContext->ch_num == 0 /*for 2ch process*/) {
                    pContext->ppMappedInCh[0][sampleCount] = (int32_t(*in++)) << 16; // L & R
                    pContext->ppMappedInCh[1][sampleCount] = (int32_t(*in++)) << 16;
                    for (int i = 2; i < 12; i++) {
                        pContext->ppMappedInCh[i][sampleCount] = 0;
                    }
               } else if (pContext->ch_num == 1 /*for 5.1 ch process*/) {
                    for (int i = 0; i < 6; i++) {
                        pContext->ppMappedInCh[i][sampleCount] = (int32_t(*in++)) << 16;
                    }
                    for (int i = 6; i < 12; i++) {
                        pContext->ppMappedInCh[i][sampleCount] = 0;
                    }
                }
            }
            (*pContext->gVirtualxapi.Truvolume_process)(pContext->ppMappedInCh,pContext->ppMappedOutCh);
            (*pContext->gVirtualxapi.VX_process)(pContext->ppMappedOutCh,pContext->ppMappedOutCh);
            (*pContext->gVirtualxapi.MBHL_process)(pContext->ppMappedOutCh,pContext->ppMappedOutCh);
            for (int sampleCount = 0;sampleCount < 256; sampleCount++) {
                *out++  = (int16_t)(pContext->ppMappedOutCh[0][sampleCount] >> 16);
                *out++  = (int16_t)(pContext->ppMappedOutCh[1][sampleCount] >> 16);
            }
        }
    }
    return 0;
}

int Virtualx_command(effect_handle_t self, uint32_t cmdCode, uint32_t cmdSize,
        void *pCmdData, uint32_t *replySize, void *pReplyData)
{
    vxContext * pContext = (vxContext *)self;
    effect_param_t *p;
    int voffset;
    if (pContext == NULL || pContext->state == VIRTUALX_STATE_UNINITIALIZED)
        return -EINVAL;
    switch (cmdCode) {
    case EFFECT_CMD_INIT:
        if (pReplyData == NULL || replySize == NULL || *replySize != sizeof(int))
            return -EINVAL;
        *(int *) pReplyData = Virtualx_init(pContext);
        break;
    case EFFECT_CMD_SET_CONFIG:
        if (pCmdData == NULL || cmdSize != sizeof(effect_config_t) || pReplyData == NULL || replySize == NULL || *replySize != sizeof(int))
            return -EINVAL;
        *(int *) pReplyData = Virtualx_configure(pContext,(effect_config_t *) pCmdData);
        break;
    case EFFECT_CMD_RESET:
        Virtualx_reset(pContext);
        break;
    case EFFECT_CMD_ENABLE:
        if (pReplyData == NULL || replySize == NULL || *replySize != sizeof(int))
            return -EINVAL;
        if (pContext->state != VIRTUALX_STATE_INITIALIZED)
            return -ENOSYS;
        pContext->state = VIRTUALX_STATE_ACTIVE;
        *(int *)pReplyData = 0;
        break;
    case EFFECT_CMD_DISABLE:
        if (pReplyData == NULL || replySize == NULL || *replySize != sizeof(int))
            return -EINVAL;
        if (pContext->state != VIRTUALX_STATE_ACTIVE)
            return -ENOSYS;
        pContext->state = VIRTUALX_STATE_INITIALIZED;
        property_set("media.libplayer.dtsMulChPcm","false");
        *(int *)pReplyData = 0;
        break;
    case EFFECT_CMD_GET_PARAM:
        if (pCmdData == NULL ||
            cmdSize != (int)(sizeof(effect_param_t) + sizeof(uint32_t)) ||
            pReplyData == NULL || replySize == NULL ||
            (*replySize < (int)(sizeof(effect_param_t) + sizeof(uint32_t) + sizeof(uint32_t)) &&
            *replySize < (int)(sizeof(effect_param_t) + sizeof(uint32_t) + 8 * sizeof(float)) &&
            *replySize < (int)(sizeof(effect_param_t) + sizeof(uint32_t) + 10 * sizeof(float)) &&
            *replySize < (int)(sizeof(effect_param_t) + sizeof(uint32_t) + 6 * sizeof(float)) &&
            *replySize < (int)(sizeof(effect_param_t) + sizeof(uint32_t) + 3 * sizeof(float)) &&
            *replySize < (sizeof(effect_param_t) + sizeof(uint32_t) + (pContext->gvxdata.aeq_bandnum) * sizeof(float))))
            return -EINVAL;
        p = (effect_param_t *)pCmdData;
        memcpy(pReplyData, pCmdData, sizeof(effect_param_t) + p->psize);
        p = (effect_param_t *)pReplyData;

        voffset = ((p->psize - 1) / sizeof(int32_t) + 1) * sizeof(int32_t);

        p->status = Virtualx_getParameter(pContext, p->data, (size_t  *)&p->vsize, p->data + voffset);
        *replySize = sizeof(effect_param_t) + voffset + p->vsize;
        break;
    case EFFECT_CMD_SET_PARAM:
        if (pCmdData == NULL ||
            (cmdSize != (int)(sizeof(effect_param_t) + sizeof(uint32_t) + sizeof(uint32_t)) &&
            cmdSize != (int)(sizeof(effect_param_t) + sizeof(uint32_t) + 8 * sizeof(float)) &&
            cmdSize != (int)(sizeof(effect_param_t) + sizeof(uint32_t) + 10 * sizeof(float)) &&
            cmdSize != (int)(sizeof(effect_param_t) + sizeof(uint32_t) + 6 * sizeof(float)) &&
            cmdSize != (int)(sizeof(effect_param_t) + sizeof(uint32_t) + 3 * sizeof(float)) &&
            cmdSize != (sizeof(effect_param_t) + sizeof(uint32_t) + (pContext->gvxdata.aeq_bandnum) * sizeof(float))) ||
            pReplyData == NULL || replySize == NULL || *replySize != sizeof(int32_t)) {
                ALOGE("%s: EFFECT_CMD_SET_PARAM cmd size error!", __FUNCTION__);
                return -EINVAL;
        }
        p = (effect_param_t *)pCmdData;
        if (p->psize != sizeof(uint32_t) || (p->vsize != sizeof(uint32_t) && p->vsize != 8 * sizeof(float) &&
            p->vsize != 10 * sizeof(float) && p->vsize != 6 * sizeof(float) && p->vsize != 3 * sizeof(float) &&
            p->vsize != (pContext->gvxdata.aeq_bandnum) * sizeof(float))) {
            ALOGE("%s: EFFECT_CMD_SET_PARAM value size error!", __FUNCTION__);
            *(int32_t *)pReplyData = -EINVAL;
            break;
        }
        *(int *)pReplyData = Virtualx_setParameter(pContext, (void *)p->data, p->data + p->psize);
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

int Virtualx_getDescriptor(effect_handle_t self, effect_descriptor_t *pDescriptor)
{
    vxContext * pContext = (vxContext *) self;

    if (pContext == NULL || pDescriptor == NULL) {
        ALOGE("%s: invalid param", __FUNCTION__);
        return -EINVAL;
    }

    *pDescriptor = VirtualxDescriptor;

    return 0;
}

//-------------------- Effect Library Interface Implementation------------------------

int VirtualxLib_Create(const effect_uuid_t *uuid, int32_t sessionId __unused, int32_t ioId __unused, effect_handle_t *pHandle)
{
    if (pHandle == NULL || uuid == NULL)
        return -EINVAL;

    if (memcmp(uuid, &VirtualxDescriptor.uuid, sizeof(effect_uuid_t)) != 0)
        return -EINVAL;

    vxContext *pContext = new vxContext;
    if (!pContext) {
        ALOGE("%s: alloc vxContext failed", __FUNCTION__);
        return -EINVAL;
    }
    memset(pContext, 0, sizeof(vxContext));
    memcpy((void *) & pContext->gvxdata.DC_usr_cfg[0], (void *) &default_DC_cfg, sizeof(pContext->gvxdata.DC_usr_cfg[0]));
    memcpy((void *) & pContext->gvxdata.TS_usr_cfg[0], (void *) &default_TS_cfg, sizeof(pContext->gvxdata.TS_usr_cfg[0]));
    memcpy((void *) & pContext->gvxdata.vxcfg,(void *) & default_Virtualxcfg,sizeof(pContext->gvxdata.vxcfg));

    if (Virtualx_load_ini_file(pContext) < 0) {
        ALOGE("%s: Load INI File faied, use default param", __FUNCTION__);
        pContext->gvxdata.enable = 1;
    }

    if (Virtualx_load_lib(pContext) < 0) {
        property_set("media.libplayer.dtsMulChPcm","false");
        ALOGE("%s: Load Library File faied", __FUNCTION__);
    }

    pContext->itfe = &VirtualxInterface;
    pContext->state = VIRTUALX_STATE_UNINITIALIZED;

    *pHandle = (effect_handle_t)pContext;

    pContext->state = VIRTUALX_STATE_INITIALIZED;

    ALOGD("%s: %p", __FUNCTION__, pContext);

    return 0;
}

int VirtualxLib_Release(effect_handle_t handle)
{
    vxContext * pContext = (vxContext *)handle;
    if (pContext == NULL)
        return -EINVAL;
    Virtualx_release(pContext);
    unload_Virtualx_lib(pContext);
    property_set("media.libplayer.dtsMulChPcm","false");
    pContext->state = VIRTUALX_STATE_UNINITIALIZED;

    delete pContext;
    ALOGD("VirtualxLib_Release");
    return 0;
}

int VirtualxLib_GetDescriptor(const effect_uuid_t *uuid, effect_descriptor_t *pDescriptor)
{
    if (pDescriptor == NULL || uuid == NULL) {
        ALOGE("%s: called with NULL pointer", __FUNCTION__);
        return -EINVAL;
    }
    if (memcmp(uuid, &VirtualxDescriptor.uuid, sizeof(effect_uuid_t)) == 0) {
        *pDescriptor = VirtualxDescriptor;
        return 0;
    }
    return  -EINVAL;
}

// effect_handle_t interface implementation for Virtualx effect
const struct effect_interface_s VirtualxInterface = {
        Virtualx_process,
        Virtualx_command,
        Virtualx_getDescriptor,
        NULL,
};

audio_effect_library_t AUDIO_EFFECT_LIBRARY_INFO_SYM = {
    .tag = AUDIO_EFFECT_LIBRARY_TAG,
    .version = EFFECT_LIBRARY_API_VERSION,
    .name = "VirtualX",
    .implementor = "DTS Labs",
    .create_effect = VirtualxLib_Create,
    .release_effect = VirtualxLib_Release,
    .get_descriptor = VirtualxLib_GetDescriptor,
};

};//extern c
