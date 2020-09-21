/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef _CTVIN_H
#define _CTVIN_H

#include <pthread.h>
#include <utils/Thread.h>
#include <utils/Mutex.h>
#include "PQType.h"
#include <vector>

#ifdef SUPPORT_ADTV
#include "../tv/CFrontEnd.h"
#endif

using namespace android;

#define SYS_VFM_MAP_PATH            "/sys/class/vfm/map"
#define SYS_DISPLAY_MODE_PATH       "/sys/class/display/mode"
#define SYS_PANEL_FRAME_RATE        "/sys/class/lcd/frame_rate"
#define FRAME_RATE_SUPPORT_LIST_PATH    "/sys/class/amhdmitx/amhdmitx0/disp_cap"//RX support display mode

#define DEPTH_LEVEL_2DTO3D 33
static const int DepthTable_2DTO3D[DEPTH_LEVEL_2DTO3D] = {
    -64, // -16
    -60, // -15
    -56, // -14
    -52, // -13
    -49, // -12
    -46, // -11
    -43, // -10
    -40, // -09
    -37, // -08
    -34, // -07
    -31, // -06
    -28, // -05
    -25, // -04
    -22, // -03
    -19, // -02
    -16, // -01
    -13, // 0
    3, // 1
    6, // 2
    9, // 3
    12, // 4
    15, // 5
    18, // 6
    21, // 7
    24, // 8
    28, // 9
    32, // 10
    36, // 11
    40, // 12
    44, // 13
    48, // 14
    52, // 15
    56, // 16
};

enum {
    MEMP_VDIN_WITHOUT_3D = 0,
    MEMP_VDIN_WITH_3D,
    MEMP_DCDR_WITHOUT_3D,
    MEMP_DCDR_WITH_3D,
    MEMP_ATV_WITHOUT_3D,
    MEMP_ATV_WITH_3D,
};

// ***************************************************************************
// *** TVIN general definition/enum/struct ***********************************
// ***************************************************************************

// tvin parameters
#define TVIN_PARM_FLAG_CAP      0x00000001 //tvin_parm_t.flag[ 0]: 1/enable or 0/disable frame capture function
#define TVIN_PARM_FLAG_CAL      0x00000002 //tvin_parm_t.flag[ 1]: 1/enable or 0/disable adc calibration
/*used for processing 3d in ppmgr set this flag to drop one field and send real height in vframe*/
#define TVIN_PARM_FLAG_2D_TO_3D 0x00000004 //tvin_parm_t.flag[ 2]: 1/enable or 0/disable 2D->3D mode

typedef struct tvin_buf_info_s {
    unsigned int vf_size;
    unsigned int buf_count;
    unsigned int buf_width;
    unsigned int buf_height;
    unsigned int buf_size;
    unsigned int wr_list_size;
} tvin_buf_info_t;

typedef struct tvin_video_buf_s {
    unsigned int index;
    unsigned int reserved;
} tvin_video_buf_t;

typedef struct tvin_parm_s {
    int                         index;    // index of frontend for vdin
    enum tvin_port_e            port;     // must set port in IOCTL
    struct tvin_info_s          info;
    unsigned int                hist_pow;
    unsigned int                luma_sum;
    unsigned int                pixel_sum;
    unsigned short              histgram[64];
    unsigned int                flag;
    unsigned short              dest_width;//for vdin horizontal scale down
    unsigned short              dest_height;//for vdin vertical scale down
    bool                h_reverse;//for vdin horizontal reverse
    bool                v_reverse;//for vdin vertical reverse
    unsigned int                reserved;
} tvin_parm_t;

typedef enum tvin_cn_type_e {
    GRAPHICS,
    PHOTO,
    CINEMA,
    GAME,
} tvin_cn_type_t;

typedef struct tvin_latency_s {
    bool allm_mode;
    bool it_content;
    tvin_cn_type_e cn_type;
} tvin_latency_t;

typedef enum best_displaymode_condition_e {
    BEST_DISPLAYMODE_CONDITION_RESOLUTION = 0,
    BEST_DISPLAYMODE_CONDITION_FRAMERATE,
} best_displaymode_condition_t;

// ***************************************************************************
// *** AFE module definition/enum/struct *************************************
// ***************************************************************************

