/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */


#ifndef COMMON_CTL_H
#define COMMON_CTL_H

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <cutils/log.h>
#include <../mediaconfig/media_config.h>
#include <amports/amstream.h>

#ifdef  __cplusplus
extern "C" {
#endif

#define UnSupport  0xFFFF
#define CTRL_PRINT ALOGD
//#define CTRL_PRINT
int media_open(const char *path, int flags);
int media_close(int fd);
int media_control(int fd, int cmd, unsigned long paramter);
int media_set_ctl(const char * path,int setval);
int media_get_ctl(const char * path);
int media_set_ctl_str(const char * path,char* setval);
int media_get_ctl_str(const char * path, char* buf, int size);
int media_sync_set_ctl(const char * path,int setval);
int media_sync_get_ctl(const char * path);
int media_sync_set_ctl_str(const char * path,char* setval);
int media_sync_get_ctl_str(const char * path, char* buf, int size);
int media_video_set_ctl(const char * path,int setval);
int media_video_get_ctl(const char * path);
int media_video_set_ctl_str(const char * path,char* setval);
int media_video_get_ctl_str(const char * path, char* buf, int size);
int media_decoder_set_ctl(const char * path,int setval);
int media_decoder_get_ctl(const char * path);
int media_decoder_set_ctl_str(const char * path,char* setval);
int media_decoder_get_ctl_str(const char * path, char* buf, int size);
int media_video_get_int(int cmd);
int media_video_set_int(int cmd, int para);
int media_vfm_get_ulong(int cmd);
int media_vfm_set_ulong(int cmd, unsigned long para);
int media_codec_mm_set_ctl_str(const char * path,char* setval);
int media_codec_mm_get_ctl_str(const char * path, char* buf, int size);
int media_get_vfm_map_str(char* val,int size);
int media_set_vfm_map_str(const char* val);
int media_rm_vfm_map_str(const char* name,const char* val);
int media_add_vfm_map_str(const char* name,const char* val);
int media_sub_getinfo(int type);
int media_sub_setinfo(int type, int info);
struct vfmctl{
char name[10];
char val[300];
};

#define VFM_IOC_MAGIC  'V'
#define VFM_IOCTL_CMD_ADD   _IOW((VFM_IOC_MAGIC), 0x00, struct vfmctl)
#define VFM_IOCTL_CMD_RM    _IOW((VFM_IOC_MAGIC), 0x01, struct vfmctl)
#define VFM_IOCTL_CMD_DUMP   _IOW((VFM_IOC_MAGIC), 0x02, struct vfmctl)
#define VFM_IOCTL_CMD_ADDDUMMY    _IOW((VFM_IOC_MAGIC), 0x03, struct vfmctl)
#define VFM_IOCTL_CMD_SET   _IOW(VFM_IOC_MAGIC, 0x04, struct vfmctl)
#define VFM_IOCTL_CMD_GET   _IOWR(VFM_IOC_MAGIC, 0x05, struct vfmctl)


enum subinfo_para_e {
	SUB_NULL = -1,
	SUB_ENABLE = 0,
	SUB_TOTAL,
	SUB_WIDTH,
	SUB_HEIGHT,
	SUB_TYPE,
	SUB_CURRENT,
	SUB_INDEX,
	SUB_WRITE_POS,
	SUB_START_PTS,
	SUB_FPS,
	SUB_SUBTYPE,
	SUB_RESET,
	SUB_DATA_T_SIZE,
	SUB_DATA_T_DATA
};

struct subinfo_para_s {
	enum subinfo_para_e subinfo_type;
	int subtitle_info;
	char *data;
};

#ifdef  __cplusplus
}
#endif

#endif
