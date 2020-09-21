/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "tvserver"
#define LOG_TV_TAG "CVpp"

#include "CVpp.h"
#include <CTvLog.h>
#include "../tvsetting/CTvSetting.h"
#include <cutils/properties.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <dlfcn.h>
#include "CTvLog.h"
#include <tvconfig.h>
#include "CAv.h"
#include "../tvin/CHDMIRxManager.h"


#define PI 3.14159265358979

CVpp *CVpp::mInstance;
CVpp *CVpp::getInstance()
{
    if (NULL == mInstance) mInstance = new CVpp();
    return mInstance;
}

CVpp::CVpp()
{

}

CVpp::~CVpp()
{

}

int CVpp::Vpp_Init()
{
    if (SSMReadNonStandardValue() & 1) {
        Set_Fixed_NonStandard(0); //close
    } else {
        Set_Fixed_NonStandard(2); //open
    }

    return 0;
}

int CVpp::LoadVppSettings(tv_source_input_t tv_source_input, tvin_sig_fmt_t sig_fmt, tvin_trans_fmt_t trans_fmt)
{
    source_input_param_t source_input_param;
    source_input_param.source_input = tv_source_input;
    source_input_param.sig_fmt = sig_fmt;
    source_input_param.trans_fmt = trans_fmt;
    return tvLoadPQSettings(source_input_param);
}

int CVpp::SetPQMode(vpp_picture_mode_t pq_mode, tv_source_input_t tv_source_input __unused,
                       tvin_sig_fmt_t sig_fmt __unused, tvin_trans_fmt_t trans_fmt __unused,
                       is_3d_type_t is3d __unused, int is_save, int is_autoswitch)
{
    return tvSetPQMode(pq_mode, is_save, is_autoswitch);
}

vpp_picture_mode_t CVpp::GetPQMode(tv_source_input_t tv_source_input __unused)
{
    return tvGetPQMode();
}

int CVpp::SavePQMode(vpp_picture_mode_t pq_mode, tv_source_input_t tv_source_input __unused)
{
    return tvSavePQMode(pq_mode);
}

void CVpp::enableMonitorMode(bool enable)
{
    if (enable) {
        tvWriteSysfs(DI_NR2_ENABLE, "0");
        tvWriteSysfs(AMVECM_PC_MODE, "0");
        /*if (mHdmiOutFbc && fbcIns) {
            fbcIns->cfbc_EnterPCMode(0);
        }*/
    } else {
        tvWriteSysfs(DI_NR2_ENABLE, "1");
        tvWriteSysfs(AMVECM_PC_MODE, "1");
        /*if (mHdmiOutFbc && fbcIns) {
            fbcIns->cfbc_EnterPCMode(1);
        }*/
    }
}

int CVpp::VPP_SetCVD2Values()

{
    return tvSetCVD2Values();
}

int CVpp::VPP_SetBackLight_Switch(int value)
{
    LOGD("VPP_SetBackLight_Switch VPP_PANEL_BACKLIGHT_DEV_PATH : %d ", value);
    FILE *fp = fopen(VPP_PANEL_BACKLIGHT_DEV_PATH, "w");
    if (fp == NULL) {
        LOGE("Open VPP_PANEL_BACKLIGHT_DEV_PATH error(%s)!\n", strerror(errno));
        return -1;
    }

    fprintf(fp, "%d", value);
    fclose(fp);
    fp = NULL;
    return 0;
}

int CVpp::VPP_GetBackLight_Switch(void)
{
    int value;
    int ret;

    FILE *fp = fopen(VPP_PANEL_BACKLIGHT_DEV_PATH, "w");
    if (fp == NULL) {
        LOGE("Open VPP_PANEL_BACKLIGHT_DEV_PATH error(%s)!\n", strerror(errno));
        return -1;
    }

    ret = fscanf(fp, "%d", &value);
    if (ret == -1)
    {
        return -1;
    }
    LOGD("VPP_GetBackLight_Switch VPP_PANEL_BACKLIGHT_DEV_PATH : %d", value);
    fclose(fp);
    fp = NULL;
    if (value < 0) {
        return 0;
    } else {
        return value;
    }
}

int CVpp::FactoryGetTestPattern(void)
{
    unsigned char data = VPP_TEST_PATTERN_NONE;
    SSMReadTestPattern(&data);
    return data;
}