typedef enum tvafe_cmd_status_e {
    TVAFE_CMD_STATUS_IDLE = 0,   // idle, be ready for TVIN_IOC_S_AFE_VGA_AUTO command
    TVAFE_CMD_STATUS_PROCESSING, // TVIN_IOC_S_AFE_VGA_AUTO command is in process
    TVAFE_CMD_STATUS_SUCCESSFUL, // TVIN_IOC_S_AFE_VGA_AUTO command is done with success
    TVAFE_CMD_STATUS_FAILED,     // TVIN_IOC_S_AFE_VGA_AUTO command is done with failure
    TVAFE_CMD_STATUS_TERMINATED, // TVIN_IOC_S_AFE_VGA_AUTO command is terminated by others related
} tvafe_cmd_status_t;

typedef struct tvafe_vga_edid_s {
    unsigned char value[256]; //256 byte EDID
} tvafe_vga_edid_t;

typedef struct tvafe_comp_wss_s {
    unsigned int wss1[5];
    unsigned int wss2[5];
} tvafe_comp_wss_t;


#define TVAFE_ADC_CAL_VALID 0x00000001
typedef struct tvafe_adc_cal_s {
    // ADC A
    unsigned short a_analog_clamp;    // 0x00~0x7f
    unsigned short a_analog_gain;     // 0x00~0xff, means 0dB~6dB
    unsigned short a_digital_offset1; // offset for fine-tuning
    // s11.0:   signed value, 11 integer bits,  0 fraction bits
    unsigned short a_digital_gain;    // 0~3.999
    // u2.10: unsigned value,  2 integer bits, 10 fraction bits
    unsigned short a_digital_offset2; // offset for format
    // s11.0:   signed value, 11 integer bits,  0 fraction bits
    // ADC B
    unsigned short b_analog_clamp;    // ditto to ADC A
    unsigned short b_analog_gain;
    unsigned short b_digital_offset1;
    unsigned short b_digital_gain;
    unsigned short b_digital_offset2;
    // ADC C
    unsigned short c_analog_clamp;    // ditto to ADC A
    unsigned short c_analog_gain;
    unsigned short c_digital_offset1;
    unsigned short c_digital_gain;
    unsigned short c_digital_offset2;
    // ADC D
    unsigned short d_analog_clamp;    // ditto to ADC A
    unsigned short d_analog_gain;
    unsigned short d_digital_offset1;
    unsigned short d_digital_gain;
    unsigned short d_digital_offset2;
    unsigned int   reserved;          // bit[ 0]: TVAFE_ADC_CAL_VALID
} tvafe_adc_cal_t;

typedef struct tvafe_adc_cal_clamp_s {
    short a_analog_clamp_diff;
    short b_analog_clamp_diff;
    short c_analog_clamp_diff;
} tvafe_adc_cal_clamp_t;

typedef struct tvafe_adc_comp_cal_s {
    struct tvafe_adc_cal_s comp_cal_val[3];
} tvafe_adc_comp_cal_t;

typedef enum tvafe_cvbs_video_e {
    TVAFE_CVBS_VIDEO_HV_UNLOCKED = 0,
    TVAFE_CVBS_VIDEO_H_LOCKED,
    TVAFE_CVBS_VIDEO_V_LOCKED,
    TVAFE_CVBS_VIDEO_HV_LOCKED,
} tvafe_cvbs_video_t;

