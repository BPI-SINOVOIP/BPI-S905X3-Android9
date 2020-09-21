/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "FBC"
#define LOG_FBC_TAG "CFbcCommunication"
#define LOG_NDEBUG 0

#include "CFbcCommunication.h"
#include <stdlib.h>
#include <string.h>
#include <CFbcLog.h>
#include "SystemControlClient.h"

CFbcCommunication *CFbcCommunication::mSingletonFBC = NULL;
CFbcCommunication *CFbcCommunication::GetSingletonFBC()
{
    LOGD("%s!\n", __FUNCTION__);
    if (mSingletonFBC == NULL) {
        mSingletonFBC = new CFbcCommunication();
    }

    return mSingletonFBC;
}

CFbcCommunication::CFbcCommunication()
{
    LOGD("%s!\n", __FUNCTION__);
    mFbcLinker = CFbcLinker::getFbcLinkerInstance();
    mFbcLinker->pUpgradeIns->setObserver(this);
}

CFbcCommunication::~CFbcCommunication()
{
    if (mFbcLinker != NULL) {
        delete mFbcLinker;
        mFbcLinker = NULL;
    }
}

int CFbcCommunication::SendCommandToFBC(unsigned char *data, int count, int flag)
{
    if (mFbcLinker == NULL) {
        LOGD("%s: getFbcLinker failed!\n", __FUNCTION__);
        return -1;
    }

    if (CMD_FBC_RELEASE == (fbc_command_t)*(data+1) && mFbcLinker != NULL) {
        LOGD("%s: don't release FBC!\n", __FUNCTION__);
        delete mFbcLinker;
        mFbcLinker = NULL;
        return -1;
    } else {
        return mFbcLinker->sendCommandToFBC(data, count, flag);
    }
}

int CFbcCommunication::cfbcSetValueInt(COMM_DEV_TYPE_E fromDev, int cmd_id, int value, int value_count)
{
    LOGV("%s, cmd_id=%d, value=%d, value_count=%d", __FUNCTION__, cmd_id, value, value_count);
    if (value_count != 1 && value_count != 4)
        return -1;

    unsigned char cmd_buf[7];
    memset(cmd_buf, 0, 7);
    cmd_buf[0] = fromDev;
    cmd_buf[1] = cmd_id;
    if (value_count == 1) {
        cmd_buf[2] = value;
    } else {
        cmd_buf[2] = (value >> 0) & 0xFF;
        cmd_buf[3] = (value >> 8) & 0xFF;
        cmd_buf[4] = (value >> 16) & 0xFF;
        cmd_buf[5] = (value >> 24) & 0xFF;
    }

    return SendCommandToFBC(cmd_buf, value_count+2, 0);
}

int CFbcCommunication::cfbcGetValueInt(COMM_DEV_TYPE_E fromDev, int cmd_id, int *value, int value_count)
{
    LOGV("%s, cmd_id=%d, value_count=%d", __FUNCTION__, cmd_id, value_count);
    int ret = -1;
    unsigned char cmd_buf[7];//2 + 4 + 1
    memset(cmd_buf, 0, 7);
    cmd_buf[0] = fromDev;
    cmd_buf[1] = cmd_id;

    ret = SendCommandToFBC(cmd_buf, 2, 1);
    if (ret > -1) {
        if (value_count == 1) {
            *value = cmd_buf[2];
        } else {
            *value |= cmd_buf[2] & 0xFF;
            *value |= (cmd_buf[3] & 0xFF) << 8;
            *value |= (cmd_buf[4] & 0xFF) << 16;
            *value |= (cmd_buf[5] & 0xFF) << 24;
        }
    }
    return ret;
}

int CFbcCommunication::cfbc_Set_Gain_Red(COMM_DEV_TYPE_E fromDev, int value)
{
    return cfbcSetValueInt(fromDev, VPU_CMD_RED_GAIN_DEF, value, 1);
}

int CFbcCommunication::cfbc_Get_Gain_Red(COMM_DEV_TYPE_E fromDev, int *value)
{
    return cfbcGetValueInt(fromDev, VPU_CMD_RED_GAIN_DEF | 0x80, value);
}

int CFbcCommunication::cfbc_Set_Gain_Green(COMM_DEV_TYPE_E fromDev, int value)
{
    return cfbcSetValueInt(fromDev, VPU_CMD_GREEN_GAIN_DEF, value, 1);
}

int CFbcCommunication::cfbc_Get_Gain_Green(COMM_DEV_TYPE_E fromDev, int *value)
{
    return cfbcGetValueInt(fromDev, VPU_CMD_GREEN_GAIN_DEF | 0x80, value);
}

int CFbcCommunication::cfbc_Set_Gain_Blue(COMM_DEV_TYPE_E fromDev, int value)
{
    return cfbcSetValueInt(fromDev, VPU_CMD_BLUE_GAIN_DEF, value, 1);
}

