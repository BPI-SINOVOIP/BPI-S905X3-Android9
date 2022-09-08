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
 *  - 1 process HDCP TX authenticate
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

#include "HDCPTxAuth.h"
#include "UEventObserver.h"
#include "../DisplayMode.h"

#ifndef RECOVERY_MODE
#include <binder/IBinder.h>
#include <binder/IServiceManager.h>
#include <binder/Parcel.h>

using namespace android;
#endif

HDCPTxAuth::HDCPTxAuth() :
    mRepeaterRxVer(REPEATER_RX_VERSION_NONE),
    mpCallback(NULL),
    mMute(false),
    pthreadIdHdcpTx(0),
    mBootAnimFinished(false) {

    setSuspendResume(false);

    if (sem_init(&pthreadTxSem, 0, 0) < 0) {
        SYS_LOGE("HDCPTxAuth, sem_init failed\n");
        exit(0);
    }

    pthread_t id;
    int ret = pthread_create(&id, NULL, TxUenventThreadLoop, this);
    if (ret != 0) {
        SYS_LOGE("Create HdmiPlugDetectThread error!\n");
    }
}

HDCPTxAuth::~HDCPTxAuth() {
    sem_destroy(&pthreadTxSem);
}

void HDCPTxAuth::setBootAnimFinished(bool finished) {
    mBootAnimFinished = finished;
}

void HDCPTxAuth::setRepeaterRxVersion(int ver) {
    mRepeaterRxVer = ver;
}

void HDCPTxAuth::setUevntCallback (TxUevntCallbak *ob) {
    mpCallback = ob;
}

void HDCPTxAuth::setFRAutoAdpt(FrameRateAutoAdaption * mFRAutoAdpt) {
   this->mFRAutoAdpt = mFRAutoAdpt;
}

//start HDCP TX authenticate
int HDCPTxAuth::start() {
    int ret;
    pthread_t thread_id;

    //SYS_LOGI("hdcp_tx thread start\n");
    if (pthread_mutex_trylock(&pthreadTxMutex) == EDEADLK) {
        SYS_LOGE("hdcp_tx create thread, Mutex is deadlock\n");
        return -1;
    }

    mExitHdcpTxThread = false;
    ret = pthread_create(&thread_id, NULL, authThread, this);
    if (ret != 0) SYS_LOGE("hdcp_tx, thread create failed\n");

    ret = sem_wait(&pthreadTxSem);
    if (ret < 0) SYS_LOGE("hdcp_tx, sem_wait failed\n");

    pthreadIdHdcpTx = thread_id;
    pthread_mutex_unlock(&pthreadTxMutex);
    SYS_LOGI("hdcp_tx, create hdcp thread id = %lu done\n", thread_id);
    return 0;
}

//stop HDCP TX authenticate
int HDCPTxAuth::stop() {
    void *threadResult;
    int ret = -1;

    stopVerAll();
    if (0 != pthreadIdHdcpTx) {
        mExitHdcpTxThread = true;
        if (pthread_mutex_trylock(&pthreadTxMutex) == EDEADLK) {
            SYS_LOGE("hdcp_tx exit thread, Mutex is deadlock\n");
            return ret;
        }

        if (0 != pthread_join(pthreadIdHdcpTx, &threadResult)) {
            SYS_LOGE("hdcp_tx exit thread failed\n");
            return ret;
        }

        pthread_mutex_unlock(&pthreadTxMutex);
        SYS_LOGI("hdcp_tx pthread exit id = %lu, %s  done\n", pthreadIdHdcpTx, (char *)threadResult);
        pthreadIdHdcpTx = 0;
        ret = 0;
    }

    return ret;
}

//Define to force authentiation regardless of keys presence
//#define HDCP_AUTHENTICATION_NO_KEYS