// for pin selection
typedef enum tvafe_adc_pin_e {
    TVAFE_ADC_PIN_NULL = 0,
#if (MESON_CPU_TYPE == MESON_CPU_TYPE_MESONG9TV)
    TVAFE_CVBS_IN0      = 1,
    TVAFE_CVBS_IN1      = 2,
    TVAFE_CVBS_IN2      = 3,
    TVAFE_CVBS_IN3      = 4,//as atvdemod to tvafe
#else
    TVAFE_ADC_PIN_A_PGA_0   = 1,
    TVAFE_ADC_PIN_A_PGA_1   = 2,
    TVAFE_ADC_PIN_A_PGA_2   = 3,
    TVAFE_ADC_PIN_A_PGA_3   = 4,
    TVAFE_ADC_PIN_A_PGA_4   = 5,
    TVAFE_ADC_PIN_A_PGA_5   = 6,
    TVAFE_ADC_PIN_A_PGA_6   = 7,
    TVAFE_ADC_PIN_A_PGA_7   = 8,
    TVAFE_ADC_PIN_A_0   = 9,
    TVAFE_ADC_PIN_A_1   = 10,
    TVAFE_ADC_PIN_A_2   = 11,
    TVAFE_ADC_PIN_A_3   = 12,
    TVAFE_ADC_PIN_A_4   = 13,
    TVAFE_ADC_PIN_A_5   = 14,
    TVAFE_ADC_PIN_A_6   = 15,
    TVAFE_ADC_PIN_A_7   = 16,
    TVAFE_ADC_PIN_B_0   = 17,
    TVAFE_ADC_PIN_B_1   = 18,
    TVAFE_ADC_PIN_B_2   = 19,
    TVAFE_ADC_PIN_B_3   = 20,
    TVAFE_ADC_PIN_B_4   = 21,
    TVAFE_ADC_PIN_B_5   = 22,
    TVAFE_ADC_PIN_B_6   = 23,
    TVAFE_ADC_PIN_B_7   = 24,
    TVAFE_ADC_PIN_C_0   = 25,
    TVAFE_ADC_PIN_C_1   = 26,
    TVAFE_ADC_PIN_C_2   = 27,
    TVAFE_ADC_PIN_C_3   = 28,
    TVAFE_ADC_PIN_C_4   = 29,
    TVAFE_ADC_PIN_C_5   = 30,
    TVAFE_ADC_PIN_C_6   = 31,
    TVAFE_ADC_PIN_C_7   = 32,
    TVAFE_ADC_PIN_D_0   = 33,
    TVAFE_ADC_PIN_D_1   = 34,
    TVAFE_ADC_PIN_D_2   = 35,
    TVAFE_ADC_PIN_D_3   = 36,
    TVAFE_ADC_PIN_D_4   = 37,
    TVAFE_ADC_PIN_D_5   = 38,
    TVAFE_ADC_PIN_D_6   = 39,
    TVAFE_ADC_PIN_D_7   = 40,
    TVAFE_ADC_PIN_SOG_0 = 41,
    TVAFE_ADC_PIN_SOG_1 = 42,
    TVAFE_ADC_PIN_SOG_2 = 43,
    TVAFE_ADC_PIN_SOG_3 = 44,
    TVAFE_ADC_PIN_SOG_4 = 45,
    TVAFE_ADC_PIN_SOG_5 = 46,
    TVAFE_ADC_PIN_SOG_6 = 47,
    TVAFE_ADC_PIN_SOG_7 = 48,
#endif
    TVAFE_ADC_PIN_MAX,
} tvafe_adc_pin_t;

typedef enum tvafe_src_sig_e {
#if (MESON_CPU_TYPE == MESON_CPU_TYPE_MESONG9TV)
    CVBS_IN0 = 0,
    CVBS_IN1,
    CVBS_IN2,
    CVBS_IN3,
#else
    CVBS0_Y = 0,
    CVBS0_SOG,
    CVBS1_Y,
    CVBS1_SOG,
    CVBS2_Y,
    CVBS2_SOG,
    CVBS3_Y,
    CVBS3_SOG,
    CVBS4_Y,
    CVBS4_SOG,
    CVBS5_Y,
    CVBS5_SOG,
    CVBS6_Y,
    CVBS6_SOG,
    CVBS7_Y,
    CVBS7_SOG,
    S_VIDEO0_Y,
    S_VIDEO0_C,
    S_VIDEO0_SOG,
    S_VIDEO1_Y,
    S_VIDEO1_C,
    S_VIDEO1_SOG,
    S_VIDEO2_Y,
    S_VIDEO2_C,
    S_VIDEO2_SOG,
    S_VIDEO3_Y,
    S_VIDEO3_C,
    S_VIDEO3_SOG,
    S_VIDEO4_Y,
    S_VIDEO4_C,
    S_VIDEO4_SOG,
    S_VIDEO5_Y,
    S_VIDEO5_C,
    S_VIDEO5_SOG,
    S_VIDEO6_Y,
    S_VIDEO6_C,
    S_VIDEO6_SOG,
    S_VIDEO7_Y,
    S_VIDEO7_C,
    S_VIDEO7_SOG,
    VGA0_G,
    VGA0_B,
    VGA0_R,
    VGA0_SOG,
    VGA1_G,
    VGA1_B,
    VGA1_R,
    VGA1_SOG,
    VGA2_G,
    VGA2_B,
    VGA2_R,
    VGA2_SOG,
    VGA3_G,
    VGA3_B,
    VGA3_R,
    VGA3_SOG,
    VGA4_G,
    VGA4_B,
    VGA4_R,
    VGA4_SOG,
    VGA5_G,
    VGA5_B,
    VGA5_R,
    VGA5_SOG,
    VGA6_G,
    VGA6_B,
    VGA6_R,
    VGA6_SOG,
    VGA7_G,
    VGA7_B,
    VGA7_R,
    VGA7_SOG,
    COMP0_Y,
    COMP0_PB,
    COMP0_PR,
    COMP0_SOG,
    COMP1_Y,
    COMP1_PB,
    COMP1_PR,
    COMP1_SOG,
    COMP2_Y,
    COMP2_PB,
    COMP2_PR,
    COMP2_SOG,
    COMP3_Y,
    COMP3_PB,
    COMP3_PR,
    COMP3_SOG,
    COMP4_Y,
    COMP4_PB,
    COMP4_PR,
    COMP4_SOG,
    COMP5_Y,
    COMP5_PB,
    COMP5_PR,
    COMP5_SOG,
    COMP6_Y,
    COMP6_PB,
    COMP6_PR,
    COMP6_SOG,
    COMP7_Y,
    COMP7_PB,
    COMP7_PR,
    COMP7_SOG,
    SCART0_G,
    SCART0_B,
    SCART0_R,
    SCART0_CVBS,
    SCART1_G,
    SCART1_B,
    SCART1_R,
    SCART1_CVBS,
    SCART2_G,
    SCART2_B,
    SCART2_R,
    SCART2_CVBS,
    SCART3_G,
    SCART3_B,
    SCART3_R,
    SCART3_CVBS,
    SCART4_G,
    SCART4_B,
    SCART4_R,
    SCART4_CVBS,
    SCART5_G,
    SCART5_B,
    SCART5_R,
    SCART5_CVBS,
    SCART6_G,
    SCART6_B,
    SCART6_R,
    SCART6_CVBS,
    SCART7_G,
    SCART7_B,
    SCART7_R,
    SCART7_CVBS,
#endif
    TVAFE_SRC_SIG_MAX_NUM,
} tvafe_src_sig_t;

