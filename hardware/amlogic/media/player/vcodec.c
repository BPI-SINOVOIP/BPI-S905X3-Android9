/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */


/**
* @file codec_ctrl.c
* @brief  Codec control lib functions
*
* @version 1.0.0
* @date 2011-02-24
*/
/* Copyright (C) 2007-2011, Amlogic Inc.
* All right reserved
*
*/
#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include "vcodec.h"

#ifdef ANDROID
#include <android/log.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#define  LOG_TAG    "amcodec"
#define CODEC_PRINT(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define CODEC_PRINT(f,s...) fprintf(stderr,f,##s)
#endif

#define msleep(n)   usleep(n*1000)

// ports
#define CODEC_VIDEO_ES_DEVICE		"/dev/amstream_vbuf"
#define CODEC_VIDEO_ES_DEVICE_SCHED	"/dev/amstream_vbuf_sched"
#define CODEC_VIDEO_ES_HEVC_DEVICE_SCHED	"/dev/amstream_hevc_sched"
#define CODEC_TS_DEVICE			"/dev/amstream_mpts"
#define CODEC_PS_DEVICE			"/dev/amstream_mpps"
#define CODEC_RM_DEVICE			"/dev/amstream_rm"
#define CODEC_VIDEO_HEVC_DEVICE		"/dev/amstream_hevc"
#define CODEC_VIDEO_DVAVC_DEVICE	"/dev/amstream_dves_avc"
#define CODEC_VIDEO_DVHEVC_DEVICE	"/dev/amstream_dves_hevc"
#define CODEC_CNTL_DEVICE		"/dev/amvideo"
#define CODEC_VIDEO_FRAME_DEVICE "/dev/amstream_vframe"
#define CODEC_HEVC_FRAME_DEVICE "/dev/amstream_hevc_frame"
/*#define CODEC_VIDEO_DVAV1_DEVICE	"/dev/amstream_dves_av1"*/

// ioctl
#define AMSTREAM_IOC_MAGIC  'S'
#define AMSTREAM_IOC_VB_SIZE	_IOW((AMSTREAM_IOC_MAGIC), 0x01, int)
#define AMSTREAM_IOC_VFORMAT	_IOW((AMSTREAM_IOC_MAGIC), 0x04, int)
#define AMSTREAM_IOC_VID	_IOW((AMSTREAM_IOC_MAGIC), 0x06, int)
#define AMSTREAM_IOC_VB_STATUS	_IOR((AMSTREAM_IOC_MAGIC), 0x08, int)
#define AMSTREAM_IOC_SYSINFO	_IOW((AMSTREAM_IOC_MAGIC), 0x0a, int)
#define AMSTREAM_IOC_PORT_INIT	_IO((AMSTREAM_IOC_MAGIC), 0x11)
#define AMSTREAM_IOC_VPAUSE	_IOW((AMSTREAM_IOC_MAGIC), 0x17, int)
#define AMSTREAM_IOC_SET_DRMMODE _IOW((AMSTREAM_IOC_MAGIC), 0x91, int)
#define AMSTREAM_IOC_GET_VERSION _IOR((AMSTREAM_IOC_MAGIC), 0xc0, int)
#define AMSTREAM_IOC_GET	_IOWR((AMSTREAM_IOC_MAGIC), 0xc1, struct am_ioctl_parm)
#define AMSTREAM_IOC_SET	_IOW((AMSTREAM_IOC_MAGIC), 0xc2, struct am_ioctl_parm)
#define AMSTREAM_IOC_GET_EX	_IOWR((AMSTREAM_IOC_MAGIC), 0xc3, struct am_ioctl_parm_ex)

#define AMSTREAM_IOC_SET_CRC _IOW((AMSTREAM_IOC_MAGIC), 0xc9, struct usr_crc_info_t)
#define AMSTREAM_IOC_GET_CRC_CMP_RESULT _IOWR((AMSTREAM_IOC_MAGIC), 0xca, int)

// cmds
#define AMSTREAM_SET_VB_SIZE		0x102
#define AMSTREAM_SET_VFORMAT		0x105
#define AMSTREAM_SET_VID		0x107
#define AMSTREAM_PORT_INIT		0x111
#define AMSTREAM_SET_DRMMODE		0x11C
#define AMSTREAM_GET_EX_VB_STATUS	0x900
#define AMSTREAM_GET_EX_VDECSTAT	0x902

