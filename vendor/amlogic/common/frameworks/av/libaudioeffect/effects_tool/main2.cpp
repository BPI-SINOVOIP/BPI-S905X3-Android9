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

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
//#include <syslog.h>
#include <utils/Log.h>
#include <errno.h>
#include <media/AudioEffect.h>
#include <media/AudioSystem.h>
#include <media/AudioParameter.h>
#include <string.h>


#ifdef LOG
#undef LOG
#endif
#define LOG(x...) printf("[AudioEffect] " x)
#define LSR (1)

using namespace android;

//-------UUID------------------------------------------
typedef enum {
    EFFECT_BALANCE = 0,
    EFFECT_SRS,
    EFFECT_TREBLEBASS,
    EFFECT_HPEQ,
    EFFECT_AVL,
    EFFECT_GEQ,
    EFFECT_VIRTUALX,
    EFFECT_MAX,
} EFFECT_params;

effect_uuid_t gEffectStr[] = {
    {0x6f33b3a0, 0x578e, 0x11e5, 0x892f, {0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b}}, // 0:Balance
    {0x8a857720, 0x0209, 0x11e2, 0xa9d8, {0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b}}, // 1:TruSurround
    {0x76733af0, 0x2889, 0x11e2, 0x81c1, {0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66}}, // 2:TrebleBass
    {0x049754aa, 0xc4cf, 0x439f, 0x897e, {0x37, 0xdd, 0x0c, 0x38, 0x11, 0x20}}, // 3:Hpeq
    {0x08246a2a, 0xb2d3, 0x4621, 0xb804, {0x42, 0xc9, 0xb4, 0x78, 0xeb, 0x9d}}, // 4:Avl
    {0x2e2a5fa6, 0xcae8, 0x45f5, 0xbb70, {0xa2, 0x9c, 0x1f, 0x30, 0x74, 0xb2}}, // 5:Geq
    {0x61821587, 0xce3c, 0x4aac, 0x9122, {0x86, 0xd8, 0x74, 0xea, 0x1f, 0xb1}}, // 6:Virtualx
};

//-------------Virtualx parameter--------------------------
typedef struct Virtualx_param_s {
    effect_param_t param;
    uint32_t command;
    union {
        int32_t v;
        float f;
    };
} Virtualx_param_t;
typedef enum {
    /*Tuning interface*/
    DTS_PARAM_MBHL_ENABLE_I32 = 0,
    DTS_PARAM_MBHL_BYPASS_GAIN_I32,
    DTS_PARAM_MBHL_REFERENCE_LEVEL_I32,
    DTS_PARAM_MBHL_VOLUME_I32,
    DTS_PARAM_MBHL_VOLUME_STEP_I32,
    DTS_PARAM_MBHL_BALANCE_STEP_I32,
    DTS_PARAM_MBHL_OUTPUT_GAIN_I32,
    DTS_PARAM_MBHL_MODE_I32,
    DTS_PARAM_MBHL_PROCESS_DISCARD_I32,
    DTS_PARAM_MBHL_CROSS_LOW_I32,
    DTS_PARAM_MBHL_CROSS_MID_I32,
    DTS_PARAM_MBHL_COMP_ATTACK_I32,
    DTS_PARAM_MBHL_COMP_LOW_RELEASE_I32,
    DTS_PARAM_MBHL_COMP_LOW_RATIO_I32,
    DTS_PARAM_MBHL_COMP_LOW_THRESH_I32,
    DTS_PARAM_MBHL_COMP_LOW_MAKEUP_I32,
    DTS_PARAM_MBHL_COMP_MID_RELEASE_I32,
    DTS_PARAM_MBHL_COMP_MID_RATIO_I32,
    DTS_PARAM_MBHL_COMP_MID_THRESH_I32,
    DTS_PARAM_MBHL_COMP_MID_MAKEUP_I32,
    DTS_PARAM_MBHL_COMP_HIGH_RELEASE_I32,
    DTS_PARAM_MBHL_COMP_HIGH_RATIO_I32,
    DTS_PARAM_MBHL_COMP_HIGH_THRESH_I32,
    DTS_PARAM_MBHL_COMP_HIGH_MAKEUP_I32,
    DTS_PARAM_MBHL_BOOST_I32,
    DTS_PARAM_MBHL_THRESHOLD_I32,
    DTS_PARAM_MBHL_SLOW_OFFSET_I32,
    DTS_PARAM_MBHL_FAST_ATTACK_I32,
    DTS_PARAM_MBHL_FAST_RELEASE_I32,
    DTS_PARAM_MBHL_SLOW_ATTACK_I32,
    DTS_PARAM_MBHL_SLOW_RELEASE_I32,
    DTS_PARAM_MBHL_DELAY_I32,
    DTS_PARAM_MBHL_ENVELOPE_FREQUENCY_I32,
    DTS_PARAM_MBHL_APP_FRT_LOWCROSS_F32,
    DTS_PARAM_MBHL_APP_FRT_MIDCROSS_F32,
    DTS_PARAM_TBHDX_ENABLE_I32,
    DTS_PARAM_TBHDX_MONO_MODE_I32,
    DTS_PARAM_TBHDX_MAXGAIN_I32,
    DTS_PARAM_TBHDX_SPKSIZE_I32,
    DTS_PARAM_TBHDX_HP_ENABLE_I32,
    DTS_PARAM_TBHDX_TEMP_GAIN_I32,
    DTS_PARAM_TBHDX_PROCESS_DISCARD_I32,
    DTS_PARAM_TBHDX_HPORDER_I32,
    DTS_PARAM_TBHDX_APP_SPKSIZE_I32,
    DTS_PARAM_TBHDX_APP_HPRATIO_F32,
    DTS_PARAM_TBHDX_APP_EXTBASS_F32,
    DTS_PARAM_VX_ENABLE_I32,
    DTS_PARAM_VX_INPUT_MODE_I32,
    DTS_PARAM_VX_OUTPUT_MODE_I32,
    DTS_PARAM_VX_HEADROOM_GAIN_I32,
    DTS_PARAM_VX_PROC_OUTPUT_GAIN_I32,
    DTS_PARAM_VX_REFERENCE_LEVEL_I32,
    DTS_PARAM_TSX_ENABLE_I32,
    DTS_PARAM_TSX_PASSIVEMATRIXUPMIX_ENABLE_I32,
    DTS_PARAM_TSX_HEIGHT_UPMIX_ENABLE_I32,
    DTS_PARAM_TSX_LPR_GAIN_I32,
    DTS_PARAM_TSX_CENTER_GAIN_I32,
    DTS_PARAM_TSX_HORIZ_VIR_EFF_CTRL_I32,
    DTS_PARAM_TSX_HEIGHTMIX_COEFF_I32,
    DTS_PARAM_TSX_PROCESS_DISCARD_I32,
    DTS_PARAM_TSX_HEIGHT_DISCARD_I32,
    DTS_PARAM_TSX_FRNT_CTRL_I32,
    DTS_PARAM_TSX_SRND_CTRL_I32,
    DTS_PARAM_VX_DC_ENABLE_I32,
    DTS_PARAM_VX_DC_CONTROL_I32,
    DTS_PARAM_VX_DEF_ENABLE_I32,
    DTS_PARAM_VX_DEF_CONTROL_I32,
    DTS_PARAM_LOUDNESS_CONTROL_ENABLE_I32,
    DTS_PARAM_LOUDNESS_CONTROL_TARGET_LOUDNESS_I32,
    DTS_PARAM_LOUDNESS_CONTROL_PRESET_I32,
    DTS_PARAM_LOUDNESS_CONTROL_IO_MODE_I32,
    /*UI interface*/
    VIRTUALX_PARAM_ENABLE,
    VIRTUALX_PARAM_DIALOGCLARTY_MODE,
    VIRTUALX_PARAM_SURROUND_MODE,
}Virtualx_params;