typedef struct tvafe_pin_mux_s {
    enum tvafe_adc_pin_e pin[TVAFE_SRC_SIG_MAX_NUM];
} tvafe_pin_mux_t;


// ***************************************************************************
// *** IOCTL command definition **********************************************
// ***************************************************************************

#define TVIN_IOC_MAGIC 'T'

//GENERAL
#define TVIN_IOC_OPEN               _IOW(TVIN_IOC_MAGIC, 0x01, struct tvin_parm_s)
#define TVIN_IOC_START_DEC          _IOW(TVIN_IOC_MAGIC, 0x02, struct tvin_parm_s)
#define TVIN_IOC_STOP_DEC           _IO( TVIN_IOC_MAGIC, 0x03)
#define TVIN_IOC_CLOSE              _IO( TVIN_IOC_MAGIC, 0x04)
#define TVIN_IOC_G_PARM             _IOR(TVIN_IOC_MAGIC, 0x05, struct tvin_parm_s)
#define TVIN_IOC_S_PARM             _IOW(TVIN_IOC_MAGIC, 0x06, struct tvin_parm_s)
#define TVIN_IOC_G_SIG_INFO         _IOR(TVIN_IOC_MAGIC, 0x07, struct tvin_info_s)
#define TVIN_IOC_G_BUF_INFO         _IOR(TVIN_IOC_MAGIC, 0x08, struct tvin_buf_info_s)
#define TVIN_IOC_START_GET_BUF      _IO( TVIN_IOC_MAGIC, 0x09)
#define TVIN_IOC_GET_BUF            _IOR(TVIN_IOC_MAGIC, 0x10, struct tvin_video_buf_s)
#define TVIN_IOC_PAUSE_DEC          _IO(TVIN_IOC_MAGIC, 0x41)
#define TVIN_IOC_RESUME_DEC         _IO(TVIN_IOC_MAGIC, 0x42)
#define TVIN_IOC_VF_REG             _IO(TVIN_IOC_MAGIC, 0x43)
#define TVIN_IOC_VF_UNREG           _IO(TVIN_IOC_MAGIC, 0x44)
#define TVIN_IOC_FREEZE_VF          _IO(TVIN_IOC_MAGIC, 0x45)
#define TVIN_IOC_UNFREEZE_VF        _IO(TVIN_IOC_MAGIC, 0x46)
#define TVIN_IOC_SNOWON             _IO(TVIN_IOC_MAGIC, 0x47)
#define TVIN_IOC_SNOWOFF            _IO(TVIN_IOC_MAGIC, 0x48)
#define TVIN_IOC_GET_COLOR_RANGE	_IOR(TVIN_IOC_MAGIC, 0X49, enum tvin_color_range_e)
#define TVIN_IOC_SET_COLOR_RANGE	_IOW(TVIN_IOC_MAGIC, 0X4a, enum tvin_color_range_e)
#define TVIN_IOC_GAME_MODE          _IOW(TVIN_IOC_MAGIC, 0x4b, unsigned int)
#define TVIN_IOC_SET_AUTO_RATIO_EN  _IOW(TVIN_IOC_MAGIC, 0x4c, unsigned int)
#define TVIN_IOC_GET_LATENCY_MODE   _IOR(TVIN_IOC_MAGIC, 0X4d, struct tvin_latency_s)

