/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef __AMSTREAM_H__
#define __AMSTREAM_H__

#include "ve.h"

#define AMSTREAM_IOC_MAGIC  'S'

#define AMSTREAM_IOC_VB_START   _IOW(AMSTREAM_IOC_MAGIC, 0x00, int)
#define AMSTREAM_IOC_VB_SIZE    _IOW(AMSTREAM_IOC_MAGIC, 0x01, int)
#define AMSTREAM_IOC_AB_START   _IOW(AMSTREAM_IOC_MAGIC, 0x02, int)
#define AMSTREAM_IOC_AB_SIZE    _IOW(AMSTREAM_IOC_MAGIC, 0x03, int)
#define AMSTREAM_IOC_VFORMAT    _IOW(AMSTREAM_IOC_MAGIC, 0x04, int)
#define AMSTREAM_IOC_AFORMAT    _IOW(AMSTREAM_IOC_MAGIC, 0x05, int)
#define AMSTREAM_IOC_VID        _IOW(AMSTREAM_IOC_MAGIC, 0x06, int)
#define AMSTREAM_IOC_AID        _IOW(AMSTREAM_IOC_MAGIC, 0x07, int)
#define AMSTREAM_IOC_VB_STATUS  _IOR(AMSTREAM_IOC_MAGIC, 0x08, unsigned long)
#define AMSTREAM_IOC_AB_STATUS  _IOR(AMSTREAM_IOC_MAGIC, 0x09, unsigned long)
#define AMSTREAM_IOC_SYSINFO    _IOW(AMSTREAM_IOC_MAGIC, 0x0a, int)
#define AMSTREAM_IOC_ACHANNEL   _IOW(AMSTREAM_IOC_MAGIC, 0x0b, int)
#define AMSTREAM_IOC_SAMPLERATE _IOW(AMSTREAM_IOC_MAGIC, 0x0c, int)
#define AMSTREAM_IOC_DATAWIDTH  _IOW(AMSTREAM_IOC_MAGIC, 0x0d, int)
#define AMSTREAM_IOC_TSTAMP     _IOW(AMSTREAM_IOC_MAGIC, 0x0e, unsigned long)
#define AMSTREAM_IOC_VDECSTAT   _IOR(AMSTREAM_IOC_MAGIC, 0x0f, unsigned long)
#define AMSTREAM_IOC_ADECSTAT   _IOR(AMSTREAM_IOC_MAGIC, 0x10, unsigned long)

#define AMSTREAM_IOC_PORT_INIT   _IO(AMSTREAM_IOC_MAGIC, 0x11)
#define AMSTREAM_IOC_TRICKMODE   _IOW(AMSTREAM_IOC_MAGIC, 0x12, unsigned long)

#define AMSTREAM_IOC_AUDIO_INFO  _IOW(AMSTREAM_IOC_MAGIC, 0x13, unsigned long)
#define AMSTREAM_IOC_TRICK_STAT  _IOR(AMSTREAM_IOC_MAGIC, 0x14, unsigned long)
#define AMSTREAM_IOC_AUDIO_RESET _IO(AMSTREAM_IOC_MAGIC, 0x15)
#define AMSTREAM_IOC_SID         _IOW(AMSTREAM_IOC_MAGIC, 0x16, int)
#define AMSTREAM_IOC_VPAUSE      _IOW(AMSTREAM_IOC_MAGIC, 0x17, int)
#define AMSTREAM_IOC_AVTHRESH    _IOW(AMSTREAM_IOC_MAGIC, 0x18, int)
#define AMSTREAM_IOC_SYNCTHRESH  _IOW(AMSTREAM_IOC_MAGIC, 0x19, int)
#define AMSTREAM_IOC_SUB_RESET   _IOW(AMSTREAM_IOC_MAGIC, 0x1a, int)
#define AMSTREAM_IOC_SUB_LENGTH  _IOR(AMSTREAM_IOC_MAGIC, 0x1b, unsigned long)
#define AMSTREAM_IOC_SET_DEC_RESET _IOW(AMSTREAM_IOC_MAGIC, 0x1c, int)
#define AMSTREAM_IOC_TS_SKIPBYTE _IOW(AMSTREAM_IOC_MAGIC, 0x1d, int)
#define AMSTREAM_IOC_SUB_TYPE    _IOW(AMSTREAM_IOC_MAGIC, 0x1e, int)
#define AMSTREAM_IOC_CLEAR_VIDEO _IOW(AMSTREAM_IOC_MAGIC, 0x1f, int)