int CFbcCommunication::cfbc_Get_Gain_Blue(COMM_DEV_TYPE_E fromDev, int *value)
{
    return cfbcGetValueInt(fromDev, VPU_CMD_BLUE_GAIN_DEF | 0x80, value);
}

int CFbcCommunication::cfbc_Set_Offset_Red(COMM_DEV_TYPE_E fromDev, int value)
{
    return cfbcSetValueInt(fromDev, VPU_CMD_PRE_RED_OFFSET_DEF, value, 1);
}

int CFbcCommunication::cfbc_Get_Offset_Red(COMM_DEV_TYPE_E fromDev, int *value)
{
    return cfbcGetValueInt(fromDev, VPU_CMD_PRE_RED_OFFSET_DEF | 0x80, value);
}

int CFbcCommunication::cfbc_Set_Offset_Green(COMM_DEV_TYPE_E fromDev, int value)
{
    return cfbcSetValueInt(fromDev, VPU_CMD_PRE_GREEN_OFFSET_DEF, value, 1);
}

int CFbcCommunication::cfbc_Get_Offset_Green(COMM_DEV_TYPE_E fromDev, int *value)
{
    return cfbcGetValueInt(fromDev, VPU_CMD_PRE_GREEN_OFFSET_DEF | 0x80, value);
}

int CFbcCommunication::cfbc_Set_Offset_Blue(COMM_DEV_TYPE_E fromDev, int value)
{
    return cfbcSetValueInt(fromDev, VPU_CMD_PRE_BLUE_OFFSET_DEF, value, 1);
}

int CFbcCommunication::cfbc_Get_Offset_Blue(COMM_DEV_TYPE_E fromDev, int *value)
{
    return cfbcGetValueInt(fromDev, VPU_CMD_PRE_BLUE_OFFSET_DEF | 0x80, value);
}

int CFbcCommunication::cfbc_Set_WB_Initial(COMM_DEV_TYPE_E fromDev, int value)
{
    return cfbcSetValueInt(fromDev, VPU_CMD_WB, value, 1);
}

int CFbcCommunication::cfbc_Get_WB_Initial(COMM_DEV_TYPE_E fromDev, int *value)
{
    return cfbcGetValueInt(fromDev, VPU_CMD_WB | 0x80, value);
}

int CFbcCommunication::cfbc_Set_ColorTemp_Mode(COMM_DEV_TYPE_E fromDev, int value)
{
    int fbcValue = value;
    switch (value) {
    case 0:     //standard
        fbcValue = 1;
        break;
    case 1:     //warm
        fbcValue = 2;
        break;
    case 2:    //cold
        fbcValue = 0;
        break;
    default:
        break;
    }
    LOGD("before set fbcValue = %d", fbcValue);

    return cfbcSetValueInt(fromDev, VPU_CMD_COLOR_TEMPERATURE_DEF, fbcValue, 1);
}

int CFbcCommunication::cfbc_Get_ColorTemp_Mode(COMM_DEV_TYPE_E fromDev, int *value)
{
    cfbcGetValueInt(fromDev, VPU_CMD_COLOR_TEMPERATURE_DEF | 0x80, value);

    switch (*value) {
    case 0:     //cold
        *value = 2;
        break;
    case 1:     //standard
        *value = 0;
        break;
    case 2:    //warm
        *value = 1;
        break;
    default:
        break;
    }

    return 0;
}

int CFbcCommunication::cfbc_Set_Test_Pattern(COMM_DEV_TYPE_E fromDev, int value)
{
    return cfbcSetValueInt(fromDev, VPU_CMD_PATTEN_SEL, value, 1);
}

int CFbcCommunication::cfbc_Get_Test_Pattern(COMM_DEV_TYPE_E fromDev, int *value)
{
    return cfbcGetValueInt(fromDev, VPU_CMD_PATTEN_SEL | 0x80, value);
}

int CFbcCommunication::cfbc_Set_Picture_Mode(COMM_DEV_TYPE_E fromDev, int value)
{
    return cfbcSetValueInt(fromDev, VPU_CMD_PICTURE_MODE, value, 1);
}

int CFbcCommunication::cfbc_Get_Picture_Mode(COMM_DEV_TYPE_E fromDev, int *value)
{
    return cfbcGetValueInt(fromDev, VPU_CMD_PICTURE_MODE | 0x80, value);
}

int CFbcCommunication::cfbc_Set_Contrast(COMM_DEV_TYPE_E fromDev, int value)
{
    return cfbcSetValueInt(fromDev, VPU_CMD_CONTRAST_DEF, value, 1);
}

int CFbcCommunication::cfbc_Get_Contrast(COMM_DEV_TYPE_E fromDev, int *value)
{
    return cfbcGetValueInt(fromDev, VPU_CMD_CONTRAST_DEF | 0x80, value);
}