void HDCPTxAuth::mute(bool mute __unused) {

#if !defined(HDCP_AUTHENTICATION_NO_KEYS)
    char hdcpTxKey[MODE_LEN] = {0};
    mSysWrite.readSysfs(DISPLAY_HDMI_HDCP_KEY, hdcpTxKey);
    if ((strlen(hdcpTxKey) == 0) || !(strcmp(hdcpTxKey, "00")))
        return;
#endif

#ifdef HDCP_AUTHENTICATION
    if (mute != mMute) {
        //mSysWrite.writeSysfs(DISPLAY_HDMI_AUDIO_MUTE, mute ? "1" : "0");
        mSysWrite.writeSysfs(DISPLAY_HDMI_VIDEO_MUTE, mute ? "1" : "0");
        mMute = mute;
        SYS_LOGI("hdcp_tx mute %s\n", mute ? "on" : "off");
    }
#endif
}

void* HDCPTxAuth::authThread(void* data) {
    bool hdcp22 = false;
    bool hdcp14 = false;
    HDCPTxAuth *pThiz = (HDCPTxAuth*)data;

    SYS_LOGI("hdcp_tx thread loop entry\n");
    sem_post(&pThiz->pthreadTxSem);

    while (!pThiz->mBootAnimFinished) {
        usleep(50*1000);//sleep 50ms
    }

    if (pThiz->authInit(&hdcp22, &hdcp14)) {
        if (!pThiz->authLoop(hdcp22, hdcp14)) {
            SYS_LOGE("HDCP authenticate fail\n");
        }
    }
    pThiz->mSysWrite.writeSysfs(SYS_DISABLE_VIDEO, VIDEO_LAYER_ENABLE);
    return NULL;
}

bool HDCPTxAuth::authInit(bool *pHdcp22, bool *pHdcp14) {
    bool useHdcp22 = false;
    bool useHdcp14 = false;
#ifdef HDCP_AUTHENTICATION
    char hdcpRxVer[MODE_LEN] = {0};
    char hdcpTxKey[MODE_LEN] = {0};

    //in general, MBOX is TX device, need to detect its TX keys.
    //            TV   is RX device, need to detect its RX keys.
    //HDCP TX: get current MBOX[TX] device contains which TX keys. Values:[14/22, 00 is no key]
    mSysWrite.readSysfs(DISPLAY_HDMI_HDCP_KEY, hdcpTxKey);
    SYS_LOGI("hdcp_tx key:%s\n", hdcpTxKey);
    if ((strlen(hdcpTxKey) == 0) || !(strcmp(hdcpTxKey, "00")))
        return false;

    //HDCP RX: get currtent TV[RX] device contains which RX key. Values:[14/22, 00 is no key]
    //Values is the hightest key. if value is 22, means the devices supports 22 and 14.
    mSysWrite.readSysfs(DISPLAY_HDMI_HDCP_VER, hdcpRxVer);
    SYS_LOGI("hdcp_tx remote version:%s\n", hdcpRxVer);
    if ((strlen(hdcpRxVer) == 0) || !(strcmp(hdcpRxVer, "00")))
        return false;

    //stop hdcp_tx
    stopVerAll();

    if (REPEATER_RX_VERSION_22 == mRepeaterRxVer) {
        SYS_LOGI("hdcp_tx 2.2 supported for RxSupportHdcp2.2Auth\n");
        useHdcp22 = true;
    } else if (/*(strstr(cap, (char *)"2160p") != NULL) && */(strstr(hdcpRxVer, (char *)"22") != NULL) &&
        (strstr(hdcpTxKey, (char *)"22") != NULL)) {
        if ((!access("/system/etc/firmware/firmware.le", 0)) || (!access("/vendor/etc/firmware/firmware.le", 0))) {
            SYS_LOGI("hdcp_tx 2.2 supported\n");
            useHdcp22 = true;
        } else {
            SYS_LOGE("!!!hdcp_tx 2.2 key is burned in This device, But /etc/firmware/fireware.le is not exist\n");
        }
    }

    if (REPEATER_RX_VERSION_14 == mRepeaterRxVer) {
        SYS_LOGI("hdcp_tx 1.4 supported for RxSupportHdcp1.4Auth\n");
        useHdcp14 = true;
    } else if (!useHdcp22 && (strstr(hdcpRxVer, (char *)"14") != NULL) &&
        (strstr(hdcpTxKey, (char *)"14") != NULL)) {
        useHdcp14 = true;
        SYS_LOGI("hdcp_tx 1.4 supported\n");
    }

    if (!useHdcp22 && !useHdcp14) {
        //do not support hdcp1.4 and hdcp2.2
        SYS_LOGE("device do not support hdcp1.4 or hdcp2.2\n");
        return false;
    }

    //start hdcp_tx
    if (useHdcp22) {
        startVer22();
    }
    else if (useHdcp14) {
        startVer14();
    }
#endif
    *pHdcp22 = useHdcp22;
    *pHdcp14 = useHdcp14;
    return true;
}

