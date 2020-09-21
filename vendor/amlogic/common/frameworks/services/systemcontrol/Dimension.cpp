/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *  @author   XiaoLiang.Wang
 *  @version  1.0
 *  @date     2016/06/13
 *  @par function description:
 *  - 1 3d set for player
 */

#define LOG_TAG "Dimension"
//#define LOG_NDEBUG 0

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>

#include "Dimension.h"

#ifndef RECOVERY_MODE
//#include <gui/SurfaceComposerClient.h> //for video 3d mode set

using namespace android;
#endif

#define ARRAY_LEN 32

Dimension::Dimension(DisplayMode *displayMode, SysWrite *sysWrite)
    :mInitDone(false),
    mLogLevel(LOG_LEVEL_DEFAULT),
    mDisplay3DFormat(0) {

    pDisplayMode = displayMode;
    pSysWrite = sysWrite;
    pTxAuth = displayMode->geTxAuth();

    strcpy(mMode3d, VIDEO_3D_OFF);
}

Dimension::~Dimension() {
}

void Dimension::setLogLevel(int level) {
    mLogLevel = level;
}

//----3d set for mbox----------------------------------------
int32_t Dimension::getLineTotal(char* buf) {
    int num = 0;
    while (true) {
        char *ptr = strchr(buf, '\n');
        if (ptr != NULL) {
            int offset = (int)(ptr - buf);
            /*if (offset < 1) {
                ALOGE("[getLineTotal]offset error.\n");
                break;
            }*/
            buf += (offset + 1);
            num++;

            if (num > NUM_MAX) {// max line should less than 64
                ALOGE("[getLineTotal]num error.\n");
                break;
            }
        }
        else { //last line
            if (strlen(buf) > 0) {
                num++;
            }
            break;
        }
    }
    return num;
}

void Dimension::getLine(int idx, int total, char* src, char* dst) {
    char *pBuf = src;
    for (int i = 0; i < total; i++) {
        char *ptr = strchr(pBuf, '\n');
        if (ptr != NULL) {
            int offset = (int)(ptr - pBuf);
            //ALOGE("[getLine]i:%d, idx:%d, ptr:%x, pBuf:%x\n", i, idx, ptr, pBuf);
            if (i == idx) {
                strncpy(dst, pBuf, offset);
                break;
            }
            pBuf += (offset + 1);
        }
        else { //last line
            if (strlen(pBuf) > 0) {
                strcpy(dst, pBuf);
            }
            break;
        }
    }
    ALOGI("[getLine] dst:%s\n", dst);
}

void Dimension::parseLine(int idx, char *line) {
    ALOGI("[parseLine] idx:%d, line:%s\n", idx, line);
    if (!strcmp(line, "3D support lists:")) {
        return;// skip comment
    }

    if (line != NULL && strlen(line) <= 0) {
        return;// skip no data
    }

    char *ptr = strchr(line, ' ');
    if (ptr != NULL) {
        int offset = (int)(ptr - line);
        strncpy(mSupport.info[idx].mode, line, offset);// store mode
        line += (offset + 1);
        strcpy(mSupport.info[idx].list, line);//store support list

        //parse mode and frequency for search the best display mode
        ptr = strchr(mSupport.info[idx].mode, 'p');
        if (ptr != NULL) {
            offset = (int)(ptr - mSupport.info[idx].mode);
            if (offset < ARRAY_LEN) {
                char mostr[ARRAY_LEN] = {0};
                strncpy(mostr, mSupport.info[idx].mode, offset);//store mode, such as 480, 576, 720, 1080, 2160...
                mSupport.info[idx].mo = atoi(mostr);
                mSupport.info[idx].hz = atoi(ptr + 1);//store frequency, such as 24, 25, 30, 50, 60...
            }
        }
        else {
            //do nothing, skip interlace and smpte(4k) mode
        }
    }
}

