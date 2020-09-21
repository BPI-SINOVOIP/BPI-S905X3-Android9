/*
 * Copyright (C) 2016 The Android Open Source Project
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
 *  @date     2017/9/25
 *  @par function description:
 *  - 1 droidlogic hdmi cec real implementation
 */

#define LOG_TAG "hdmicecd"
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <android/log.h>
#include <cutils/properties.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include "HdmiCecControl.h"
#include "HdmiCecCompat.h"

namespace android {

HdmiCecControl::HdmiCecControl() : mMsgHandler(this)
{
    ALOGD("[hcc] HdmiCecControl");
    init();
}

HdmiCecControl::~HdmiCecControl(){}

HdmiCecControl::MsgHandler::MsgHandler(HdmiCecControl *hdmiControl)
{
    mControl = hdmiControl;
}

HdmiCecControl::MsgHandler::~MsgHandler(){}

HdmiCecControl::TvEventListner::TvEventListner(HdmiCecControl *hdmiControl)
{
    mControl = hdmiControl;
}

HdmiCecControl::TvEventListner::~TvEventListner(){}

void HdmiCecControl::MsgHandler::handleMessage (CMessage &msg)
{
    cec_message_t message;
    ALOGV("HdmiCecControl::MsgHandler::handleMessage type = %d", msg.mType);
    switch (msg.mType) {
        case HdmiCecControl::MsgHandler::MSG_FILTER_OTP_TIMEOUT:
            mControl->mLock.lock();
            mControl->mCecDevice.mFilterOtpEnabled = false;
            ALOGD ("reset mFilterOtpEnabled to false.");
            mControl->mLock.unlock();
            break;
        case HdmiCecControl::MsgHandler::MSG_WAIT_EDIDUPDATE_FINISHED_TIMEOUT:
            mControl->mLock.lock();
            mControl->mCecDevice.mWaitEdidUpdatedFinished = true;
            ALOGD ("reset mWaitEdidUpdatedFinished to true.");
            mControl->mLock.unlock();
            break;
        case HdmiCecControl::MsgHandler::MSG_GIVE_PHYSICAL_ADDRESS:
            if (mControl->mCecDevice.isTvDeviceType) {
                message.initiator = (cec_logical_address_t)DEV_TYPE_TV;
                message.destination = (cec_logical_address_t)(msg.mpPara[0]);
                message.body[0] = CEC_MESSAGE_GIVE_PHYSICAL_ADDRESS;
                message.length = 1;
                mControl->send(&message);
            }
            break;
        case HdmiCecControl::MsgHandler::MSG_TURN_ON_DEVICE:
            mControl->turnOnDevice(msg.mpPara[0]);
            break;
        case HdmiCecControl::MsgHandler::MSG_SET_STREAM_PATH:
            if (mControl->mCecDevice.isTvDeviceType) {
                 message.initiator = (cec_logical_address_t)DEV_TYPE_TV;
                 message.destination = (cec_logical_address_t)ADDR_BROADCAST;
                 message.body[0] = CEC_MESSAGE_SET_STREAM_PATH;
                 message.body[1] = (msg.mpPara[0]) & 0xff;
                 message.body[2] = (msg.mpPara[1]) & 0xff;
                 message.length = 3;
                 mControl->send(&message);
            }
            break;
        case HdmiCecControl::MsgHandler::MSG_USER_CONTROL_PRESSED:
            if (mControl->mCecDevice.isTvDeviceType) {
                 message.initiator = (cec_logical_address_t)((msg.mpPara[0]) & 0xff);
                 message.destination = (cec_logical_address_t)((msg.mpPara[1]) & 0xff);
                 message.body[0] = CEC_MESSAGE_USER_CONTROL_PRESSED;
                 message.body[1] = (msg.mpPara[2]) & 0xff;
                 message.length = 2;
                 mControl->send(&message);
            }
            break;
        case HdmiCecControl::MsgHandler::GET_MENU_LANGUAGE:
            mControl->sendMessage(mControl->mCecDevice.mLogicalAddress, DEV_TYPE_TV, 1, CEC_MESSAGE_GET_MENU_LANGUAGE);
            break;
        case HdmiCecControl::MsgHandler::GIVE_OSD_NAEM:
            mControl->sendMessage(mControl->mCecDevice.mLogicalAddress, DEV_TYPE_TV, 1, CEC_MESSAGE_GIVE_OSD_NAME);
            break;
        case HdmiCecControl::MsgHandler::MSG_GIVE_DEVICE_VENDOR_ID:
            mControl->sendMessage(mControl->mCecDevice.mLogicalAddress, DEV_TYPE_TV, 1, CEC_MESSAGE_GIVE_DEVICE_VENDOR_ID);
            break;
    }
}

void HdmiCecControl::TvEventListner::notify (const tv_parcel_t &parcel)
{
    //ALOGD ("HdmiCecControl::TvEventListner::notify type = %d", parcel.msgType);
    int deviceId = -1;
    switch (parcel.msgType) {
        case HdmiCecControl::TvEventListner::TV_EVENT_SOURCE_SWITCH:
            mControl->mLock.lock();
            deviceId = parcel.bodyInt[0]; //hdmi: 5-8; home: -1; non hdmi: other than home and hdmi;
            ALOGD ("TvEventListner notify input deviceId from: %d to: %d", mControl->mCecDevice.mSelectedDeviceId, deviceId);
            CMessage msg;
            msg.mType = HdmiCecControl::MsgHandler::MSG_FILTER_OTP_TIMEOUT;
            msg.mDelayMs = DELAY_TIMEOUT_MS;
            mControl->mMsgHandler.removeMsg(msg);
            mControl->mCecDevice.mSelectedDeviceId = deviceId;
            if (deviceId >= 5 && deviceId <= 8) {
                mControl->mCecDevice.mFilterOtpEnabled = true;
                mControl->mMsgHandler.sendMsg(msg);
            } else {
                mControl->mCecDevice.mFilterOtpEnabled = false;
            }
            ALOGD ("Set mFilterOtpEnabled to %s", mControl->mCecDevice.mFilterOtpEnabled ? "true" : "false");
            mControl->mLock.unlock();
            break;
    }
}

/**
 * initialize some cec flags before opening cec deivce.
 */
void HdmiCecControl::init()
{
    char value[PROPERTY_VALUE_MAX] = {0};
    mCecDevice.isTvDeviceType = false;
    mCecDevice.isPlaybackDeviceType = false;
    mCecDevice.mDeviceTypes = NULL;
    mCecDevice.mAddedPhyAddrs = NULL;
    mCecDevice.mTotalDevice = 0;
    getDeviceTypes();
    memset(value, 0, PROPERTY_VALUE_MAX);
    //property_get("persist.vendor.sys.hdmi.keep_awake", value, "true");
    //mCecDevice.mExtendControl = (!strcmp(value, "false")) ? 1 : 0;
    mCecDevice.mExtendControl= 1;
    ALOGD("[hcc] ext_control: %d", mCecDevice.mExtendControl);
    mSystemControl = new SystemControlClient();
}

void HdmiCecControl::getDeviceTypes() {
    int index = 0;
    char value[PROPERTY_VALUE_MAX] = {0};
    const char * split = ",";
    char * type;
    mCecDevice.mDeviceTypes = new int[DEV_TYPE_VIDEO_PROCESSOR];
    if (!mCecDevice.mDeviceTypes) {
        ALOGE("[hcc] alloc mDeviceTypes failed");
        return;
    }
    property_get("ro.vendor.platform.hdmi.device_type", value, "4");
    type = strtok(value, split);
    mCecDevice.mDeviceTypes[index] = atoi(type);
    while (type != NULL) {
        type = strtok(NULL,split);
        if (type != NULL)
            mCecDevice.mDeviceTypes[++index] = atoi(type);
    }
    mCecDevice.mTotalDevice = index + 1;
    index = 0;
    for (index = 0; index < mCecDevice.mTotalDevice; index++) {
        if (mCecDevice.mDeviceTypes[index] == DEV_TYPE_TV) {
            mCecDevice.isTvDeviceType = true;
        } else if (mCecDevice.mDeviceTypes[index] == DEV_TYPE_PLAYBACK) {
            mCecDevice.isPlaybackDeviceType = true;
        }
        ALOGD("[hcc] mCecDevice.mDeviceTypes[%d]: %d", index, mCecDevice.mDeviceTypes[index]);
    }
}
/**
 * close cec device, reset some cec flags, and recycle some resources.
 */
int HdmiCecControl::closeCecDevice()
{
    mCecDevice.mRun = false;
    while (!mCecDevice.mExited) {
        usleep(100 * 1000);
    }

    close(mCecDevice.mFd);
    delete mCecDevice.mpPortData;
    delete mCecDevice.mDeviceTypes;
    delete mCecDevice.mAddedPhyAddrs;
    ALOGD("[hcc] %s, cec has closed.", __FUNCTION__);
    return 0;
}

/**
 * initialize all cec flags when open cec devices, get the {@code fd} of cec devices,
 * and create a thread for cec working.
 * {@Return}  fd of cec device.
 */
int HdmiCecControl::openCecDevice()
{
    int index = 0;
    mCecDevice.mRun = true;
    mCecDevice.mExited = false;
    mCecDevice.mThreadId = 0;
    mCecDevice.mTotalPort = 0;
    mCecDevice.mConnectStatus = 0;
    mCecDevice.mpPortData = NULL;
    mCecDevice.mAddrBitmap = (1 << CEC_ADDR_BROADCAST);
    mCecDevice.mLogicalAddress = CEC_ADDR_BROADCAST;
    mCecDevice.isCecEnabled = true;
    mCecDevice.isCecControlled = false;
    mCecDevice.mFilterOtpEnabled = false;
    mCecDevice.mWaitEdidUpdatedFinished = true;
    mCecDevice.mSelectedDeviceId = -1;
    mCecDevice.mFd = open(CEC_FILE, O_RDWR);
    if (mCecDevice.mFd < 0) {
        ALOGE("[hcc] can't open device. fd < 0");
        return -EINVAL;
    }
    for (index = 0; index < mCecDevice.mTotalDevice; index++) {
        int deviceType =  mCecDevice.mDeviceTypes[index];
        ALOGD("[hcc] set device type index : %d, type: %d", index, deviceType);
        int ret = ioctl(mCecDevice.mFd, CEC_IOC_SET_DEV_TYPE, deviceType);
    }
    getBootConnectStatus();
    mCecDevice.mAddedPhyAddrs = new int[ADDR_BROADCAST];
    for (index = 0; index < ADDR_BROADCAST; index++) {
        mCecDevice.mAddedPhyAddrs[index] = 0;
    }
    pMonitor = new EARCSupportMonitor();

    pthread_create(&mCecDevice.mThreadId, NULL, __threadLoop, this);
    pthread_setname_np(mCecDevice.mThreadId, "hdmi_cec_loop");
    mMsgHandler.startMsgQueue();
    if (mCecDevice.isTvDeviceType) {
        mTvSession = TvServerHidlClient::connect(CONNECT_TYPE_HAL);
        mTvEventListner = new HdmiCecControl::TvEventListner(this);
        mTvSession->setListener(mTvEventListner);
    } else if (mCecDevice.isPlaybackDeviceType) {
        mCecDevice.mTvOsdName = 0;
        mCecDevice.mTvDeviceVendorId = 0;
        getDeviceExtraInfo(1);
    }
    return mCecDevice.mFd;
}

/**
 * get connect status when boot
 */
void HdmiCecControl::getBootConnectStatus()
{
    unsigned int total, i;
    int port, connect, ret;

    ret = ioctl(mCecDevice.mFd, CEC_IOC_GET_PORT_NUM, &total);
    ALOGD("[hcc] total port:%d, ret:%d", total, ret);
    if (ret < 0)
        return ;

    if (total > MAX_PORT)
        total = MAX_PORT;
    hdmi_port_info_t  *portData = NULL;
    portData = new hdmi_port_info[total];
    if (!portData) {
        ALOGE("[hcc] alloc port_data failed");
        return;
    }
    ioctl(mCecDevice.mFd, CEC_IOC_GET_PORT_INFO, portData);
    mCecDevice.mConnectStatus = 0;
    for (i = 0; i < total; i++) {
        port = portData[i].port_id;
        connect = port;
        ret = ioctl(mCecDevice.mFd, CEC_IOC_GET_CONNECT_STATUS, &connect);
        if (ret) {
            ALOGD("[hcc] get port %d connected status failed, ret:%d", i, ret);
            continue;
        }
        mCecDevice.mConnectStatus |= ((connect ? 1 : 0) << port);
    }
    delete portData;
    ALOGD("[hcc] mConnectStatus: %d", mCecDevice.mConnectStatus);
}

void* HdmiCecControl::__threadLoop(void *user)
{
    HdmiCecControl *const self = static_cast<HdmiCecControl *>(user);
    self->threadLoop();
    return 0;
}

void HdmiCecControl::threadLoop()
{
    unsigned char msgBuf[CEC_MESSAGE_BODY_MAX_LENGTH];
    hdmi_cec_event_t event;
    int r = -1;

    //ALOGD("[hcc] threadLoop start.");
    while (mCecDevice.mFd < 0) {
        usleep(1000 * 1000);
        mCecDevice.mFd = open(CEC_FILE, O_RDWR);
    }
    ALOGD("[hcc] file open ok, fd = %d.", mCecDevice.mFd);

    while (mCecDevice.mRun) {
        if (!mCecDevice.isCecEnabled) {
            usleep(1000 * 1000);
            continue;
        }
        checkConnectStatus();

        memset(msgBuf, 0, sizeof(msgBuf));
        //try to get a message from dev.
        r = readMessage(msgBuf, CEC_MESSAGE_BODY_MAX_LENGTH);
        if (r <= 1)//ignore received ping messages
            continue;

        printCecMsgBuf((const char*)msgBuf, r);

        event.eventType = 0;
        memcpy(event.cec.body, msgBuf + 1, r - 1);
        event.cec.initiator = cec_logical_address_t((msgBuf[0] >> 4) & 0xf);
        event.cec.destination = cec_logical_address_t((msgBuf[0] >> 0) & 0xf);
        event.cec.length = r - 1;

        if (mCecDevice.isCecControlled) {
             event.eventType |= HDMI_EVENT_CEC_MESSAGE;
            /* call java method to process cec message for ext control
            if (mCecDevice.mExtendControl) {
                event.eventType |= HDMI_EVENT_RECEIVE_MESSAGE;
            }
            */
        } else {
            /* wakeup playback device */
            if (isWakeUpMsg((char*)msgBuf, r)) {
                memset(event.cec.body, 0, sizeof(event.cec.body));
                memcpy(event.cec.body, msgBuf + 1, r - 1);
                event.eventType |= HDMI_EVENT_CEC_MESSAGE;
                ALOGD("[hcc] receive wake up message for hdmi_tx");
            }
        }
        messageValidateAndHandle(&event);
        handleOTPMsg(&event);
        supportForEARC(&event, NULL, false);
        if (mEventListener != NULL && event.eventType != 0) {
            mEventListener->onEventUpdate(&event);
        }
    }
    //ALOGE("thread end.");
    mCecDevice.mExited = true;
}

/**
 * Check if received a wakeup message if  mCecDevice.isCecControlled  is false
* @param msgBuf is a message Buf
*   msgBuf[1]: message type
*   msgBuf[2]-msgBuf[n]: message para
* @param len is message lenth
* @param deviceType is type of device
*/

bool HdmiCecControl::isWakeUpMsg(char *msgBuf, int len)
{
    bool ret = false;
    int index = 0;
    for (index = 0; index < mCecDevice.mTotalDevice; index++) {
        if (mCecDevice.mDeviceTypes[index] == DEV_TYPE_PLAYBACK) {
            switch (msgBuf[1]) {
                case CEC_MESSAGE_SET_STREAM_PATH:
                case CEC_MESSAGE_DECK_CONTROL:
                case CEC_MESSAGE_PLAY:
                case CEC_MESSAGE_ACTIVE_SOURCE:
                    ret = true;
                    break;
                case CEC_MESSAGE_ROUTING_CHANGE:
                    /* build <set stream path> message*/
                    msgBuf[1] = CEC_MESSAGE_SET_STREAM_PATH;
                    msgBuf[2] =  msgBuf[len - 2];
                    msgBuf[3] =  msgBuf[len - 1];
                    msgBuf[len - 2] &= 0x0;
                    msgBuf[len - 1] &= 0x0;
                    ret = true;
                    break;
                case CEC_MESSAGE_USER_CONTROL_PRESSED:
                    if (msgBuf[2] == CEC_KEYCODE_POWER
                    || msgBuf[2] == CEC_KEYCODE_ROOT_MENU
                    || msgBuf[2] == CEC_KEYCODE_POWER_ON_FUNCTION) {
                        ret = true;
                    }
                    break;
                default:
                    break;
            }
        } else if (mCecDevice.mDeviceTypes[index] == DEV_TYPE_TV) {
            switch (msgBuf[1]) {
                case CEC_MESSAGE_IMAGE_VIEW_ON:
                case CEC_MESSAGE_TEXT_VIEW_ON:
                case CEC_MESSAGE_ACTIVE_SOURCE:
                    ret = true;
                    break;
                default:
                    break;
            }
        } else if (mCecDevice.mDeviceTypes[index] == DEV_TYPE_AUDIO_SYSTEM) {
            switch (msgBuf[1]) {
                case CEC_MESSAGE_SYSTEM_AUDIO_MODE_REQUEST:
                case CEC_MESSAGE_USER_CONTROL_PRESSED:
                    ret = true;
                    break;
                default:
                    break;
            }
        }
    }
    return ret;
}

/**
 * Check if received a valid message.
* @param msgBuf is a message Buf
*   msgBuf[1]: message type
*   msgBuf[2]-msgBuf[n]: message para
* @param len is message lenth
* @param deviceType is type of device
*/

void HdmiCecControl::messageValidateAndHandle(hdmi_cec_event_t* event)
{
    cec_message_t message;
    CMessage msg;
    int initiator = (int)(event->cec.initiator);
    int destination = (int)(event->cec.destination);
    int devPhyAddr = 0;
    int opcode = event->cec.body[0];
    if (mCecDevice.isTvDeviceType) {
        switch (event->cec.body[0]) {
            case CEC_MESSAGE_REPORT_PHYSICAL_ADDRESS:
                devPhyAddr = ((event->cec.body[1] & 0xff) << 8) +  (event->cec.body[2] & 0xff);
                if (event->cec.body[1] == 0) {
                    msg.mType = HdmiCecControl::MsgHandler::MSG_GIVE_PHYSICAL_ADDRESS;
                    msg.mDelayMs = DELAY_TIMEOUT_MS/14;
                    msg.mpPara[0] = initiator;
                    mMsgHandler.removeMsg (msg);
                    mMsgHandler.sendMsg (msg);
                    event->eventType &= ~HDMI_EVENT_CEC_MESSAGE;
                    ALOGD("[hcc] receviced message: %02x validate fail and drop", event->cec.body[0]);
                } else {
                    mCecDevice.mAddedPhyAddrs[initiator] = devPhyAddr;
                }
                break;
            case CEC_MESSAGE_REPORT_POWER_STATUS:
                if (initiator == mCecDevice.mActiveLogicalAddr
                    && (event->cec.body[0] == POWER_STATUS_STANDBY
                    || event->cec.body[0] == POWER_STATUS_TRANSIENT_TO_STANDBY)) {
                    msg.mType = HdmiCecControl::MsgHandler::MSG_TURN_ON_DEVICE;
                    msg.mDelayMs = DELAY_TIMEOUT_MS/7;
                    msg.mpPara[0] = initiator;
                    mMsgHandler.sendMsg (msg);
                }
                break;
            case CEC_MESSAGE_ROUTING_INFORMATION:
                if (destination != ADDR_BROADCAST) {
                    LOGD("[hcc] receviced message: %02x validate fail and drop", event->cec.body[0]);
                } else if (((event->cec.body[1]) & 0x0f) != 0) {
                    msg.mType = HdmiCecControl::MsgHandler::MSG_SET_STREAM_PATH;
                    msg.mDelayMs = 10;
                    msg.mpPara[0] = (event->cec.body[1] & 0xff);
                    msg.mpPara[1] = (event->cec.body[2] & 0xff);
                    mMsgHandler.removeMsg (msg);
                    mMsgHandler.sendMsg (msg);
                    ALOGD ("send <Set Stream Path> to get <Active Source> in order to correct mActiveSource then to control sub device.");
                }
                break;
            default:
                break;
        }
    } else if (mCecDevice.isPlaybackDeviceType) {
        switch (event->cec.body[0]) {
            case CEC_MESSAGE_SET_OSD_NAME:
                mCecDevice.mTvOsdName = ((event->cec.body[1] & 0xff) << 8) +  (event->cec.body[2] & 0xff);
                break;
            case CEC_MESSAGE_DEVICE_VENDOR_ID:
                mCecDevice.mTvDeviceVendorId = ((event->cec.body[1] & 0xff) << 16) +  ((event->cec.body[2] & 0xff) << 8) +  (event->cec.body[3] & 0xff);
                break;
            case CEC_MESSAGE_SET_MENU_LANGUAGE:
                handleSetMenuLanguage(event);
                break;
        }
    }
}

void HdmiCecControl::handleOTPMsg(hdmi_cec_event_t* event)
{
    cec_message_t message;
    int initiator = (int)(event->cec.initiator);
    int devPhyAddr = 0;
    int mask = 0xf000;
    int i, portId = 0;
    int offset = 4;
    if (mCecDevice.isTvDeviceType && (event->cec.body[0] == CEC_MESSAGE_ACTIVE_SOURCE)) {
        devPhyAddr = ((event->cec.body[1] & 0xff) << 8) +  (event->cec.body[2] & 0xff);
        for (i = 0; i < mCecDevice.mTotalPort && mCecDevice.mpPortData != NULL; i++) {
            if (mCecDevice.mpPortData[i].physical_address == (devPhyAddr & mask))
                portId = mCecDevice.mpPortData[i].port_id;
        }
        mLock.lock();
        ALOGD ("#receive <Active Source> from: %d, phyAddress: %02x, mFilterOtpEnabled: %x, mSelectedDeviceId: %d, portId: %d",
            initiator, devPhyAddr, mCecDevice.mFilterOtpEnabled, mCecDevice.mSelectedDeviceId, portId);
        if (mCecDevice.mFilterOtpEnabled
            && mCecDevice.mSelectedDeviceId != (portId + offset)) {
            event->eventType &= ~HDMI_EVENT_CEC_MESSAGE;
            ALOGD("[hcc] handleOTPMsg filtered, continue.");
        } else {
            mCecDevice.mActiveLogicalAddr = initiator;
            mCecDevice.mActiveRoutingPath = devPhyAddr;
        }
        mLock.unlock();
    }
}

void HdmiCecControl::handleSetMenuLanguage(hdmi_cec_event_t* event)
{
    // Todo the language control logic should be comprized in system_server
    if (!mSystemControl->getPropertyBoolean("persist.vendor.sys.cec.set_menu_language", true)) {
        event->eventType &= ~HDMI_EVENT_CEC_MESSAGE;
        ALOGE("Auto Language Change disable");
        return;
    }

    int language = ((event->cec.body[1] & 0xff) << 16) + ((event->cec.body[2] & 0xff) << 8) +  (event->cec.body[3] & 0xff);
    ALOGD("handleSetMenuLanguage tv vendorId %x lang %x", mCecDevice.mTvDeviceVendorId, language);
    // Compat for the vendors which use different code for "zho"
    compatLangIfneeded(mCecDevice.mTvDeviceVendorId, language, event);
}


void HdmiCecControl::handleHotplug(int port, bool connected)
{
    //ALOGD ("handleHotplug port %d connected %d", port, connected);
    if (mCecDevice.isPlaybackDeviceType && (connected != 0)) {
        getDeviceExtraInfo(1);
    } else if (mCecDevice.isTvDeviceType && (connected != 0)) {
        mLock.lock();
        mCecDevice.mWaitEdidUpdatedFinished = false;
        mLock.unlock();
        CMessage msg;
        msg.mType = HdmiCecControl::MsgHandler::MSG_WAIT_EDIDUPDATE_FINISHED_TIMEOUT;
        msg.mDelayMs = DELAY_TIMEOUT_MS*2/7;
        mMsgHandler.removeMsg (msg);
        mMsgHandler.sendMsg (msg);
    }
}

void HdmiCecControl::supportForEARC(hdmi_cec_event_t* event, cec_message_t* message, bool send)
{
    bool supportEARC = false;
    int opcode = 0;
    if (send && message != NULL) {
        opcode = message->body[0] & 0xff;
    } else if (!send && event != NULL) {
        opcode = event->cec.body[0] & 0xff;
    }
    switch (event->cec.body[0]) {
        case CEC_MESSAGE_INITIATE_ARC:
        case CEC_MESSAGE_REPORT_ARC_INITIATED:
        case CEC_MESSAGE_REPORT_ARC_TERMINATED:
        case CEC_MESSAGE_REQUEST_ARC_INITIATION:
        case CEC_MESSAGE_REQUEST_ARC_TERMINATION:
        case CEC_MESSAGE_TERMINATE_ARC:
            if (pMonitor != NULL) {
                supportEARC = pMonitor->getEARCSupport();
            }
            if (supportEARC) {
                if (send) {
                    message->body[0] = CEC_MESSAGE_FEATURE_ABORT;
                } else {
                    event->eventType &= ~HDMI_EVENT_CEC_MESSAGE;
                }
            }
            break;
    }
}

void HdmiCecControl::getDeviceExtraInfo(int flag)
{
    CMessage msg;
    msg.mType = HdmiCecControl::MsgHandler::GIVE_OSD_NAEM;
    msg.mDelayMs = DELAY_TIMEOUT_MS*flag;
    mMsgHandler.removeMsg (msg);
    mMsgHandler.sendMsg (msg);
    msg.mType = HdmiCecControl::MsgHandler::MSG_GIVE_DEVICE_VENDOR_ID;
    msg.mDelayMs = DELAY_TIMEOUT_MS*flag;
    mMsgHandler.removeMsg (msg);
    mMsgHandler.sendMsg (msg);
    msg.mType = HdmiCecControl::MsgHandler::GET_MENU_LANGUAGE;
    msg.mDelayMs = DELAY_TIMEOUT_MS*flag*2;
    mMsgHandler.removeMsg (msg);
    mMsgHandler.sendMsg (msg);
}

void HdmiCecControl::checkConnectStatus()
{
    unsigned int index, prevStatus, bit;
    int i, port, connect, ret;
    hdmi_cec_event_t event;

    prevStatus = mCecDevice.mConnectStatus;
    for (i = 0; i < mCecDevice.mTotalPort && mCecDevice.mpPortData != NULL; i++) {
        port = mCecDevice.mpPortData[i].port_id;
        if (mCecDevice.mTotalPort == 1 && mCecDevice.mDeviceTypes[0] == DEV_TYPE_PLAYBACK) {
            //playback for tx hotplug para is always 0
            port = 0;
        }
        connect = port;
        ret = ioctl(mCecDevice.mFd, CEC_IOC_GET_CONNECT_STATUS, &connect);
        if (ret) {
            ALOGE("[hcc] get port %d connected status failed, ret:%d\n", mCecDevice.mpPortData[i].port_id, ret);
            continue;
        }
        bit = prevStatus & (1 << port);
        if (bit ^ ((connect ? 1 : 0) << port)) {//connect status has changed
            ALOGD("[hcc] port:%d, connect status changed, now:%d, prevStatus:%x\n",
                    mCecDevice.mpPortData[i].port_id, connect, prevStatus);
            if (mEventListener != NULL && mCecDevice.isCecEnabled && mCecDevice.isCecControlled) {
                event.eventType = HDMI_EVENT_HOT_PLUG;
                event.hotplug.connected = connect;
                event.hotplug.port_id = mCecDevice.mpPortData[i].port_id;
                mEventListener->onEventUpdate(&event);
                handleHotplug(mCecDevice.mpPortData[i].port_id, connect);
            }
            prevStatus &= ~(bit);
            prevStatus |= ((connect ? 1 : 0) << port);
            //ALOGD("[hcc] now mask:%x\n", prevStatus);
        }
    }
    mCecDevice.mConnectStatus = prevStatus;
}

int HdmiCecControl::readMessage(unsigned char *buf, int msgCount)
{
    if (msgCount <= 0 || !buf) {
        return 0;
    }

    int ret = -1;
    /* maybe blocked at driver */
    ret = read(mCecDevice.mFd, buf, msgCount);
    if (ret < 0) {
        ALOGE("[hcc] read :%s failed, ret:%d\n", CEC_FILE, ret);
        return -1;
    }

    return ret;
}

void HdmiCecControl::getPortInfos(hdmi_port_info_t* list[], int* total)
{
    if (assertHdmiCecDevice())
        return;

    ioctl(mCecDevice.mFd, CEC_IOC_GET_PORT_NUM, total);

    ALOGD("[hcc] total port:%d", *total);
    if (*total > MAX_PORT)
        *total = MAX_PORT;

    if (NULL != mCecDevice.mpPortData)
        delete mCecDevice.mpPortData;
    mCecDevice.mpPortData = new hdmi_port_info[*total];
    if (!mCecDevice.mpPortData) {
        ALOGE("[hcc] alloc port_data failed");
        *total = 0;
        return;
    }

    ioctl(mCecDevice.mFd, CEC_IOC_GET_PORT_INFO, mCecDevice.mpPortData);

    for (int i = 0; i < *total; i++) {
        ALOGD("[hcc] portId: %d, type:%s, cec support:%d, arc support:%d, physical address:%x",
                mCecDevice.mpPortData[i].port_id,
                mCecDevice.mpPortData[i].type ? "output" : "input",
                mCecDevice.mpPortData[i].cec_supported,
                mCecDevice.mpPortData[i].arc_supported,
                mCecDevice.mpPortData[i].physical_address);
    }

    *list = mCecDevice.mpPortData;
    mCecDevice.mTotalPort = *total;
}

int HdmiCecControl::addLogicalAddress(cec_logical_address_t address)
{
    if (assertHdmiCecDevice())
        return -EINVAL;

    if (address < CEC_ADDR_BROADCAST) {
        mCecDevice.mLogicalAddress = address;
        mCecDevice.mAddrBitmap |= (1 << address);
    }

    ALOGD("[hcc] addr:%x, bitmap:%x\n", mCecDevice.mLogicalAddress, mCecDevice.mAddrBitmap);
    return ioctl(mCecDevice.mFd, CEC_IOC_ADD_LOGICAL_ADDR, address);
}

void HdmiCecControl::clearLogicaladdress()
{
    ALOGD("clearLogicaladdress");
    if (assertHdmiCecDevice())
        return;

    mCecDevice.mAddrBitmap = (1 << CEC_ADDR_BROADCAST);
    mCecDevice.mLogicalAddress = CEC_ADDR_BROADCAST;
    if (mCecDevice.mExtendControl) {
        mCecDevice.mExtendControl &= ~(0x02);
    }

    ioctl(mCecDevice.mFd, CEC_IOC_CLR_LOGICAL_ADDR, 0);
}

void HdmiCecControl::setOption(int flag, int value)
{
    if (assertHdmiCecDevice())
        return;

    int ret = -1;
    int wakeReason = 0;
    switch (flag) {
        case HDMI_OPTION_ENABLE_CEC:
            //ret = ioctl(mCecDevice.mFd, CEC_IOC_SET_OPTION_ENALBE_CEC, value);
            mCecDevice.isCecEnabled = (value == 1) ? true : false;
            if (!mCecDevice.isCecEnabled) {
                ioctl(mCecDevice.mFd, CEC_IOC_SET_OPTION_WAKEUP, value);
                ioctl(mCecDevice.mFd, CEC_IOC_SET_AUTO_DEVICE_OFF, value);
            } else {
                optionInit();
            }
            handleHdmiHpdStatus(HDMI_OPTION_ENABLE_CEC);
            break;

        case HDMI_OPTION_WAKEUP:
            ret = ioctl(mCecDevice.mFd, CEC_IOC_SET_OPTION_WAKEUP, value);
            mSystemControl->setProperty("persist.vendor.sys.cec.autowakeup", (value == 1) ? "true" : "false");
            handleHdmiHpdStatus(HDMI_OPTION_WAKEUP);
            break;

        case HDMI_OPTION_SYSTEM_CEC_CONTROL:
            ret = ioctl(mCecDevice.mFd, CEC_IOC_SET_OPTION_SYS_CTRL, value);
            mCecDevice.isCecEnabled = (value == 1 && !mCecDevice.isCecEnabled) ? true : mCecDevice.isCecEnabled;
            mCecDevice.isCecControlled = (value == 1) ? true : false;
            if (mCecDevice.isCecControlled) {
                optionInit();
            } else {
                if (!mCecDevice.isCecEnabled) {
                    ioctl(mCecDevice.mFd, CEC_IOC_SET_OPTION_WAKEUP, value);
                    ioctl(mCecDevice.mFd, CEC_IOC_SET_AUTO_DEVICE_OFF, value);
                }
            }
            handleHdmiHpdStatus(HDMI_OPTION_SYSTEM_CEC_CONTROL);
            break;

        /* set device auto-power off by uboot */
        case HDMI_OPTION_CEC_AUTO_DEVICE_OFF:
            ret = ioctl(mCecDevice.mFd, CEC_IOC_SET_AUTO_DEVICE_OFF, value);
            mSystemControl->setProperty("persist.vendor.sys.cec.deviceautopoweroff", (value == 1) ? "true" : "false");
            break;

        case HDMI_OPTION_SET_LANG:
            ret = ioctl(mCecDevice.mFd, CEC_IOC_SET_OPTION_SET_LANG, value);
            break;

        default:
            break;
    }
    ALOGD("[hcc] %s, flag:0x%x, value:0x%x, ret:%d, isCecControlled:%x", __FUNCTION__, flag, value, ret, mCecDevice.isCecControlled);
}

void HdmiCecControl::setAudioReturnChannel(int port, bool flag)
{
    if (assertHdmiCecDevice())
        return;

    int ret = ioctl(mCecDevice.mFd, CEC_IOC_SET_ARC_ENABLE, flag);
    ALOGD("[hcc] %s, port id:%d, flag:%x, ret:%d\n", __FUNCTION__, port, flag, ret);
}

bool HdmiCecControl::isConnected(int port)
{
    if (assertHdmiCecDevice())
        return false;

    int status = -1, ret;
    /* use status pass port id */
    status = port;
    ret = ioctl(mCecDevice.mFd, CEC_IOC_GET_CONNECT_STATUS, &status);
    if (ret)
        return false;
    ALOGD("[hcc] %s, port:%d, connected:%s", __FUNCTION__, port, status ? "yes" : "no");
    return status;
}

int HdmiCecControl::getVersion(int* version)
{
    if (assertHdmiCecDevice())
        return -1;

    int ret = ioctl(mCecDevice.mFd, CEC_IOC_GET_VERSION, version);
    ALOGD("[hcc] %s, version:%x, ret = %d", __FUNCTION__, *version, ret);
    return ret;
}

int HdmiCecControl::getVendorId(uint32_t* vendorId)
{
    if (assertHdmiCecDevice())
        return -1;

    int ret = ioctl(mCecDevice.mFd, CEC_IOC_GET_VENDOR_ID, vendorId);
    ALOGD("[hcc] %s, vendorId: %x, ret = %d", __FUNCTION__, *vendorId, ret);
    return ret;
}

int HdmiCecControl::getPhysicalAddress(uint16_t* addr)
{
    if (assertHdmiCecDevice())
        return -EINVAL;
    if (mCecDevice.isTvDeviceType) {
        *addr= 0x0;
        //ALOGD("[hcc] return for tv physical addr ");
        return 0;
    }
    if (mCecDevice.isPlaybackDeviceType) {
        if ((mCecDevice.mConnectStatus & (1)) == 0) {
            *addr= 0x0;
            ALOGD("[hcc] return for playback physical addr ");
            return 0;
        }
    }
    int ret = ioctl(mCecDevice.mFd, CEC_IOC_GET_PHYSICAL_ADDR, addr);
    ALOGD("[hcc] %s, physical addr: %x, ret = %d", __FUNCTION__, *addr, ret);
    return ret;
}

/*
 * checkWakedByCec
 * When system wakes up, it will enable the cec control and it's the timing that we could check
 * If the system is waked up by cec devices. In consideration of the situation where one more devices
 * Connected to tv and it is waked up by the playback which is not the current channel, tv should
 * Not do the route control started by PortSelectRequestBuffer to set stream path to the previous channel.
*/
int HdmiCecControl::getCecWakePort() {
    int wakeReason = 0;
    int wakePort = -1;
    ioctl(mCecDevice.mFd, CEC_IOC_GET_BOOT_REASON, &wakeReason);
    wakeReason = wakeReason & 0xff;
    if (wakeReason != CEC_WAKEUP) {
        return -1;
    }
    // wakeup address is 0xb1000.
    ioctl(mCecDevice.mFd, CEC_IOC_GET_BOOT_ADDR, &wakePort);
    wakePort = (wakePort >> 12) & 0xf;
    ALOGD("getCecWakePort reason %x port %x", wakeReason, wakePort);
    // there should be only 4 ports for tv now.
    if (wakePort > 0 && wakePort < 5) {
        return wakePort;
    }
    return -1;
}

int HdmiCecControl::sendMessage(const cec_message_t* message, bool isExtend)
{
    int initiator = message->initiator;
    int dest = message->destination;
    int opcode = message->body[0] & 0xff;
    //ALOGD("sendMessage initiator %d dest %d opcode %d", initiator, dest, opcode);

    int ret;
    if (assertHdmiCecDevice()) {
        ALOGE("sendMessage not valid cec device!");
        return -EINVAL;
    }

    if (!mCecDevice.isCecEnabled) {
        ALOGE("sendMessage cec not enabled!");
        return -EINVAL;
    }
    if (isExtend) {
        //ALOGD("[hcc] isExtend = %d, mExtendControl = %d", isExtend, mCecDevice.mExtendControl);
        return sendExtMessage(message);
    }
    /* don't send message if controlled by extend */
    if (mCecDevice.mExtendControl == 0x03 && hasHandledByExtend(message)) {
        return HDMI_RESULT_SUCCESS;
    }
    if (preHandleOfSend(message) < 0) {
        return HDMI_RESULT_SUCCESS;
    }
    ret = send(message);
    postHandleOfSend(message, ret);
    return ret;
}

int HdmiCecControl::sendMessage(int initiator, int destination, int length, cec_message_type type)
{
    cec_message_t message;
    message.initiator = (cec_logical_address_t)initiator;
    message.destination = (cec_logical_address_t)destination;
    message.length = length;
    message.body[0] = type;
    return send(&message);
}

int HdmiCecControl::preHandleOfSend(const cec_message_t* message)
{
    int ret = 0;
    int initiator, dest, opcode, para, value, avrAddr, index;
    hdmi_cec_event_t event;
    CMessage msg;
    dest = message->destination;
    opcode = message->body[0] & 0xff;
    switch (opcode) {
        case CEC_MESSAGE_ROUTING_CHANGE:
        case CEC_MESSAGE_SET_STREAM_PATH:
            if (opcode == CEC_MESSAGE_ROUTING_CHANGE)
                para = ((message->body[3] & 0xff) << 8) + (message->body[4] & 0xff);
            else if (opcode == CEC_MESSAGE_SET_STREAM_PATH) {
                para = ((message->body[1] & 0xff) << 8) + (message->body[2] & 0xff);
            }
            avrAddr = mCecDevice.mAddedPhyAddrs[DEV_TYPE_AUDIO_SYSTEM];
            if (avrAddr == para) {
                ALOGD("[hcc] tv should not send Set Stream Path & Routing Change to avr which does not have sub device and connect to arc port.");
                for (index = 0; index < mCecDevice.mTotalPort && mCecDevice.mpPortData != NULL; index++) {
                    if (mCecDevice.mpPortData[index].physical_address == avrAddr && mCecDevice.mpPortData[index].arc_supported) {
                        ret = -1;
                        ALOGD("[hcc] Avr connect to arc port.");
                        break;
                    }
                }
                for (index = 0; index < ADDR_BROADCAST; index++) {
                    value = mCecDevice.mAddedPhyAddrs[index];
                    if ((index != DEV_TYPE_AUDIO_SYSTEM) && (value - avrAddr > 0) && ((value & 0xf000) == (avrAddr & 0xf000))) {
                        ret = 0;
                        ALOGD("[hcc] Avr has sub device.");
                        break;
                    }
                }
            }
            if (opcode == CEC_MESSAGE_ROUTING_CHANGE) {
                //wake up all the current port device when change to.
                for (index = 0; index < ADDR_BROADCAST; index++) {
                    value = mCecDevice.mAddedPhyAddrs[index];
                    if ((para != 0) && ((value & 0xf000) == (para & 0xf000)) && (value - para >= 0)) {
                        dest = index;
                        msg.mType = HdmiCecControl::MsgHandler::MSG_TURN_ON_DEVICE;
                        msg.mDelayMs = DELAY_TIMEOUT_MS*2/7;
                        msg.mpPara[0] = dest;
                        //mMsgHandler.removeMsg (msg);
                        mMsgHandler.sendMsg (msg);
                        break;
                    }
                }
            }
            break;
        case CEC_MESSAGE_USER_CONTROL_PRESSED:
            para = (message->body[1] & 0xff);
            if (dest != mCecDevice.mActiveLogicalAddr && (para >= CEC_KEYCODE_UP && para <= CEC_KEYCODE_RIGHT)) {
                //send user control press to sub device of avr
                msg.mType = HdmiCecControl::MsgHandler::MSG_USER_CONTROL_PRESSED;
                msg.mDelayMs = DELAY_TIMEOUT_MS/14;
                msg.mpPara[0] = message->initiator;
                msg.mpPara[1] = mCecDevice.mActiveLogicalAddr;
                msg.mpPara[2] = message->body[1] & 0xff;
                mMsgHandler.sendMsg (msg);
            }
            break;
        case CEC_MESSAGE_GIVE_PHYSICAL_ADDRESS:
            if (dest == DEV_TYPE_AUDIO_SYSTEM) {
                /*there are two case:
                1. avr report physical address maybe wrong when edid updated not finished after hotplug
                2. time between right and wrong physical address too short may lead arc connect not succes*/
                mLock.lock();
                if (!mCecDevice.mWaitEdidUpdatedFinished) {
                    msg.mType = HdmiCecControl::MsgHandler::MSG_GIVE_PHYSICAL_ADDRESS;
                    msg.mDelayMs = DELAY_TIMEOUT_MS*2/7;
                    msg.mpPara[0] = dest;
                    mMsgHandler.removeMsg (msg);
                    mMsgHandler.sendMsg (msg);
                    ret = -1;
                }
                mLock.unlock();
            }
            break;
        case CEC_MESSAGE_STANDBY:
            if (!mSystemControl->getPropertyBoolean("persist.vendor.sys.cec.deviceautopoweroff", false)) {
                ALOGD("[hcc] filter <Standby>.");
                ret = -1;
            }
            break;
        case CEC_MESSAGE_SET_MENU_LANGUAGE:
            if (!mSystemControl->getPropertyBoolean("persist.vendor.sys.cec.set_menu_language", true)) {
                ALOGD("[hcc] filter <Set Menu Language>.");
                ret = -1;
            }
            break;
        default:
            break;
    }
    return ret;
}

int HdmiCecControl::postHandleOfSend(const cec_message_t* message, int result)
{
    int ret = 0;
    int initiator, dest, opcode, len;
    len = message->length;
    dest = message->destination;
    if (len == 0 && result != SUCCESS && mCecDevice.mAddedPhyAddrs[dest] != 0) {
        mCecDevice.mAddedPhyAddrs[dest] = 0;
        ALOGE("[hcc] Polling %d fail, reset mCecDevice.mAddedPhyAddrs[%d].", dest, dest);
    }
    return ret;
}

void HdmiCecControl::turnOnDevice(int logicalAddress)
{
    cec_message_t message;
    message.initiator = (cec_logical_address_t)mCecDevice.mLogicalAddress;
    message.destination = (cec_logical_address_t)logicalAddress;
    message.body[0] = CEC_MESSAGE_USER_CONTROL_PRESSED;
    message.body[1] = (CEC_KEYCODE_POWER_ON_FUNCTION & 0xff);
    message.length = 2;
    send(&message);
    message.body[0] = CEC_MESSAGE_USER_CONTROL_RELEASED;
    message.length = 1;
    send(&message);
    ALOGD("[hcc] send wakeUp message.");
}

void HdmiCecControl::optionInit()
{
    /*
    some option must be inited when started, these value maybe stored in java setting,
    I just do like this , it is better to do by reading propety which sync with java setting
   */
    setOption(HDMI_CONTROL_AUTO_WAKEUP_ENABLED, mSystemControl->getPropertyBoolean("persist.vendor.sys.cec.autowakeup", true));
    // The default value of this switch is off
    setOption(HDMI_CONTROL_AUTO_DEVICE_OFF_ENABLED, mSystemControl->getPropertyBoolean("persist.vendor.sys.cec.deviceautopoweroff", false));
}

void HdmiCecControl::handleHdmiHpdStatus(int flag)
{
    if (mCecDevice.isTvDeviceType) {
        int autowakeupEnabled = 0;
        char value[4] = {0};
        autowakeupEnabled = mSystemControl->getPropertyBoolean("persist.vendor.sys.cec.autowakeup", false);
        if ((flag == HDMI_OPTION_ENABLE_CEC)
            || (flag == HDMI_OPTION_WAKEUP)) {
            if (mCecDevice.isCecEnabled) {
                if (autowakeupEnabled) {
                    strcpy(value, AUTO_WAKEUP_ON_CEC_ON);
                } else {
                    strcpy(value, AUTO_WAKEUP_OFF_CEC_ON);
                }
            } else {
                if (autowakeupEnabled) {
                    strcpy(value, AUTO_WAKEUP_ON_CEC_OFF);
                } else {
                    strcpy(value, AUTO_WAKEUP_OFF_CEC_OFF);
                }
            }
        }
        if ((flag == HDMI_OPTION_SYSTEM_CEC_CONTROL)
            && (mCecDevice.isCecControlled)) {
            if (mCecDevice.isCecEnabled) {
                if (autowakeupEnabled) {
                    strcpy(value, AUTO_WAKEUP_ON_CEC_ON_BOOT);
                } else {
                    strcpy(value, AUTO_WAKEUP_OFF_CEC_ON_BOOT);
                }
            }
        }
        mSystemControl->writeSysfs(HDMIRX_SYSFS, value);
        ALOGD("[hcc] handleHdmiHpdStatus %s", value);
    }
}

bool HdmiCecControl::hasHandledByExtend(const cec_message_t* message)
{
    bool ret = false;
    int opcode = message->body[0] & 0xff;
    switch (opcode) {
        case CEC_MESSAGE_SET_MENU_LANGUAGE:
        case CEC_MESSAGE_INACTIVE_SOURCE:
            ret = true;
            break;
        default:
            break;
    }
    return ret;
}

int HdmiCecControl::sendExtMessage(const cec_message_t* message)
{
    int i, addr;
    cec_message_t msg;
    if (!mCecDevice.isTvDeviceType) {
        addr = mCecDevice.mAddrBitmap;
        addr &= 0x7fff;
        for (i = 0; i < 15; i++) {
            if (addr & 1) {
                break;
            }
            addr >>= 1;
        }
        msg.initiator = (cec_logical_address_t) i;
    } else {
        msg.initiator = CEC_ADDR_TV; /* root for TV */
    }
    if (msg.initiator == CEC_ADDR_UNREGISTERED)//source can not be 0xf
        return -1;
    msg.destination = message->destination;
    msg.length = message->length;
    memcpy(msg.body, message->body, msg.length);
    return send(&msg);
}

int HdmiCecControl::send(const cec_message_t* message)
{
    AutoMutex _l(mSendLock);
    unsigned char msgBuf[CEC_MESSAGE_BODY_MAX_LENGTH];
    int ret = -1;
    memset(msgBuf, 0, sizeof(msgBuf));
    msgBuf[0] = ((message->initiator & 0xf) << 4) | (message->destination & 0xf);
    memcpy(msgBuf + 1, message->body, message->length);
    ret = write(mCecDevice.mFd, msgBuf, message->length + 1);
    //printCecMessage(message, ret);
    return ret;
}

bool HdmiCecControl::assertHdmiCecDevice()
{
    return mCecDevice.mFd < 0;
}

void HdmiCecControl::setEventObserver(const sp<HdmiCecEventListener> &eventListener)
{
    mEventListener = eventListener;
}

};//namespace android