int CFbcCommunication::cfbc_Set_Brightness(COMM_DEV_TYPE_E fromDev, int value)
{
    return cfbcSetValueInt(fromDev, VPU_CMD_BRIGHTNESS_DEF, value, 1);
}

int CFbcCommunication::cfbc_Get_Brightness(COMM_DEV_TYPE_E fromDev, int *value)
{
    return cfbcGetValueInt(fromDev, VPU_CMD_BRIGHTNESS_DEF | 0x80, value);
}

int CFbcCommunication::cfbc_Set_Saturation(COMM_DEV_TYPE_E fromDev, int value)
{
    return cfbcSetValueInt(fromDev, VPU_CMD_COLOR_DEF, value, 1);
}

int CFbcCommunication::cfbc_Get_Saturation(COMM_DEV_TYPE_E fromDev, int *value)
{
    return cfbcGetValueInt(fromDev, VPU_CMD_COLOR_DEF | 0x80, value);
}

int CFbcCommunication::cfbc_Set_HueColorTint(COMM_DEV_TYPE_E fromDev, int value)
{
    return cfbcSetValueInt(fromDev, VPU_CMD_HUE_DEF, value, 1);
}

int CFbcCommunication::cfbc_Get_HueColorTint(COMM_DEV_TYPE_E fromDev, int *value)
{
    return cfbcGetValueInt(fromDev, VPU_CMD_HUE_DEF | 0x80, value);
}

int CFbcCommunication::cfbc_Set_Backlight(COMM_DEV_TYPE_E fromDev, int value)
{
    return cfbcSetValueInt(fromDev, VPU_CMD_BACKLIGHT_DEF, value, 1);
}

int CFbcCommunication::cfbc_Get_Backlight(COMM_DEV_TYPE_E fromDev, int *value)
{
    return cfbcGetValueInt(fromDev, VPU_CMD_BACKLIGHT_DEF | 0x80, value);
}

int CFbcCommunication::cfbc_Set_Source(COMM_DEV_TYPE_E fromDev, int value)
{
    return cfbcSetValueInt(fromDev, VPU_CMD_SOURCE, value, 1);
}

int CFbcCommunication::cfbc_Get_Source(COMM_DEV_TYPE_E fromDev, int *value)
{
    return cfbcGetValueInt(fromDev, VPU_CMD_SOURCE | 0x80, value);
}

int CFbcCommunication::cfbc_Set_Mute(COMM_DEV_TYPE_E fromDev, int value)
{
    LOGD("cfbc_Set_Mute = %d", value);
    return cfbcSetValueInt(fromDev, AUDIO_CMD_SET_MUTE, value, 1);
}

int CFbcCommunication::cfbc_Set_FBC_Audio_Source(COMM_DEV_TYPE_E fromDev, int value)
{
    return cfbcSetValueInt(fromDev, AUDIO_CMD_SET_SOURCE, value, 1);
}

int CFbcCommunication::cfbc_Get_Mute(COMM_DEV_TYPE_E fromDev, int *value)
{
    return cfbcGetValueInt(fromDev, AUDIO_CMD_GET_MUTE, value);
}

int CFbcCommunication::cfbc_Set_Volume_Bar(COMM_DEV_TYPE_E fromDev, int value)
{
    return cfbcSetValueInt(fromDev, AUDIO_CMD_SET_VOLUME_BAR, value, 1);
}

int CFbcCommunication::cfbc_Get_Volume_Bar(COMM_DEV_TYPE_E fromDev, int *value)
{
    return cfbcGetValueInt(fromDev, AUDIO_CMD_GET_VOLUME_BAR, value);
}

int CFbcCommunication::cfbc_Set_Balance(COMM_DEV_TYPE_E fromDev, int value)
{
    return cfbcSetValueInt(fromDev, AUDIO_CMD_SET_BALANCE, value, 1);
}

int CFbcCommunication::cfbc_Get_Balance(COMM_DEV_TYPE_E fromDev, int *value)
{
    return cfbcGetValueInt(fromDev, AUDIO_CMD_GET_BALANCE, value);
}

int CFbcCommunication::cfbc_Set_Master_Volume(COMM_DEV_TYPE_E fromDev, int value)
{
    return cfbcSetValueInt(fromDev, AUDIO_CMD_SET_MASTER_VOLUME, value, 1);
}

int CFbcCommunication::cfbc_Get_Master_Volume(COMM_DEV_TYPE_E fromDev, int *value)
{
    return cfbcGetValueInt(fromDev, AUDIO_CMD_GET_MASTER_VOLUME, value);
}

int CFbcCommunication::cfbc_Set_CM(COMM_DEV_TYPE_E fromDev, unsigned char value)
{
    unsigned char cmd[4];

    cmd[0] = fromDev;
    cmd[1] = VPU_CMD_ENABLE;
    cmd[2] = VPU_MODULE_CM2;
    cmd[3] = value;//value:0~1
    return SendCommandToFBC(cmd, 4, 0);
}