void Dimension::init() {
    ALOGI("[init] mInitDone:%d\n", mInitDone);
    char line[LINE_LEN] = {0};
    if (!mInitDone) {
        mInitDone = true;

        //store current display mode
        memset(mSupport.stored, 0, sizeof(mSupport.stored));
        pSysWrite->readSysfs(SYSFS_DISPLAY_MODE, mSupport.stored);

        //store current window axis
        char axis[MAX_STR_LEN] = {0};
        pSysWrite->readSysfsOriginal(DISPLAY_FB0_WINDOW_AXIS, axis);
        if (sscanf(axis, "%d %d %d %d", &mWindowAxis.stored.x0, &mWindowAxis.stored.y0, &mWindowAxis.stored.x1, &mWindowAxis.stored.y1) != 4) {
            ALOGE("[init]mWindowAxis.stored fail.\n");
            return;
        }

        //read disp_cap_3d
        char disp_cap_3d[MAX_STR_LEN] = {0};
        pSysWrite->readSysfsOriginal(DISPLAY_HDMI_DISP_CAP_3D, disp_cap_3d);

        //parse line
        mSupport.total = getLineTotal(disp_cap_3d);
        for (int i = 0; i < mSupport.total; i++) {
            memset(line, 0, LINE_LEN);
            memset(mSupport.info[i].mode, 0, sizeof(mSupport.info[i].mode));
            memset(mSupport.info[i].list, 0, sizeof(mSupport.info[i].list));
            mSupport.info[i].mo = 0;
            mSupport.info[i].hz = 0;

            getLine(i, mSupport.total, disp_cap_3d, line);
            parseLine(i, line);
        }

        // for debug
        for (int i = 0; i < mSupport.total; i++) {
            ALOGI("[init]mSupport.info[%d].mode:%s, mSupport.info[%d].list:%s\n", i, mSupport.info[i].mode, i, mSupport.info[i].list);
            ALOGI("[init]mSupport.info[%d].mo:%d, mSupport.info[%d].hz:%d\n", i, mSupport.info[i].mo, i, mSupport.info[i].hz);
        }
    }
}

void Dimension::setWindowAxis(const char* mode3d) {
    if (strcmp(mode3d, VIDEO_3D_OFF)) {
        if (!strcmp(mode3d, VIDEO_3D_FRAME_PACKING)) {
            if (strstr(mSupport.dst, "720") != NULL) {
                mWindowAxis.dst.x0 = 0;
                mWindowAxis.dst.y0 = 0;
                mWindowAxis.dst.x1 = 1280 - 1;
                mWindowAxis.dst.y1 = 720 + 750 - 1;
            }
            else if (strstr(mSupport.dst, "1080") != NULL) {
                mWindowAxis.dst.x0 = 0;
                mWindowAxis.dst.y0 = 0;
                mWindowAxis.dst.x1 = 1920 - 1;
                mWindowAxis.dst.y1 = 1080 + 1125 - 1;
            }
        }
        else {
            if (strstr(mSupport.dst, "720") != NULL) {
                mWindowAxis.dst.x0 = 0;
                mWindowAxis.dst.y0 = 0;
                mWindowAxis.dst.x1 = 1280 - 1;
                mWindowAxis.dst.y1 = 720 - 1;
            }
            else if (strstr(mSupport.dst, "1080") != NULL) {
                mWindowAxis.dst.x0 = 0;
                mWindowAxis.dst.y0 = 0;
                mWindowAxis.dst.x1 = 1920 - 1;
                mWindowAxis.dst.y1 = 1080 - 1;
            }
        }
    }
    else {
        mWindowAxis.dst.x0 = mWindowAxis.stored.x0;
        mWindowAxis.dst.y0 = mWindowAxis.stored.y0;
        mWindowAxis.dst.x1 = mWindowAxis.stored.x1;
        mWindowAxis.dst.y1 = mWindowAxis.stored.y1;
    }

    char axis[MAX_STR_LEN] = {0};
    sprintf(axis, "%d %d %d %d",
        mWindowAxis.dst.x0,
        mWindowAxis.dst.y0,
        mWindowAxis.dst.x1,
        mWindowAxis.dst.y1);
    pSysWrite->writeSysfs(DISPLAY_FB0_WINDOW_AXIS, axis);
    usleep(50 * 1000);
    pSysWrite->writeSysfs(DISPLAY_FB0_FREESCALE, "0x0");
    usleep(50 * 1000);
    pSysWrite->writeSysfs(DISPLAY_FB0_FREESCALE, "0x10001");
}