#define AMSTREAM_IOC_APTS               _IOR(AMSTREAM_IOC_MAGIC, 0x40, unsigned long)
#define AMSTREAM_IOC_VPTS               _IOR(AMSTREAM_IOC_MAGIC, 0x41, unsigned long)
#define AMSTREAM_IOC_PCRSCR             _IOR(AMSTREAM_IOC_MAGIC, 0x42, unsigned long)
#define AMSTREAM_IOC_SYNCENABLE         _IOW(AMSTREAM_IOC_MAGIC, 0x43, unsigned long)
#define AMSTREAM_IOC_GET_SYNC_ADISCON   _IOR(AMSTREAM_IOC_MAGIC, 0x44, unsigned long)
#define AMSTREAM_IOC_SET_SYNC_ADISCON   _IOW(AMSTREAM_IOC_MAGIC, 0x45, unsigned long)
#define AMSTREAM_IOC_GET_SYNC_VDISCON   _IOR(AMSTREAM_IOC_MAGIC, 0x46, unsigned long)
#define AMSTREAM_IOC_SET_SYNC_VDISCON   _IOW(AMSTREAM_IOC_MAGIC, 0x47, unsigned long)
#define AMSTREAM_IOC_GET_VIDEO_DISABLE  _IOR(AMSTREAM_IOC_MAGIC, 0x48, unsigned long)
#define AMSTREAM_IOC_SET_VIDEO_DISABLE  _IOW(AMSTREAM_IOC_MAGIC, 0x49, unsigned long)
#define AMSTREAM_IOC_SET_PCRSCR         _IOW(AMSTREAM_IOC_MAGIC, 0x4a, unsigned long)
#define AMSTREAM_IOC_GET_VIDEO_AXIS     _IOR(AMSTREAM_IOC_MAGIC, 0x4b, unsigned long)
#define AMSTREAM_IOC_SET_VIDEO_AXIS     _IOW(AMSTREAM_IOC_MAGIC, 0x4c, unsigned long)
#define AMSTREAM_IOC_GET_VIDEO_CROP     _IOR(AMSTREAM_IOC_MAGIC, 0x4d, unsigned long)
#define AMSTREAM_IOC_SET_VIDEO_CROP     _IOW(AMSTREAM_IOC_MAGIC, 0x4e, unsigned long)

// VPP.VE IOCTL command list
#define AMSTREAM_IOC_VE_BEXT     _IOW(AMSTREAM_IOC_MAGIC, 0x20, struct ve_bext_s  )
#define AMSTREAM_IOC_VE_DNLP     _IOW(AMSTREAM_IOC_MAGIC, 0x21, struct ve_dnlp_s  )
#define AMSTREAM_IOC_VE_HSVS     _IOW(AMSTREAM_IOC_MAGIC, 0x22, struct ve_hsvs_s  )
#define AMSTREAM_IOC_VE_CCOR     _IOW(AMSTREAM_IOC_MAGIC, 0x23, struct ve_ccor_s  )
#define AMSTREAM_IOC_VE_BENH     _IOW(AMSTREAM_IOC_MAGIC, 0x24, struct ve_benh_s  )
#define AMSTREAM_IOC_VE_DEMO     _IOW(AMSTREAM_IOC_MAGIC, 0x25, struct ve_demo_s  )
#define AMSTREAM_IOC_VE_VDO_MEAS _IOW(AMSTREAM_IOC_MAGIC, 0x27, struct vdo_meas_s )
#define AMSTREAM_IOC_VE_DEBUG    _IOWR(AMSTREAM_IOC_MAGIC, 0x28, unsigned long long)
#define AMSTREAM_IOC_VE_REGMAP   _IOW(AMSTREAM_IOC_MAGIC, 0x29, struct ve_regmap_s)

// VPP.CM IOCTL command list
#define AMSTREAM_IOC_CM_REGION  _IOW(AMSTREAM_IOC_MAGIC, 0x30, struct cm_region_s)
#define AMSTREAM_IOC_CM_TOP     _IOW(AMSTREAM_IOC_MAGIC, 0x31, struct cm_top_s   )
#define AMSTREAM_IOC_CM_DEMO    _IOW(AMSTREAM_IOC_MAGIC, 0x32, struct cm_demo_s  )
#define AMSTREAM_IOC_CM_DEBUG   _IOWR(AMSTREAM_IOC_MAGIC, 0x33, unsigned long long)
#define AMSTREAM_IOC_CM_REGMAP  _IOW(AMSTREAM_IOC_MAGIC, 0x34, struct cm_regmap_s)

//VPP.3D IOCTL command list
//#define  AMSTREAM_IOC_SET_3D_TYPE  _IOW(AMSTREAM_IOC_MAGIC, 0x3c, unsigned int)
//#define  AMSTREAM_IOC_GET_3D_TYPE  _IOW(AMSTREAM_IOC_MAGIC, 0x3d, unsigned int)