int CFbcCommunication::cfbc_Get_CM(COMM_DEV_TYPE_E fromDev, int *value)
{
    unsigned char cmd[512];
    cmd[0] = fromDev;
    cmd[1] = VPU_CMD_ENABLE | 0x80;
    cmd[2] = VPU_MODULE_CM2;

    SendCommandToFBC(cmd, 3, 1);
    *value = cmd[3];
    return 0;
}
int CFbcCommunication::cfbc_Set_DNLP(COMM_DEV_TYPE_E fromDev, unsigned char value)
{
    unsigned char cmd[512];

    cmd[0] = fromDev;
    cmd[1] = VPU_CMD_ENABLE;
    cmd[2] = VPU_MODULE_DNLP;
    cmd[3] = value;//value:0~1

    return SendCommandToFBC(cmd, 4, 0);
}

int CFbcCommunication::cfbc_Get_DNLP(COMM_DEV_TYPE_E fromDev, int *value)
{
    unsigned char cmd[512];

    cmd[0] = fromDev;
    cmd[1] = VPU_CMD_ENABLE | 0x80;
    cmd[2] = VPU_MODULE_DNLP;

    SendCommandToFBC(cmd, 3, 1);
    *value = cmd[3];
    return 0;
}

int CFbcCommunication::cfbc_Get_FBC_MAINCODE_Version(COMM_DEV_TYPE_E fromDev, char sw_ver[], char build_time[], char git_ver[], char git_branch[], char build_name[])
{
    int rx_len = 0, tmp_ind = 0;
    unsigned char cmd[512];

    if (sw_ver == NULL || build_time == NULL || git_ver == NULL || git_branch == NULL || build_name == NULL) {
        return -1;
    }

    memset(cmd, 0, 512);
    cmd[0] = fromDev;
    cmd[1] = CMD_FBC_MAIN_CODE_VERSION;
    rx_len = SendCommandToFBC(cmd, 2, 1);

    sw_ver[0] = 0;
    build_time[0] = 0;
    git_ver[0] = 0;
    git_branch[0] = 0;
    build_name[0] = 0;

    if (rx_len <= 0) {
        return -1;
    }

    if (cmd[0] != CMD_FBC_MAIN_CODE_VERSION || cmd[1] != 0x88 || cmd[2] != 0x99) {
        return -1;
    }

    tmp_ind = 3;

    strcpy(sw_ver, (char *)(cmd + tmp_ind));
    tmp_ind += strlen(sw_ver);
    tmp_ind += 1;

    strcpy(build_time, (char *)(cmd + tmp_ind));
    tmp_ind += strlen(build_time);
    tmp_ind += 1;

    strcpy(git_ver, (char *)(cmd + tmp_ind));
    tmp_ind += strlen(git_ver);
    tmp_ind += 1;

    strcpy(git_branch, (char *)(cmd + tmp_ind));
    tmp_ind += strlen(git_branch);
    tmp_ind += 1;

    strcpy(build_name, (char *)(cmd + tmp_ind));
    tmp_ind += strlen(build_name);
    tmp_ind += 1;
    LOGD("sw_ver=%s, buildt=%s, gitv=%s, gitb=%s,bn=%s", sw_ver, build_time, git_ver, git_branch, build_name);
    return 0;
}

int CFbcCommunication::cfbc_Set_FBC_Factory_SN(COMM_DEV_TYPE_E fromDev, const char *pSNval)
{
    unsigned char cmd[512];
    int len = strlen(pSNval);
    cmd[0] = fromDev;
    cmd[1] = CMD_SET_FACTORY_SN;
    memcpy(cmd + 2, pSNval, len);

    LOGD("cmd : %s\n", cmd);
    return SendCommandToFBC(cmd, len+2, 0);
}

int CFbcCommunication::cfbc_Get_FBC_Factory_SN(COMM_DEV_TYPE_E fromDev, char FactorySN[])
{
    int rx_len = 0;
    unsigned char cmd[512];
    cmd[0] = fromDev;
    cmd[1] = CMD_GET_FACTORY_SN;
    rx_len = SendCommandToFBC(cmd, 2, 1);
    if (rx_len <= 0) {
        return -1;
    }
    strncpy(FactorySN, (char *)(cmd+2), rx_len);

    LOGD("panelModel=%s", FactorySN);
    return 0;
}

int CFbcCommunication::cfbc_Get_FBC_Get_PANel_INFO(COMM_DEV_TYPE_E fromDev, char panel_model[])
{
    int rx_len = 0;
    unsigned char cmd[512];
    cmd[0] = fromDev;
    cmd[1] = CMD_DEVICE_ID;
    rx_len = SendCommandToFBC(cmd, 2, 1);
    if (rx_len <= 0) {
        LOGD("%s failed!\n", __FUNCTION__);
        return -1;
    } else {
        strcpy(panel_model, (char *)(cmd + 2));
        LOGD("panelModel=%s", panel_model);
        return 0;
    }
}

