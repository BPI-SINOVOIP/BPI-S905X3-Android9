/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC
 */

#define LOG_NDEBUG 0
#define LOG_CEE_TAG "HdmiCecService"

#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>

#include "HdmiCecService.h"

namespace android {

void HdmiCecService::instantiate()
{
    android::status_t ret = defaultServiceManager()->addService(
            String16("hdmi_cec"), new HdmiCecService());

    if (ret != android::OK) {
        LOGE("Couldn't register hdmi cec service!");
    }

    LOGI("instantiate add hdmi cec service result:%d", ret);
}

HdmiCecService::HdmiCecService()
{
    mHdmiCecControl = new HdmiCecControl();
    mHdmiCecControl->setEventObserver(this);
}

HdmiCecService::~HdmiCecService()
{
    delete mHdmiCecControl;
}

int HdmiCecService::openCecDevice()
{
    if (NULL != mHdmiCecControl) {
        return mHdmiCecControl->openCecDevice();
    }
    return -1;
}

int HdmiCecService::closeCecDevice()
{
    if (NULL != mHdmiCecControl) {
        return mHdmiCecControl->closeCecDevice();
    }
    return -1;
}

int HdmiCecService::getVersion(int* version)
{
    if (NULL != mHdmiCecControl) {
        return mHdmiCecControl->getVersion(version);
    }
    return -1;
}

int HdmiCecService::getVendorId(uint32_t* vendorId)
{
    if (NULL != mHdmiCecControl) {
        return mHdmiCecControl->getVendorId(vendorId);
    }
    return -1;
}

int HdmiCecService::getPhysicalAddress(uint16_t* addr)
{
    if (NULL != mHdmiCecControl) {
        return mHdmiCecControl->getPhysicalAddress(addr);
    }
    return -1;
}

int HdmiCecService::sendMessage(const cec_message_t* message, bool isExtend)
{
    if (NULL != mHdmiCecControl) {
        return mHdmiCecControl->sendMessage(message, isExtend);
    }
    return -1;
}

bool HdmiCecService::isConnected(int port)
{
    if (NULL != mHdmiCecControl) {
        return mHdmiCecControl->isConnected(port);
    }
    return false;
}

void HdmiCecService::setAudioReturnChannel(int port, bool flag)
{
    if (NULL != mHdmiCecControl) {
        mHdmiCecControl->setAudioReturnChannel(port, flag);
    }
}

void HdmiCecService::setOption(int flag, int value)
{
    if (NULL != mHdmiCecControl) {
        mHdmiCecControl->setOption(flag, value);
    }
}

void HdmiCecService::clearLogicaladdress()
{
    if (NULL != mHdmiCecControl) {
        mHdmiCecControl->clearLogicaladdress();
    }
}

int HdmiCecService::addLogicalAddress(cec_logical_address_t address)
{
    if (NULL != mHdmiCecControl) {
        return mHdmiCecControl->addLogicalAddress(address);
    }
    return -1;
}

void HdmiCecService::getPortInfos(hdmi_port_info_t* list[], int* total)
{
    if (NULL != mHdmiCecControl) {
        mHdmiCecControl->getPortInfos(list, total);
    }
}

int HdmiCecService::connect(const sp<IHdmiCecCallback> &client)
{
    if (client != NULL) {
        mClients.add(client);
    }
    return -1;
}

void HdmiCecService::onEventUpdate(const hdmi_cec_event_t* event)
{
    LOGV("%s, %s", __FUNCTION__, getEventType(event->eventType));
    int clientSize = mClients.size();
    for (int i = 0; i < clientSize; i++) {
        mClients[i]->notifyCallback(event);
    }
}

void HdmiCecService::dumpHelp(String8 &result)
{
    result.appendFormat(
        "hdmi_cec service use to control the hdmi cec.\n"
        "usage: \n"
        "    dumpsys hdmi_cec -l [value] \n"
        "    -l: set or get debug level \n"
        "    -h: help \n");
}

status_t HdmiCecService::dump(int fd, const Vector<String16>& args)
{
    const size_t SIZE = 256;
    char buffer[SIZE];
    String8 result;
    if (false/*checkCallingPermission(String16("android.permission.DUMP")) == false*/) {
        snprintf(buffer, SIZE, "Permission Denial: "
            "can't dump hdmi_cec from pid=%d, uid=%d\n",
                IPCThreadState::self()->getCallingPid(), IPCThreadState::self()->getCallingUid());
        result.append(buffer);
    } else {
        Mutex::Autolock lock(mLock);

        int len = args.size();

        for (int i = 0; i < len; i++) {
            String16 debugLevel("-l");
            String16 help("-h");
            if (args[i] == debugLevel) {
                if (i + 1 < len) {
                    String8 levelStr(args[i + 1]);
                    int level = atoi(levelStr.string());
                    setLogLevel(level);

                    result.appendFormat("Setting log level to %d\n", level);
                    break;
                } else {
                    result.appendFormat("current log level is %d\n", getLogLevel());
                    break;
                }
            } else if (args[i] == help) {
                dumpHelp(result);
                break;
            }
        }
        if (len <= 0) {
            dumpHelp(result);
        }
    }

    write(fd, result.string(), result.size());
    return NO_ERROR;
}

};//namespace android