bool Dimension::isSupportFramePacking(const char* mode3d) {
    char keystr[32] = {0};
    if (!strcmp(mode3d, VIDEO_3D_FRAME_PACKING)) {
        strcpy(keystr, KEY_FRAME_PACKING);
        for (int j = 0; j < mSupport.total; j++) {
            if (strstr(mSupport.info[j].list, keystr) != NULL) {
                return true;
            }
        }
    }
    return false;
}

void Dimension::setDispMode(const char* mode3d) {
    char mode[MODE_LEN] = {0};
    char keystr[32] = {0};

    if (strcmp(mode3d, VIDEO_3D_OFF)) {
        memset(mSupport.dst, 0, sizeof(mSupport.dst));
        if (!strcmp(mode3d, VIDEO_3D_SIDE_BY_SIDE)) {
            strcpy(keystr, KEY_SIDE_BY_SIDE);
        }
        else if (!strcmp(mode3d, VIDEO_3D_TOP_BOTTOM)) {
            strcpy(keystr, KEY_TOP_BOTTOM);
        }
        else if (!strcmp(mode3d, VIDEO_3D_FRAME_PACKING)) {
            strcpy(keystr, KEY_FRAME_PACKING);
        }

        if (!strcmp(keystr, KEY_FRAME_PACKING) ||
            (strstr(mSupport.stored, "2160p") != NULL || strstr(mSupport.stored, "smpte") != NULL)) {
            int mo = 0;
            int hz = 0;
            int idx = -1;
            char mostr[8] = {0};
            char hzstr[8] = {0};

            //searching the best display mode to switch
            for (int j = 0; j < mSupport.total; j++) {
                if (strstr(mSupport.info[j].list, keystr) != NULL) {
                    if (!strcmp(mSupport.info[j].mode, mSupport.stored)) {
                        mo = mSupport.info[j].mo;
                        hz = mSupport.info[j].hz;
                        idx = j;
                        break;
                    }

                    //ALOGI("[setDispMode] mSupport.info[%d].mo:%d, mo:%d, mSupport.info[%d].hz:%d, hz:%d\n", j, mSupport.info[j].mo, mo, j, mSupport.info[j].hz, hz);
                    if (mSupport.info[j].mo > mo) {
                        mo = mSupport.info[j].mo;
                        hz = mSupport.info[j].hz;
                        idx = j;
                    }
                    else if (mSupport.info[j].mo == mo) {
                        if (mSupport.info[j].hz > hz) {
                            hz = mSupport.info[j].hz;
                            idx = j;
                        }
                    }
                }
            }

            sprintf(mostr, "%d", mo);
            sprintf(hzstr, "%d", hz);
            strcat(mSupport.dst, mostr);
            if (!strcmp(keystr, KEY_FRAME_PACKING)) {
                strcat(mSupport.dst, "fp");
            }
            else {
                strcat(mSupport.dst, "p");
            }
            strcat(mSupport.dst, hzstr);
            strcat(mSupport.dst, "hz");

        }
        else {
            //check current display mode is support mode for 3d that will switch to
            for (int i = 0; i < mSupport.total; i++) {
                if (!strcmp(mSupport.info[i].mode, mSupport.stored)) {
                    if (strstr(mSupport.info[i].list, keystr) != NULL) {
                        //no need to switch mode
                        strcpy(mSupport.dst, mSupport.stored);
                    }
                    else {
                        //3d mode is not support
                        ALOGE("[setDispMode] keystr:%s is not support for %s\n", keystr, mSupport.stored);
                    }
                    break;
                }
            }
        }

        //set display mode if stored is different with destination mode
        //if (strcmp(mSupport.dst, mSupport.stored)) {
            strcpy(mode, mSupport.dst);
        //}
    }
    else {
        if (strlen(mSupport.stored) != 0) {
            strcpy(mode, mSupport.stored);

            //make sure getting support info and store current for every set from off mode
            mInitDone = false;
        }
    }

    ALOGI("[setDispMode]setMboxOutputMode mode:%s\n", mode);
    char mode_tmp[MODE_LEN] = {0};
    pSysWrite->readSysfs(SYSFS_DISPLAY_MODE, mode_tmp);
    if (strlen(mode) > 0 &&
        strcmp(mode, "0fp0hz") &&
        strcmp(mode, mode_tmp)) {//skip 0fp0hz and same mode
        pDisplayMode->setSourceOutputMode(mode);
    }
}

