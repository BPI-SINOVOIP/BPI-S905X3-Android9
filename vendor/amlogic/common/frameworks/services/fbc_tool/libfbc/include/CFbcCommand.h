/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef _C_FBC_COMMAND_H_
#define _C_FBC_COMMAND_H_

typedef enum COMM_DEV_TYPE_NO {
    COMM_DEV_CEC = 0,
    COMM_DEV_SERIAL = 1,
} COMM_DEV_TYPE_E;

enum fbc_command_t {
    FBC_REBOOT_UPGRADE_AUTO_SPEED = 0,
    FBC_REBOOT_UPGRADE = 0x1,
    FBC_USER_SETTING_DEFAULT = 0x02,
    FBC_USER_SETTING_SET,
    FBC_GET_HDCP_KEY,
    FBC_PANEL_POWER,
    FBC_SUSPEND_POWER,
    //TOP CMD num:6
    VPU_CMD_INIT = 0x8, //parameter num 0
    VPU_CMD_ENABLE,     //parameter num 1;bit0~6:module;bit7:enable(1) or disable(0)
    VPU_CMD_BYPASS,     //parameter num 1;bit0~6:module;bit7:bypass(1) or not(0)
    VPU_CMD_OUTPUT_MUX, //parameter num 1;1:lvds;2:vx1;3:minilvds
    VPU_CMD_TIMING,     //parameter num 1;reference vpu_timing_t
    VPU_CMD_SOURCE,     //parameter num 1;reference vpu_source_t
    VPU_CMD_GAMMA_MOD,  //parameter num 1;reference vpu_gammamod_t
    VPU_CMD_D2D3 = 0xf,     //0xf:D2D3
    //

    CMD_BRIDGE_SW_VER = 0x10,
    CMD_DEVICE_ID,
    CMD_CLIENT_TYPE,
    CMD_DEVICE_NUM,
    CMD_ACTIVE_KEY,
    CMD_ACTIVE_STATUS,
    CMD_PANEL_INFO,
    CMD_LVDS_SSG_SET,

    CMD_DBG_REGISTER_ACCESS = 0x18,
    CMD_DBG_MEMORY_ACCESS,
    CMD_DBG_SPI_ACCESS,
    CMD_DBG_VPU_MEMORY_ACCESS,
    CMD_DBG_MEMORY_TRANSFER,
    CMD_INPUT_DOWN,
    CMD_INPUT_UP,
    CMD_FBC_MAIN_CODE_VERSION,

    //0x1f reserved
    //PQ+debug CMD num:32
    VPU_CMD_NATURE_LIGHT_EN = 0x20, //0 or 1;on or off  ????
    VPU_CMD_BACKLIGHT_EN,       //0 or 1;on or off
    VPU_CMD_BRIGHTNESS, //parameter num 2;parameter1:distinguish two modules;parameter2:ui value
    VPU_CMD_CONTRAST,   //parameter num 2;parameter1:distinguish two modules;parameter2:ui value
    VPU_CMD_BACKLIGHT,  //parameter num 1;
    VPU_CMD_RES1,       //reserved
    VPU_CMD_SATURATION, //parameter num 1;
    VPU_CMD_DYNAMIC_CONTRAST,   //0 or 1;??
    VPU_CMD_PICTURE_MODE,   //??
    VPU_CMD_PATTERN_EN, //0 or 1;on or off
    VPU_CMD_PATTEN_SEL, //0x2a parameter num 1;PATTEN SELECT
    VPU_CMD_RES2,
    VPU_CMD_GAMMA_PATTERN,
    VPU_CMD_RES4,
    VPU_CMD_RES5,
    VPU_CMD_USER_VMUTE = 0x2e,
    VPU_CMD_USER_GAMMA,
    //0x30:sound_mode_def
    VPU_CMD_COLOR_TEMPERATURE_DEF = 0x31,   //def:factory setting
    VPU_CMD_BRIGHTNESS_DEF,
    VPU_CMD_CONTRAST_DEF,
    VPU_CMD_COLOR_DEF,
    VPU_CMD_HUE_DEF,
    VPU_CMD_BACKLIGHT_DEF,
    VPU_CMD_RES7,
    VPU_CMD_AUTO_LUMA_EN = 0x38,//0 or 1;on or off;appoint to backlight?
    VPU_CMD_HIST,       //parameter num 0;read hist info
    VPU_CMD_BLEND,      //parameter num ?;
    VPU_CMD_DEMURA,     //parameter num ?;
    VPU_CMD_CSC,        //parameter num ?;
    VPU_CMD_CM2,        //parameter num 1;index
    VPU_CMD_GAMMA,      //parameter num 1;index
    VPU_CMD_SRCIF,
    //WB CMD num:10
    VPU_CMD_RED_GAIN_DEF = 0x40,
    VPU_CMD_GREEN_GAIN_DEF,
    VPU_CMD_BLUE_GAIN_DEF,
    VPU_CMD_PRE_RED_OFFSET_DEF,
    VPU_CMD_PRE_GREEN_OFFSET_DEF,
    VPU_CMD_PRE_BLUE_OFFSET_DEF,
    VPU_CMD_POST_RED_OFFSET_DEF,
    VPU_CMD_POST_GREEN_OFFSET_DEF,
    VPU_CMD_POST_BLUE_OFFSET_DEF,
    VPU_CMD_EYE_PROTECTION,
    VPU_CMD_WB = 0x4a,
    //DNLP PARM
    VPU_CMD_DNLP_PARM,
    VPU_CMD_WB_VALUE,
    VPU_CMD_GRAY_PATTERN,
    VPU_CMD_BURN,
    CMD_HDMI_STAT,
    VPU_CMD_READ = 0x80,
    //VPU_CMD_HUE_ADJUST,   //parameter num 1;
    //VPU_CMD_WB,       //parameter num 3;one parameter include two items so that six items can all be included
    VPU_CMD_MAX = 50,//temp define 50       //