Virtualx_param_t gVirtualxParam[] = {
    {{0, 4, 4}, DTS_PARAM_MBHL_ENABLE_I32, {1}},
    {{0, 4, 4}, DTS_PARAM_MBHL_BYPASS_GAIN_I32, {1}},
    {{0, 4, 4}, DTS_PARAM_MBHL_REFERENCE_LEVEL_I32, {1}},
    {{0, 4, 4}, DTS_PARAM_MBHL_VOLUME_I32, {1}},
    {{0, 4, 4}, DTS_PARAM_MBHL_VOLUME_STEP_I32, {100}},
    {{0, 4, 4}, DTS_PARAM_MBHL_BALANCE_STEP_I32, {0}},
    {{0, 4, 4}, DTS_PARAM_MBHL_OUTPUT_GAIN_I32, {1}},
    {{0, 4, 4}, DTS_PARAM_MBHL_MODE_I32, {0}},
    {{0, 4, 4}, DTS_PARAM_MBHL_PROCESS_DISCARD_I32, {0}},
    {{0, 4, 4}, DTS_PARAM_MBHL_CROSS_LOW_I32, {7}},
    {{0, 4, 4}, DTS_PARAM_MBHL_CROSS_MID_I32, {15}},
    {{0, 4, 4}, DTS_PARAM_MBHL_COMP_ATTACK_I32, {5}},
    {{0, 4, 4}, DTS_PARAM_MBHL_COMP_LOW_RELEASE_I32, {250}},
    {{0, 4, 4}, DTS_PARAM_MBHL_COMP_LOW_RATIO_I32, {4}},
    {{0, 4, 4}, DTS_PARAM_MBHL_COMP_LOW_THRESH_I32, {0}},
    {{0, 4, 4}, DTS_PARAM_MBHL_COMP_LOW_MAKEUP_I32, {1}},
    {{0, 4, 4}, DTS_PARAM_MBHL_COMP_MID_RELEASE_I32, {250}},
    {{0, 4, 4}, DTS_PARAM_MBHL_COMP_MID_RATIO_I32, {4}},
    {{0, 4, 4}, DTS_PARAM_MBHL_COMP_MID_THRESH_I32, {0}},
    {{0, 4, 4}, DTS_PARAM_MBHL_COMP_MID_MAKEUP_I32, {1}},
    {{0, 4, 4}, DTS_PARAM_MBHL_COMP_HIGH_RELEASE_I32, {250}},
    {{0, 4, 4}, DTS_PARAM_MBHL_COMP_HIGH_RATIO_I32, {4}},
    {{0, 4, 4}, DTS_PARAM_MBHL_COMP_HIGH_THRESH_I32, {0}},
    {{0, 4, 4}, DTS_PARAM_MBHL_COMP_HIGH_MAKEUP_I32, {1}},
    {{0, 4, 4}, DTS_PARAM_MBHL_BOOST_I32, {1}},
    {{0, 4, 4}, DTS_PARAM_MBHL_THRESHOLD_I32, {1}},
    {{0, 4, 4}, DTS_PARAM_MBHL_SLOW_OFFSET_I32, {1}},
    {{0, 4, 4}, DTS_PARAM_MBHL_FAST_ATTACK_I32, {5}},
    {{0, 4, 4}, DTS_PARAM_MBHL_FAST_RELEASE_I32, {50}},
    {{0, 4, 4}, DTS_PARAM_MBHL_SLOW_ATTACK_I32, {500}},
    {{0, 4, 4}, DTS_PARAM_MBHL_SLOW_RELEASE_I32, {500}},
    {{0, 4, 4}, DTS_PARAM_MBHL_DELAY_I32, {8}},
    {{0, 4, 4}, DTS_PARAM_MBHL_ENVELOPE_FREQUENCY_I32, {20}},
    {{0, 4, 4}, DTS_PARAM_MBHL_APP_FRT_LOWCROSS_F32,{0}},
    {{0, 4, 4}, DTS_PARAM_MBHL_APP_FRT_MIDCROSS_F32,{0}},
    {{0, 4, 4}, DTS_PARAM_TBHDX_ENABLE_I32, {0}},
    {{0, 4, 4}, DTS_PARAM_TBHDX_MONO_MODE_I32, {0}},
    {{0, 4, 4}, DTS_PARAM_TBHDX_MAXGAIN_I32, {0}},
    {{0, 4, 4}, DTS_PARAM_TBHDX_SPKSIZE_I32, {2}},
    {{0, 4, 4}, DTS_PARAM_TBHDX_HP_ENABLE_I32, {1}},
    {{0, 4, 4}, DTS_PARAM_TBHDX_TEMP_GAIN_I32, {0}},
    {{0, 4, 4}, DTS_PARAM_TBHDX_PROCESS_DISCARD_I32, {1}},
    {{0, 4, 4}, DTS_PARAM_TBHDX_HPORDER_I32, {4}},
    {{0, 4, 4}, DTS_PARAM_TBHDX_APP_SPKSIZE_I32,{0}},
    {{0, 4, 4}, DTS_PARAM_TBHDX_APP_HPRATIO_F32,{0}},
    {{0, 4, 4}, DTS_PARAM_TBHDX_APP_EXTBASS_F32,{0}},
    {{0, 4, 4}, DTS_PARAM_VX_ENABLE_I32, {1}},
    {{0, 4, 4}, DTS_PARAM_VX_INPUT_MODE_I32, {4}},
    {{0, 4, 4}, DTS_PARAM_VX_OUTPUT_MODE_I32, {0}},
    {{0, 4, 4}, DTS_PARAM_VX_HEADROOM_GAIN_I32, {1}},
    {{0, 4, 4}, DTS_PARAM_VX_PROC_OUTPUT_GAIN_I32, {1}},
    {{0, 4, 4}, DTS_PARAM_VX_REFERENCE_LEVEL_I32, {0}},
    {{0, 4, 4}, DTS_PARAM_TSX_ENABLE_I32, {1}},
    {{0, 4, 4}, DTS_PARAM_TSX_PASSIVEMATRIXUPMIX_ENABLE_I32, {1}},
    {{0, 4, 4}, DTS_PARAM_TSX_HEIGHT_UPMIX_ENABLE_I32, {1}},
    {{0, 4, 4}, DTS_PARAM_TSX_LPR_GAIN_I32, {1}},
    {{0, 4, 4}, DTS_PARAM_TSX_CENTER_GAIN_I32, {1}},
    {{0, 4, 4}, DTS_PARAM_TSX_HORIZ_VIR_EFF_CTRL_I32, {0}},
    {{0, 4, 4}, DTS_PARAM_TSX_HEIGHTMIX_COEFF_I32, {1}},
    {{0, 4, 4}, DTS_PARAM_TSX_PROCESS_DISCARD_I32, {0}},
    {{0, 4, 4}, DTS_PARAM_TSX_HEIGHT_DISCARD_I32, {0}},
    {{0, 4, 4}, DTS_PARAM_TSX_FRNT_CTRL_I32, {1}},
    {{0, 4, 4}, DTS_PARAM_TSX_SRND_CTRL_I32, {1}},
    {{0, 4, 4}, DTS_PARAM_VX_DC_ENABLE_I32, {0}},
    {{0, 4, 4}, DTS_PARAM_VX_DC_CONTROL_I32, {0}},
    {{0, 4, 4}, DTS_PARAM_VX_DEF_ENABLE_I32, {0}},
    {{0, 4, 4}, DTS_PARAM_VX_DEF_CONTROL_I32, {0}},
    {{0, 4, 4}, DTS_PARAM_LOUDNESS_CONTROL_ENABLE_I32, {1}},
    {{0, 4, 4}, DTS_PARAM_LOUDNESS_CONTROL_TARGET_LOUDNESS_I32, {-24}},
    {{0, 4, 4}, DTS_PARAM_LOUDNESS_CONTROL_PRESET_I32, {0}},
    {{0, 4, 4}, DTS_PARAM_LOUDNESS_CONTROL_IO_MODE_I32,{0}},
    {{0, 4, 4}, VIRTUALX_PARAM_ENABLE, {1}},
    {{0, 4, 4}, VIRTUALX_PARAM_DIALOGCLARTY_MODE, {1}},
    {{0, 4, 4}, VIRTUALX_PARAM_SURROUND_MODE, {1}},

};
const char *VXStatusstr[] = {"Disable", "Enable"};

