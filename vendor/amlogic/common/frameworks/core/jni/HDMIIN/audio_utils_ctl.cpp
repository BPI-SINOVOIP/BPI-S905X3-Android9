/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC AUDIO_UTILS_CTL
 */

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <unistd.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <android/log.h>

#include "audio_global_cfg.h"
#include "audio_utils_ctl.h"

#define AUDIO_UTILS     "/dev/amaudio_utils"

//#define LOG_TAG "LibAudioCtl"
//#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
//#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#undef LOGD
#undef LOGE
#define LOGD printf
#define LOGE printf
static int mAudioUtils_fd = -1;

static int OpenAudioUtils(void);
static int CloseAudioUtils(void);

static int OpenAudioUtils(void) {
#if CC_DISABLE_UTILS_MODULE == 0
    mAudioUtils_fd = open(AUDIO_UTILS, O_RDWR);
    if (mAudioUtils_fd < 0) {
        LOGE("Open AUDIO_EFFECT failed!\n");
        return -1;
    }

    return 0;
#else
    return 0;
#endif
}

static int CloseAudioUtils(void) {
#if CC_DISABLE_UTILS_MODULE == 0
    if (mAudioUtils_fd >= 0) {
        close(mAudioUtils_fd);
        mAudioUtils_fd = -1;
        LOGD("Close AUDIO_EFFECT.\n");
        return 0;
    }

    return -1;
#else
    return 0;
#endif
}

int AudioUtilsInit(void) {
#if CC_DISABLE_UTILS_MODULE == 0
    if (OpenAudioUtils() < 0)
        return -1;
    return 0;
#else
    return 0;
#endif
}

int AudioUtilsUninit(void) {
#if CC_DISABLE_UTILS_MODULE == 0
    CloseAudioUtils();
    return 0;
#else
    return 0;
#endif
}

int AudioUtilsStartLineIn(void) {
#if CC_DISABLE_UTILS_MODULE == 0
    if (mAudioUtils_fd >= 0) {
        return ioctl(mAudioUtils_fd, AMAUDIO_IOC_START_LINE_IN, 0);
    }

    LOGE("Please Open Audio Utils first!\n");
    return mAudioUtils_fd;
#else
    return 0;
#endif
}

int AudioUtilsStartHDMIIn(void) {
#if CC_DISABLE_UTILS_MODULE == 0
    if (mAudioUtils_fd >= 0) {
        return ioctl(mAudioUtils_fd, AMAUDIO_IOC_START_HDMI_IN, 0);
    }

    LOGE("Please Open Audio Utils first!\n");
    return mAudioUtils_fd;
#else
    return 0;
#endif
}

int AudioUtilsStopLineIn(void) {
#if CC_DISABLE_UTILS_MODULE == 0
    if (mAudioUtils_fd >= 0) {
        return ioctl(mAudioUtils_fd, AMAUDIO_IOC_STOP_LINE_IN, 0);
    }

    LOGE("Please Open Audio Utils first!\n");
    return mAudioUtils_fd;
#else
    return 0;
#endif
}

int AudioUtilsStopHDMIIn(void) {
#if CC_DISABLE_UTILS_MODULE == 0
    if (mAudioUtils_fd >= 0) {
        return ioctl(mAudioUtils_fd, AMAUDIO_IOC_STOP_HDMI_IN, 0);
    }

    LOGE("Please Open Audio Utils first!\n");
    return mAudioUtils_fd;
#else
    return 0;
#endif
}