    //audio cmd
    AUDIO_CMD_SET_SOURCE = 0x50,
    AUDIO_CMD_SET_MASTER_VOLUME,
    AUDIO_CMD_SET_CHANNEL_VOLUME,
    AUDIO_CMD_SET_SUBCHANNEL_VOLUME,
    AUDIO_CMD_SET_MASTER_VOLUME_GAIN,
    AUDIO_CMD_SET_CHANNEL_VOLUME_INDEX,
    AUDIO_CMD_SET_VOLUME_BAR,
    AUDIO_CMD_SET_MUTE,
    AUDIO_CMD_SET_EQ_MODE,
    AUDIO_CMD_SET_BALANCE,
    AUDIO_CMD_GET_SOURCE,
    AUDIO_CMD_GET_MASTER_VOLUME,
    AUDIO_CMD_GET_CHANNEL_VOLUME,
    AUDIO_CMD_GET_SUBCHANNEL_VOLUME,
    AUDIO_CMD_GET_MASTER_VOLUME_GAIN,
    AUDIO_CMD_GET_CHANNEL_VOLUME_INDEX,
    AUDIO_CMD_GET_VOLUME_BAR,
    AUDIO_CMD_GET_MUTE,
    AUDIO_CMD_GET_EQ_MODE,
    AUDIO_CMD_GET_BALANCE,

    VPU_CMD_SET_ELEC_MODE = 0x64,
    CMD_SET_LED_MODE   = 0x65,

    CMD_SET_FACTORY_SN = 0x66,
    CMD_GET_FACTORY_SN,
    CMD_COMMUNICATION_TEST,
    CMD_CLR_SETTINGS_DEFAULT,
    CMD_BLUETOOTH_I2S_STATUS = 0x6a,
    CMD_PANEL_ON_OFF = 0x6b,

    CMD_HDMI_REG   = 0x70,
    CMD_SET_PROJECT_SELECT = 0x71,
    CMD_GET_PROJECT_SELECT = 0x72,
    CMD_SET_LOCKN_DISABLE = 0x73, //0x73
    CMD_SET_SPLIT_SCREEN_DEMO = 0X74,

    CMD_FBC_RELEASE = 0x75,

    CMD_SET_UBOOT_STAGE = 0x7b,

    CMD_SET_AUTO_BACKLIGHT_ONFF = 0x85,
    CMD_GET_AUTO_BACKLIGHT_ONFF = 0x86,

	CMD_SET_ENTER_PCMODE = 0x88,

};

typedef enum fbc_upgrade_mode {
    CC_UPGRADE_MODE_BOOT_MAIN = 0,
    CC_UPGRADE_MODE_BOOT,
    CC_UPGRADE_MODE_MAIN,
    CC_UPGRADE_MODE_COMPACT_BOOT,
    CC_UPGRADE_MODE_ALL,
    CC_UPGRADE_MODE_MAIN_PQ_WB,
    CC_UPGRADE_MODE_ALL_PQ_WB,
    CC_UPGRADE_MODE_MAIN_WB,
    CC_UPGRADE_MODE_ALL_WB,
    CC_UPGRADE_MODE_MAIN_PQ,
    CC_UPGRADE_MODE_ALL_PQ,
    CC_UPGRADE_MODE_PQ_WB_ONLY,
    CC_UPGRADE_MODE_WB_ONLY,
    CC_UPGRADE_MODE_PQ_ONLY,
    CC_UPGRADE_MODE_CUR_PQ_BIN,
    CC_UPGRADE_MODE_ALL_PQ_BIN,
    CC_UPGRADE_MODE_BURN,
    CC_UPGRADE_MODE_DUMMY
} fbc_upgrade_mode_t;
#endif