int CFbcCommunication::cfbc_Set_FBC_panel_power_switch(COMM_DEV_TYPE_E fromDev, int switch_val)
{
    unsigned char cmd[512];
    cmd[0] = fromDev;
    cmd[1] = FBC_PANEL_POWER;
    cmd[2] = switch_val; //0 is fbc panel power off, 1 is panel power on.

    return SendCommandToFBC(cmd, 3, 0);
}

int CFbcCommunication::cfbc_Set_FBC_suspend(COMM_DEV_TYPE_E fromDev, int switch_val)
{
    unsigned char cmd[512];
    cmd[0] = fromDev;
    cmd[1] = FBC_SUSPEND_POWER;
    cmd[2] = switch_val; //0

    return SendCommandToFBC(cmd, 3, 0);
}

int CFbcCommunication::cfbc_Set_FBC_User_Setting_Default(COMM_DEV_TYPE_E fromDev, int param)
{
    unsigned char cmd[512];
    cmd[0] = fromDev;
    cmd[1] = FBC_USER_SETTING_DEFAULT;
    cmd[2] = param; //0
    return SendCommandToFBC(cmd, 3, 0);
}

int CFbcCommunication::cfbc_SendRebootToUpgradeCmd(COMM_DEV_TYPE_E fromDev, int value)
{
    return cfbcSetValueInt(fromDev, FBC_REBOOT_UPGRADE, value, 4);
}

int CFbcCommunication::cfbc_FBC_Send_Key_To_Fbc(COMM_DEV_TYPE_E fromDev, int keycode __unused, int param __unused)
{
    unsigned char cmd[512];
    cmd[0] = fromDev;
    cmd[1] = CMD_ACTIVE_KEY;
    cmd[2] = keycode;
    return SendCommandToFBC(cmd, 3, 0);
}

int CFbcCommunication::cfbc_Get_FBC_PANEL_REVERSE(COMM_DEV_TYPE_E fromDev, int *value)
{
    unsigned char cmd[512];
    cmd[0] = fromDev;
    cmd[1] = CMD_PANEL_INFO;
    //0://panel reverse
    //1://panel output_mode
    //2://panel byte num
    cmd[2] = 0;

    SendCommandToFBC(cmd, 3, 1);
    //cmd[0] cmdid-PANEL-INFO
    //cmd[1] param[0] - panel reverse
    *value = cmd[3];

    return 0;
}

int CFbcCommunication::cfbc_Get_FBC_PANEL_OUTPUT(COMM_DEV_TYPE_E fromDev, int *value)
{
    unsigned char cmd[512];
    cmd[0] = fromDev;
    cmd[1] = CMD_PANEL_INFO;
    //0://panel reverse
    //1://panel output_mode
    //2://panel byte num
    cmd[2] = 1;

    SendCommandToFBC(cmd, 3, 1);
    //cmd[0] cmdid-PANEL-INFO
    //cmd[1] param[0] - panel reverse
    *value = cmd[3];

    return 0;
}

int CFbcCommunication::cfbc_Set_FBC_project_id(COMM_DEV_TYPE_E fromDev, int prj_id)
{
    unsigned char cmd[512];
    cmd[0] = fromDev;
    cmd[1] = CMD_SET_PROJECT_SELECT;
    cmd[2] = prj_id;
    return SendCommandToFBC(cmd, 3, 0);
}

int CFbcCommunication::cfbc_Get_FBC_project_id(COMM_DEV_TYPE_E fromDev, int *prj_id)
{
    return cfbcGetValueInt(fromDev, CMD_GET_PROJECT_SELECT, prj_id);
}

int CFbcCommunication::fbcSetEyeProtection(COMM_DEV_TYPE_E fromDev, int mode)
{
    unsigned char cmd[3];
    cmd[0] = fromDev;
    cmd[1] = VPU_CMD_EYE_PROTECTION;
    cmd[2] = mode;
    return SendCommandToFBC(cmd, 3, 0);
}

int CFbcCommunication::fbcSetGammaValue(COMM_DEV_TYPE_E fromDev, int gamma_curve, int is_save)
{
    unsigned char cmd[4];
    cmd[0] = fromDev;
    cmd[1] = VPU_CMD_USER_GAMMA;
    cmd[2] = gamma_curve;
    cmd[3] = is_save;
    return SendCommandToFBC(cmd, 4, 0);
}

int CFbcCommunication::fbcSetGammaPattern(COMM_DEV_TYPE_E fromDev, unsigned short gamma_r, unsigned short gamma_g, unsigned short gamma_b)
{
    unsigned char cmd[5];
    cmd[0] = fromDev;
    cmd[1] = VPU_CMD_GAMMA_PATTERN;
    cmd[2] = gamma_r;
    cmd[3] = gamma_g;
    cmd[4] = gamma_b;
    return SendCommandToFBC(cmd, 5, 0);
}