int CVpp::VPP_SetVideoCrop(int Voffset0, int Hoffset0, int Voffset1, int Hoffset1)
{
    char set_str[32];

    LOGD("VPP_SetVideoCrop /sys/class/video/crop : %d %d %d %d##", Voffset0, Hoffset0, Voffset1, Hoffset1);
    int fd = open("/sys/class/video/crop", O_RDWR);
    if (fd < 0) {
        LOGE("Open /sys/class/video/crop error(%s)!\n", strerror(errno));
        return -1;
    }

    memset(set_str, 0, 32);
    sprintf(set_str, "%d %d %d %d", Voffset0, Hoffset0, Voffset1, Hoffset1);
    write(fd, set_str, strlen(set_str));
    close(fd);

    return 0;
}

int CVpp::VPP_SetScalerPathSel(const unsigned int value)
{
    FILE *fp = NULL;

    fp = fopen(SYS_VIDEO_SCALER_PATH_SEL, "w");
    LOGD("VPP_SetScalerPathSel %s : %d", SYS_VIDEO_SCALER_PATH_SEL, value);
    if (fp == NULL) {
        LOGE("Open %s error(%s)!\n",SYS_VIDEO_SCALER_PATH_SEL, strerror(errno));
        return -1;
    }
    fprintf(fp, "%d", value);
    fclose(fp);
    fp = NULL;
    return 0;
}

int CVpp::VPP_SSMRestorToDefault(int id, bool resetAll)
{
    int i = 0;
    int tmp_panorama_nor = 0, tmp_panorama_full = 0;

    if (resetAll || VPP_DATA_USER_NATURE_SWITCH_START == id)
        SSMSaveUserNatureLightSwitch(1);

    if (resetAll || SSM_RW_UI_GRHPHY_BACKLIGHT_START == id)
        SSMSaveGraphyBacklight(100);

    if (resetAll || VPP_DATA_POS_DBC_START == id)
        SSMSaveDBCStart(0);

    if (resetAll || VPP_DATA_POS_DNLP_START == id)
        SSMSaveDnlpStart(0); //0: ON,1: OFF,default is on

    if (resetAll || VPP_DATA_APL_START == id)
        SSMSaveAPL(30);

    if (resetAll || VPP_DATA_APL2_START == id)
        SSMSaveAPL2(30);

    if (resetAll || VPP_DATA_BD_START == id)
        SSMSaveBD(30);

    if (resetAll || VPP_DATA_BP_START == id)
        SSMSaveBP(30);

    if (resetAll || VPP_DATA_POS_FBC_ELECMODE_START == id)
        SSMSaveFBCELECmodeVal(11);

    if (resetAll || VPP_DATA_POS_FBC_BACKLIGHT_START == id)
        SSMSaveFBCN360BackLightVal(10);

    if (resetAll || VPP_DATA_POS_FBC_COLORTEMP_START == id)
        SSMSaveFBCN360ColorTempVal(1); // standard colortemp

    if (resetAll || VPP_DATA_POS_FBC_N310_COLORTEMP_START == id)
        SSMSaveFBCN310ColorTempVal(0);

    if (resetAll || VPP_DATA_POS_FBC_N310_LIGHTSENSOR_START == id)
        SSMSaveFBCN310LightsensorVal(0);

    if (resetAll || VPP_DATA_POS_FBC_N310_DREAMPANEL_START == id)
        SSMSaveFBCN310Dream_PanelVal(1);

    if (resetAll || VPP_DATA_POS_FBC_N310_MULTI_PQ_START == id)
        SSMSaveFBCN310MULT_PQVal(1);

    if (resetAll || VPP_DATA_POS_FBC_N310_MEMC_START == id)
        SSMSaveFBCN310MEMCVal(2);

    if (resetAll || VPP_DATA_POS_FBC_N310_BACKLIGHT_START == id)
        SSMSaveFBCN310BackLightVal(254);

    tmp_panorama_nor = VPP_PANORAMA_MODE_NORMAL;
    tmp_panorama_full = VPP_PANORAMA_MODE_FULL;
    for (i = 0; i < SOURCE_MAX; i++) {
        if (resetAll || VPP_DATA_POS_PANORAMA_START == id) {
            if ((i == SOURCE_HDMI1) ||
                (i == SOURCE_HDMI2) ||
                (i == SOURCE_HDMI3) ||
                (i == SOURCE_HDMI4)) {
                SSMSavePanoramaStart(i, tmp_panorama_full);
            } else {
                SSMSavePanoramaStart(i, tmp_panorama_nor);
            }
        }
    }

    return 0;
}

int CVpp::TV_SSMRecovery()
{
    int ret = 0 ;
    ret |= VPP_SSMRestorToDefault(0, true);

    return ret;
}
