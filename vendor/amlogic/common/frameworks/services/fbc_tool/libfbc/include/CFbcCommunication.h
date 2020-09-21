/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef C_FBC_COMMUNICATION_H
#define C_FBC_COMMUNICATION_H

#include <CFbcCommand.h>
#include "CFbcLinker.h"

typedef enum OUTPUT_MODE {
    T_1080P50HZ = 0,
    T_2160P50HZ420,
    T_1080P50HZ44410BIT,
    T_2160P50HZ42010BIT,
    T_2160P50HZ42210BIT,
    T_2160P50HZ444,
} OUTPUT_MODE_E;

typedef enum vpu_modules_e {
    VPU_MODULE_NULL = 0,
    VPU_MODULE_VPU,        //vpu unsigned int
    VPU_MODULE_TIMGEN,
    VPU_MODULE_PATGEN,
    VPU_MODULE_GAMMA,
    VPU_MODULE_WB,        //WhiteBalance
    VPU_MODULE_BC,        //Brightness&Contrast
    VPU_MODULE_BCRGB,    //RGB Brightness&Contrast
    VPU_MODULE_CM2,
    VPU_MODULE_CSC1,
    VPU_MODULE_DNLP,
    VPU_MODULE_CSC0,
    VPU_MODULE_OSD,
    VPU_MODULE_BLEND,
    VPU_MODULE_DEMURE,    //15
    VPU_MODULE_OUTPUT,    //LVDS/VX1 output
    VPU_MODULE_OSDDEC,    //OSD decoder
    VPU_MODULE_MAX,
} vpu_modules_t;

