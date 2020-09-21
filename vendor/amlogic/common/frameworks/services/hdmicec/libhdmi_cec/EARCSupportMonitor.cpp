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
 *  @author   Jinping Wang
 *  @version  1.0
 *  @date     2019/08/16
 *  @par function description:
 *  - 1 process Hdmi EARC uevent monitor
 */

#define LOG_TAG "hdmicecd"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <cutils/log.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "EARCSupportMonitor.h"
#include "UEventObserver.h"

static char MonitorEvent[128] = {0};
// /sys/class/extcon/earc_event/state

static void GetMonitorEventPath() {
    snprintf(MonitorEvent, sizeof(MonitorEvent), "DEVPATH=%s", "");

    ALOGD("MonitorEvent = %s\n", MonitorEvent);

}

EARCSupportMonitor::EARCSupportMonitor() {
    //GetMonitorEventPath();
    pthread_t id;
    int ret = pthread_create(&id, NULL, MonitorUenventThreadLoop, this);
    if (ret != 0) {
        ALOGE("Create MonitorUenventThreadLoop error!\n");
    }
}

EARCSupportMonitor::~EARCSupportMonitor() {
}

bool EARCSupportMonitor::getEARCSupport() {
    AutoMutex _l(mLock);
    return mEARC;
}

void EARCSupportMonitor::setEARCSupport(bool status) {
    AutoMutex _l(mLock);
    ALOGD("setEARCSupport: %d\n", status);
    mEARC = status;
}

// HDMI ARC EARC uevent prcessed in this loop
void* EARCSupportMonitor::MonitorUenventThreadLoop(void* data) {
    EARCSupportMonitor *pThiz = (EARCSupportMonitor*)data;

    uevent_data_t ueventData;
    memset(&ueventData, 0, sizeof(uevent_data_t));

    UEventObserver ueventObserver;
    ueventObserver.addMatch(HDMI_EARC_TX_UEVENT);
    ueventObserver.addMatch(HDMI_EARC_RX_UEVENT);

    while (true) {
        ueventObserver.waitForNextEvent(&ueventData);
        ALOGD("Uevent switch_name: %s ,arc_state: %s, earc_state: %s\n", ueventData.switchName, ueventData.arcState, ueventData.earcState);
        if (!strcmp(ueventData.switchName, HDMI_EARC_TX_NAME)) {
            if (!strcmp(ueventData.earcState, SUPPORT) && !strcmp(ueventData.arcState, UNSUPPORT)) {
                pThiz->setEARCSupport(true);
            } else if (!strcmp(ueventData.earcState, UNSUPPORT)&& !strcmp(ueventData.arcState, SUPPORT)) {
                pThiz->setEARCSupport(false);
            }
        }
    }

    return NULL;
}