static int Virtualx_effect_func(AudioEffect* gAudioEffect, int gParamIndex, int gParamValue,float gParamScale)
{
    int rc = 0;
    switch (gParamIndex) {
        case VIRTUALX_PARAM_ENABLE:
            if (gParamValue < 0 || gParamValue > 1) {
                LOG("Vritualx: Status gParamValue = %d invalid\n", gParamValue);
                return -1;
            }
            gVirtualxParam[gParamIndex].v = gParamValue;
            if (!gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param))
                LOG("=================successful\n");
            else
                LOG("=====================failed\n");
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("Virtualx: Status is %d -> %s\n", gParamValue, VXStatusstr[gVirtualxParam[gParamIndex].v]);
            return 0;
       case DTS_PARAM_MBHL_ENABLE_I32:
            if (gParamValue < 0 || gParamValue > 1) {
                LOG("Vritualx: MBHL ENABLE gParamValue = %d invalid\n", gParamValue);
                return -1;
            }
            gVirtualxParam[gParamIndex].v = gParamValue;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("mbhl enable is %d \n",gVirtualxParam[gParamIndex].v);
            return 0;
        case DTS_PARAM_MBHL_BYPASS_GAIN_I32:
            if (gParamScale < 0 || gParamScale > 1.0) {
                LOG("Vritualx: mbhl bypass gain gParamValue = %f invalid\n", gParamScale);
                return -1;
            }
            gVirtualxParam[gParamIndex].f = gParamScale;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("mbhl bypassgain is %f \n",gVirtualxParam[gParamIndex].f);
            return 0;
        case DTS_PARAM_MBHL_REFERENCE_LEVEL_I32:
            if (gParamScale < 0.0009 || gParamScale > 1.0) {
                LOG("Vritualx: mbhl reference level gParamValue = %f invalid\n", gParamScale);
                return -1;
            }
            gVirtualxParam[gParamIndex].f = gParamScale;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("mbhl reference level is %f \n",gVirtualxParam[gParamIndex].f);
            return 0;
        case DTS_PARAM_MBHL_VOLUME_I32:
            if (gParamScale < 0 || gParamScale > 1.0) {
                LOG("Vritualx: mbhl volume gParamValue = %f invalid\n", gParamScale);
                return -1;
            }
            gVirtualxParam[gParamIndex].f = gParamScale;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("mbhl volume is %f \n",gVirtualxParam[gParamIndex].f);
            return 0;
        case DTS_PARAM_MBHL_VOLUME_STEP_I32:
            if (gParamValue < 0 || gParamValue > 100) {
                LOG("Vritualx: mbhl volumestep gParamValue = %d invalid\n", gParamValue);
                return -1;
            }
            gVirtualxParam[gParamIndex].v = gParamValue;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("mbhl volume step is %d \n",gVirtualxParam[gParamIndex].v);
            return 0;
        case DTS_PARAM_MBHL_BALANCE_STEP_I32:
            if (gParamValue < -10 || gParamValue > 10) {
                LOG("Vritualx: mbhl banlance step gParamValue = %d invalid\n", gParamValue);
                return -1;
            }
            gVirtualxParam[gParamIndex].v = gParamValue;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("mbhl balance step is %d \n",gVirtualxParam[gParamIndex].v);
            return 0;
        case DTS_PARAM_MBHL_OUTPUT_GAIN_I32:
            if (gParamScale < 0 || gParamScale > 1.0) {
                LOG("Vritualx: mbhl output gain gParamValue = %f invalid\n", gParamScale);
                return -1;
            }
            gVirtualxParam[gParamIndex].f = gParamScale;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
           // gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("mbhl output gain is %f \n",gVirtualxParam[gParamIndex].f);
            return 0;
        case DTS_PARAM_MBHL_BOOST_I32:
            if (gParamScale < 0.001 || gParamScale > 1000) {
                LOG("Vritualx: mbhl boost  gParamValue = %f invalid\n", gParamScale);
                return -1;
            }
            gVirtualxParam[gParamIndex].f = gParamScale;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("mbhl boost is %f \n",gVirtualxParam[gParamIndex].f);
            return 0;
        case DTS_PARAM_MBHL_THRESHOLD_I32:
            if (gParamScale < 0.064 || gParamScale > 1.0) {
                LOG("Vritualx: mbhl threshold  gParamValue = %f invalid\n", gParamScale);
                return -1;
            }
            gVirtualxParam[gParamIndex].f = gParamScale;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("mbhl threshold is %f \n",gVirtualxParam[gParamIndex].f);
            return 0;
        case DTS_PARAM_MBHL_SLOW_OFFSET_I32:
            if (gParamScale < 0.3170 || gParamScale > 3.1619) {
                LOG("Vritualx: mbhl slow offset  gParamValue = %f invalid\n", gParamScale);
                return -1;
            }
            gVirtualxParam[gParamIndex].f = gParamScale;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("mbhl slow offset is %f \n",gVirtualxParam[gParamIndex].f);
            return 0;
        case DTS_PARAM_MBHL_FAST_ATTACK_I32:
            if (gParamScale < 0 || gParamScale > 10) {
                LOG("Vritualx: mbhl fast attack  gParamValue = %f invalid\n", gParamScale);
                return -1;
            }
            gVirtualxParam[gParamIndex].f = gParamScale;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("mbhl fast attack is %f \n",gVirtualxParam[gParamIndex].f);
            return 0;
        case DTS_PARAM_MBHL_FAST_RELEASE_I32:
            if (gParamValue < 10 || gParamValue > 500) {
                LOG("Vritualx: mbhl fast release gParamValue = %d invalid\n", gParamValue);
                return -1;
            }
            gVirtualxParam[gParamIndex].v = gParamValue;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("mbhl fast release is %d \n",gVirtualxParam[gParamIndex].v);
            return 0;
        case DTS_PARAM_MBHL_SLOW_ATTACK_I32:
            if (gParamValue < 100 || gParamValue > 1000) {
                LOG("Vritualx: mbhl slow attack gParamValue = %d invalid\n", gParamValue);
                return -1;
            }
            gVirtualxParam[gParamIndex].v = gParamValue;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("mbhl slow attack is %d \n",gVirtualxParam[gParamIndex].v);
            return 0;
        case DTS_PARAM_MBHL_SLOW_RELEASE_I32:
            if (gParamValue < 100 || gParamValue > 2000) {
                LOG("Vritualx: mbhl slow release gParamValue = %d invalid\n", gParamValue);
                return -1;
            }
            gVirtualxParam[gParamIndex].v = gParamValue;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("mbhl slow release is %d \n",gVirtualxParam[gParamIndex].v);
            return 0;
        case DTS_PARAM_MBHL_DELAY_I32:
            if (gParamValue < 1 || gParamValue > 16) {
                LOG("Vritualx: mbhl delay gParamValue = %d invalid\n", gParamValue);
                return -1;
            }
            gVirtualxParam[gParamIndex].v = gParamValue;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("mbhl delay is %d \n",gVirtualxParam[gParamIndex].v);
            return 0;
        case DTS_PARAM_MBHL_ENVELOPE_FREQUENCY_I32:
            if (gParamValue < 5 || gParamValue > 500) {
                LOG("Vritualx: mbhl envelope freq gParamValue = %d invalid\n", gParamValue);
                return -1;
            }
            gVirtualxParam[gParamIndex].v = gParamValue;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("mbhl envelope freq is %d \n",gVirtualxParam[gParamIndex].v);
            return 0;
        case DTS_PARAM_MBHL_MODE_I32:
            if (gParamValue < 0 || gParamValue > 4) {
                LOG("Vritualx: mbhl mode gParamValue = %d invalid\n", gParamValue);
                return -1;
            }
            gVirtualxParam[gParamIndex].v = gParamValue;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("mbhl mode is %d \n",gVirtualxParam[gParamIndex].v);
            return 0;
        case DTS_PARAM_MBHL_PROCESS_DISCARD_I32:
            if (gParamValue < 0 || gParamValue > 1) {
                LOG("Vritualx: mbhl process discard gParamValue = %d invalid\n", gParamValue);
                return -1;
            }
            gVirtualxParam[gParamIndex].v = gParamValue;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("mbhl process discard is %d \n",gVirtualxParam[gParamIndex].v);
            return 0;
        case DTS_PARAM_MBHL_CROSS_LOW_I32:
            if (gParamValue < 0 || gParamValue > 20) {
                LOG("Vritualx: mbhl cross low gParamValue = %d invalid\n", gParamValue);
                return -1;
            }
            gVirtualxParam[gParamIndex].v = gParamValue;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("mbhl cross low  is %d \n",gVirtualxParam[gParamIndex].v);
            return 0;
        case DTS_PARAM_MBHL_CROSS_MID_I32:
            if (gParamValue < 0 || gParamValue > 20) {
                LOG("Vritualx: mbhl cross mid gParamValue = %d invalid\n", gParamValue);
                return -1;
            }
            gVirtualxParam[gParamIndex].v = gParamValue;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("mbhl cross mid is %d \n",gVirtualxParam[gParamIndex].v);
            return 0;
        case DTS_PARAM_MBHL_COMP_ATTACK_I32:
            if (gParamValue < 0 || gParamValue > 100) {
                LOG("Vritualx: mbhl compressor attack time gParamValue = %d invalid\n", gParamValue);
                return -1;
            }
            gVirtualxParam[gParamIndex].v = gParamValue;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("mbhl compressor attack time is %d \n",gVirtualxParam[gParamIndex].v);
            return 0;
        case DTS_PARAM_MBHL_COMP_LOW_RELEASE_I32:
            if (gParamValue < 50 || gParamValue > 2000) {
                LOG("Vritualx: mbhl compressor low release time gParamValue = %d invalid\n", gParamValue);
                return -1;
            }
            gVirtualxParam[gParamIndex].v = gParamValue;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("mbhl compressor low release time is %d \n",gVirtualxParam[gParamIndex].v);
            return 0;
        case DTS_PARAM_MBHL_COMP_LOW_RATIO_I32:
            if (gParamScale < 1.0 || gParamScale > 20.0) {
                LOG("Vritualx: mbhl compressor low ratio gParamValue = %f invalid\n", gParamScale);
                return -1;
            }
            gVirtualxParam[gParamIndex].f = gParamScale;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("mbhl compressor low ratio is %f \n",gVirtualxParam[gParamIndex].f);
            return 0;
        case DTS_PARAM_MBHL_COMP_LOW_THRESH_I32:
            if (gParamScale < 0.0640 || gParamScale > 15.8479) {
                LOG("Vritualx: mbhl compressor low threshold gParamValue = %f invalid\n", gParamScale);
                return -1;
            }
            gVirtualxParam[gParamIndex].f = gParamScale;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("mbhl compressor low threshold is %f \n",gVirtualxParam[gParamIndex].f);
            return 0;
        case DTS_PARAM_MBHL_COMP_LOW_MAKEUP_I32:
            if (gParamScale < 0.0640 || gParamScale > 15.8479) {
                LOG("Vritualx: mbhl compressor low makeup gParamValue = %f invalid\n", gParamScale);
                return -1;
            }
            gVirtualxParam[gParamIndex].f = gParamScale;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("mbhl compressor low makeup is %f \n",gVirtualxParam[gParamIndex].f);
            return 0;
        case DTS_PARAM_MBHL_COMP_MID_RELEASE_I32:
            if (gParamValue < 50 || gParamValue > 2000) {
                LOG("Vritualx: mbhl compressor mid release time gParamValue = %d invalid\n", gParamValue);
                return -1;
            }
            gVirtualxParam[gParamIndex].v = gParamValue;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("mbhl compressor mid release time is %d \n",gVirtualxParam[gParamIndex].v);
            return 0;
        case DTS_PARAM_MBHL_COMP_MID_RATIO_I32:
             if (gParamScale < 1.0 || gParamScale > 20.0) {
                LOG("Vritualx: mbhl compressor mid ratio gParamValue = %f invalid\n", gParamScale);
                return -1;
            }
            gVirtualxParam[gParamIndex].f = gParamScale;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("mbhl compressor mid ratio is %f \n",gVirtualxParam[gParamIndex].f);
            return 0;
        case DTS_PARAM_MBHL_COMP_MID_THRESH_I32:
            if (gParamScale < 0.0640 || gParamScale > 15.8479) {
                LOG("Vritualx: mbhl compressor mid threshold gParamValue = %f invalid\n", gParamScale);
                return -1;
            }
            gVirtualxParam[gParamIndex].f = gParamScale;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("mbhl compressor mid threshold is %f \n",gVirtualxParam[gParamIndex].f);
            return 0;
        case DTS_PARAM_MBHL_COMP_MID_MAKEUP_I32:
             if (gParamScale < 0.0640 || gParamScale > 15.8479) {
                LOG("Vritualx: mbhl compressor mid makeup gParamValue = %f invalid\n", gParamScale);
                return -1;
            }
            gVirtualxParam[gParamIndex].f = gParamScale;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("mbhl compressor mid makeup is %f \n",gVirtualxParam[gParamIndex].f);
            return 0;
        case DTS_PARAM_MBHL_COMP_HIGH_RELEASE_I32:
            if (gParamValue < 50 || gParamValue > 2000) {
                LOG("Vritualx: mbhl compressor high release time gParamValue = %d invalid\n", gParamValue);
                return -1;
            }
            gVirtualxParam[gParamIndex].v = gParamValue;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("mbhl compressor high release time is %d \n",gVirtualxParam[gParamIndex].v);
            return 0;
        case DTS_PARAM_MBHL_COMP_HIGH_RATIO_I32:
            if (gParamScale < 1.0 || gParamScale > 20.0) {
                LOG("Vritualx: mbhl compressor high ratio gParamValue = %f invalid\n", gParamScale);
                return -1;
            }
            gVirtualxParam[gParamIndex].f = gParamScale;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("mbhl compressor high ratio is %f \n",gVirtualxParam[gParamIndex].f);
            return 0;
        case DTS_PARAM_MBHL_COMP_HIGH_THRESH_I32:
            if (gParamScale < 0.0640 || gParamScale > 15.8479) {
                LOG("Vritualx: mbhl compressor high threshold gParamValue = %f invalid\n", gParamScale);
                return -1;
            }
            gVirtualxParam[gParamIndex].f = gParamScale;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("mbhl compressor high threshold is %f \n",gVirtualxParam[gParamIndex].f);
            return 0;
        case DTS_PARAM_MBHL_COMP_HIGH_MAKEUP_I32:
             if (gParamScale < 0.0640 || gParamScale > 15.8479) {
                LOG("Vritualx: mbhl compressor high makeup gParamValue = %f invalid\n", gParamScale);
                return -1;
            }
            gVirtualxParam[gParamIndex].f = gParamScale;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("mbhl compressor high makeup is %f \n",gVirtualxParam[gParamIndex].f);
            return 0;
        case DTS_PARAM_TBHDX_ENABLE_I32:
            if (gParamValue < 0 || gParamValue > 1) {
                LOG("Vritualx: tbhdx enable gParamValue = %d invalid\n", gParamValue);
                return -1;
            }
            gVirtualxParam[gParamIndex].v = gParamValue;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("tbhdx enable is %d \n",gVirtualxParam[gParamIndex].v);
            return 0;
        case DTS_PARAM_TBHDX_MONO_MODE_I32:
            if (gParamValue < 0 || gParamValue > 1) {
                LOG("Vritualx: tbhdx mono mode gParamValue = %d invalid\n", gParamValue);
                return -1;
            }
            gVirtualxParam[gParamIndex].v = gParamValue;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("tbhdx mono mode is %d \n",gVirtualxParam[gParamIndex].v);
            return 0;
        case DTS_PARAM_TBHDX_SPKSIZE_I32:
            if (gParamValue < 0 || gParamValue > 12) {
                LOG("Vritualx: tbhdx spksize gParamValue = %d invalid\n", gParamValue);
                return -1;
            }
            gVirtualxParam[gParamIndex].v = gParamValue;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("tbhdx spksize is %d \n",gVirtualxParam[gParamIndex].v);
            return 0;
        case DTS_PARAM_TBHDX_TEMP_GAIN_I32:
            if (gParamScale < 0.0 || gParamScale > 1.0) {
                LOG("Vritualx: tbhdx temp gain gParamValue = %f invalid\n", gParamScale);
                return -1;
            }
            gVirtualxParam[gParamIndex].f = gParamScale;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("tbhdx temp gain is %f \n",gVirtualxParam[gParamIndex].f);
            return 0;
        case DTS_PARAM_TBHDX_MAXGAIN_I32:
            if (gParamScale < 0.0 || gParamScale > 1.0) {
                LOG("Vritualx: tbhdx max gain gParamValue = %f invalid\n", gParamScale);
                return -1;
            }
            gVirtualxParam[gParamIndex].f = gParamScale;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("tbhdx max gain is %f \n",gVirtualxParam[gParamIndex].f);
            return 0;
        case DTS_PARAM_TBHDX_PROCESS_DISCARD_I32:
            if (gParamValue < 0 || gParamValue > 1) {
                LOG("Vritualx: tbhdx process discard gParamValue = %d invalid\n", gParamValue);
                return -1;
            }
            gVirtualxParam[gParamIndex].v = gParamValue;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
           //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("tbhdx process discard is %d \n",gVirtualxParam[gParamIndex].v);
            return 0;
        case DTS_PARAM_TBHDX_HPORDER_I32:
            if (gParamValue < 0 || gParamValue > 8) {
                LOG("Vritualx: tbhdx high pass filter order gParamValue = %d invalid\n", gParamValue);
                return -1;
            }
            gVirtualxParam[gParamIndex].v = gParamValue;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("tbhdx high pass filter order is %d \n",gVirtualxParam[gParamIndex].v);
            return 0;
        case DTS_PARAM_TBHDX_HP_ENABLE_I32:
            if (gParamValue < 0 || gParamValue > 1) {
                LOG("Vritualx: tbhdx high pass enable gParamValue = %d invalid\n", gParamValue);
                return -1;
            }
            gVirtualxParam[gParamIndex].v = gParamValue;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("tbhdx high pass enable is %d \n",gVirtualxParam[gParamIndex].v);
            return 0;
        case DTS_PARAM_VX_ENABLE_I32:
            if (gParamValue < 0 || gParamValue > 1) {
                LOG("Vritualx: vxlib1 enable gParamValue = %d invalid\n", gParamValue);
                return -1;
            }
            gVirtualxParam[gParamIndex].v = gParamValue;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("vxlib1 enable is %d \n",gVirtualxParam[gParamIndex].v);
            return 0;
        case DTS_PARAM_VX_INPUT_MODE_I32:
            if (gParamValue < 0 || gParamValue > 4) {
                LOG("Vritualx: vxlib1 input mode gParamValue = %d invalid\n", gParamValue);
                return -1;
            }
            gVirtualxParam[gParamIndex].v = gParamValue;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("vxlib1 input mode is %d \n",gVirtualxParam[gParamIndex].v);
            return 0;
        case DTS_PARAM_VX_HEADROOM_GAIN_I32:
            if (gParamScale < 0.1250 || gParamScale > 1.0) {
                LOG("Vritualx: vxlib1 headroom gain gParamValue = %f invalid\n", gParamScale);
                return -1;
            }
            gVirtualxParam[gParamIndex].f = gParamScale;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("vxlib1 headroom gain is %f \n",gVirtualxParam[gParamIndex].f);
            return 0;
        case DTS_PARAM_VX_PROC_OUTPUT_GAIN_I32:
            if (gParamScale < 0.5 || gParamScale > 4.0) {
                LOG("Vritualx: vxlib1 output gain gParamValue = %f invalid\n", gParamScale);
                return -1;
            }
            gVirtualxParam[gParamIndex].f = gParamScale;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("vxlib1 output gain is %f \n",gVirtualxParam[gParamIndex].f);
            return 0;
        case DTS_PARAM_TSX_ENABLE_I32:
            if (gParamValue < 0 || gParamValue > 1) {
                LOG("Vritualx: tsx enable gParamValue = %d invalid\n", gParamValue);
                return -1;
            }
            gVirtualxParam[gParamIndex].v = gParamValue;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("vxlib1 tsx enable is %d \n",gVirtualxParam[gParamIndex].v);
            return 0;
        case DTS_PARAM_TSX_PASSIVEMATRIXUPMIX_ENABLE_I32:
            if (gParamValue < 0 || gParamValue > 1) {
                LOG("Vritualx: tsx passive matrix upmixer enable gParamValue = %d invalid\n", gParamValue);
                return -1;
            }
            gVirtualxParam[gParamIndex].v = gParamValue;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("vxlib1 tsx passive matrix upmixer enable is %d \n",gVirtualxParam[gParamIndex].v);
            return 0;
        case DTS_PARAM_TSX_HORIZ_VIR_EFF_CTRL_I32:
            if (gParamValue < 0 || gParamValue > 1) {
                LOG("Vritualx: tsx horizontal Effect gParamValue = %d invalid\n", gParamValue);
                return -1;
            }
            gVirtualxParam[gParamIndex].v = gParamValue;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("vxlib1 tsx horizontal Effect is %d \n",gVirtualxParam[gParamIndex].v);
            return 0;
        case DTS_PARAM_TSX_FRNT_CTRL_I32:
            if (gParamScale < 0.5 || gParamScale > 2.0) {
                LOG("Vritualx: tsx frnt ctrl gParamValue = %f invalid\n", gParamScale);
                return -1;
            }
            gVirtualxParam[gParamIndex].f = gParamScale;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("tsx frnt ctrl is %f \n",gVirtualxParam[gParamIndex].f);
            return 0;
        case DTS_PARAM_TSX_SRND_CTRL_I32:
            if (gParamScale < 0.5 || gParamScale > 2.0) {
                LOG("Vritualx: tsx srnd ctrl gParamValue = %f invalid\n", gParamScale);
                return -1;
            }
            gVirtualxParam[gParamIndex].f = gParamScale;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("tsx srnd ctrl is %f \n",gVirtualxParam[gParamIndex].f);
            return 0;
        case DTS_PARAM_TSX_LPR_GAIN_I32:
             if (gParamScale < 0.0 || gParamScale > 2.0) {
                LOG("Vritualx: tsx lprtoctr mix gain gParamValue = %f invalid\n", gParamScale);
                return -1;
            }
            gVirtualxParam[gParamIndex].f = gParamScale;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("tsx lprtoctr mix gain is %f \n",gVirtualxParam[gParamIndex].f);
            return 0;
        case DTS_PARAM_TSX_HEIGHTMIX_COEFF_I32:
             if (gParamScale < 0.5 || gParamScale > 2.0) {
                LOG("Vritualx: tsx heightmix coeff gParamValue = %f invalid\n", gParamScale);
                return -1;
            }
            gVirtualxParam[gParamIndex].f = gParamScale;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("tsx heightmix coeff is %f \n",gVirtualxParam[gParamIndex].f);
            return 0;
        case DTS_PARAM_TSX_CENTER_GAIN_I32:
             if (gParamScale < 1.0 || gParamScale > 2.0) {
                LOG("Vritualx: tsx center gain gParamValue = %f invalid\n", gParamScale);
                return -1;
            }
            gVirtualxParam[gParamIndex].f = gParamScale;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("tsx center gain is %f \n",gVirtualxParam[gParamIndex].f);
            return 0;
        case DTS_PARAM_TSX_HEIGHT_DISCARD_I32:
            if (gParamValue < 0 || gParamValue > 1) {
                LOG("Vritualx: tsx height discard gParamValue = %d invalid\n", gParamValue);
                return -1;
            }
            gVirtualxParam[gParamIndex].v = gParamValue;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("tsx height discard is %d \n",gVirtualxParam[gParamIndex].v);
            return 0;
        case DTS_PARAM_TSX_PROCESS_DISCARD_I32:
            if (gParamValue < 0 || gParamValue > 1) {
                LOG("Vritualx: tsx process discard gParamValue = %d invalid\n", gParamValue);
                return -1;
            }
            gVirtualxParam[gParamIndex].v = gParamValue;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("tsx process discard is %d \n",gVirtualxParam[gParamIndex].v);
            return 0;
        case DTS_PARAM_TSX_HEIGHT_UPMIX_ENABLE_I32:
            if (gParamValue < 0 || gParamValue > 1) {
                LOG("Vritualx: tsx height upmix enable gParamValue = %d invalid\n", gParamValue);
                return -1;
            }
            gVirtualxParam[gParamIndex].v = gParamValue;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("tsx height upmix enable is %d \n",gVirtualxParam[gParamIndex].v);
            return 0;
        case DTS_PARAM_VX_DC_ENABLE_I32:
            if (gParamValue < 0 || gParamValue > 1) {
                LOG("Vritualx: tsx dc enable gParamValue = %d invalid\n", gParamValue);
                return -1;
            }
            gVirtualxParam[gParamIndex].v = gParamValue;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("tsx dc enable is %d \n",gVirtualxParam[gParamIndex].v);
            return 0;
        case DTS_PARAM_VX_DC_CONTROL_I32:
             if (gParamScale < 0.0 || gParamScale > 1.0) {
                LOG("Vritualx: tsx dc level gParamValue = %f invalid\n", gParamScale);
                return -1;
            }
            gVirtualxParam[gParamIndex].f = gParamScale;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("tsx dc level is %f \n",gVirtualxParam[gParamIndex].f);
            return 0;
        case DTS_PARAM_VX_DEF_ENABLE_I32:
            if (gParamValue < 0 || gParamValue > 1) {
                LOG("Vritualx: tsx def enable gParamValue = %d invalid\n", gParamValue);
                return -1;
            }
            gVirtualxParam[gParamIndex].v = gParamValue;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("tsx def enable is %d \n",gVirtualxParam[gParamIndex].v);
            return 0;
        case DTS_PARAM_VX_DEF_CONTROL_I32:
             if (gParamScale < 0.0 || gParamScale > 1.0) {
                LOG("Vritualx: tsx def level gParamValue = %f invalid\n", gParamScale);
                return -1;
            }
            gVirtualxParam[gParamIndex].f = gParamScale;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("tsx def level is %f \n",gVirtualxParam[gParamIndex].f);
            return 0;
        case DTS_PARAM_LOUDNESS_CONTROL_ENABLE_I32:
            if (gParamValue < 0 || gParamValue > 1) {
                LOG("loudness control = %d invalid\n", gParamValue);
                return -1;
            }
            gVirtualxParam[gParamIndex].v = gParamValue;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("loudness control is %d \n",gVirtualxParam[gParamIndex].v);
            return 0;
        case DTS_PARAM_LOUDNESS_CONTROL_TARGET_LOUDNESS_I32:
            if (gParamValue < -40 || gParamValue > 0) {
                LOG("loudness control target = %d invalid\n", gParamValue);
                return -1;
            }
            gVirtualxParam[gParamIndex].v = gParamValue;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("loudness control target is %d \n",gVirtualxParam[gParamIndex].v);
            return 0;
        case DTS_PARAM_LOUDNESS_CONTROL_PRESET_I32:
            if (gParamValue < 0 || gParamValue > 2) {
                LOG("loudness control preset = %d invalid\n", gParamValue);
                return -1;
            }
            gVirtualxParam[gParamIndex].v = gParamValue;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            //gAudioEffect->getParameter(&gVirtualxParam[gParamIndex].param);
            LOG("loudness control preset is %d \n",gVirtualxParam[gParamIndex].v);
            return 0;
        case DTS_PARAM_TBHDX_APP_SPKSIZE_I32:
            if (gParamValue < 40 || gParamValue > 600) {
                LOG("app spksize = %d invaild\n",gParamValue);
                return -1;
            }
            gVirtualxParam[gParamIndex].v = gParamValue;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            LOG("app spksize %d\n",gVirtualxParam[gParamIndex].v);
            return 0;
        case DTS_PARAM_TBHDX_APP_HPRATIO_F32:
            if (gParamScale < 0.0 || gParamScale > 1.0) {
                LOG("app hpratio = %f invaild\n",gParamScale);
                return -1;
            }
            gVirtualxParam[gParamIndex].f = gParamScale;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            LOG("app hpratio is %f\n",gVirtualxParam[gParamIndex].f);
            return 0;
        case DTS_PARAM_TBHDX_APP_EXTBASS_F32:
            if (gParamScale < 0.0 || gParamScale > 1.0) {
                LOG("app ettbass = %f invaild\n",gParamScale);
                return -1;
            }
            gVirtualxParam[gParamIndex].f = gParamScale;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            LOG("app ettbass is %f\n",gVirtualxParam[gParamIndex].f);
            return 0;
        case DTS_PARAM_MBHL_APP_FRT_LOWCROSS_F32:
            if (gParamScale < 40 || gParamScale > 8000.0) {
                LOG("app low freq = %f invaild\n",gParamScale);
                return -1;
            }
             gVirtualxParam[gParamIndex].f = gParamScale;
            rc =gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            LOG("rc is %d\n",rc);
            LOG("app low freq is %f\n",gVirtualxParam[gParamIndex].f);
            return 0;
        case DTS_PARAM_MBHL_APP_FRT_MIDCROSS_F32:
            if (gParamScale < 40.0 || gParamScale > 8000.0) {
                LOG("app mid freq = %f invaild\n",gParamScale);
                return -1;
            }
            gVirtualxParam[gParamIndex].f = gParamScale;
            gAudioEffect->setParameter(&gVirtualxParam[gParamIndex].param);
            LOG("app mid freq is %f\n",gVirtualxParam[gParamIndex].f);
            return 0;
    default:
        LOG("Virtualx: ParamIndex = %d invalid\n", gParamIndex);
        return -1;
    }
}