#define AMSTREAM_IOC_SUB_NUM    _IOR(AMSTREAM_IOC_MAGIC, 0x50, unsigned long)
#define AMSTREAM_IOC_SUB_INFO   _IOR(AMSTREAM_IOC_MAGIC, 0x51, unsigned long)
#define AMSTREAM_IOC_GET_BLACKOUT_POLICY   _IOR(AMSTREAM_IOC_MAGIC, 0x52, unsigned long)
#define AMSTREAM_IOC_SET_BLACKOUT_POLICY   _IOW(AMSTREAM_IOC_MAGIC, 0x53, unsigned long)
#define AMSTREAM_IOC_GET_SCREEN_MODE _IOR(AMSTREAM_IOC_MAGIC, 0x58, int)
#define AMSTREAM_IOC_SET_SCREEN_MODE _IOW(AMSTREAM_IOC_MAGIC, 0x59, int)
#define AMSTREAM_IOC_GET_VIDEO_DISCONTINUE_REPORT _IOR(AMSTREAM_IOC_MAGIC, 0x5a, int)
#define AMSTREAM_IOC_SET_VIDEO_DISCONTINUE_REPORT _IOW(AMSTREAM_IOC_MAGIC, 0x5b, int)
#define AMSTREAM_IOC_VF_STATUS  _IOR(AMSTREAM_IOC_MAGIC, 0x60, unsigned long)
#define AMSTREAM_IOC_CLEAR_VBUF _IO(AMSTREAM_IOC_MAGIC, 0x80)

#define AMSTREAM_IOC_APTS_LOOKUP    _IOR(AMSTREAM_IOC_MAGIC, 0x81, unsigned long)
#define GET_FIRST_APTS_FLAG         _IOR(AMSTREAM_IOC_MAGIC, 0x82, long)

#define AMSTREAM_IOC_GET_SYNC_ADISCON_DIFF  _IOR(AMSTREAM_IOC_MAGIC, 0x83, unsigned long)
#define AMSTREAM_IOC_GET_SYNC_VDISCON_DIFF  _IOR(AMSTREAM_IOC_MAGIC, 0x84, unsigned long)
#define AMSTREAM_IOC_SET_SYNC_ADISCON_DIFF  _IOW(AMSTREAM_IOC_MAGIC, 0x85, unsigned long)
#define AMSTREAM_IOC_SET_SYNC_VDISCON_DIFF  _IOW(AMSTREAM_IOC_MAGIC, 0x86, unsigned long)
#define AMSTREAM_IOC_GET_FREERUN_MODE  _IOR(AMSTREAM_IOC_MAGIC, 0x87, unsigned long)
#define AMSTREAM_IOC_SET_FREERUN_MODE  _IOW(AMSTREAM_IOC_MAGIC, 0x88, unsigned long)
#define AMSTREAM_IOC_SET_DEMUX         _IOW(AMSTREAM_IOC_MAGIC, 0x90, unsigned long)

#define AMSTREAM_IOC_SET_VIDEO_DELAY_LIMIT_MS _IOW(AMSTREAM_IOC_MAGIC, 0xa0, unsigned long)
#define AMSTREAM_IOC_GET_VIDEO_DELAY_LIMIT_MS _IOR(AMSTREAM_IOC_MAGIC, 0xa1, unsigned long)
#define AMSTREAM_IOC_SET_AUDIO_DELAY_LIMIT_MS _IOW(AMSTREAM_IOC_MAGIC, 0xa2, unsigned long)
#define AMSTREAM_IOC_GET_AUDIO_DELAY_LIMIT_MS _IOR(AMSTREAM_IOC_MAGIC, 0xa3, unsigned long)
#define AMSTREAM_IOC_GET_AUDIO_CUR_DELAY_MS _IOR(AMSTREAM_IOC_MAGIC, 0xa4, unsigned long)
#define AMSTREAM_IOC_GET_VIDEO_CUR_DELAY_MS _IOR(AMSTREAM_IOC_MAGIC, 0xa5, unsigned long)
#define AMSTREAM_IOC_GET_AUDIO_AVG_BITRATE_BPS _IOR(AMSTREAM_IOC_MAGIC, 0xa6, unsigned long)
#define AMSTREAM_IOC_GET_VIDEO_AVG_BITRATE_BPS _IOR(AMSTREAM_IOC_MAGIC, 0xa7, unsigned long)

#define TRICKMODE_NONE       0x00
#define TRICKMODE_I          0x01
#define TRICKMODE_FFFB       0x02

#define TRICK_STAT_DONE      0x01
#define TRICK_STAT_WAIT      0x00

#define AUDIO_EXTRA_DATA_SIZE   (4096)
#define MAX_SUB_NUM     32

typedef struct tcon_gamma_table_s {
    unsigned short data[256];
} tcon_gamma_table_t;

typedef struct tcon_rgb_ogo_s {
    unsigned int en;
    int r_pre_offset;  // s11.0, range -1024~+1023, default is 0
    int g_pre_offset;  // s11.0, range -1024~+1023, default is 0
    int b_pre_offset;  // s11.0, range -1024~+1023, default is 0
    unsigned int r_gain;        // u1.10, range 0~2047, default is 1024 (1.0x)
    unsigned int g_gain;        // u1.10, range 0~2047, default is 1024 (1.0x)
    unsigned int b_gain;        // u1.10, range 0~2047, default is 1024 (1.0x)
    int r_post_offset; // s11.0, range -1024~+1023, default is 0
    int g_post_offset; // s11.0, range -1024~+1023, default is 0
    int b_post_offset; // s11.0, range -1024~+1023, default is 0
} tcon_rgb_ogo_t;

#endif //__AMSTREAM_H__
