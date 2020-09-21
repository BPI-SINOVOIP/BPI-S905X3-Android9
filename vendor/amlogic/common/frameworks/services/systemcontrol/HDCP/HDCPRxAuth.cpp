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
 *  @author   Tellen Yu
 *  @version  1.0
 *  @date     2016/09/06
 *  @par function description:
 *  - 1 process HDCP RX authenticate
 */

#define LOG_TAG "SystemControl"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <utils/Log.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "HDCPRxAuth.h"
#include "UEventObserver.h"
#include "HDCPRx22ImgKey.h"
#include "HDCPRxKey.h"
#include "../DisplayMode.h"

static char HdmiRxPlugEvent[128] = {0};
static char HdmiRxAuthEvent[128] = {0};

static void GetHdmiRxEventPath() {
    char tmpHdmiRxPlugEvent[128] = {0};
    char tmpHdmiRxAuthEvent[128] = {0};
    char tmpHdmiRxPlugEvent1[128] = {0};
    char tmpHdmiRxAuthEvent1[128] = {0};
    readlink(HDMI_RX_PLUG_PATH, tmpHdmiRxPlugEvent, 128);
    readlink(HDMI_RX_AUTH_PATH, tmpHdmiRxAuthEvent, 128);

    //The first 5 characters need to be discarded, event path don't need;
    strncpy(tmpHdmiRxPlugEvent1, tmpHdmiRxPlugEvent + 5, sizeof(tmpHdmiRxPlugEvent) - 5);
    strncpy(tmpHdmiRxAuthEvent1, tmpHdmiRxAuthEvent + 5, sizeof(tmpHdmiRxAuthEvent) - 5);

    snprintf(HdmiRxPlugEvent, sizeof(tmpHdmiRxPlugEvent1), "DEVPATH=%s", tmpHdmiRxPlugEvent1);
    snprintf(HdmiRxAuthEvent, sizeof(tmpHdmiRxAuthEvent1), "DEVPATH=%s", tmpHdmiRxAuthEvent1);
    SYS_LOGD("mHdmiRxPlugEvent = %s\n", HdmiRxPlugEvent);
    SYS_LOGD("mHdmiRxAuthEvent = %s\n", HdmiRxAuthEvent);

}

HDCPRxAuth::HDCPRxAuth(HDCPTxAuth *txAuth) :
    pTxAuth(txAuth) {

    initKey();
    GetHdmiRxEventPath();

    pthread_t id;
    int ret = pthread_create(&id, NULL, RxUenventThreadLoop, this);
    if (ret != 0) {
        SYS_LOGE("HDCP RX, Create RxUenventThreadLoop error!\n");
    }
}

HDCPRxAuth::~HDCPRxAuth() {

}

void HDCPRxAuth::initKey() {
    /*
    //init HDCP 1.4 key
    HDCPRxKey hdcpRx14(HDCP_RX_14_KEY);
    hdcpRx14.refresh();

    //init HDCP 2.2 key
    HDCPRxKey hdcpRx22(HDCP_RX_22_KEY);
    hdcpRx22.refresh();
    */

#ifdef HDCP_RX_KEY_PC_TOOL
    if (access(HDCP_RX_DES_FW_PATH, F_OK) || (access(HDCP_NEW_KEY_CREATED, F_OK) == F_OK)) {
        SYS_LOGI("HDCP rx 2.2 firmware do not exist or new key come, first create it\n");
    #ifndef RECOVERY_MODE
        int ret = generateHdcpFwFromStorage(HDCP_RX_SRC_FW_PATH, HDCP_RX_DES_FW_PATH);
        if (ret < 0) {
            remove(HDCP_RX_DES_FW_PATH);
            SYS_LOGE("HDCP rx 2.2 generate firmware fail\n");
        }
    #endif
    }
#else

    SysWrite write;
    char value[8] = {0};

    write.readSysfs(HDMI_TX_REPEATER_PATH, value);
    //init HDCP 1.4 key
    HDCPRxKey hdcpRx14(HDCP_RX_14_KEY);
    bool bhdcpRx14 = hdcpRx14.refresh();

    //init HDCP 2.2 key
    HDCPRxKey hdcpRx22(HDCP_RX_22_KEY);
    bool bhdcpRx22 = hdcpRx22.refresh();

    if (!strcmp(value, "1") && access(HDCP_RPTX22_DES_FW_PATH, F_OK)) {
        SYS_LOGI("firmware_rptx.le don't exist, and copy it from vendor/etc/firmware/hdcp_rp22/");
        int ret = hdcpRx22.copyHdcpFwToParam(HDCP_RPTX22_SRC_FW_PATH, HDCP_RPTX22_DES_FW_PATH);
        if (ret == -1) {
            SYS_LOGE("copy RPTX firwmare fail\n");
            remove(HDCP_RPTX22_DES_FW_PATH);
        }
    }

    SYS_LOGI("hdcp  value =%s, bhdcpRx14 = %d, bhdcpRx22 = %d\n", value, bhdcpRx14, bhdcpRx22);

    if (!strcmp(value, "1")) {
        if (bhdcpRx14 || bhdcpRx22) {
            write.setProperty("ctl.start", "hdcp_rp22");
            SYS_LOGI("start hdcp rp22");
        }
    }else {
        if (bhdcpRx22) {
            write.setProperty("ctl.start", "hdcp_rx22");
            SYS_LOGI("start hdcp rx22");
        }
    }

#endif

}

