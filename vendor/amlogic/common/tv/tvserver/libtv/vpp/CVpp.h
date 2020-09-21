/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef _C_VPP_H
#define _C_VPP_H
#include "../tvin/CTvin.h"
#include <tvutils.h>

#define VPP_PANEL_BACKLIGHT_DEV_PATH   "/sys/class/aml_bl/power"
#define DI_NR2_ENABLE                  "/sys/module/di/parameters/nr2_en"
#define AMVECM_PC_MODE                 "/sys/class/amvecm/pc_mode"

// default backlight value 10%
#define DEFAULT_BACKLIGHT_BRIGHTNESS 10

#define MODE_VPP_3D_DISABLE     0x00000000
#define MODE_VPP_3D_ENABLE      0x00000001
#define MODE_VPP_3D_AUTO        0x00000002
#define MODE_VPP_3D_LR          0x00000004
#define MODE_VPP_3D_TB          0x00000008
#define MODE_VPP_3D_LA          0x00000010
#define MODE_VPP_3D_FA         0x00000020
#define MODE_VPP_3D_LR_SWITCH   0x00000100
#define MODE_VPP_3D_TO_2D_L     0x00000200
#define MODE_VPP_3D_TO_2D_R     0x00000400

typedef enum vpp_panorama_mode_e {
    VPP_PANORAMA_MODE_FULL,
    VPP_PANORAMA_MODE_NORMAL,
    VPP_PANORAMA_MODE_MAX,
} vpp_panorama_mode_t;

typedef enum vpp_dream_panel_e {
    VPP_DREAM_PANEL_OFF,
    VPP_DREAM_PANEL_LIGHT,
    VPP_DREAM_PANEL_SCENE,
    VPP_DREAM_PANEL_FULL,
    VPP_DREAM_PANEL_DEMO,
    VPP_DREAM_PANEL_MAX,
} vpp_dream_panel_t;


class CVpp {
public:
    CVpp();
    ~CVpp();
    int Vpp_Init();
    int LoadVppSettings ( tv_source_input_t tv_source_input, tvin_sig_fmt_t sig_fmt, tvin_trans_fmt_t trans_fmt );
    int SetPQMode ( vpp_picture_mode_t pq_mode, tv_source_input_t tv_source_input, tvin_sig_fmt_t sig_fmt, tvin_trans_fmt_t trans_fmt, is_3d_type_t is3d, int is_save, int is_autoswitch);
    vpp_picture_mode_t GetPQMode ( tv_source_input_t tv_source_input );
    int SavePQMode ( vpp_picture_mode_t pq_mode,  tv_source_input_t tv_source_input );
    void enableMonitorMode(bool enable);
    int VPP_SetCVD2Values (void);
    int FactoryGetTestPattern ( void );
    //api
    int VPP_SetVideoCrop ( int Voffset0, int Hoffset0, int Voffset1, int Hoffset1 );
    int VPP_SetBackLight_Switch ( int value );
    int VPP_GetBackLight_Switch ( void );
    int VPP_SetScalerPathSel (const unsigned int value);
    int VPP_SSMRestorToDefault(int id, bool resetAll);
    int TV_SSMRecovery();
    static CVpp *getInstance();
private:
    static CVpp *mInstance;
};
#endif