struct am_ioctl_parm {
    union {
        unsigned int data_32;
        unsigned long long data_64;
        int data_vformat;
        char data[8];
    };
    unsigned int cmd;
    char reserved[4];
};

struct am_io_param {
    union {
        int data;
        int id;
    };

    int len;

    union {
        char buf[1];
        struct buf_status status;
    };
};

struct am_ioctl_parm_ex {
    union {
        struct buf_status status;
	struct vdec_status vstatus;
        char data[24];
    };
    unsigned int cmd;
    char reserved[4];
};

struct vcodec_amd_table {
    unsigned int cmd;
    unsigned int parm_cmd;
};

static struct vcodec_amd_table cmd_tables[] = {
    /*amstream*/
    { AMSTREAM_IOC_VB_SIZE, AMSTREAM_SET_VB_SIZE },
    { AMSTREAM_IOC_VFORMAT, AMSTREAM_SET_VFORMAT },
    { AMSTREAM_IOC_VID, AMSTREAM_SET_VID },
    { AMSTREAM_IOC_VB_STATUS, AMSTREAM_GET_EX_VB_STATUS },
    { AMSTREAM_IOC_PORT_INIT, AMSTREAM_PORT_INIT },
    { AMSTREAM_IOC_SET_DRMMODE, AMSTREAM_SET_DRMMODE },
    { 0, 0 },
};

static int support_new_cmd = 0;

static void vcodec_h_set_support_new_cmd(int value)
{
    support_new_cmd = value;
}

