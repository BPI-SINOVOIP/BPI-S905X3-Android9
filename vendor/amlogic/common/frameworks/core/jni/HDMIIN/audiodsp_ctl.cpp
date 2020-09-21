/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC audiodsp_ctl
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <errno.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <android/log.h>

#include "audio_global_cfg.h"
#include "audiodsp_control.h"

//#define LOG_TAG "Libaudiodsp"
//#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
//#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#undef LOGD
#undef LOGE
#define LOGD printf
#define LOGE printf
struct firmware_s {
    int id;
    int support_fmt;
    char name[64];
};

static struct firmware_s firmware_list[] = {
        { 0, MCODEC_FMT_MPEG123, "audiodsp_codec_mp3_lp.bin" },
        { 1, MCODEC_FMT_AAC, "audiodsp_codec_aac.bin" },
        { 2, MCODEC_FMT_AC3, "audiodsp_codec_ac3.bin" },
        { 3, MCODEC_FMT_DTS, "audiodsp_codec_dca.bin" },
        { 4, MCODEC_FMT_FLAC, "audiodsp_codec_flac.bin" },
        { 5, MCODEC_FMT_COOK, "audiodsp_codec_cook.bin" },
        { 6, MCODEC_FMT_AMR, "audiodsp_codec_amr.bin" },
        { 7, MCODEC_FMT_RAAC, "audiodsp_codec_raac.bin" },
        { 8, MCODEC_FMT_ADPCM, "audiodsp_codec_adpcm.bin" },
        { 9, MCODEC_FMT_WMA, "audiodsp_codec_wma.bin" },
        { 10, MCODEC_FMT_PCM, "audiodsp_codec_pcm.bin" },
        { 11, MCODEC_FMT_WMAPRO, "audiodsp_codec_wmapro.bin" },
        { 17, MCODEC_FMT_NULL, "audiodsp_codec_null.bin" },
};

#define DSP_DEV_NOD "/dev/audiodsp0"
static int dsp_file_fd = -1;

static int register_firmware(int fd, int fmt, char *name) {
#if CC_DISABLE_DSP_MODULE == 0
    struct audiodsp_cmd cmd;

    cmd.cmd = AUDIODSP_REGISTER_FIRMWARE;
    cmd.fmt = fmt;
    cmd.data = name;
    cmd.data_len = strlen(name);

    return ioctl(fd, AUDIODSP_REGISTER_FIRMWARE, &cmd);
#else
    return 0;
#endif
}

static struct firmware_s * find_firmware_by_fmt(int m_fmt) {
#if CC_DISABLE_DSP_MODULE == 0
    unsigned int i;
    struct firmware_s *f;

    for (i = 0; i < sizeof(firmware_list) / sizeof(struct firmware_s); i++) {
        f = &firmware_list[i];
        if (f->support_fmt & m_fmt)
            return f;
    }

    return NULL;
#else
    return NULL;
#endif
}

static int audiodsp_init(void) {
#if CC_DISABLE_DSP_MODULE == 0
    unsigned i;
    int ret;
    struct firmware_s *f;

    if (dsp_file_fd < 0)
        dsp_file_fd = open(DSP_DEV_NOD, O_RDONLY, 0644);

    if (dsp_file_fd < 0) {
        LOGE( "unable to open audio dsp  %s,err: %s",DSP_DEV_NOD, strerror(errno));
        return -1;
    }

    ioctl(dsp_file_fd, AUDIODSP_UNREGISTER_ALLFIRMWARE, 0);
    for (i = 0; i < sizeof(firmware_list) / sizeof(struct firmware_s); i++) {
        f = &firmware_list[i];
        ret = register_firmware(dsp_file_fd, f->support_fmt, f->name);
        if (ret != 0) {
            LOGE("register firmware error=%d,fmt:%d,name:%s\n",ret,f->support_fmt,f->name);
        }
    }

    if (ret != 0) {
        close(dsp_file_fd);
        dsp_file_fd = -1;
    }

    return ret;
#else
    return 0;
#endif
}

int audiodsp_start(void) {
#if CC_DISABLE_DSP_MODULE == 0
    int m_fmt;
    int ret = -1;
    int eret = -1;
    unsigned long val;

    audiodsp_stop();

    ret = audiodsp_init();
    if (ret) {
        LOGE("audiodsp init failed,err num %d\n",ret);
        goto exit;
    }

    //m_fmt=switch_audiodsp(feeder->format);
    m_fmt = MCODEC_FMT_NULL;
    LOGD("[%s:%d]audio_fmt=%d\n", __FUNCTION__, __LINE__, m_fmt);
    if (find_firmware_by_fmt(m_fmt) == NULL){
    	ret = -2;
        goto exit;
    }
    ioctl(dsp_file_fd, AUDIODSP_SET_FMT, m_fmt);

    ret = ioctl(dsp_file_fd, AUDIODSP_START, 0);
    if (ret != 0){
        ret= -3;
        goto exit;
    }

    ret = -1;
    while (ret != 0 && dsp_file_fd >= 0)
        ret = ioctl(dsp_file_fd, AUDIODSP_DECODE_START, 0);

    ret = -1;
    while(ret!=0 && dsp_file_fd>=0){        
 	ret=ioctl(dsp_file_fd,AUDIODSP_WAIT_FORMAT,0);
 	usleep(100000);
    }
  exit:               
   LOGD("audiodsp_start exit ret %d\n",ret);
  //sleep some time for dsp start
    usleep( 1000 * 200);
    return ret;
#else
    return 0;
#endif
}

int audiodsp_stop(void) {
#if CC_DISABLE_DSP_MODULE == 0
    if (dsp_file_fd >= 0) {
        ioctl(dsp_file_fd, AUDIODSP_STOP, 0);
        close(dsp_file_fd);
        dsp_file_fd = -1;

        return 0;
    }

    return -1;
#else
    return 0;
#endif
}
