/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */


#define LOG_TAG "media_ctl"
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
#include "common_ctl.h"
#ifdef  __cplusplus
extern "C" {
#endif

int media_set_ctl(const char * path,int setval)
{
    int ret = 0;
    char val[32];
    CTRL_PRINT("set %s =%d\n",path,setval);
    sprintf(val, "%d", setval);
    ret = media_set_cmd_str(path, val);
    if (ret)
    {
        ret = UnSupport;/*if fail we also return unsupport then will use open sysfs to read*/
    }
    return ret;
}

int media_get_ctl(const char * path)
{
    char buf[32];
    int ret = 0;
    int val = 0;
    ret = media_get_cmd_str(path, buf, 32);
    if (!ret) {
        if (strstr(buf, "0x"))
            sscanf(buf, "0x%x", &val);
        else if (strstr(buf, "0X"))
            sscanf(buf, "0X%x", &val);
        else
            sscanf(buf, "%d", &val);
    }
    CTRL_PRINT("get path=%s val =%d\n",path,val);
    return val;
}

int media_set_ctl_str(const char * path,char* setval)
{
   int ret = 0;
   CTRL_PRINT("setstr %s =%s\n",path,setval);
   ret = media_set_cmd_str(path, setval);
   if (ret)
   {
       ret = UnSupport;/*if fail we also return unsupport then will use open sysfs to read*/
   }
    return ret;
}

int media_get_ctl_str(const char * path, char* buf, int size)
{
    int ret = 0;
    char getbuf[300];
    CTRL_PRINT("getstr path %s size=%d\n",path,size);
    if (size >300)
		size = 300;
    ret = media_get_cmd_str(path, getbuf, size);
    strncpy(buf,getbuf,size);
    return ret;
}


int media_sync_set_ctl(const char * path,int setval)
{
    int ret = 0;
    char val[32];
    CTRL_PRINT("set %s =%d\n",path,setval);
    sprintf(val, "%d", setval);
    ret = media_sync_set_cmd_str(path, val);
    return ret;
}

int media_sync_get_ctl(const char * path)
{
    char buf[32];
    int ret = 0;
    int val = 0;
    ret = media_sync_get_cmd_str(path, buf, 32);
    if (!ret) {
        sscanf(buf, "%d", &val);
    }
    CTRL_PRINT("get val =%d\n",val);
    return val == 1 ? val : 0;
}

int media_sync_set_ctl_str(const char * path,char* setval)
{
    return  media_sync_set_cmd_str(path, setval);
}

int media_sync_get_ctl_str(const char * path, char* buf, int size)
{
    int ret = 0;
    ret = media_sync_get_cmd_str(path, buf, size);
    return ret;
}

int media_video_set_ctl(const char * path,int setval)
{
    int ret = 0;
    char val[32];
    CTRL_PRINT("set %s =%d\n",path,setval);
    sprintf(val, "%d", setval);
    ret = media_video_set_cmd_str(path, val);
    return ret;
}

int media_video_get_ctl(const char * path)
{
    char buf[32];
    int ret = 0;
    int val = 0;
    ret = media_video_get_cmd_str(path, buf, 32);
    if (!ret) {
        sscanf(buf, "%d", &val);
    }
	CTRL_PRINT("get val =%d\n",val);
    return val == 1 ? val : 0;
}

int media_video_set_ctl_str(const char * path,char* setval)
{
    media_video_set_cmd_str(path, setval);
    return 0;
}

int media_video_get_ctl_str(const char * path, char* buf, int size)
{
    int ret = 0;
    ret = media_video_get_cmd_str(path, buf, size);
    return ret;
}

int media_decoder_set_ctl(const char * path,int setval)
{
    char val[32];
    CTRL_PRINT("set %s =%d\n",path,setval);
    sprintf(val, "%d", setval);
    media_decoder_set_cmd_str(path, val);
	return 0;
}

int media_decoder_get_ctl(const char * path)
{
    char buf[32];
    int ret = 0;
    int val = 0;
    ret = media_decoder_get_cmd_str(path, buf, 32);
    if (!ret) {
        sscanf(buf, "%d", &val);
    }
	CTRL_PRINT("get val =%d\n",val);
    return val == 1 ? val : 0;
}

int media_decoder_set_ctl_str(const char * path,char* setval)
{
    media_decoder_set_cmd_str(path, setval);
    return 0;
}

int media_decoder_get_ctl_str(const char * path, char* buf, int size)
{
    int ret = 0;
    ret = media_decoder_get_cmd_str(path, buf, size);
    return ret;
}

int media_open(const char *path, int flags)
{
    int fd = 0;
	fd = open(path, flags);
    if (fd < 0) {
        CTRL_PRINT("open [%s] failed,ret = %d errno=%d\n", path, fd, errno);
        return fd;
    }
    return fd;
}

int media_close(int fd)
{
    int res = 0;
    if (fd) {
        res = close(fd);
	}
    return res;
}

int media_control(int fd, int cmd, unsigned long paramter)
{
    int r;

    if (fd < 0) {
        return -1;
    }
    r = ioctl(fd, cmd, paramter);
    if (r < 0) {
        CTRL_PRINT("send control failed,handle=%d,cmd=%x,paramter=%lx, t=%x errno=%d\n", fd, cmd, paramter, r, errno);
        return r;
    }
    return 0;
}

int media_get_int(const char* dev, int cmd)
{
    int fd,res;
	unsigned long para;
	fd = media_open(dev,O_RDWR);
	res = media_control(fd, cmd,(unsigned long)&para);
	if (0 == res)
	media_close(fd);
	ALOGD("get para =%ld\n",para);
	return para == 1 ? para : 0;
}
int media_set_int(const char* dev, int cmd, int setpara)
{
    int fd,res;
	unsigned long para=setpara;
	CTRL_PRINT("set para =%ld\n",para);
	fd = media_open(dev,O_RDWR);
	res = media_control(fd, cmd, (unsigned long)&para);
	if (fd >= 0)
	media_close(fd);
	return res;
}

int media_video_get_int(int cmd)
{
	return media_get_int("/dev/amvideo", cmd);
}

int media_video_set_int(int cmd, int para)
{
	return media_set_int("/dev/amvideo", cmd, para);
}

int media_vfm_set_ulong(int cmd, unsigned long para)
{   int fd,res;
    fd = media_open("dev/vfm",O_RDWR);
    if (fd < 0)
        return fd;
	res = media_control(fd, cmd, para);
	if (fd >= 0)
	media_close(fd);
	return res;
}


int media_set_vfm_map_str(const char* val)
{
    //int fd,res;
    struct vfmctl setvfmctl;
    if (val == NULL)
        return -1;
    setvfmctl.name[0] = '\0';
    strncpy(setvfmctl.val, val, sizeof(setvfmctl.val));
    return media_vfm_set_ulong(VFM_IOCTL_CMD_SET, (unsigned long)&setvfmctl);
}

int media_get_vfm_map_str(char* val,int size)
{
    int len;
    struct vfmctl setvfmctl;
    if (val == NULL)
        return -1;
    setvfmctl.name[0] = '\0';
    setvfmctl.val[0] = '\0';
    media_vfm_set_ulong(VFM_IOCTL_CMD_GET, (unsigned long)&setvfmctl);
	len = sizeof(setvfmctl.val);
    if (len > size)
        len = size;
	strncpy(val, setvfmctl.val, len);
	CTRL_PRINT("get vfm:val %s \n",val);
	return 0;
}

int media_rm_vfm_map_str(const char* name,const char* val)
{
    //int fd,res;
    struct vfmctl setvfmctl;
    if (val == NULL || name == NULL)
        return -1;
    strncpy(setvfmctl.name,name,sizeof(setvfmctl.name));
    strncpy(setvfmctl.val,val,sizeof(setvfmctl.val));
    CTRL_PRINT("rm vfm: cmd=%s,val %s \n",name,val);
    return media_vfm_set_ulong(VFM_IOCTL_CMD_RM,(unsigned long)&setvfmctl);
}

int media_add_vfm_map_str(const char* name,const char* val)
{
    //int fd,res;
    struct vfmctl setvfmctl;
    if (val == NULL || name == NULL)
        return -1;
    strncpy(setvfmctl.name,name,sizeof(setvfmctl.name));
    strncpy(setvfmctl.val,val,sizeof(setvfmctl.val));
	CTRL_PRINT("add vfm: cmd=%s,val %s \n",name,val);
	return media_vfm_set_ulong(VFM_IOCTL_CMD_ADD,(unsigned long)&setvfmctl);
}

int media_codec_mm_set_ctl_str(const char * path,char* setval)
{
    return media_codecmm_set_cmd_str(path, setval);
}

int media_codec_mm_get_ctl_str(const char * path, char* buf, int size)
{
    int ret = 0;
    ret = media_codecmm_get_cmd_str(path, buf, size);
    return ret;
}


int media_sub_getinfo(int type)
{   int fd,res;
	struct subinfo_para_s subinfo;
	subinfo.subinfo_type = (subinfo_para_e)type;
	subinfo.subtitle_info= 0;
	subinfo.data = NULL;
	fd = media_open("/dev/amsubtitle",O_RDONLY );
	res = media_control(fd,AMSTREAM_IOC_GET_SUBTITLE_INFO, (unsigned long)&subinfo);
	if (fd >= 0)
	media_close(fd);
	return subinfo.subtitle_info;

}

int media_sub_setinfo(int type, int info)
{   int fd,res;
	struct subinfo_para_s subinfo;
	subinfo.subinfo_type = (subinfo_para_e)type;
	subinfo.subtitle_info= info;
	subinfo.data = NULL;
	fd = media_open("/dev/amsubtitle",O_WRONLY);
	res = media_control(fd, AMSTREAM_IOC_SET_SUBTITLE_INFO, (unsigned long)&subinfo);
	if (fd >= 0)
	media_close(fd);
	return res;
}

#ifdef  __cplusplus
}
#endif