//TVAFE
#define TVIN_IOC_S_AFE_ADC_CAL      _IOW(TVIN_IOC_MAGIC, 0x11, struct tvafe_adc_cal_s)
#define TVIN_IOC_G_AFE_ADC_CAL      _IOR(TVIN_IOC_MAGIC, 0x12, struct tvafe_adc_cal_s)
#define TVIN_IOC_G_AFE_COMP_WSS     _IOR(TVIN_IOC_MAGIC, 0x13, struct tvafe_comp_wss_s)
#define TVIN_IOC_S_AFE_VGA_EDID     _IOW(TVIN_IOC_MAGIC, 0x14, struct tvafe_vga_edid_s)
#define TVIN_IOC_G_AFE_VGA_EDID     _IOR(TVIN_IOC_MAGIC, 0x15, struct tvafe_vga_edid_s)
#define TVIN_IOC_S_AFE_VGA_PARM     _IOW(TVIN_IOC_MAGIC, 0x16, struct tvafe_vga_parm_s)
#define TVIN_IOC_G_AFE_VGA_PARM     _IOR(TVIN_IOC_MAGIC, 0x17, struct tvafe_vga_parm_s)
#define TVIN_IOC_S_AFE_VGA_AUTO     _IO( TVIN_IOC_MAGIC, 0x18)
#define TVIN_IOC_G_AFE_CMD_STATUS   _IOR(TVIN_IOC_MAGIC, 0x19, enum tvafe_cmd_status_e)
#define TVIN_IOC_G_AFE_CVBS_LOCK    _IOR(TVIN_IOC_MAGIC, 0x1a, enum tvafe_cvbs_video_e)
#define TVIN_IOC_S_AFE_CVBS_STD     _IOW(TVIN_IOC_MAGIC, 0x1b, enum tvin_sig_fmt_e)
#define TVIN_IOC_CALLMASTER_SET     _IOW(TVIN_IOC_MAGIC, 0x1c, enum tvin_port_e)
#define TVIN_IOC_CALLMASTER_GET     _IO( TVIN_IOC_MAGIC, 0x1d)
#define TVIN_IOC_S_AFE_ADC_COMP_CAL  _IOW(TVIN_IOC_MAGIC, 0x1e, struct tvafe_adc_comp_cal_s)
#define TVIN_IOC_G_AFE_ADC_COMP_CAL  _IOR(TVIN_IOC_MAGIC, 0x1f, struct tvafe_adc_comp_cal_s)
//#define TVIN_IOC_LOAD_REG           _IOW(TVIN_IOC_MAGIC, 0x20, struct am_regs_s)
#define TVIN_IOC_S_AFE_ADC_DIFF     _IOW(TVIN_IOC_MAGIC, 0x21, struct tvafe_adc_cal_clamp_s)
#define TVIN_IOC_S_AFE_SONWON       _IO(TVIN_IOC_MAGIC, 0x22)
#define TVIN_IOC_S_AFE_SONWOFF      _IO(TVIN_IOC_MAGIC, 0x23)
#define TVIN_IOC_S_AFE_SONWCFG      _IOW(TVIN_IOC_MAGIC, 0x27, unsigned int)


// ***************************************************************************
// *** add more **********************************************
// ***************************************************************************

enum {
    TV_PATH_VDIN_AMLVIDEO2_PPMGR_DEINTERLACE_AMVIDEO,
    TV_PATH_DECODER_AMLVIDEO2_PPMGR_DEINTERLACE_AMVIDEO,
};

