/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef __AMVECM_H
#define __AMVECM_H

#include "ve.h"
#include "cm.h"
#include "amstream.h"
#include "ldim.h"


//struct ve_dnlp_s          video_ve_dnlp;

#define FLAG_RSV31              (1 << 31)
#define FLAG_RSV30              (1 << 30)
#define FLAG_VE_DNLP            (1 << 29)
#define FLAG_VE_NEW_DNLP        (1 << 28)
#define FLAG_VE_LC_CURV         (1 << 27)
#define FLAG_RSV26              (1 << 26)
#define FLAG_RSV25              (1 << 25)
#define FLAG_RSV24              (1 << 24)
#define FLAG_3D_SYNC_DIS        (1 << 23)
#define FLAG_3D_SYNC_EN         (1 << 22)
#define FLAG_VLOCK_PLL          (1 << 21)
#define FLAG_VLOCK_ENC          (1 << 20)
#define FLAG_VE_DNLP_EN         (1 << 19)
#define FLAG_VE_DNLP_DIS        (1 << 18)
#define FLAG_RSV17              (1 << 17)
#define FLAG_RSV16              (1 << 16)
#define FLAG_GAMMA_TABLE_EN     (1 << 15)
#define FLAG_GAMMA_TABLE_DIS    (1 << 14)
#define FLAG_GAMMA_TABLE_R      (1 << 13)
#define FLAG_GAMMA_TABLE_G      (1 << 12)
#define FLAG_GAMMA_TABLE_B      (1 << 11)
#define FLAG_RGB_OGO            (1 << 10)
#define FLAG_RSV9               (1 <<  9)
#define FLAG_RSV8               (1 <<  8)
#define FLAG_BRI_CON            (1 <<  7)
#define FLAG_LVDS_FREQ_SW       (1 <<  6)
#define FLAG_REG_MAP5           (1 <<  5)
#define FLAG_REG_MAP4           (1 <<  4)
#define FLAG_REG_MAP3           (1 <<  3)
#define FLAG_REG_MAP2           (1 <<  2)
#define FLAG_REG_MAP1           (1 <<  1)
#define FLAG_REG_MAP0           (1 <<  0)


#define AMVECM_IOC_MAGIC  'C'

#define AMVECM_IOC_VE_DNLP         _IOW(AMVECM_IOC_MAGIC, 0x21, struct ve_dnlp_s  )
#define AMVECM_IOC_G_HIST_AVG      _IOW(AMVECM_IOC_MAGIC, 0x22, struct ve_hist_s  )
#define AMVECM_IOC_VE_DNLP_EN      _IO(AMVECM_IOC_MAGIC, 0x23)
#define AMVECM_IOC_VE_DNLP_DIS     _IO(AMVECM_IOC_MAGIC, 0x24)
#define AMVECM_IOC_VE_NEW_DNLP     _IOW(AMVECM_IOC_MAGIC, 0x25, struct ve_dnlp_curve_param_s)
#define AMVECM_IOC_G_HIST_BIN      _IOW(AMVECM_IOC_MAGIC, 0x26, struct vpp_hist_param_s)

// VPP.CM IOCTL command list
#define AMVECM_IOC_LOAD_REG        _IOW(AMVECM_IOC_MAGIC, 0x30, struct am_regs_s)
/*vpp get color primary*/
#define AMVECM_IOC_G_COLOR_PRI     _IOR(AMVECM_IOC_MAGIC, 0x28, enum color_primary_e)

// VPP.GAMMA IOCTL command list
#define AMVECM_IOC_GAMMA_TABLE_EN  _IO(AMVECM_IOC_MAGIC, 0x40)
#define AMVECM_IOC_GAMMA_TABLE_DIS _IO(AMVECM_IOC_MAGIC, 0x41)
#define AMVECM_IOC_GAMMA_TABLE_R   _IOW(AMVECM_IOC_MAGIC, 0x42, struct tcon_gamma_table_s)
#define AMVECM_IOC_GAMMA_TABLE_G   _IOW(AMVECM_IOC_MAGIC, 0x43, struct tcon_gamma_table_s)
#define AMVECM_IOC_GAMMA_TABLE_B   _IOW(AMVECM_IOC_MAGIC, 0x44, struct tcon_gamma_table_s)
#define AMVECM_IOC_S_RGB_OGO       _IOW(AMVECM_IOC_MAGIC, 0x45, struct tcon_rgb_ogo_s)
#define AMVECM_IOC_G_RGB_OGO       _IOR(AMVECM_IOC_MAGIC, 0x46, struct tcon_rgb_ogo_s)

// VPP.Local dimming command list
#define LDIM_IOC_PARA              _IOW(AMVECM_IOC_MAGIC, 0x50, struct vpu_ldim_param_s)

// VPP.display mode command list
#define AMVECM_IOC_SET_OVERSCAN    _IOW(AMVECM_IOC_MAGIC, 0x52, struct ve_pq_load_s)
//DNLP IOCTL command list
#define AMVECM_IOC_G_DNLP_STATE    _IOR(AMVECM_IOC_MAGIC, 0x53, enum ve_dnlp_state_e)
#define AMVECM_IOC_S_DNLP_STATE    _IOW(AMVECM_IOC_MAGIC, 0x54, enum ve_dnlp_state_e)

//PQ moudle IOCTL command list
#define AMVECM_IOC_G_PQ_STATUE     _IOR(AMVECM_IOC_MAGIC, 0x55, enum ve_pq_moudle_state_e)
#define AMVECM_IOC_S_PQ_STATUE     _IOW(AMVECM_IOC_MAGIC, 0x56, enum ve_pq_moudle_state_e)

//HDR IOCTL command list
#define AMVECM_IOC_G_CSCTYPE       _IOR(AMVECM_IOC_MAGIC, 0x57, enum ve_csc_type_e)
#define AMVECM_IOC_S_CSCTYPE       _IOW(AMVECM_IOC_MAGIC, 0x58, enum ve_csc_type_e)

//PIC_MODE IOCTL command list
#define AMVECM_IOC_G_PIC_MODE      _IOR(AMVECM_IOC_MAGIC, 0x59, struct am_pic_mode_s)
#define AMVECM_IOC_S_PIC_MODE      _IOW(AMVECM_IOC_MAGIC, 0x60, struct am_pic_mode_s)

// VPP.DI IOCTL command list
#define _DI_    'D'
#define AMDI_IOC_SET_PQ_PARM       _IOW(_DI_, 0x51, struct am_pq_param_s)

/*Local contrast command list*/
#define AMVECM_IOC_S_LC_CURVE      _IOW(AMVECM_IOC_MAGIC, 0x62, struct ve_lc_curve_parm_s)

#endif /* __AMVECM_H */