int CFbcCommunication::cfbc_Set_Gamma(COMM_DEV_TYPE_E fromDev, unsigned char value)
{
    unsigned char cmd[512];
    cmd[0] = fromDev;
    cmd[1] = VPU_CMD_ENABLE;
    cmd[2] = VPU_MODULE_GAMMA;
    cmd[3] = value;//value:0~1
    return SendCommandToFBC(cmd, 4, 0);
}

int CFbcCommunication::cfbc_Get_Gamma(COMM_DEV_TYPE_E fromDev, int *value)
{
    unsigned char cmd[512];
    cmd[0] = fromDev;
    cmd[1] = VPU_CMD_ENABLE | 0x80;
    cmd[2] = VPU_MODULE_GAMMA;

    SendCommandToFBC(cmd, 3, 1);
    *value = cmd[3];
    return 0;
}

int CFbcCommunication::cfbc_Set_VMute(COMM_DEV_TYPE_E fromDev, unsigned char value)
{
    unsigned char cmd[512];
    cmd[0] = fromDev;
    cmd[1] = VPU_CMD_USER_VMUTE;
    cmd[2] = value;
    LOGD("cfbc_Set_VMute=%d", cmd[2]);

    return SendCommandToFBC(cmd, 3, 0);
}

int CFbcCommunication::cfbc_Set_WhiteBalance_OnOff(COMM_DEV_TYPE_E fromDev, unsigned char value)
{
    unsigned char cmd[512];
    cmd[0] = fromDev;
    cmd[1] = VPU_CMD_ENABLE;
    cmd[2] = VPU_MODULE_WB;
    cmd[3] = value;//value:0~1
    return SendCommandToFBC(cmd, 4, 0);
}

int CFbcCommunication::cfbc_Get_WhiteBalance_OnOff(COMM_DEV_TYPE_E fromDev, int *value)
{
    unsigned char cmd[512];
    cmd[0] = fromDev;
    cmd[1] = VPU_CMD_ENABLE | 0x80;
    cmd[2] = VPU_MODULE_WB;

    SendCommandToFBC(cmd, 3, 1);
    *value = cmd[3];
    return 0;
}

int CFbcCommunication::cfbc_Set_WB_Batch(COMM_DEV_TYPE_E fromDev, unsigned char mode, unsigned char r_gain, unsigned char g_gain, unsigned char b_gain, unsigned char r_offset, unsigned char g_offset, unsigned char b_offset)
{
    unsigned char cmd[9];
    cmd[0] = fromDev;
    cmd[1] = VPU_CMD_WB_VALUE;
    cmd[2] = mode;
    cmd[3] = r_gain;
    cmd[4] = g_gain;
    cmd[5] = b_gain;
    cmd[6] = r_offset;
    cmd[7] = g_offset;
    cmd[8] = b_offset;

    return SendCommandToFBC(cmd, 9, 0);
}

int CFbcCommunication::cfbc_TestPattern_Select(COMM_DEV_TYPE_E fromDev, int value)
{
    int ret = -1;
    unsigned char cmd[4];

    LOGD("Call vpp 63 2 1\n");
    cmd[0] = fromDev;
    cmd[1] = VPU_CMD_SRCIF;
    cmd[2] = 2;
    cmd[3] = 1;
    ret = SendCommandToFBC(cmd, 4, 0);//close csc0
    if (ret == 0) {
        LOGD("Call vpp 9 11 1\n");
        cmd[0] = fromDev;
        cmd[1] = VPU_CMD_ENABLE;
        cmd[2] = 11;
        cmd[3] = 1;
        ret = SendCommandToFBC(cmd, 4, 0);
        if (ret == 0) {
            LOGD("Call vpp 42 1-17\n");
            cfbcSetValueInt(fromDev, VPU_CMD_PATTEN_SEL, value, 1);
        }
    }

    return ret;
}