#define CAMERA_IOC_MAGIC 'C'
#define CAMERA_IOC_START        _IOW(CAMERA_IOC_MAGIC, 0x01, struct camera_info_s)
#define CAMERA_IOC_STOP         _IO(CAMERA_IOC_MAGIC, 0x02)
#define CAMERA_IOC_SET_PARA     _IOW(CAMERA_IOC_MAGIC, 0x03, struct camera_info_s)
#define CAMERA_IOC_GET_PARA     _IOR(CAMERA_IOC_MAGIC, 0x04, struct camera_info_s)


#define CC_HIST_GRAM_BUF_SIZE   (64)
/*******************************extend define*******************************/

typedef enum adc_cal_type_e {
    CAL_YPBPR = 0,
    CAL_VGA,
    CAL_CVBS,
} adc_cal_type_t;

typedef enum signal_range_e {
    RANGE100 = 0,
    RANGE75,
} signal_range_t;

typedef enum tv_hdmi_port_id_e {
    HDMI_PORT_1 = 1,
    HDMI_PORT_2,
    HDMI_PORT_3,
    HDMI_PORT_4,
    HDMI_PORT_MAX,
} tv_hdmi_port_id_t;

typedef enum tv_hdmi_hdcp_version_e {
    HDMI_HDCP_VER_14 = 0,
    HDMI_HDCP_VER_22 ,
} tv_hdmi_hdcp_version_t;

typedef enum tv_hdmi_edid_version_e {
    HDMI_EDID_VER_14 = 0,
    HDMI_EDID_VER_20 ,
} tv_hdmi_edid_version_t;

typedef enum tv_hdmi_hdcpkey_enable_e {
    hdcpkey_enable = 0,
    hdcpkey_disable ,
} tv_hdmi_hdcpkey_enable_t;

typedef enum tvin_color_range_e {
    TVIN_COLOR_RANGE_AUTO = 0,
    TVIN_COLOR_RANGE_FULL,
    TVIN_COLOR_RANGE_LIMIT,
} tvin_color_range_t;

typedef struct adc_cal_s {
    unsigned int rcr_max;
    unsigned int rcr_min;
    unsigned int g_y_max;
    unsigned int g_y_min;
    unsigned int bcb_max;
    unsigned int bcb_min;
    unsigned int cr_white;
    unsigned int cb_white;
    unsigned int cr_black;
    unsigned int cb_black;
} adc_cal_t;

typedef struct tvin_window_pos_s {
    int x1;
    int y1;
    int x2;
    int y2;
} tvin_window_pos_t;


typedef enum tv_path_type_e {
    TV_PATH_TYPE_DEFAULT,
    TV_PATH_TYPE_TVIN,
    TV_PATH_TYPE_MAX,
} tv_path_type_t;

typedef enum tv_path_status_e {
    TV_PATH_STATUS_NO_DEV = -2,
    TV_PATH_STATUS_ERROR = -1,
    TV_PATH_STATUS_INACTIVE = 0,
    TV_PATH_STATUS_ACTIVE = 1,
    TV_PATH_STATUS_MAX,
} tv_path_status_t;

typedef enum tv_audio_channel_e {
    TV_AUDIO_LINE_IN_0,
    TV_AUDIO_LINE_IN_1,
    TV_AUDIO_LINE_IN_2,
    TV_AUDIO_LINE_IN_3,
    TV_AUDIO_LINE_IN_4,
    TV_AUDIO_LINE_IN_5,
    TV_AUDIO_LINE_IN_6,
    TV_AUDIO_LINE_IN_7,
    TV_AUDIO_LINE_IN_MAX,
} tv_audio_channel_t;

typedef enum tv_audio_in_source_type_e {
    TV_AUDIO_IN_SOURCE_TYPE_LINEIN,
    TV_AUDIO_IN_SOURCE_TYPE_ATV,
    TV_AUDIO_IN_SOURCE_TYPE_HDMI,
    TV_AUDIO_IN_SOURCE_TYPE_SPDIF,
    TV_AUDIO_IN_SOURCE_TYPE_MAX,
} tv_audio_in_source_type_t;

#define CC_RESOLUTION_1366X768_W      (1366)
#define CC_RESOLUTION_1366X768_H      (768)
#define CC_RESOLUTION_1920X1080_W     (1920)
#define CC_RESOLUTION_1920X1080_H     (1080)
#define CC_RESOLUTION_3840X2160_W     (3840)
#define CC_RESOLUTION_3840X2160_H     (2160)