class CFbcCommunication : public CFbcUpgrade::IUpgradeFBCObserver {
public:
    static CFbcCommunication *GetSingletonFBC();
    CFbcCommunication();
    ~CFbcCommunication();
    int SendCommandToFBC(unsigned char *data, int count, int flag);
    int cfbc_Set_Gain_Red(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Get_Gain_Red(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_Gain_Green(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Get_Gain_Green(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_Gain_Blue(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Get_Gain_Blue(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_Offset_Red(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Get_Offset_Red(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_Offset_Green(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Get_Offset_Green(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_Offset_Blue(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Get_Offset_Blue(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_WB_Initial(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Get_WB_Initial(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_ColorTemp_Mode(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Get_ColorTemp_Mode(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_Test_Pattern(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Get_Test_Pattern(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_Picture_Mode(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Get_Picture_Mode(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_Contrast(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Get_Contrast(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_Brightness(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Get_Brightness(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_Saturation(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Get_Saturation(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_HueColorTint(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Get_HueColorTint(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_Backlight(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Get_Backlight(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_Source(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Get_Source(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_Mute(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Get_Mute(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_Volume_Bar(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Get_Volume_Bar(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_Balance(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Get_Balance(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_Master_Volume(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Get_Master_Volume(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_CM(COMM_DEV_TYPE_E fromDev, unsigned char value);
    int cfbc_Get_CM(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_DNLP(COMM_DEV_TYPE_E fromDev, unsigned char value);
    int cfbc_Get_DNLP(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_Gamma(COMM_DEV_TYPE_E fromDev, unsigned char value);
    int cfbc_Get_Gamma(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_WhiteBalance_OnOff(COMM_DEV_TYPE_E fromDev, unsigned char value);
    int cfbc_Get_WhiteBalance_OnOff(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_Auto_Backlight_OnOff(COMM_DEV_TYPE_E fromDev, unsigned char value);
    int cfbc_Get_Auto_Backlight_OnOff(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_WhiteBalance_GrayPattern_OnOff(COMM_DEV_TYPE_E fromDev, int onOff);
    int cfbc_TestPattern_Select(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_WhiteBalance_SetGrayPattern(COMM_DEV_TYPE_E fromDev, unsigned char value);
    int cfbc_Get_WB_Batch(COMM_DEV_TYPE_E fromDev, unsigned char mode, unsigned char *r_gain, unsigned char *g_gain, unsigned char *b_gain, unsigned char *r_offset, unsigned char *g_offset, unsigned char *b_offset);
    int cfbc_Set_WB_Batch(COMM_DEV_TYPE_E fromDev, unsigned char mode, unsigned char r_gain, unsigned char g_gain, unsigned char b_gain, unsigned char r_offset, unsigned char g_offset, unsigned char b_offset);
    int cfbc_Set_backlight_onoff(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Get_backlight_onoff(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_LVDS_SSG_Set(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Set_AUTO_ELEC_MODE(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Get_FBC_MAINCODE_Version(COMM_DEV_TYPE_E fromDev, char sw_ver[], char build_time[], char git_ver[], char git_branch[], char build_name[]);
    int cfbc_Get_FBC_Get_PANel_INFO(COMM_DEV_TYPE_E fromDev, char panel_model[]);
    int cfbc_Set_FBC_panel_power_switch(COMM_DEV_TYPE_E fromDev, int switch_val);
    int cfbc_Set_FBC_suspend(COMM_DEV_TYPE_E fromDev, int switch_val);
    int cfbc_FBC_Send_Key_To_Fbc(COMM_DEV_TYPE_E fromDev, int keycode, int param);
    int cfbc_Get_FBC_PANEL_REVERSE(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Get_FBC_PANEL_OUTPUT(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_FBC_project_id(COMM_DEV_TYPE_E fromDev, int prj_id);
    int cfbc_Get_FBC_project_id(COMM_DEV_TYPE_E fromDev, int *prj_id);
    int cfbc_SendRebootToUpgradeCmd(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Set_FBC_User_Setting_Default(COMM_DEV_TYPE_E fromDev, int param);
    int cfbc_Set_FBC_Factory_SN(COMM_DEV_TYPE_E fromDev, const char *pSNval) ;
    int cfbc_Get_FBC_Factory_SN(COMM_DEV_TYPE_E fromDev, char FactorySN[]);
    int cfbc_Set_FBC_Audio_Source(COMM_DEV_TYPE_E fromDev, int value);

    int cfbc_Set_Thermal_state(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Set_LightSensor_N310(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Set_Dream_Panel_N310(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Set_MULT_PQ_N310(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Set_MEMC_N310(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Set_LockN_state(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Set_VMute(COMM_DEV_TYPE_E fromDev, unsigned char value);
    int cfbc_SET_SPLIT_SCREEN_DEMO(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Set_Bluetooth_IIS_onoff(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Get_Bluetooth_IIS_onoff(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_Led_onoff(COMM_DEV_TYPE_E fromDev, int val_1, int val_2, int val_3);
    int fbcSetPanelStatus(COMM_DEV_TYPE_E fromDev, int value);
    int fbcGetPanelStatus(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_Fbc_Uboot_Stage(COMM_DEV_TYPE_E fromDev, int value);
    int fbcSetEyeProtection(COMM_DEV_TYPE_E fromDev, int mode);
    int fbcSetGammaValue(COMM_DEV_TYPE_E fromDev, int gamma_curve, int is_save);
    int fbcSetGammaPattern(COMM_DEV_TYPE_E fromDev, unsigned short gamma_r, unsigned short gamma_g, unsigned short gamma_b);
    int fbcStartUpgrade(char *file_name, int mode, int blk_size);
    int fbcRelease();
	int cfbc_EnterPCMode(int value);

    virtual void onUpgradeStatus(int status, int progress);

private:
    int cfbcSetValueInt(COMM_DEV_TYPE_E fromDev, int cmd_id, int value, int value_count);
    int cfbcGetValueInt(COMM_DEV_TYPE_E fromDev, int cmd_id, int *value, int value_count = 1);

    CFbcLinker *mFbcLinker;
    static CFbcCommunication *mSingletonFBC;
};
#endif