bool HDCPTxAuth::authLoop(bool useHdcp22, bool useHdcp14) {
    SYS_LOGI("hdcp_tx begin to authenticate hdcp22:%d, hdcp14:%d\n", useHdcp22, useHdcp14);

    bool success = false;
#ifdef HDCP_AUTHENTICATION
    int count = 0;
    while (!mExitHdcpTxThread) {
        usleep(200*1000);//sleep 200ms

        char auth[MODE_LEN] = {0};
        mSysWrite.readSysfs(DISPLAY_HDMI_HDCP_AUTH, auth);
        if (strstr(auth, (char *)"1")) {//Authenticate is OK
            success = true;
            mSysWrite.writeSysfs(DISPLAY_HDMI_AVMUTE, "-1");
            break;
        }

        count++;
        if (count > 40) { //max 200msx40 = 8s it will authenticate completely
            if (useHdcp22) {
                SYS_LOGE("hdcp_tx 2.2 authenticate fail for 8s timeout, change to hdcp_tx 1.4 authenticate\n");

                count = 0;
                useHdcp22 = false;
                useHdcp14 = true;
                stopVerAll();
                //if support hdcp22, must support hdcp14
                startVer14();
                continue;
            }
            else if (useHdcp14) {
                SYS_LOGE("hdcp_tx 1.4 authenticate fail, 8s timeout\n");
                startVer14();
            }
            mSysWrite.writeSysfs(DISPLAY_HDMI_AVMUTE, "-1");
            break;
        }
    }
#endif
    SYS_LOGI("hdcp_tx authenticate success: %d\n", success?1:0);
    return success;
}

void HDCPTxAuth::startVer22() {
    //start hdcp_tx 2.2
    SYS_LOGI("start hdcp_tx 2.2\n");
    mSysWrite.writeSysfs(DISPLAY_HDMI_HDCP_MODE, DISPLAY_HDMI_HDCP_22);
    usleep(50*1000);

    mSysWrite.setProperty("ctl.start", "hdcp_tx22");
}

void HDCPTxAuth::startVer14() {
    //start hdcp_tx 1.4
    //SYS_LOGI("hdcp_tx 1.4 start\n");
    mSysWrite.writeSysfs(DISPLAY_HDMI_HDCP_MODE, DISPLAY_HDMI_HDCP_14);
}

void HDCPTxAuth::stopVerAll() {
    char hdcpRxVer[MODE_LEN] = {0};
    //stop hdcp_tx 2.2 & 1.4
    SYS_LOGI("hdcp_tx 2.2 & 1.4 stop hdcp pwr\n");
    mSysWrite.writeSysfs(DISPLAY_HDMI_HDCP_POWER, "1");
    usleep(20000);
    mSysWrite.setProperty("ctl.stop", "hdcp_tx22");
    usleep(20000);
    mSysWrite.readSysfs(DISPLAY_HDMI_HDCP_POWER, hdcpRxVer);

    mSysWrite.writeSysfs(DISPLAY_HDMI_HDCP_CONF, DISPLAY_HDMI_HDCP14_STOP);
    mSysWrite.writeSysfs(DISPLAY_HDMI_HDCP_CONF, DISPLAY_HDMI_HDCP22_STOP);
    usleep(2000);
}

void HDCPTxAuth::setSuspendResume(bool status) {
   mSuspendResume = status;
}

bool HDCPTxAuth::getSuspendResume(void) {
   return mSuspendResume;
}