typedef enum tv_source_connect_detect_status_e {
    CC_SOURCE_PLUG_OUT = 0,
    CC_SOURCE_PLUG_IN = 1,
} tv_source_connect_detect_status_t;



#define CC_REQUEST_LIST_SIZE            (32)
#define CC_SOURCE_DEV_REFRESH_CNT       (E_LA_MAX)
class CTvin {
public:
    CTvin();
    ~CTvin();
    int OpenTvin();
    int init_vdin();
    int uninit_vdin ( void );
    int Tv_init_afe ( void );
    int Tv_uninit_afe ( void );
    int Tvin_RemovePath ( tv_path_type_t pathtype );
    int Tvin_CheckPathActive ( tvin_port_t source_port );
    int Tvin_CheckVideoPathComplete ( tv_path_type_t path_type );
    int setMpeg2Vdin(int enable);
    //pre apis
    int AFE_DeviceIOCtl ( int request, ... );
    void TvinApi_CloseAFEModule ( void );
    int TvinApi_SetVdinHVScale ( int vdinx, int hscale, int vscale );
    int TvinApi_SetStartDropFrameCn ( int count );
    int TvinApi_SetCompPhaseEnable ( int enable );
    tvin_trans_fmt TvinApi_Get3DDectMode();
    int TvinApi_GetHDMIAudioStatus ( void );
    int Tvin_StartDecoder ( tvin_info_t &info );
    int Tvin_StopDecoder();
    int SwitchPort (tvin_port_t source_port );
    int SwitchSnow(bool enable);
    bool getSnowStatus(void);
    int Tvin_WaitPathInactive ( tvin_port_t source_port );
    int VDIN_AddPath ( const char *videopath );
    int VDIN_RmDefPath ( void );
    int VDIN_RmTvPath ( void );
    int VDIN_AddVideoPath ( int selPath );

    int VDIN_OpenModule();
    int VDIN_CloseModule();
    int VDIN_DeviceIOCtl ( int request, ... );
    int VDIN_GetDeviceFileHandle();
    int VDIN_OpenPort ( tvin_port_t port );
    int VDIN_ClosePort();
    int VDIN_StartDec ( const struct tvin_parm_s *vdinParam );
    int VDIN_StopDec();
    int VDIN_GetSignalInfo ( struct tvin_info_s *SignalInfo );
    int VDIN_SetVdinParam ( const struct tvin_parm_s *vdinParam );
    int VDIN_GetVdinParam ( const struct tvin_parm_s *vdinParam );
    int VDIN_GetDisplayVFreq (int need_freq, int *iSswitch, char * display_mode);
    int VDIN_SetDisplayVFreq ( int freq);
    int VDIN_Get_avg_luma(void);
    int VDIN_SetMVCViewMode ( int mode );
    int VDIN_GetMVCViewMode ( void );
    int VDIN_SetDIBuffMgrMode ( int mgr_mode );
    int VDIN_SetDICFG ( int cfg );
    int VDIN_SetDI3DDetc ( int enable );
    int VDIN_Get3DDetc ( void );
    int VDIN_GetVscalerStatus ( void );
    int VDIN_TurnOnBlackBarDetect ( int isEnable );
    int VDIN_SetVideoFreeze ( int enable );
    int VDIN_SetDIBypasshd ( int enable );
    int VDIN_SetDIBypassAll ( int enable );
    int VDIN_SetDIBypass_Get_Buf_Threshold ( int enable );
    int VDIN_SetDIBypassProg ( int enable );
    int VDIN_SetDIBypassDynamic ( int flag );
    int VDIN_EnableRDMA ( int enable );
    int VDIN_SetColorRangeMode(tvin_color_range_t range_mode);
    int VDIN_GetColorRangeMode(void);
    int VDIN_UpdateForPQMode(pq_status_update_e gameStatus, pq_status_update_e pcStatus);
    int VDIN_GetVdinDeviceFd(void);
    int VDIN_SetWssStatus(int status);
    int VDIN_GetDisplayModeSupportList(std::vector<std::string> *SupportDisplayModes);
    int VDIN_GetBestDisplayMode(best_displaymode_condition_t condition,
                                         std::string ConditionParam,
                                         std::vector<std::string> SupportDisplayModes,
                                         char *BestDisplayMode);
    int VDIN_GetFrameRateSupportList(std::vector<std::string> *supportFrameRates);