void HDCPRxAuth::startVer22() {
    SysWrite write;
    char value[8] = {0};
    write.readSysfs(HDMI_TX_REPEATER_PATH, value);
    SYS_LOGI("hdcp startVer22 value =%s\n", value);
    if (!strcmp(value, "1")) {
        SYS_LOGI("hdcp_rp22");
        write.setProperty("ctl.start", "hdcp_rp22");
    } else {
        SYS_LOGI("hdcp_rx22");
        write.setProperty("ctl.start", "hdcp_rx22");
    }
}

void HDCPRxAuth::stopVer22() {
    SysWrite write;
    char value[8] = {0};
    write.readSysfs(HDMI_TX_REPEATER_PATH, value);
    if (!strcmp(value, "1")) {
        write.setProperty("ctl.stop", "hdcp_rp22");
    } else {
        write.setProperty("ctl.stop", "hdcp_rx22");
    }
}

void HDCPRxAuth::forceFlushVideoLayer() {
#ifndef RECOVERY_MODE
    SysWrite write;
    char valueStr[10] = {0};

    write.readSysfs(SYSFS_VIDEO_LAYER_STATE, valueStr);
    int curVideoState = atoi(valueStr);

    if (curVideoState != mLastVideoState) {
        SYS_LOGI("hdcp_rx Video Layer1 switch_state: %d\n", curVideoState);
        pTxAuth->sfRepaintEverything();
        mLastVideoState = curVideoState;
    }
    usleep(200*1000);//sleep 200ms
#endif
}

// HDMI RX uevent prcessed in this loop
void* HDCPRxAuth::RxUenventThreadLoop(void* data) {
    HDCPRxAuth *pThiz = (HDCPRxAuth*)data;

    SysWrite sysWrite;
    uevent_data_t ueventData;
    memset(&ueventData, 0, sizeof(uevent_data_t));

    UEventObserver ueventObserver;
    ueventObserver.addMatch(HdmiRxPlugEvent);
    ueventObserver.addMatch(HdmiRxAuthEvent);

    while (true) {
        ueventObserver.waitForNextEvent(&ueventData);
        SYS_LOGI("HDCP RX switch_name: %s ,switch_state: %s\n", ueventData.switchName, ueventData.switchState);

        if (!strcmp(ueventData.matchName, HdmiRxPlugEvent)) {
            if (!strcmp(ueventData.switchState, HDMI_RX_PLUG_IN)) {
                pThiz->pTxAuth->stop();
                pThiz->stopVer22();
                usleep(50*1000);
                pThiz->startVer22();
            } else if (!strcmp(ueventData.switchState, HDMI_RX_PLUG_OUT)) {
                pThiz->pTxAuth->stop();
                pThiz->stopVer22();
                pThiz->pTxAuth->start();
            }
        }
        else if (!strcmp(ueventData.matchName, HdmiRxAuthEvent)) {
            if (!strcmp(ueventData.switchState, HDMI_RX_AUTH_FAIL)) {
                SYS_LOGI("HDCP RX, switch_state: %s error\n", ueventData.switchState);
                continue;
            }

            if (!strcmp(ueventData.switchState, HDMI_RX_AUTH_HDCP14)) {
                SYS_LOGI("HDCP RX, 1.4 hdmi is plug in\n");
                pThiz->pTxAuth->setRepeaterRxVersion(REPEATER_RX_VERSION_14);
            }
            else if (!strcmp(ueventData.switchState, HDMI_RX_AUTH_HDCP22)) {
                SYS_LOGI("HDCP RX, 2.2 hdmi is plug in\n");
                pThiz->pTxAuth->setRepeaterRxVersion(REPEATER_RX_VERSION_22);
            }

            pThiz->forceFlushVideoLayer();

            char hdmiTxState[MODE_LEN] = {0};
            sysWrite.readSysfs(HDMI_TX_PLUG_STATE, hdmiTxState);
            if (!strcmp(hdmiTxState, "1")) {
                sysWrite.writeSysfs(DISPLAY_HDMI_AVMUTE, "1");

                SYS_LOGI("HDCP RX, repeater TX hdmi is plug in\n");
                pThiz->pTxAuth->stop();
                pThiz->pTxAuth->start();
            }
            else {
                SYS_LOGI("HDCP RX, repeater TX hdmi is plug out\n");
            }
        }
    }

    return NULL;
}