static void effectCallback(int32_t event, void* user, void *info)
{
    LOG("%s : %s():line:%d\n", __FILE__, __FUNCTION__, __LINE__);
}

static int create_audio_effect(AudioEffect **gAudioEffect, String16 name16, int index)
{
    status_t status = NO_ERROR;
    AudioEffect *pAudioEffect = NULL;
    audio_session_t gSessionId = AUDIO_SESSION_OUTPUT_MIX;

    if (*gAudioEffect != NULL)
        return 0;

    pAudioEffect = new AudioEffect(name16);
    if (!pAudioEffect) {
        LOG("create audio effect object failed\n");
        return -1;
    }

    status = pAudioEffect->set(NULL,
            &(gEffectStr[index]), // specific uuid
            0, // priority,
            effectCallback,
            NULL, // callback user data
            gSessionId,
            0); // default output device
    if (status != NO_ERROR) {
        LOG("set effect parameters failed\n");
        return -1;
    }

    status = pAudioEffect->initCheck();
    if (status != NO_ERROR) {
        LOG("init audio effect failed\n");
        return -1;
    }

    pAudioEffect->setEnabled(true);
    LOG("effect %d is %s\n", index, pAudioEffect->getEnabled()?"enabled":"disabled");

    *gAudioEffect = pAudioEffect;
    return 0;
}