void HDCPTxAuth::setSysCtrlReady(bool status) {
    if (status == true) {
        mSysWrite.writeSysfs(DISPLAY_HDMI_SYSCTRL_READY, "1");
    } else {
        mSysWrite.writeSysfs(DISPLAY_HDMI_SYSCTRL_READY, "0");
    }
}

// HDMI TX uevent prcessed in this loop
void* HDCPTxAuth::TxUenventThreadLoop(void* data) {
    HDCPTxAuth *pThiz = (HDCPTxAuth*)data;

    uevent_data_t ueventData;
    memset(&ueventData, 0, sizeof(uevent_data_t));

    UEventObserver ueventObserver;
    ueventObserver.addMatch(HDMI_TX_POWER_UEVENT);
    #ifndef HWC_DYNAMIC_SWITCH_VIU
    ueventObserver.addMatch(HDMI_TX_PLUG_UEVENT);
    #endif
    ueventObserver.addMatch(VIDEO_LAYER1_UEVENT);
    ueventObserver.addMatch(HDMI_TX_HDR_UEVENT);
    ueventObserver.addMatch(HDMI_TX_HDCP_UEVENT);
    ueventObserver.addMatch(HDMI_TX_HDCP14_LOG_UEVENT);
    ueventObserver.addMatch(HDMI_TX_HDMI_AUDIO_UEVENT);
#ifdef FRAME_RATE_AUTO_ADAPTER
    ueventObserver.addMatch(HDMI_TVOUT_FRAME_RATE_UEVENT);
#endif
    //systemcontrol ready for driver
    pThiz->setSysCtrlReady(true);

    while (true) {
        ueventObserver.waitForNextEvent(&ueventData);
        SYS_LOGI("HDCP TX switch_name: %s ,switch_state: %s\n", ueventData.switchName, ueventData.switchState);

        if (pThiz->getSuspendResume()) {
            if ((!strcmp(ueventData.matchName, HDMI_TX_POWER_UEVENT)) &&
                (!strcmp(ueventData.switchName, HDMI_UEVENT_HDMI_POWER)) &&
                (!strcmp(ueventData.switchState, HDMI_TX_RESUME)) &&
                (NULL != pThiz->mpCallback)) {
                pThiz->setSuspendResume(false);
                pThiz->mpCallback->onTxEvent(ueventData.switchName, ueventData.switchState, OUPUT_MODE_STATE_POWER);
            }
        } else {
            //hot plug string is the hdmi audio, hdmi power, hdmi hdr substring
            if (!strcmp(ueventData.matchName, HDMI_TX_PLUG_UEVENT) && !strcmp(ueventData.switchName, HDMI_UEVENT_HDMI) && (NULL != pThiz->mpCallback)) {
                pThiz->mpCallback->onTxEvent(ueventData.switchName, ueventData.switchState, OUPUT_MODE_STATE_POWER);
            }
            else if (!strcmp(ueventData.matchName, HDMI_TX_POWER_UEVENT) && !strcmp(ueventData.switchName, HDMI_UEVENT_HDMI_POWER)) {
                //0: hdmi suspend  1: hdmi resume
                if (!strcmp(ueventData.switchState, HDMI_TX_RESUME) && (NULL != pThiz->mpCallback)) {
                    pThiz->setSuspendResume(false);
                    pThiz->mpCallback->onTxEvent(ueventData.switchName, ueventData.switchState, OUPUT_MODE_STATE_POWER);
                }
                else if (!strcmp(ueventData.switchState, HDMI_TX_SUSPEND) && (NULL != pThiz->mpCallback)) {
                    pThiz->setSuspendResume(true);
                    pThiz->mSysWrite.writeSysfs(DISPLAY_HDMI_HDCP_POWER, "1");
                    pThiz->mpCallback->onTxEvent(ueventData.switchName, ueventData.switchState, OUPUT_MODE_STATE_POWER);
                }
            }
            else if (!strcmp(ueventData.matchName, HDMI_TX_HDR_UEVENT) && !strcmp(ueventData.switchName, HDMI_UEVENT_HDMI_HDR)) {
                //0: exit hdr mode  1: enter hdr mode
                char hdrState[MODE_LEN] = {0};
                pThiz->mSysWrite.readSysfs(HDMI_TX_SWITCH_HDR, hdrState);
                if (!strcmp(hdrState, "0")) {
                    pThiz->mSysWrite.writeSysfs(DISPLAY_HDMI_AVMUTE, "1");
                    usleep(100000);//100ms
                    pThiz->stopVerAll();
                    pThiz->stop();
                    pThiz->mSysWrite.writeSysfs(DISPLAY_HDMI_PHY, "0"); /* Turn off TMDS PHY */
                    usleep(200000);//200ms
                    pThiz->mSysWrite.writeSysfs(DISPLAY_HDMI_PHY, "1"); /* Turn on TMDS PHY */
                    pThiz->start();
                }
            }
            else if (!strcmp(ueventData.matchName, HDMI_TX_HDCP_UEVENT) && !strcmp(ueventData.switchName, HDMI_UEVENT_HDCP)) {
                //0: hdcp failure -> mute a/v
                //if (!strcmp(ueventData.switchState, "0"))
                    //pThiz->mute(true);
                //1:  hdcp success -> unmute a/v
                //else
                    //pThiz->mute(false);
            }
            else if (!strcmp(ueventData.matchName, HDMI_TX_HDCP14_LOG_UEVENT) && !strcmp(ueventData.switchName, HDMI_UEVENT_HDCP_LOG)) {
                //char logBuf[MAX_STR_LEN+1] = {0};
                //pThiz->mSysWrite.readSysfsOriginal(HDMI_TX_HDCP14_LOG_SYS, logBuf);
                //SYS_LOGI("HDCP log:%s", logBuf);
            }
            else if (!strcmp(ueventData.matchName, HDMI_TVOUT_FRAME_RATE_UEVENT)) {
                   pThiz->mFRAutoAdpt->onTxUeventReceived(&ueventData);
            }
            else if (!strcmp(ueventData.matchName, HDMI_TX_HDMI_AUDIO_UEVENT) && !strcmp(ueventData.switchName, HDMI_UEVENT_HDMI_AUDIO)  && (NULL != pThiz->mpCallback)) {
                   pThiz->mpCallback->onTxEvent(ueventData.switchName, ueventData.switchState, OUPUT_MODE_STATE_POWER);
            }
#ifndef RECOVERY_MODE
            if (!strcmp(ueventData.matchName, VIDEO_LAYER1_UEVENT)) {
                //0: no aml video data, 1: aml video data aviliable
                if (!strcmp(ueventData.switchName, "video_layer1") && !strcmp(ueventData.switchState, "1")) {
                    SYS_LOGI("Video Layer1 switch_state: %s switch_name: %s\n", ueventData.switchState, ueventData.switchName);
                    pThiz->sfRepaintEverything();
                }
            }
#endif
        }
    }
    return NULL;
}

#ifndef RECOVERY_MODE
void HDCPTxAuth::sfRepaintEverything() {
    sp<IServiceManager> sm = defaultServiceManager();
    sp<IBinder> sf = sm->getService(String16("SurfaceFlinger"));
    if (sf != NULL) {
        Parcel data;
        data.writeInterfaceToken(String16("android.ui.ISurfaceComposer"));
        //SYS_LOGI("send message to sf to repaint everything!\n");
        sf->transact(1004, data, NULL);
    }
}

void HDCPTxAuth::isAuthSuccess(int *status) {
    int ret = pthread_mutex_trylock(&pthreadTxMutex);
    if (ret != 0) {
         SYS_LOGE("try lock pthreadTxMutex return status: %d\n",ret);
         *status = -1;
    }
    char auth[MODE_LEN] = {0};
    mSysWrite.readSysfs(DISPLAY_HDMI_HDCP_AUTH, auth);
    if (strstr(auth, (char *)"1")) {//Authenticate is OK
        *status = 1;
    }
    pthread_mutex_unlock(&pthreadTxMutex);
    SYS_LOGI("HDCPTx Auth status is: %d",*status);
}
#endif