void Dimension::mode3DImpl(const char* mode3d) {
    int format = SURFACE_3D_OFF;
    if (!strcmp(mode3d, VIDEO_3D_OFF)) {
        format = SURFACE_3D_OFF;
    }
    else if (!strcmp(mode3d, VIDEO_3D_SIDE_BY_SIDE)) {
        format = SURFACE_3D_SIDE_BY_SIDE;
    }
    else if (!strcmp(mode3d, VIDEO_3D_TOP_BOTTOM)) {
        format = SURFACE_3D_TOP_BOTTOM;
    }
    else if (!strcmp(mode3d, VIDEO_3D_FRAME_PACKING)) {
        format = SURFACE_3D_FRAME_PACKING;
    }

    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("[mode3DImpl]mode3d = %s, format = %d\n", mode3d, format);
    }

#if 0//ndef RECOVERY_MODE
    SurfaceComposerClient::openGlobalTransaction();
    SurfaceComposerClient::setDisplay2Stereoscopic(0, format);
    SurfaceComposerClient::closeGlobalTransaction();
#endif

    pSysWrite->writeSysfs(AV_HDMI_CONFIG, mode3d);
}

int32_t Dimension::set3DMode(const char* mode3d) {
    char is3DSupport[8] = {0}; //"1" means tv support 3d

    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("set 3d mode :%s", String8(mode3d).string());
    }

    pSysWrite->readSysfs(AV_HDMI_3D_SUPPORT, is3DSupport);
    if (strcmp(is3DSupport, "1")) {
        ALOGE("[set3DMode]3d is not support.\n");
        return -1;
    }

    if (!strcmp(mMode3d, mode3d)) {
        ALOGE("[set3DMode]mMode3d equals to mode3d:%s\n", mode3d);
        return 0;
    }

    //init support list
    init();
    if (!strcmp(mode3d, VIDEO_3D_FRAME_PACKING) && !isSupportFramePacking(mode3d)) {
        ALOGE("[set3DMode]FramePacking is not support.\n");
        return -1;
    }
    pSysWrite->writeSysfs(DISPLAY_HDMI_AVMUTE, "1");
    usleep(100 * 1000);
    pTxAuth->stopVerAll();
    pSysWrite->writeSysfs(DISPLAY_HDMI_HDCP_MODE, "-1"); // "-1" means stop hdcp 14/22
    usleep(100 * 1000);
    pSysWrite->writeSysfs(DISPLAY_HDMI_PHY, "0"); // Turn off TMDS PHY

    //1. set display_mode null
    pSysWrite->writeSysfs(SYSFS_DISPLAY_MODE, "null");

    //2. 3D mode implement
    strcpy(mMode3d, mode3d);
    mode3DImpl(mode3d);
    usleep(100 * 1000);

    //3. check display mode and set
    setDispMode(mode3d);

    //4. check window axis and set
    setWindowAxis(mode3d);

    usleep(100 * 1000);
    pSysWrite->writeSysfs(DISPLAY_HDMI_PHY, "1"); // Turn on TMDS PHY
    usleep(100 * 1000);
    pSysWrite->writeSysfs(DISPLAY_HDMI_AVMUTE, "-1");
    pTxAuth->stop();
    pTxAuth->start();
    return 0;
}