    int AFE_OpenModule ( void );
    void AFE_CloseModule ( void );    int AFE_SetCVBSStd ( tvin_sig_fmt_t cvbs_fmt );
    int AFE_SetVGAEdid ( const unsigned char *ediddata );
    int AFE_GetVGAEdid ( unsigned char *ediddata );
    int AFE_SetADCTimingAdjust ( const struct tvafe_vga_parm_s *timingadj );
    int AFE_GetADCCurrentTimingAdjust ( struct tvafe_vga_parm_s *timingadj );
    int AFE_VGAAutoAdjust ( struct tvafe_vga_parm_s *timingadj );
    int AFE_SetVGAAutoAjust ( void );
    int AFE_GetVGAAutoAdjustCMDStatus ( tvafe_cmd_status_t *Status );
    int AFE_GetAdcCal ( struct tvafe_adc_cal_s *adccalvalue );
    int AFE_SetAdcCal ( struct tvafe_adc_cal_s *adccalvalue );
    int AFE_GetAdcCompCal ( struct tvafe_adc_comp_cal_s *adccalvalue );
    int AFE_SetAdcCompCal ( struct tvafe_adc_comp_cal_s *adccalvalue );
    int AFE_GetYPbPrWSSinfo ( struct tvafe_comp_wss_s *wssinfo );
    int AFE_EnableSnowByConfig ( bool enable );
    unsigned int data_limit ( float data );
    void matrix_convert_yuv709_to_rgb ( unsigned int y, unsigned int u, unsigned int v, unsigned int *r, unsigned int *g, unsigned int *b );
    void re_order ( unsigned int *a, unsigned int *b );
    char *get_cap_addr ( enum adc_cal_type_e calType );
    inline unsigned char get_mem_data ( char *dp, unsigned int addr );
    int get_frame_average ( enum adc_cal_type_e calType, struct adc_cal_s *mem_data );
    struct adc_cal_s get_n_frame_average ( enum adc_cal_type_e calType ) ;
    int AFE_GetMemData ( int typeSel, struct adc_cal_s *mem_data );
    int AFE_GetCVBSLockStatus ( enum tvafe_cvbs_video_e *cvbs_lock_status );
    static int CvbsFtmToColorStdEnum(tvin_sig_fmt_t fmt);
    int VDIN_GetPortConnect ( int port );
    int VDIN_OpenHDMIPinMuxOn ( bool flag );
    int VDIN_GetAllmInfo(tvin_latency_s *AllmInfo);
    int VDIN_SetGameMode(pq_status_update_e mode);
    /*******************************************extend funs*********************/
    static tv_source_input_type_t Tvin_SourcePortToSourceInputType ( tvin_port_t source_port );
    static tv_source_input_type_t Tvin_SourceInputToSourceInputType ( tv_source_input_t source_input );
    static tvin_port_t Tvin_GetSourcePortBySourceType ( tv_source_input_type_t source_type );
    static tvin_port_t Tvin_GetSourcePortBySourceInput ( tv_source_input_t source_input );
    static unsigned int Tvin_TransPortStringToValue(const char *port_str);
    static void Tvin_LoadSourceInputToPortMap();
    static int Tvin_GetSourcePortByCECPhysicalAddress(int physical_addr);
    static tv_audio_in_source_type_t Tvin_GetAudioInSourceType ( tv_source_input_t source_input );
    static tv_source_input_t Tvin_PortToSourceInput ( tvin_port_t port );
    static int isVgaFmtInHdmi ( tvin_sig_fmt_t fmt );
    static int isSDFmtInHdmi ( tvin_sig_fmt_t fmt );
    static bool Tvin_is50HzFrameRateFmt ( tvin_sig_fmt_t fmt );
    static bool Tvin_IsDeinterlaceFmt ( tvin_sig_fmt_t fmt );
    static unsigned long CvbsFtmToV4l2ColorStd(tvin_sig_fmt_t fmt);

    static CTvin *getInstance();

private:
    static CTvin *mInstance;
    int mAfeDevFd;
    int mVdin0DevFd;
    bool m_snow_status;
    tvin_parm_t m_tvin_param;
    tvin_parm_t gTvinVDINParam;
    tvin_info_t gTvinVDINSignalInfo;
    tvin_parm_t gTvinAFEParam;
    tvin_info_t gTvinAFESignalInfo;
    static int mSourceInputToPortMap[SOURCE_MAX];
    char gVideoPath[256];
    bool mDecoderStarted;
    bool mbResolutionPriority;

    char config_tv_path[64];
    char config_default_path[64];
    tvin_port_t mSourcePort;
};
#endif