int CFbcCommunication::cfbc_WhiteBalance_GrayPattern_OnOff(COMM_DEV_TYPE_E fromDev, int onOff)
{
    int ret = -1;
    unsigned char cmd[4];

    if (onOff == 0) { //On
        LOGD("Call vpp 63 2 1");
        cmd[0] = fromDev;
        cmd[1] = VPU_CMD_SRCIF;
        cmd[2] = 2;
        cmd[3] = 1;
        ret = SendCommandToFBC(cmd, 4, 0);//close csc0
        if (ret == 0) {
            LOGD("Call vpp 9 9 0");
            cmd[0] = fromDev;
            cmd[1] = VPU_CMD_ENABLE;
            cmd[2] = 9;
            cmd[3] = 0;
            ret = SendCommandToFBC(cmd, 4, 0);//close csc1
            if (ret == 0) {
                LOGD("Call vpp 9 10 0");
                cmd[0] = fromDev;
                cmd[1] = VPU_CMD_ENABLE;
                cmd[2] = 10;
                cmd[3] = 0;
                ret = SendCommandToFBC(cmd, 4, 0);//close dnlp
                if (ret == 0) {
                    LOGD("Call vpp 9 8 0");
                    cmd[0] = fromDev;
                    cmd[1] = VPU_CMD_ENABLE;
                    cmd[2] = 8;
                    cmd[3] = 0;
                    ret = SendCommandToFBC(cmd, 4, 0);//close cm
                }
            }
        }
    } else { //Off
        LOGD("Call vpp 63 2 2");
        cmd[0] = fromDev;
        cmd[1] = VPU_CMD_SRCIF;
        cmd[2] = 2;
        cmd[3] = 2;
        ret = SendCommandToFBC(cmd, 4, 0);
        if (ret == 0) {
            LOGD("Call vpp 9 9 1");
            cmd[0] = fromDev;
            cmd[1] = VPU_CMD_ENABLE;
            cmd[2] = 9;
            cmd[3] = 1;
            ret = SendCommandToFBC(cmd, 4, 0);//open csc1
            if (ret == 0) {
                LOGD("Call vpp 9 10 1");
                cmd[0] = fromDev;
                cmd[1] = VPU_CMD_ENABLE;
                cmd[2] = 10;
                cmd[3] = 1;
                ret = SendCommandToFBC(cmd, 4, 0);//open dnlp
                if (ret == 0) {
                    LOGD("Call vpp 9 8 1");
                    cmd[0] = fromDev;
                    cmd[1] = VPU_CMD_ENABLE;
                    cmd[2] = 8;
                    cmd[3] = 1;
                    ret = SendCommandToFBC(cmd, 4, 0);//open cm
                }
            }
        }
    }
    return ret;
}

int CFbcCommunication::cfbc_WhiteBalance_SetGrayPattern(COMM_DEV_TYPE_E fromDev, unsigned char value)
{
    int ret = -1;
    unsigned char cmd[5];
    cmd[0] = fromDev;
    cmd[1] = VPU_CMD_GRAY_PATTERN;
    cmd[2] = value;
    cmd[3] = value;
    cmd[4] = value;
    ret = SendCommandToFBC(cmd, 5, 0);
    return ret;
}

int CFbcCommunication::cfbc_Set_Auto_Backlight_OnOff(COMM_DEV_TYPE_E fromDev, unsigned char value)
{
    unsigned char cmd[3];
    cmd[0] = fromDev;
    cmd[1] = CMD_SET_AUTO_BACKLIGHT_ONFF;
    cmd[2] = value;
    LOGD("cfbc_Set_naturelight_onoff\n");
    return SendCommandToFBC(cmd, 3, 0);
}

int CFbcCommunication::cfbc_Get_Auto_Backlight_OnOff(COMM_DEV_TYPE_E fromDev, int *value)
{
    LOGD("cfbc_get_naturelight_onoff\n");
    return cfbcGetValueInt(fromDev, CMD_GET_AUTO_BACKLIGHT_ONFF, value);
}

int CFbcCommunication::cfbc_Get_WB_Batch(COMM_DEV_TYPE_E fromDev, unsigned char mode,
    unsigned char *r_gain, unsigned char *g_gain, unsigned char *b_gain,
    unsigned char *r_offset, unsigned char *g_offset, unsigned char *b_offset)
{
    unsigned char cmd[512] = {0};
    cmd[0] = fromDev;
    cmd[1] = VPU_CMD_WB_VALUE | 0x80;
    cmd[2] = mode;

    SendCommandToFBC(cmd, 3, 1);
    //*mode = cmd[1];
    *r_gain = cmd[3];
    *g_gain = cmd[4];
    *b_gain = cmd[5];
    *r_offset = cmd[6];
    *g_offset = cmd[7];
    *b_offset = cmd[8];
    return 0;
}

int CFbcCommunication::cfbc_Set_backlight_onoff(COMM_DEV_TYPE_E fromDev, int value)
{
    return cfbcSetValueInt(fromDev, VPU_CMD_BACKLIGHT_EN, value, 1);
}

int CFbcCommunication::cfbc_Get_backlight_onoff(COMM_DEV_TYPE_E fromDev, int *value)
{
    return cfbcGetValueInt(fromDev, VPU_CMD_BACKLIGHT_EN, value);
}

int CFbcCommunication::cfbc_Set_LVDS_SSG_Set(COMM_DEV_TYPE_E fromDev, int value)
{
    return cfbcSetValueInt(fromDev, CMD_LVDS_SSG_SET, value, 1);
}

int CFbcCommunication::cfbc_Set_AUTO_ELEC_MODE(COMM_DEV_TYPE_E fromDev, int value)
{
    LOGD("%s  cmd =0x%x, value=%d", __FUNCTION__, VPU_CMD_SET_ELEC_MODE, value);
    return cfbcSetValueInt(fromDev, VPU_CMD_SET_ELEC_MODE, value, 1);
}