//----3d set for tv----------------------------------------
void Dimension::init3DSetting(void) {
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("init3DSetting\n");
    }

    setDisplay3DFormat(FORMAT_3D_OFF);//for mbox
    setDisplay3DTo2DFormat(FORMAT_3D_OFF);//for tv
}

int32_t Dimension::getVideo3DFormat(void) {
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("getVideo3DFormat\n");
    }

    int32_t format = -1;
    int32_t vpp3Dformat = -1;
    int video_fd = open(VIDEO_PATH, O_RDWR);
    if (video_fd < 0) {
        return -1;
    }
    ioctl(video_fd, AMSTREAM_IOC_GET_SOURCE_VIDEO_3D_TYPE, &vpp3Dformat);
    format = get3DFormatByVpp(vpp3Dformat);
    close(video_fd);

    return format;
}

int32_t Dimension::getDisplay3DTo2DFormat(void) {
    int32_t format = -1;
    unsigned int operation = 0;
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("getDisplay3DTo2DFormat\n");
    }

    int video_fd = open(VIDEO_PATH, O_RDWR);
    if (video_fd < 0) {
        return -1;
    }
    ioctl(video_fd, AMSTREAM_IOC_GET_3D_TYPE, &operation);
    close(video_fd);
    format = get3DFormatByOperation(operation);

    return format;
}

bool Dimension::setDisplay3DTo2DFormat(int format) {
    bool ret = false;
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("setDisplay3DTo2DFormat format:%d\n", format);
    }

    //add for xiaomi customize 20160523
    if (format == FORMAT_3D_AUTO) {
        format = FORMAT_3D_TO_2D_LEFT_EYE;
    }
    else if (format == FORMAT_3D_SIDE_BY_SIDE) {
        format = FORMAT_3D_SIDE_BY_SIDE_FORCE;
    }
    else if (format == FORMAT_3D_TOP_AND_BOTTOM || format == FORMAT_3D_FRAME_ALTERNATIVE) {
        format = FORMAT_3D_TOP_AND_BOTTOM_FORCE;
    }

    //setDiBypassAll(format);
    int video_fd = open(VIDEO_PATH, O_RDWR);
    if (video_fd < 0) {
        ALOGE("setDisplay3DTo2DFormat video_fd:%d\n", video_fd);
        return false;
    }
    unsigned int operation = get3DOperationByFormat(format);
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("setDisplay3DTo2DFormat operation:%d\n", operation);
    }
    if (ioctl(video_fd, AMSTREAM_IOC_SET_3D_TYPE, operation) == 0) {
        ret = true;
    }
    close(video_fd);

    return ret;
}

bool Dimension::setDisplay3DFormat(int format) {
    bool ret = false;
    ///String16 di3DformatStr;
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("setDisplay3DFormat format:%d\n", format);
    }

    if (format == FORMAT_3D_AUTO) {
        format = getVideo3DFormat();
    }

    mDisplay3DFormat = format;
    char format3DStr[64] = {0};
    get3DFormatStr(format, format3DStr);
    if (set3DMode(format3DStr) == 0) {
        ret = true;
    }

    return ret;
}

int32_t Dimension::getDisplay3DFormat(void) {
    return mDisplay3DFormat;
}

bool Dimension::setOsd3DFormat(int format) {
    bool ret = false;
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("setOsd3DFormat format:%d\n", format);
    }

    // TODO: needn't implement right now

    return ret;
}