int main(int argc,char **argv)
{
    int i;
    int ret = -1;
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    char *Stringptr;
    ssize_t read;
    int gEffectIndex = 6;
    int gParamIndex = 0;
    int gParamValue = 0;
    float gParamScale = 0.0f;
    status_t status = NO_ERROR;
    String16 name16[EFFECT_MAX] = {String16("AudioEffectEQTest"), String16("AudioEffectSRSTest"), String16("AudioEffectHPEQTest"),
        String16("AudioEffectAVLTest"), String16("AudioEffectGEQTest"),String16("AudioEffectVirtualxTest")};
    AudioEffect* gAudioEffect[EFFECT_MAX] = {0};
    audio_session_t gSessionId = AUDIO_SESSION_OUTPUT_MIX;
    fp = fopen("vxp2_newConfig.txt", "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);
    while (1) {
        while ((read = getline(&line, &len, fp)) != -1) {
            if ( (strncmp(line, "virtualx.vxlib1.enable", 22)) == 0)
            {
                Stringptr = line;
                Stringptr += 22;
                gParamValue = strtol(Stringptr, NULL, 0 );
                gParamIndex = (int)DTS_PARAM_VX_ENABLE_I32;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");

            }
            else if ((strncmp(line, "virtualx.mbhl.enable", 20)) == 0)
            {
                Stringptr = line;
                Stringptr += 20;
                gParamValue = strtol(Stringptr, NULL, 0);
                //printf("mbhl_enable is %d\n",data.mbhl_enable);
                gParamIndex = (int)DTS_PARAM_MBHL_ENABLE_I32;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");

            }
            else if ((strncmp(line, "virtualx.tbhdx.enable", 21)) == 0)
            {
                Stringptr = line;
                Stringptr += 21;
                gParamValue = strtol(Stringptr, NULL, 0);
                gParamIndex = (int)DTS_PARAM_TBHDX_ENABLE_I32;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");
            }
            else if ((strncmp(line, "virtualx.vxlib1.out_mode", 24)) == 0)
            {
                Stringptr = line;
                Stringptr += 24;
                gParamValue = strtol(Stringptr, NULL, 0);
                //do nothing
            }
            else if ((strncmp(line, "virtualx.vxlib1.headroom_gain", 29)) == 0)
            {
                Stringptr = line;
                Stringptr += 29;
                gParamScale = strtof(Stringptr, NULL);
                //printf("headroom_gain is %f\n",data.headroom_gain);
                gParamIndex = (int)DTS_PARAM_VX_HEADROOM_GAIN_I32;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");
            }
            else if ((strncmp(line, "virtualx.vxlib1.processing_output_gain", 38)) == 0)
            {
                Stringptr = line;
                Stringptr += 38;
                gParamScale = strtof(Stringptr, NULL);
                gParamIndex = (int)DTS_PARAM_VX_PROC_OUTPUT_GAIN_I32;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");

            }
            else if ((strncmp(line, "virtualx.vxlib1.tsx.enable", 26)) == 0)
            {
                Stringptr = line;
                Stringptr += 26;
                gParamValue = strtol(Stringptr, NULL, 0);
                gParamIndex = 52;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");

            }
            else if ((strncmp(line, "virtualx.vxlib1.tsx.pssv_mtrx_enable", 36)) == 0)
            {
                Stringptr = line;
                Stringptr += 36;
                gParamValue = strtol(Stringptr, NULL, 0);
                gParamIndex = 53;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");

            }
            else if ((strncmp(line, "virtualx.vxlib1.tsx.hght_upmix_enable", 37)) == 0)
            {
                Stringptr = line;
                Stringptr += 37;
                gParamValue = strtol(Stringptr, NULL, 0);
                gParamIndex = 54;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");

            }
            else if ((strncmp(line, "virtualx.vxlib1.tsx.horiznt_effect_ctrl", 39)) == 0)
            {
                Stringptr = line;
                Stringptr += 39;
                gParamValue = strtol(Stringptr, NULL, 0);
                gParamIndex = 57;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");

            }
            else if ((strncmp(line, "virtualx.vxlib1.tsx.LpRtoCtr_mix_gain", 37)) == 0)
            {
                Stringptr = line;
                Stringptr += 37;
                gParamScale = strtof(Stringptr, NULL);
                gParamIndex = 55;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");

            }
            else if ((strncmp(line, "virtualx.vxlib1.tsx.surround_control", 36)) == 0)
            {
                Stringptr = line;
                Stringptr += 36;
                gParamScale = strtof(Stringptr, NULL);
                gParamIndex = 62;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");

            }
            else if ((strncmp(line, "virtualx.vxlib1.tsx.front_control", 33)) == 0)
            {
                Stringptr = line;
                Stringptr += 33;
                gParamScale = strtof(Stringptr, NULL);
                gParamIndex = 61;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");

            }
            else if ((strncmp(line, "virtualx.vxlib1.tsx.heightmix_coeff", 35)) == 0)
            {
                Stringptr = line;
                Stringptr += 35;
                gParamScale = strtof(Stringptr, NULL);
                gParamIndex = 58;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");
            }
            else if ((strncmp(line, "virtualx.vxlib1.tsx.ctrgain", 27)) == 0)
            {
                Stringptr = line;
                Stringptr += 27;
                gParamScale = strtof(Stringptr, NULL);
                gParamIndex = 56;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");

            }
            else if ((strncmp(line, "virtualx.vxlib1.dialogClarity.enable", 36)) == 0)
            {
                Stringptr = line;
                Stringptr += 36;
                gParamValue = strtol(Stringptr, NULL, 0);
                gParamIndex = 63;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");

            }
            else if ((strncmp(line, "virtualx.vxlib1.dialogClarity.level", 35)) == 0)
            {
                Stringptr = line;
                Stringptr += 35;
                gParamScale = strtof(Stringptr, NULL);
                gParamIndex = 64;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");

            }
            else if ((strncmp(line, "virtualx.vxlib1.definition.enable", 33)) == 0)
            {
                Stringptr = line;
                Stringptr += 33;
                gParamValue = strtol(Stringptr, NULL, 0);
                gParamIndex = 65;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");

            }
            else if ((strncmp(line, "virtualx.vxlib1.definition.level", 32)) == 0)
            {
                Stringptr = line;
                Stringptr += 32;
                gParamScale = strtof(Stringptr, NULL);
                gParamIndex = 66;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");

            }
            else if ((strncmp(line, "virtualx.tbhdx.mono", 19)) == 0)
            {
                Stringptr = line;
                Stringptr += 19;
                gParamValue = strtol(Stringptr, NULL, 0);
                gParamIndex = 36;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");
            }
            else if ((strncmp(line, "virtualx.tbhdx.speaker_size", 27)) == 0)
            {
                Stringptr = line;
                Stringptr += 27;
                gParamValue = strtol(Stringptr, NULL, 0);
                gParamIndex = 38;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");

            }
            else if ((strncmp(line, "virtualx.tbhdx.max_gain", 23)) == 0)
            {
                Stringptr = line;
                Stringptr += 23;
                gParamScale = strtof(Stringptr, NULL);
                gParamIndex = 37;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");
            }
            else if ((strncmp(line, "virtualx.tbhdx.hp_enable", 24)) == 0)
            {
                Stringptr = line;
                Stringptr += 24;
                gParamValue = strtol(Stringptr, NULL, 0);
                gParamIndex = 39;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");
            }
            else if ((strncmp(line, "virtualx.tbhdx.hp_order", 23)) == 0)
            {
                Stringptr = line;
                Stringptr += 23;
                gParamValue = strtol(Stringptr, NULL, 0);
                gParamIndex = 42;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");
            }
            else if ((strncmp(line, "virtualx.tbhdx.temporal_gain", 28)) == 0)
            {
                Stringptr = line;
                Stringptr += 28;
                gParamScale = strtof(Stringptr, NULL);
                gParamIndex = 40;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");
            }
            else if ((strncmp(line, "virtualx.mbhl.bypass_gain", 25)) == 0)
            {
                Stringptr = line;
                Stringptr += 25;
                gParamScale = strtof(Stringptr, NULL);
                //printf("set bysspass gain is %f\n",data.mbhl_bypassgain);
                gParamIndex = 1;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");
            }
            else if ((strncmp(line, "virtualx.mbhl.reference_level", 29)) == 0)
            {
                Stringptr = line;
                Stringptr += 29;
                gParamScale = strtof(Stringptr, NULL);
                //printf("mbhl_reflevel is %f\n",data.mbhl_reflevel);
                gParamIndex = 2;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");
            }
            else if ((strncmp(line, "virtualx.mbhl.volume_step", 25)) == 0)
            {
                Stringptr = line;
                Stringptr += 25;
                gParamValue = strtol(Stringptr, NULL, 0);
                //printf("volume step is %d\n",data.mbhl_volumesttep);
                gParamIndex = 4;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");
            }
            else if ((strncmp(line, "virtualx.mbhl.volume", 20)) == 0)
            {
                Stringptr = line;
                Stringptr += 20;
                gParamScale = strtof(Stringptr, NULL);
                //printf("volume is %f\n",data.mbhl_volume);
                gParamIndex = 3;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");
            }
            else if ((strncmp(line, "virtualx.mbhl.balance_step", 26)) == 0)
            {
                Stringptr = line;
                Stringptr += 26;
                gParamValue = strtol(Stringptr, NULL, 0);
                gParamIndex = 5;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");
            }

            else if ((strncmp(line, "virtualx.mbhl.output_gain", 25)) == 0)
            {
                Stringptr = line;
                Stringptr += 25;
                gParamScale = strtof(Stringptr, NULL);
                gParamIndex = 6;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");
            }
            else if ((strncmp(line, "virtualx.mbhl.boost", 19)) == 0)
            {
                Stringptr = line;
                Stringptr += 19;
                gParamScale = strtof(Stringptr, NULL);
                gParamIndex = 24;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");
            }
            else if ((strncmp(line, "virtualx.mbhl.threshold", 23)) == 0)
            {
                Stringptr = line;
                Stringptr += 23;
                gParamScale = strtof(Stringptr, NULL);
                gParamIndex = 25;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");
            }
            else if ((strncmp(line, "virtualx.mbhl.slow_offset", 25)) == 0)
            {
                Stringptr = line;
                Stringptr += 25;
                gParamScale = strtof(Stringptr, NULL);
                gParamIndex = 26;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");
            }
            else if ((strncmp(line, "virtualx.mbhl.fast_attack", 25)) == 0)
            {
                Stringptr = line;
                Stringptr += 25;
                gParamScale = strtof(Stringptr, NULL);
                gParamIndex = 27;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");
            }
            else if ((strncmp(line, "virtualx.mbhl.fast_release", 26)) == 0)
            {
                Stringptr = line;
                Stringptr += 26;
                gParamValue = strtol(Stringptr, NULL, 0);
                gParamIndex = 28;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");
            }
            else if ((strncmp(line, "virtualx.mbhl.slow_attack", 25)) == 0)
            {
                Stringptr = line;
                Stringptr += 25;
                gParamValue = strtol(Stringptr, NULL, 0);
                gParamIndex = 29;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");
            }
            else if ((strncmp(line, "virtualx.mbhl.slow_release", 26)) == 0)
            {
                Stringptr = line;
                Stringptr += 26;
                gParamValue = strtol(Stringptr, NULL, 0);
                gParamIndex = 30;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");
            }
            else if ((strncmp(line, "virtualx.mbhl.delay", 19)) == 0)
            {
                Stringptr = line;
                Stringptr += 19;
                gParamValue = strtol(Stringptr, NULL, 0);
                gParamIndex = 31;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");
            }
            else if ((strncmp(line, "virtualx.mbhl.envelope_frequency", 32)) == 0)
            {
                Stringptr = line;
                Stringptr += 32;
                gParamValue = strtol(Stringptr, NULL, 0);
                gParamIndex = 32;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");
            }
            else if ((strncmp(line, "virtualx.mbhl.mode", 18)) == 0)
            {
                Stringptr = line;
                Stringptr += 18;
                gParamValue = strtol(Stringptr, NULL, 0);
                gParamIndex = 7;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");
            }
            else if ((strncmp(line, "virtualx.mbhl.cross_low", 23)) == 0)
            {
                Stringptr = line;
                Stringptr += 23;
                gParamValue = strtol(Stringptr, NULL, 0);
                gParamIndex = 9;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");
            }
            else if ((strncmp(line, "virtualx.mbhl.cross_mid", 23)) == 0)
            {
                Stringptr = line;
                Stringptr += 23;
                gParamValue = strtol(Stringptr, NULL, 0);
                gParamIndex = 10;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");
            }
            else if ((strncmp(line, "virtualx.mbhl.comp_attack", 25)) == 0)
            {
                Stringptr = line;
                Stringptr += 25;
                gParamValue = strtol(Stringptr, NULL, 0);
                gParamIndex = 11;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");
            }
            else if ((strncmp(line, "virtualx.mbhl.comp_low_release", 30)) == 0)
            {
                Stringptr = line;
                Stringptr += 30;
                gParamValue = strtol(Stringptr, NULL, 0);
                gParamIndex = 12;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");
            }
            else if ((strncmp(line, "virtualx.mbhl.comp_low_ratio", 28)) == 0)
            {
                Stringptr = line;
                Stringptr += 28;
                gParamScale = strtof(Stringptr, NULL);
                gParamIndex = 13;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");
            }
            else if ((strncmp(line, "virtualx.mbhl.comp_low_thresh", 29)) == 0)
            {
                Stringptr = line;
                Stringptr += 29;
                gParamScale = strtof(Stringptr, NULL);
                //printf("mbhl_compressolt is %f",data.mbhl_compressolt);
                gParamIndex = 14;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");
            }
            else if ((strncmp(line, "virtualx.mbhl.comp_low_makeup", 29)) == 0)
            {
                Stringptr = line;
                Stringptr += 29;
                gParamScale =  strtof(Stringptr, NULL);
                gParamIndex = 15;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");
            }
            else if ((strncmp(line, "virtualx.mbhl.comp_mid_release", 30)) == 0)
            {
                Stringptr = line;
                Stringptr += 30;
                gParamValue = strtol(Stringptr, NULL, 0);
                gParamIndex = (int)DTS_PARAM_MBHL_COMP_MID_RELEASE_I32;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");
            }
            else if ((strncmp(line, "virtualx.mbhl.comp_mid_ratio", 28)) == 0)
            {
                Stringptr = line;
                Stringptr += 28;
                gParamScale = strtof(Stringptr, NULL);
                gParamIndex = (int)DTS_PARAM_MBHL_COMP_MID_RATIO_I32;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");
            }
            else if ((strncmp(line, "virtualx.mbhl.comp_mid_thresh", 29)) == 0)
            {
                Stringptr = line;
                Stringptr += 29;
                gParamScale = strtof(Stringptr, NULL);
                gParamIndex = (int)DTS_PARAM_MBHL_COMP_MID_THRESH_I32;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");
            }
            else if ((strncmp(line, "virtualx.mbhl.comp_mid_makeup", 29)) == 0)
            {
                Stringptr = line;
                Stringptr += 29;
                gParamScale = strtof(Stringptr, NULL);
                gParamIndex = (int)DTS_PARAM_MBHL_COMP_MID_MAKEUP_I32;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");
            }
            else if ((strncmp(line, "virtualx.mbhl.comp_high_release", 31)) == 0)
            {
                Stringptr = line;
                Stringptr += 31;
                gParamValue = strtol(Stringptr, NULL, 0);
                gParamIndex = (int)DTS_PARAM_MBHL_COMP_HIGH_RELEASE_I32;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");
            }
            else if ((strncmp(line, "virtualx.mbhl.comp_high_ratio", 29)) == 0)
            {
                Stringptr = line;
                Stringptr += 29;
                gParamScale = strtof(Stringptr, NULL);
                gParamIndex = (int)DTS_PARAM_MBHL_COMP_HIGH_RATIO_I32;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");
            }
            else if ((strncmp(line, "virtualx.mbhl.comp_high_thresh", 30)) == 0)
            {
                Stringptr = line;
                Stringptr += 30;
                gParamScale = strtof(Stringptr, NULL);
                gParamIndex = (int)DTS_PARAM_MBHL_COMP_HIGH_THRESH_I32;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");
            }
            else if ((strncmp(line, "virtualx.mbhl.comp_high_makeup", 30)) == 0)
            {
                Stringptr = line;
                Stringptr += 30;
                gParamScale = strtof(Stringptr, NULL);
                gParamIndex = (int)DTS_PARAM_MBHL_COMP_HIGH_MAKEUP_I32;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");
            }
            else if ((strncmp(line, "virtualx.tbhdx.app_custom_speaker_size", 38)) == 0)
            {
                Stringptr = line;
                Stringptr += 38;
                gParamValue = strtol(Stringptr, NULL, 0);
                gParamIndex = (int)DTS_PARAM_TBHDX_APP_SPKSIZE_I32;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");

            }
            else if ((strncmp(line, "virtualx.tbhdx.app_hp_ratio", 27)) == 0)
            {
                Stringptr = line;
                Stringptr += 27;
                gParamScale = strtof(Stringptr, NULL);
                gParamIndex = (int)DTS_PARAM_TBHDX_APP_HPRATIO_F32;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");
            }
            else if ((strncmp(line, "virtualx.tbhdx.app_extbass_level", 32)) == 0)
            {
                Stringptr = line;
                Stringptr += 32;
                gParamScale = strtof(Stringptr, NULL);
                gParamIndex = (int)DTS_PARAM_TBHDX_APP_EXTBASS_F32;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");
            }
            else if((strncmp(line, "virtualx.mbhl.frt_lowcross", 26)) == 0)
            {

                Stringptr = line;
                Stringptr += 26;
                gParamScale = strtof(Stringptr, NULL);
                gParamIndex = (int)DTS_PARAM_MBHL_APP_FRT_LOWCROSS_F32;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");

            }
            else if((strncmp(line, "virtualx.mbhl.frt_midcross", 26)) == 0)
            {
                Stringptr = line;
                Stringptr += 26;
                gParamScale = strtof(Stringptr, NULL);
                gParamIndex = (int)DTS_PARAM_MBHL_APP_FRT_MIDCROSS_F32;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");
            }
            else if ( (strncmp(line, "loudness-control-en", 19)) == 0)
            {
                Stringptr = line;
                Stringptr += 19;
                gParamValue = strtol(Stringptr, NULL, 0 );
                //printf("Truvolume_enable is %d\n",data.Truvolume_enable);
                gParamIndex = (int)DTS_PARAM_LOUDNESS_CONTROL_ENABLE_I32;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");
            }
            else if ((strncmp(line, "loudness-control-target-loudness", 32)) == 0)
            {
                Stringptr = line;
                Stringptr += 32;
                gParamValue = strtol(Stringptr, NULL, 0);
               // printf("targetlevel is %d\n",data.targetlevel);
                gParamIndex = (int)DTS_PARAM_LOUDNESS_CONTROL_TARGET_LOUDNESS_I32;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");
            }
            else if ((strncmp(line, "loudness-control-preset", 23)) == 0)
            {
                Stringptr = line;
                Stringptr += 23;
                gParamValue = strtol(Stringptr, NULL, 0);
                gParamIndex = (int)DTS_PARAM_LOUDNESS_CONTROL_PRESET_I32;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");
            }
            else if ((strncmp(line,"virtualx.tbhdx.discard",22)) == 0)
            {
                Stringptr = line;
                Stringptr += 22;
                gParamValue = strtol(Stringptr, NULL, 0);
                gParamIndex = (int)DTS_PARAM_TBHDX_PROCESS_DISCARD_I32;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");
            }
            else if((strncmp(line,"virtualx.mbhl.discard",21)) == 0)
            {
                Stringptr = line;
                Stringptr += 21;
                gParamValue = strtol(Stringptr, NULL, 0);
                gParamIndex = (int)DTS_PARAM_MBHL_PROCESS_DISCARD_I32;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");
            }
            else if((strncmp(line,"virtualx.vxlib1.tsx.discard",27)) == 0)
            {
                Stringptr = line;
                Stringptr += 27;
                gParamValue = strtol(Stringptr, NULL, 0);
                gParamIndex = (int)DTS_PARAM_TSX_PROCESS_DISCARD_I32;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");
            }
            else if((strncmp(line,"virtualx.vxlib1.height.discard",30)) == 0)
            {
                Stringptr = line;
                Stringptr += 30;
                gParamValue = strtol(Stringptr, NULL, 0);
                gParamIndex = (int)DTS_PARAM_TSX_HEIGHT_DISCARD_I32;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");
            }
            else if ((strncmp (line,"enable",6)) == 0)
            {
                Stringptr = line;
                Stringptr += 6;
                gParamValue = strtol(Stringptr, NULL, 0);
                gParamIndex = (int)VIRTUALX_PARAM_ENABLE;
                ret = create_audio_effect(&gAudioEffect[EFFECT_VIRTUALX], name16[EFFECT_VIRTUALX], EFFECT_VIRTUALX);
                if (ret < 0) {
                    printf("create Virtualx effect failed\n");
                    goto Error;
                }
                if (Virtualx_effect_func(gAudioEffect[gEffectIndex], gParamIndex, gParamValue,gParamScale) < 0)
                    printf("Virtualx Test failed\n");
            }
       }
       printf("Please enter param: <27> to Exit\n");
       scanf("%d", &gEffectIndex);
       if (gEffectIndex == 27) {
            printf("Exit...\n");
            break;
        }

    }
    ret = 0;
Error:
    for (i = 0; i < EFFECT_MAX; i++) {
        if (gAudioEffect[i] != NULL) {
            gAudioEffect[i] = NULL;
            delete gAudioEffect[i];
        }
    }
    free(line);
    return ret;
}



