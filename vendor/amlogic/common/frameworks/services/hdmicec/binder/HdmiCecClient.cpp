/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: C++ file
 */

#define LOG_NDEBUG 0
#define LOG_CEE_TAG "HdmiCecClient"

#include <binder/IServiceManager.h>
#include <utils/Log.h>

#include "HdmiCecClient.h"

namespace android {

Mutex HdmiCecClient::mLock;
sp<IHdmiCecService> HdmiCecClient::mHdmiCecService;
sp<HdmiCecClient::DeathNotifier> HdmiCecClient::mDeathNotifier;

HdmiCecClient::HdmiCecClient()
{
    getHdmiCecService();
}

HdmiCecClient::~HdmiCecClient()
{
}

sp<HdmiCecClient> HdmiCecClient::connect()
{
    sp<HdmiCecClient> client = new HdmiCecClient();
    if (mHdmiCecService != NULL) {
        mHdmiCecService->connect(client);
    }
    return client;
}

int HdmiCecClient::openCecDevice()
{
    if (mHdmiCecService != NULL)
        return mHdmiCecService->openCecDevice();
    return -1;
}

int HdmiCecClient::closeCecDevice()
{
    if (mHdmiCecService != NULL)
        return mHdmiCecService->closeCecDevice();
    return -1;
}

void HdmiCecClient::getPortInfos(hdmi_port_info_t* list[], int* total)
{
    if (mHdmiCecService != NULL)
        mHdmiCecService->getPortInfos(list, total);
}

int HdmiCecClient::addLogicalAddress(cec_logical_address_t address)
{
    if (mHdmiCecService != NULL)
        return mHdmiCecService->addLogicalAddress(address);
    return -1;
}

void HdmiCecClient::clearLogicaladdress()
{
    if (mHdmiCecService != NULL)
        mHdmiCecService->clearLogicaladdress();
}

void HdmiCecClient::setOption(int flag, int value)
{
    if (mHdmiCecService != NULL)
        mHdmiCecService->setOption(flag, value);
}

void HdmiCecClient::setAudioReturnChannel(int port, bool flag)
{
    if (mHdmiCecService != NULL)
        mHdmiCecService->setAudioReturnChannel(port, flag);
}

bool HdmiCecClient::isConnected(int port)
{
    if (mHdmiCecService != NULL)
        return mHdmiCecService->isConnected(port);
    return false;
}

int HdmiCecClient::getVersion(int* version)
{
    if (mHdmiCecService != NULL)
        return mHdmiCecService->getVersion(version);
    return 0;
}

int HdmiCecClient::getVendorId(uint32_t* vendorId)
{
    if (mHdmiCecService != NULL)
        return mHdmiCecService->getVendorId(vendorId);
    return 0;
}

int HdmiCecClient::getPhysicalAddress(uint16_t* addr)
{
    if (mHdmiCecService != NULL)
        return mHdmiCecService->getPhysicalAddress(addr);
    return 0;
}

int HdmiCecClient::sendMessage(const cec_message_t* message, bool isExtend)
{
    if (mHdmiCecService != NULL)
        return mHdmiCecService->sendMessage(message, isExtend);
    return 0;
}

const sp<IHdmiCecService> HdmiCecClient::getHdmiCecService()
{
    Mutex::Autolock _l(mLock);
    #if 0 //after O need use hwbinder
    if (mHdmiCecService.get() == 0) {
        sp<IServiceManager> sm = defaultServiceManager();
        sp<IBinder> binder;
        do {
            binder = sm->getService(String16("hdmi_cec"));
            if (binder != 0)
                break;

            ALOGW("Hdmi cec not published, waiting...");
            usleep(500000); // 0.5 s
        } while (true);

        if (mDeathNotifier == NULL) {
            mDeathNotifier = new DeathNotifier();
        }

        binder->linkToDeath(mDeathNotifier);
        mHdmiCecService = interface_cast<IHdmiCecService> (binder);
    }
    #endif
    if (mHdmiCecService == NULL)
        ALOGW("wrong... can't get hdmi cec service");
    return mHdmiCecService;
}

void HdmiCecClient::notifyCallback(const hdmi_cec_event_t* event)
{
    if (mEventListener != NULL) {
        mEventListener->onEventUpdate(event);
    }
}

void HdmiCecClient::setEventObserver(const sp<HdmiCecEventListener> &eventListener)
{
    mEventListener = eventListener;
}

void HdmiCecClient::DeathNotifier::binderDied(const wp<IBinder>& who __unused)
{
    LOGW("hdmi cec died.");
    Mutex::Autolock _l(mLock);
    mHdmiCecService.clear();
    LOGE("kill myself");
    kill(getpid(), SIGKILL);
}

};//namespace android