bool Dimension::switch3DTo2D(int format) {
    bool ret = false;
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("switch3DTo2D format:%d\n", format);
    }

    //setDiBypassAll(format);
    int video_fd = open(VIDEO_PATH, O_RDWR);
    if (video_fd < 0) {
        return false;
    }
    unsigned int operation = get3DOperationByFormat(format);
    if (ioctl(video_fd, AMSTREAM_IOC_SET_3D_TYPE, operation) == 0) {
        ret = true;
    }
    close(video_fd);

    return ret;
}

bool Dimension::switch2DTo3D(int format) {
    bool ret = false;
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("switch2DTo3D format:%d\n", format);
    }

    // TODO: implement later

    return ret;
}

void* Dimension::detect3DThread(void* data) {
    Dimension *pThiz = (Dimension*)data;
    int retry = RETRY_MAX;
    int di3Dformat[5] = {0};
    int times[5] = {0};
   // String16 di3DformatStr;
    char format3DStr[64] = {0};

    while (retry > 0) {
        retry--;
        di3Dformat[retry] = pThiz->getVideo3DFormat();
        //ALOGI("[detect3DThread]di3Dformat[%d]:%d\n", retry, di3Dformat[retry]);
        usleep(200000);//200ms
    }

    //get the 3d format which was detected most times
    for (int i = 0; i < RETRY_MAX - 1; i++) {
        for (int j = i + 1; j < RETRY_MAX; j++) {
            if (di3Dformat[i] == di3Dformat[j]) {
                times[i]++;
            }
        }
    }
    int max = times[0];
    int idx = 0;
    for (int i = 0; i < RETRY_MAX - 1; i++) {
        if (times[i] > max) {
            max = times[i];
            idx = i;
        }
    }
    //can't detect 3d format correctly, 3d off
    if (max == 1) {
        idx = 0;
        di3Dformat[0] = 0;
    }

    if (pThiz->mLogLevel > LOG_LEVEL_1) {
        ALOGI("[detect3DThread]after queue di3Dformat[%d]:%d\n", idx, di3Dformat[idx]);
    }

    pThiz->mDisplay3DFormat = di3Dformat[idx];
    pThiz->get3DFormatStr(di3Dformat[idx], format3DStr);
    pThiz->set3DMode(format3DStr);
    return NULL;
}

void Dimension::autoDetect3DForMbox() {
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("autoDetect3DForMbox\n");
    }

    pthread_t id;
    int ret = pthread_create(&id, NULL, detect3DThread, this);
    if (ret != 0) {
        ALOGE("[autoDetect3DForMbox:%d]ERROR; pthread_create failed rc=%d\n",__LINE__, ret);
    }
}

void Dimension::setDiBypassAll(int format) {
    if (FORMAT_3D_OFF == format) {
        pSysWrite->writeSysfs(DI_BYPASS_POST, "0");
    }
    else {
        pSysWrite->writeSysfs(DI_BYPASS_POST, "1");
    }
}

unsigned int Dimension::get3DOperationByFormat(int format) {
    unsigned int operation = MODE_3D_DISABLE;

    switch (format) {
        case FORMAT_3D_OFF:
            operation = MODE_3D_DISABLE;
            break;
        case FORMAT_3D_AUTO:
            operation = MODE_3D_AUTO;
            break;
        case FORMAT_3D_SIDE_BY_SIDE:
            operation = MODE_3D_LR;
            break;
        case FORMAT_3D_TOP_AND_BOTTOM:
            operation = MODE_3D_TB;
            break;
        case FORMAT_3D_LINE_ALTERNATIVE:
            operation = MODE_3D_LA;
            break;
        case FORMAT_3D_FRAME_ALTERNATIVE:
            operation = MODE_3D_FA;
            break;
        case FORMAT_3D_TO_2D_LEFT_EYE:
            operation = MODE_3D_TO_2D_L;
            break;
        case FORMAT_3D_TO_2D_RIGHT_EYE:
            operation = MODE_3D_TO_2D_R;
            break;
        case FORMAT_3D_SIDE_BY_SIDE_FORCE:
            operation = MODE_FORCE_3D_TO_2D_LR;
            break;
        case FORMAT_3D_TOP_AND_BOTTOM_FORCE:
            operation = MODE_FORCE_3D_TO_2D_TB;
            break;
        default:
            operation = MODE_3D_DISABLE;
            break;
    }

    return operation;
}