int CFbcCommunication::cfbc_Set_Thermal_state(COMM_DEV_TYPE_E fromDev, int value)
{
    LOGD("%s  cmd =0x%x, data%d\n", __FUNCTION__, CMD_HDMI_STAT, value);
    return cfbcSetValueInt(fromDev, CMD_HDMI_STAT, value, 4);
}

int CFbcCommunication::cfbc_Set_LightSensor_N310(COMM_DEV_TYPE_E fromDev, int value)
{
    return cfbcSetValueInt(fromDev, 0x84, value, 1);
}

int CFbcCommunication::cfbc_Set_Dream_Panel_N310(COMM_DEV_TYPE_E fromDev, int value)
{
    return cfbcSetValueInt(fromDev, 0x83, value, 1);
}

int CFbcCommunication::cfbc_Set_MULT_PQ_N310(COMM_DEV_TYPE_E fromDev, int value)
{
    return cfbcSetValueInt(fromDev, 0x81, value, 1);
}

int CFbcCommunication::cfbc_Set_MEMC_N310(COMM_DEV_TYPE_E fromDev, int value)
{
    return cfbcSetValueInt(fromDev, 0x82, value, 1);
}

int CFbcCommunication::cfbc_Set_Bluetooth_IIS_onoff(COMM_DEV_TYPE_E fromDev, int value)
{
    return cfbcSetValueInt(fromDev, CMD_BLUETOOTH_I2S_STATUS, value, 1);
}

int CFbcCommunication::cfbc_Get_Bluetooth_IIS_onoff(COMM_DEV_TYPE_E fromDev, int *value)
{
    return cfbcGetValueInt(fromDev, CMD_BLUETOOTH_I2S_STATUS | 0x80, value);
}

int CFbcCommunication::cfbc_Set_Led_onoff(COMM_DEV_TYPE_E fromDev, int val_1, int val_2, int val_3)
{
    unsigned char cmd[512];
    cmd[0] = fromDev;
    cmd[1] = CMD_SET_LED_MODE;
    cmd[2] = val_1;
    cmd[3] = val_2;
    cmd[4] = val_3;
    return SendCommandToFBC(cmd, 5, 0);
}

int CFbcCommunication::cfbc_Set_LockN_state(COMM_DEV_TYPE_E fromDev, int value)
{
    LOGD("%s  cmd =0x%x, data%d\n", __FUNCTION__, CMD_SET_LOCKN_DISABLE, value);
    return cfbcSetValueInt(fromDev, CMD_SET_LOCKN_DISABLE, value, 1);
}

int CFbcCommunication::cfbc_SET_SPLIT_SCREEN_DEMO(COMM_DEV_TYPE_E fromDev, int value)
{
    LOGD("%s,cmd[%#x], data[%d]\n", __FUNCTION__, CMD_SET_SPLIT_SCREEN_DEMO, value);
    return cfbcSetValueInt(fromDev, CMD_SET_SPLIT_SCREEN_DEMO, value, 1);
}

int CFbcCommunication::fbcSetPanelStatus(COMM_DEV_TYPE_E fromDev, int value)
{
    return cfbcSetValueInt(fromDev, CMD_PANEL_ON_OFF, value, 1);
}

int CFbcCommunication::fbcGetPanelStatus(COMM_DEV_TYPE_E fromDev, int *value)
{
    return cfbcGetValueInt(fromDev, CMD_PANEL_ON_OFF | 0x80, value);
}

int CFbcCommunication::cfbc_Set_Fbc_Uboot_Stage(COMM_DEV_TYPE_E fromDev, int value)
{
    return cfbcSetValueInt(fromDev, CMD_SET_UBOOT_STAGE, value, 1);
}

int CFbcCommunication::fbcStartUpgrade(char *file_name, int mode, int upgrade_blk_size)
{
    if (mFbcLinker == NULL) {
        LOGD("%s: getFbcLinker failed!\n", __FUNCTION__);
        return -1;
    } else {
        return mFbcLinker->startFBCUpgrade(file_name, mode, upgrade_blk_size);
    }
}

int CFbcCommunication::fbcRelease()
{
    return cfbcSetValueInt(COMM_DEV_SERIAL, CMD_FBC_RELEASE, 0, 0);
}

int CFbcCommunication::cfbc_EnterPCMode(int value)
{
    return cfbcSetValueInt(COMM_DEV_SERIAL, CMD_SET_ENTER_PCMODE, value, 1);
}

void CFbcCommunication:: onUpgradeStatus(int status, int progress)
{
    /*TvEvent::UpgradeFBCEvent ev;
    ev.mState = state;
    ev.param = param;
    sendTvEvent(ev);*/
    sp<SystemControlClient> systemControlClient = new SystemControlClient();
    if (systemControlClient != NULL) {
        systemControlClient->UpdateFBCUpgradeStatus(status, progress);
    } else {
        LOGD("%s: binder to systemControl failed!\n", __FUNCTION__);
    }
}