static int vcodec_h_is_support_new_cmd()
{
    return support_new_cmd;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  vcodec_h_open  Open codec devices by file name
*
* @param[in]  port_addr  File name of codec device
* @param[in]  flags      Open flags
*
* @return     The handler of codec device
*/
/* --------------------------------------------------------------------------*/
static int vcodec_h_open(const char *port_addr, int flags)
{
    int r;
    int retry_open_times = 0;
retry_open:
    r = open(port_addr, flags);
    if (r < 0 /*&& r == EBUSY */) {
        //--------------------------------
        retry_open_times++;
        if (retry_open_times == 1) {
            CODEC_PRINT("Init [%s] failed,ret = %d error=%d retry_open!\n", port_addr, r, errno);
        }
        msleep(10);
        if (retry_open_times < 1000) {
            goto retry_open;
        }
        CODEC_PRINT("retry_open [%s] failed,ret = %d error=%d used_times=%d*10(ms)\n", port_addr, r, errno, retry_open_times);
        //--------------------------------
        //CODEC_PRINT("Init [%s] failed,ret = %d error=%d\n", port_addr, r, errno);
        return r;
    }
    if (retry_open_times > 0) {
        CODEC_PRINT("retry_open [%s] success,ret = %d error=%d used_times=%d*10(ms)\n", port_addr, r, errno, retry_open_times);
    }
    return (int)r;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  vcodec_h_close  Close codec devices
*
* @param[in]  h  Handler of codec device
*
* @return     0 for success
*/
/* --------------------------------------------------------------------------*/
static int vcodec_h_close(int h)
{
    int r;
    if (h >= 0) {
        r = close(h);
        if (r < 0) {
            CODEC_PRINT("close failed,handle=%d,ret=%d errno=%d\n", h, r, errno);
        }
    }
    return 0;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  vcodec_h_control  IOCTL commands for codec devices
*
* @param[in]  h         Codec device handler
* @param[in]  cmd       IOCTL commands
* @param[in]  paramter  IOCTL commands parameter
*
* @return     0 for success, non-0 for fail
*/
/* --------------------------------------------------------------------------*/
static int vcodec_h_control(int h, int cmd, unsigned long paramter)
{
    int r;

    if (h < 0) {
        return -1;
    }
    r = ioctl(h, cmd, paramter);
    if (r < 0) {
        CODEC_PRINT("send control failed,handle=%d,cmd=%x,paramter=%lx, t=%x errno=%d\n", h, cmd, paramter, r, errno);
        return r;
    }
    return 0;
}

static int vcodec_h_ioctl_set(int h, int subcmd, unsigned long paramter)
{
    int r;
    int cmd_new = AMSTREAM_IOC_SET;
    unsigned long parm_new;
    switch (subcmd) {
    case AMSTREAM_SET_VB_SIZE:
    case AMSTREAM_SET_VID:
    case AMSTREAM_SET_DRMMODE: {
        struct am_ioctl_parm parm;
        memset(&parm, 0, sizeof(parm));
        parm.cmd = subcmd;
        parm.data_32 = paramter;
        parm_new = (unsigned long)&parm;
        r = ioctl(h, cmd_new, parm_new);
    }
    break;
    case AMSTREAM_SET_VFORMAT: {
        struct am_ioctl_parm parm;
        memset(&parm, 0, sizeof(parm));
        parm.cmd = subcmd;
        parm.data_vformat = paramter;
        parm_new = (unsigned long)&parm;
        r = ioctl(h, cmd_new, parm_new);
    }
    break;
    case AMSTREAM_PORT_INIT: {
        struct am_ioctl_parm parm;
        memset(&parm, 0, sizeof(parm));
        parm.cmd = subcmd;
        parm_new = (unsigned long)&parm;
        r = ioctl(h, cmd_new, parm_new);
    }
    break;
    default: {
        struct am_ioctl_parm parm;
        memset(&parm, 0, sizeof(parm));
        parm.cmd = subcmd;
        parm.data_32 = paramter;
        parm_new = (unsigned long)&parm;
        r = ioctl(h, cmd_new, parm_new);
    }
    break;
    }

    if (r < 0) {
        CODEC_PRINT("vcodec_h_ioctl_set failed,handle=%d,cmd=%x,paramter=%lx, t=%x errno=%d\n", h, subcmd, paramter, r, errno);
        return r;
    }
    return 0;
}

static int vcodec_h_ioctl_get(int h, int subcmd, unsigned long paramter)
{
    int r;

    struct am_ioctl_parm parm;
    unsigned long parm_new;
    memset(&parm, 0, sizeof(parm));
    parm.cmd = subcmd;
    parm.data_32 = *(unsigned int *)paramter;
    parm_new = (unsigned long)&parm;
    r = ioctl(h, AMSTREAM_IOC_GET, parm_new);
    if (r < 0) {
        CODEC_PRINT("vcodec_h_ioctl_get failed,handle=%d,subcmd=%x,paramter=%lx, t=%x errno=%d\n", h, subcmd, paramter, r, errno);
        return r;
    }
    if (paramter != 0) {
        *(unsigned int *)paramter = parm.data_32;
    }
    return 0;
}

static int get_old_cmd(unsigned int cmd)
{
    struct vcodec_amd_table *p;
    for (p = cmd_tables; p->cmd; p++) {
        if (p->parm_cmd == cmd) {
            return p->cmd;
        }
    }
    return -1;
}

static int vcodec_h_ioctl_get_ex(int h, int subcmd, unsigned long paramter)
{
    int r;
    int cmd_new = AMSTREAM_IOC_GET_EX;
    unsigned long parm_new;

    switch (subcmd) {
    case AMSTREAM_GET_EX_VB_STATUS: {
        struct am_ioctl_parm_ex parm;
        memset(&parm, 0, sizeof(parm));
        parm.cmd = subcmd;
        parm_new = (unsigned long)&parm;
        r = ioctl(h, cmd_new, parm_new);
        if (r >= 0 && paramter != 0) {
            memcpy((void *)paramter, &parm.status, sizeof(struct buf_status));
        }
    }
    break;
    case AMSTREAM_GET_EX_VDECSTAT: {
        struct am_ioctl_parm_ex parm;
        memset(&parm, 0, sizeof(parm));
        parm.cmd = subcmd;
        parm_new = (unsigned long)&parm;
        r = ioctl(h, cmd_new, parm_new);
        if (r >= 0 && paramter != 0) {
            memcpy((void *)paramter, &parm.vstatus, sizeof(struct vdec_status));
        }
    }
    break;
    default:
        r = -1;
        break;
    }
    if (r < 0) {
        CODEC_PRINT("vcodec_h_ioctl_get_ex failed,handle=%d,subcmd=%x,paramter=%lx, t=%x errno=%d\n", h, subcmd, paramter, r, errno);
        return r;
    }
    return 0;

}

/* --------------------------------------------------------------------------*/
/**
* @brief  vcodec_h_control  IOCTL commands for codec devices
*
* @param[in]  h         Codec device handler
* @param[in]  cmd       IOCTL commands
* @param[in]  paramter  IOCTL commands parameter
*
* @return     0 for success, non-0 for fail
*/
/* --------------------------------------------------------------------------*/
static int vcodec_h_ioctl(int h, int cmd, int subcmd, unsigned long paramter)
{
    int r;

    if (h < 0) {
        return -1;
    }

    if (!vcodec_h_is_support_new_cmd()) {
        int old_cmd = get_old_cmd(subcmd);
        if (old_cmd == -1) {
            return -1;
        }
        return vcodec_h_control(h, old_cmd, paramter);
    }

    switch (cmd) {
    case AMSTREAM_IOC_SET:
        r = vcodec_h_ioctl_set(h, subcmd, paramter);
        break;
    case AMSTREAM_IOC_GET:
        r = vcodec_h_ioctl_get(h, subcmd, paramter);
        break;
    case AMSTREAM_IOC_GET_EX:
        r = vcodec_h_ioctl_get_ex(h, subcmd, paramter);
        break;
    default:
        r = ioctl(h, cmd, paramter);
        break;
    }

    if (r < 0) {
        CODEC_PRINT("vcodec_h_ioctl failed,handle=%d,cmd=%x,subcmd=%x, paramter=%lx, t=%x errno=%d\n", h, cmd, subcmd, paramter, r, errno);
        return r;
    }
    return 0;
}


/* --------------------------------------------------------------------------*/
/**
* @brief  vcodec_h_read  Read data from codec devices
*
* @param[in]   handle  Codec device handler
* @param[out]  buffer  Buffer for the data read from codec device
* @param[in]   size    Size of the data to be read
*
* @return      read length or fail if < 0
*/
/* --------------------------------------------------------------------------*/
static int vcodec_h_read(int handle, void *buffer, int size)
{
    int r;
    r = read(handle, buffer, size);
    if (r < 0) {
        CODEC_PRINT("read failed,handle=%d,ret=%d errno=%d\n", handle, r, errno);
    }
    return r;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  vcodec_h_write  Write data to codec devices
*
* @param[in]   handle  Codec device handler
* @param[out]  buffer  Buffer for the data to be written to codec device
* @param[in]   size    Size of the data to be written
*
* @return      write length or fail if < 0
*/
/* --------------------------------------------------------------------------*/
static int vcodec_h_write(int handle, void *buffer, int size)
{
    int r;
    r = write(handle, buffer, size);
    if (r < 0 && errno != EAGAIN) {
        CODEC_PRINT("write failed,handle=%d,ret=%d errno=%d\n", handle, r, errno);
    }
    return r;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  vcodec_change_buf_size  Change buffer size of codec device
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     Success of fail error type
*/
/* --------------------------------------------------------------------------*/
static int vcodec_change_buf_size(vcodec_para_t *pcodec)
{
    int r;

    if (pcodec->vbuf_size > 0) {
        r = vcodec_h_ioctl(pcodec->handle, AMSTREAM_IOC_SET, AMSTREAM_SET_VB_SIZE, pcodec->vbuf_size);
        if (r < 0) {
            return r;
        }
    }
    return CODEC_ERROR_NONE;
}
/* --------------------------------------------------------------------------*/
/**
* @brief  set_video_format  Set video format to codec device
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     0 for success, or error type
*/
/* --------------------------------------------------------------------------*/
static  int set_video_format(vcodec_para_t *pcodec)
{
    int format = pcodec->video_type;
    int r;

    if (format < 0 || format > VFORMAT_MAX) {
        return -CODEC_ERROR_VIDEO_TYPE_UNKNOW;
    }

    r = vcodec_h_ioctl(pcodec->handle, AMSTREAM_IOC_SET, AMSTREAM_SET_VFORMAT, format);
    if (pcodec->video_pid >= 0) {
        r = vcodec_h_ioctl(pcodec->handle, AMSTREAM_IOC_SET, AMSTREAM_SET_VID, pcodec->video_pid);
        if (r < 0) {
            return r;
        }
    }
    if (r < 0) {
        return r;
    }
    return 0;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  set_video_vcodec_info  Set video information(width, height...) to codec device
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     0 for success, or error type
*/
/* --------------------------------------------------------------------------*/
static  int set_video_codec_info(vcodec_para_t *pcodec)
{
    dec_sysinfo_t am_sysinfo = pcodec->am_sysinfo;
    int r;
    r = vcodec_h_control(pcodec->handle, AMSTREAM_IOC_SYSINFO, (unsigned long)&am_sysinfo);
    if (r < 0) {
        return r;
    }
    return 0;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  vcodec_check_new_cmd  Check new cmd for ioctl
*
*/
/* --------------------------------------------------------------------------*/
static inline void vcodec_check_new_cmd(int handle)
{
    if (!vcodec_h_is_support_new_cmd()) {
        int r;
        int version = 0;
        r = vcodec_h_control(handle, AMSTREAM_IOC_GET_VERSION, (unsigned long)&version);
        if ((r == 0) && (version >= 0x20000)) {
            CODEC_PRINT("vcodec_init amstream version : %d.%d\n", (version & 0xffff0000) >> 16, version & 0xffff);
            vcodec_h_set_support_new_cmd(1);
        } else {
            CODEC_PRINT("vcodec_init amstream use old cmd\n");
            vcodec_h_set_support_new_cmd(0);
        }

    }
}

static int vcodec_set_drmmode(vcodec_para_t *pcodec, unsigned int setval)
{
    return vcodec_h_ioctl(pcodec->handle, AMSTREAM_IOC_SET, AMSTREAM_SET_DRMMODE, setval);
}

/* --------------------------------------------------------------------------*/
/**
* @brief  vcodec_get_vbuf_state  Get the state of video buffer by codec device
*
* @param[in]   p    Pointer of codec parameter structure
* @param[out]  buf  Pointer of buffer status structure to get video buffer state
*
* @return      Success or fail type
*/
/* --------------------------------------------------------------------------*/
int vcodec_get_vbuf_state(vcodec_para_t *p, struct buf_status *buf)
{
    int r;
    if (vcodec_h_is_support_new_cmd()) {
        struct buf_status status;
        r = vcodec_h_ioctl(p->handle, AMSTREAM_IOC_GET_EX, AMSTREAM_GET_EX_VB_STATUS, (unsigned long)&status);
        memcpy(buf, &status, sizeof(*buf));
    } else {
        struct am_io_param am_io;
        r = vcodec_h_control(p->handle, AMSTREAM_IOC_VB_STATUS, (unsigned long)&am_io);
        memcpy(buf, &am_io.status, sizeof(*buf));
    }
    return r;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  vcodec_video_es_init  Initialize the codec device for es video
*
* @param[in]  pcodec  Pointer of codec parameter structure
*             sched   Chose single mode device or stream mode device
*
* @return     Success or fail error type
*/
/* --------------------------------------------------------------------------*/
static inline int vcodec_video_es_init(vcodec_para_t *pcodec, int sched)
{
    int handle;
    int r;
    int codec_r;
    int flags = O_WRONLY;
    char *amstream_dev = CODEC_VIDEO_ES_DEVICE;

    if (!pcodec->has_video) {
        return CODEC_ERROR_NONE;
    }

    flags |= pcodec->noblock ? O_NONBLOCK : 0;

    if (pcodec->video_type == VFORMAT_AV1) {
        if (sched == FRAME_MODE)
            amstream_dev = CODEC_HEVC_FRAME_DEVICE;	//av1 hevc frame mode
        else
            amstream_dev = CODEC_VIDEO_HEVC_DEVICE;
    } else if (pcodec->video_type == VFORMAT_HEVC ||
               pcodec->video_type == VFORMAT_VP9 ||
               pcodec->video_type == VFORMAT_AVS2) {
        if (sched != SINGLE_MODE)
            amstream_dev = CODEC_VIDEO_ES_HEVC_DEVICE_SCHED;
        else
            amstream_dev = CODEC_VIDEO_HEVC_DEVICE;

        if ((pcodec->dv_enable) && (pcodec->video_type == VFORMAT_HEVC))
                amstream_dev = CODEC_VIDEO_DVHEVC_DEVICE;
    } else {
        if ((pcodec->video_type == VFORMAT_H264) ||
            (pcodec->video_type == VFORMAT_MPEG12) ||
            (pcodec->video_type == VFORMAT_MPEG4) ||
            (pcodec->video_type == VFORMAT_MJPEG)) {
            if (sched != SINGLE_MODE)
                amstream_dev = CODEC_VIDEO_ES_DEVICE_SCHED;
            else
                amstream_dev = CODEC_VIDEO_ES_DEVICE;
        } else
            amstream_dev = CODEC_VIDEO_ES_DEVICE;

        if (pcodec->video_type == VFORMAT_H264 && pcodec->dv_enable)
            amstream_dev = CODEC_VIDEO_DVAVC_DEVICE;
    }
    printf("vcodec open device %s \n", amstream_dev);
    handle = vcodec_h_open(amstream_dev, flags);
    if (handle < 0) {
        return CODEC_OPEN_HANDLE_FAILED;
    }
    pcodec->handle = handle;

    vcodec_check_new_cmd(handle);

    r = set_video_format(pcodec);
    if (r < 0) {
        vcodec_h_close(handle);
        codec_r = r;
        CODEC_PRINT("%d,%s,%d\n", errno, __FUNCTION__, __LINE__);
        return codec_r;
    }
    r = set_video_codec_info(pcodec);
    if (r < 0) {
        vcodec_h_close(handle);
        codec_r = r;
        CODEC_PRINT("%d,%s,%d\n", errno, __FUNCTION__, __LINE__);
        return codec_r;
    }
    r = vcodec_set_drmmode(pcodec, pcodec->drmmode);
    if (r < 0) {
        vcodec_h_close(handle);
        codec_r = r;
        CODEC_PRINT("%d,%s,%d\n", errno, __FUNCTION__, __LINE__);
        return codec_r;
    }
    return CODEC_ERROR_NONE;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  vcodec_init  Initialize the codec device based on stream type
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     Success or fail error type
*/
/* --------------------------------------------------------------------------*/
int vcodec_init(vcodec_para_t *pcodec)
{
    int ret;

    pcodec->handle = -1;
    pcodec->cntl_handle = -1;

    switch (pcodec->stream_type) {
    case STREAM_TYPE_ES_VIDEO:
        ret = vcodec_video_es_init(pcodec, pcodec->mode);
        break;

    case STREAM_TYPE_UNKNOW:
    default:
        return -CODEC_ERROR_STREAM_TYPE_UNKNOW;
    }
    if (ret != 0) {
        return ret;
    }

    ret = vcodec_init_cntl(pcodec);
    if (ret != CODEC_ERROR_NONE) {
        return ret;
    }
    ret = vcodec_change_buf_size(pcodec);
    if (ret != 0) {
        return -CODEC_ERROR_SET_BUFSIZE_FAILED;
    }

    ret = vcodec_h_ioctl(pcodec->handle, AMSTREAM_IOC_SET, AMSTREAM_PORT_INIT, 0);
    if (ret != 0) {

        return -CODEC_ERROR_INIT_FAILED;
    }

    return ret;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  vcodec_write  Write data to codec device
*
* @param[in]  pcodec  Pointer of codec parameter structure
* @param[in]  buffer  Buffer for data to be written
* @param[in]  len     Length of the data to be written
*
* @return     Length of the written data, or fail if < 0
*/
/* --------------------------------------------------------------------------*/
int vcodec_write(vcodec_para_t *pcodec, void *buffer, int len)
{
    return vcodec_h_write(pcodec->handle, buffer, len);
}

/* --------------------------------------------------------------------------*/
/**
* @brief  vcodec_read  Read data from codec device
*
* @param[in]   pcodec  Pointer of codec parameter structure
* @param[out]  buffer  Buffer for data read from codec device
* @param[in]   len     Length of the data to be read
*
* @return      Length of the read data, or fail if < 0
*/
/* --------------------------------------------------------------------------*/
int vcodec_read(vcodec_para_t *pcodec, void *buffer, int len)
{
    return vcodec_h_read(pcodec->handle, buffer, len);
}

/* --------------------------------------------------------------------------*/
/**
* @brief  vcodec_close  Close codec device
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     0 for success, or fail type if < 0
*/
/* --------------------------------------------------------------------------*/
int vcodec_close(vcodec_para_t *pcodec)
{
    int res = 0;

    res |= vcodec_close_cntl(pcodec);
    res |= vcodec_h_close(pcodec->handle);
    return res;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  vcodec_pause  Pause all playing(A/V) by codec device
*
* @param[in]  p  Pointer of codec parameter structure
*
* @return     Success or fail error type
*/
/* --------------------------------------------------------------------------*/
int vcodec_pause(vcodec_para_t *p)
{
    int ret = CODEC_ERROR_NONE;

    if (p) {
        if (p->has_video) {
            ret = vcodec_h_control(p->cntl_handle, AMSTREAM_IOC_VPAUSE, 1);
        }
    } else {
        ret = CODEC_ERROR_PARAMETER;
    }
    return ret;
}
/* --------------------------------------------------------------------------*/
/**
* @brief  vcodec_resume  Resume playing(A/V) by codec device
*
* @param[in]  p  Pointer of codec parameter structure
*
* @return     Success or fail error type
*/
/* --------------------------------------------------------------------------*/
int vcodec_resume(vcodec_para_t *p)
{
    int ret = CODEC_ERROR_NONE;

    if (p) {
        if (p->has_video) {
            ret = vcodec_h_control(p->cntl_handle, AMSTREAM_IOC_VPAUSE, 0);
        }
    } else {
        ret = CODEC_ERROR_PARAMETER;
    }
    return ret;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  vcodec_reset  Reset codec device
*
* @param[in]  p  Pointer of codec parameter structure
*
* @return     Success or fail error type
*/
/* --------------------------------------------------------------------------*/
int vcodec_reset(vcodec_para_t *p)
{
    int ret;
    ret = vcodec_close(p);
    if (ret != 0) {
        return ret;
    }
    ret = vcodec_init(p);
    CODEC_PRINT("[%s:%d]ret=%x\n", __FUNCTION__, __LINE__, ret);
    return ret;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  vcodec_init_cntl  Initialize the video control device
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     Success or fail error type
*/
/* --------------------------------------------------------------------------*/
int vcodec_init_cntl(vcodec_para_t *pcodec)
{
    int cntl;

    cntl = vcodec_h_open(CODEC_CNTL_DEVICE, O_RDWR);
    if (cntl < 0) {
        CODEC_PRINT("get %s failed\n", CODEC_CNTL_DEVICE);
        return cntl;
    }

    pcodec->cntl_handle = cntl;

    return CODEC_ERROR_NONE;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  vcodec_close_cntl  Close video control device
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     Success or fail error type
*/
/* --------------------------------------------------------------------------*/
int vcodec_close_cntl(vcodec_para_t *pcodec)
{
    int res = CODEC_ERROR_NONE;

    if (pcodec) {
        if (pcodec->cntl_handle) {
            res = vcodec_h_close(pcodec->cntl_handle);
        }
    }
    return res;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  vcodec_set_frame_cmp_crc  write every frame crc for compare;
*
* @param[in]  pcodec Pointer of codec parameter structure, crc poll pointer and size
*
* @return     Success or fail error
*/
/* --------------------------------------------------------------------------*/
int vcodec_set_frame_cmp_crc(vcodec_para_t *vcodec, const int *crc, int size)
{
    int ret = -1;
    struct usr_crc_info_t crc_info;
    int i;

    if (vcodec == NULL)
        return -1;

    for (i = 0; i < size; i++) {
        crc_info.id = 0;    /* only for one instance */
        crc_info.pic_num = i;
        crc_info.y_crc = crc[i * 2];
        crc_info.uv_crc = crc[i * 2 + 1];
        ret = vcodec_h_control(vcodec->handle,
            AMSTREAM_IOC_SET_CRC, (unsigned long)&crc_info);
        if (ret < 0) {
            printf("set frame compare crc32 value failed, i = %d\n", i);
            return -1;
        }
    }

    return 0;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  vcodec_get_crc_check_result  get all crc cmp result
*
* @param[in]  pcodec  Pointer of codec parameter structure, Vdec ID
*
* @return     Success or fail error type, cmp result.
*/
/* --------------------------------------------------------------------------*/
int vcodec_get_crc_check_result(vcodec_para_t *vcodec, int vdec_id)
{
    int ret, result = 0x00ff;

    if (vcodec == NULL)
        return -1;

    result &= vdec_id; /*set the vdec id */
    ret = vcodec_h_control(vcodec->handle,
            AMSTREAM_IOC_GET_CRC_CMP_RESULT, (unsigned long)&result);
    //printf("get result ret = %d, result = %d\n", ret, result);
    if (ret < 0)
        return ret;

    return result;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  is_dec_cmp_ongoing  require decoding and compare crc num;
*
* @param[in]  pcodec  Pointer of codec parameter structure, Vdec ID.
*
* @return     Fail error type, or rest compare frame num;
*/
/* --------------------------------------------------------------------------*/
int is_crc_cmp_ongoing(vcodec_para_t *vcodec, int vdec_id)
{
    int ret = 0;
    int rest_num = 0xff00;

    if (vcodec == NULL)
        return -1;

    rest_num |= vdec_id;
    ret = vcodec_h_control(vcodec->handle,
            AMSTREAM_IOC_GET_CRC_CMP_RESULT, (unsigned long)&rest_num);
    if (ret < 0)
        return ret;

    return rest_num;
}