int Dimension::get3DFormatByOperation(unsigned int operation) {
    int format = -1;

    switch (operation) {
        case MODE_3D_DISABLE:
            format = FORMAT_3D_OFF;
            break;
        case MODE_3D_AUTO:
            format = FORMAT_3D_AUTO;
            break;
        case MODE_3D_LR:
            format = FORMAT_3D_SIDE_BY_SIDE;
            break;
        case MODE_3D_TB:
            format = FORMAT_3D_TOP_AND_BOTTOM;
            break;
        case MODE_3D_LA:
            format = FORMAT_3D_LINE_ALTERNATIVE;
            break;
        case MODE_3D_FA:
            format = FORMAT_3D_FRAME_ALTERNATIVE;
            break;
        case MODE_3D_TO_2D_L:
            format = FORMAT_3D_TO_2D_LEFT_EYE;
            break;
        case MODE_3D_TO_2D_R:
            format = FORMAT_3D_TO_2D_RIGHT_EYE;
            break;
        case MODE_FORCE_3D_TO_2D_LR:
            format = FORMAT_3D_SIDE_BY_SIDE_FORCE;
            break;
        case MODE_FORCE_3D_TO_2D_TB:
            format = FORMAT_3D_TOP_AND_BOTTOM_FORCE;
            break;
        default:
            format = FORMAT_3D_OFF;
            break;
    }

    return format;
}

int Dimension::get3DFormatByVpp(int vpp3Dformat) {
    int format = -1;

    switch (vpp3Dformat) {
        case VPP_3D_MODE_NULL:
            format = FORMAT_3D_OFF;
            break;
        case VPP_3D_MODE_LR:
            format = FORMAT_3D_SIDE_BY_SIDE;
            break;
        case VPP_3D_MODE_TB:
            format = FORMAT_3D_TOP_AND_BOTTOM;
            break;
        case VPP_3D_MODE_LA:
            format = FORMAT_3D_LINE_ALTERNATIVE;
            break;
        case VPP_3D_MODE_FA:
            format = FORMAT_3D_FRAME_ALTERNATIVE;
            break;
        default:
            format = FORMAT_3D_OFF;
            break;
    }

    return format;
}

void Dimension::get3DFormatStr(int format, char *str) {
    if (NULL == str) {
        return;
    }

    switch (format) {
        case FORMAT_3D_AUTO:
            strcpy(str, "3dauto");
            break;
        case FORMAT_3D_SIDE_BY_SIDE:
            strcpy(str, "3dlr");
            break;
        case FORMAT_3D_TOP_AND_BOTTOM:
            strcpy(str, "3dtb");
            break;
        case FORMAT_3D_OFF:
        case FORMAT_3D_LINE_ALTERNATIVE:
        case FORMAT_3D_FRAME_ALTERNATIVE:
        default:
            strcpy(str, "3doff");
            break;
    }
}

int Dimension::dump(char *result) {
    if (NULL == result)
        return -1;

    char buf[2048] = {0};
    char format3DStr[64] = {0};
    get3DFormatStr(getDisplay3DFormat(), format3DStr);
    sprintf(buf, "\n display 3d format: %s , display 3d to 2d format:%d\n", format3DStr, getDisplay3DTo2DFormat());
    strcat(result, buf);
    return 0;
}
