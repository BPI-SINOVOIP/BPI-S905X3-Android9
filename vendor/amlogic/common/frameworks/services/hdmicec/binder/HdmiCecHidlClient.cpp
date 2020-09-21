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
 *  - 1 droidlogic hdmi cec hwbinder client
 */

#define LOG_TAG "hdmicecd-client"

#include <utils/RefBase.h>
#include <binder/IServiceManager.h>
#include <utils/Log.h>

#include "HdmiCecHidlClient.h"

namespace android {

Mutex HdmiCecHidlClient::mLock;

HdmiCecHidlClient::HdmiCecHidlClient(cec_connect_type_t type): mType(type)
{
    mHdmiCecService = getHdmiCecService();
    if (type != CONNECT_TYPE_POWER) {
        mHdmiCecHidlCallback = new HdmiCecHidlCallback(this);
        Return<void> ret = mHdmiCecService->setCallback(mHdmiCecHidlCallback, static_cast<ConnectType>(type));
    }
}

HdmiCecHidlClient::~HdmiCecHidlClient()
{
    if (nullptr != mpPortInfo)
        delete mpPortInfo;
}

HdmiCecHidlClient* HdmiCecHidlClient::connect(cec_connect_type_t type)
{
    //sp<HdmiCecHidlClient> client = new HdmiCecHidlClient();
    //return client;
    return new HdmiCecHidlClient(type);
}

void HdmiCecHidlClient::reconnect()
{
    ALOGI("hdmi cec client type:%d reconnect", mType);
    mHdmiCecService.clear();
    //reconnect to server
    mHdmiCecService = getHdmiCecService();
    mHdmiCecService->setCallback(mHdmiCecHidlCallback, static_cast<ConnectType>(mType));
}

int HdmiCecHidlClient::openCecDevice()
{
    int fd = -1;
    Return<void> ret = mHdmiCecService->openCecDevice([&fd](Result result, int32_t addr) {
            if (Result::SUCCESS == result) {
                fd = addr;
            }
        });

    ALOGI("open cec device fd:%d", fd);
    return fd;
}

int HdmiCecHidlClient::closeCecDevice()
{
    Return<void> ret = mHdmiCecService->closeCecDevice();
    if (!ret.isOk()) {
        ALOGE("Failed to closeCecDevice.");
        return -1;
    }

    return 0;
}

void HdmiCecHidlClient::getPortInfos(hdmi_port_info_t* list[], int* total)
{
    hidl_vec<HdmiPortInfo> ports;
    Return<void> ret = mHdmiCecService->getPortInfo([&ports](hidl_vec<HdmiPortInfo> hidlList) {
            ports = hidlList;
        });

    if (ports.size() <= 0) {
        *total = 0;
        ALOGE("getPortInfos fail");
        return;
    }

    if (nullptr != mpPortInfo)
        delete mpPortInfo;

    mpPortInfo = new hdmi_port_info_t[ports.size()];
    if (!mpPortInfo) {
        ALOGE("alloc port_data failed");
        *total = 0;
        return;
    }

    for (size_t i = 0; i < ports.size(); ++i) {
        /*
        ALOGD("droidhdmicec client port %d, type:%s, id:%d, cec support:%d, arc support:%d, physical address:%x",
                i, (HDMI_OUTPUT==static_cast<hdmi_port_type_t>(ports[i].type)) ? "output" : "input",
                ports[i].portId,
                ports[i].cecSupported?1:0,
                ports[i].arcSupported?1:0,
                ports[i].physicalAddress);*/

        mpPortInfo[i].type = static_cast<hdmi_port_type_t>(ports[i].type);
        mpPortInfo[i].port_id = ports[i].portId;
        mpPortInfo[i].cec_supported = ports[i].cecSupported;
        mpPortInfo[i].arc_supported = ports[i].arcSupported;
        mpPortInfo[i].physical_address = ports[i].physicalAddress;
    }

    *list = mpPortInfo;
    *total = ports.size();
}

int HdmiCecHidlClient::addLogicalAddress(cec_logical_address_t address)
{
    Return<Result> ret = mHdmiCecService->addLogicalAddress(static_cast<CecLogicalAddress>(address));
    if (!ret.isOk()) {
        ALOGE("Failed to add a logical address.");
        return -1;
    }
    return 0;
}

void HdmiCecHidlClient::clearLogicaladdress()
{
    Return<void> ret = mHdmiCecService->clearLogicalAddress();
    if (!ret.isOk()) {
        ALOGE("Failed to clearLogicaladdress.");
    }
}

void HdmiCecHidlClient::setOption(int flag, int value)
{
    Return<void> ret = mHdmiCecService->setOption(static_cast<OptionKey>(flag), value);
    if (!ret.isOk()) {
        ALOGE("Failed to set option.");
    }
}

void HdmiCecHidlClient::setAudioReturnChannel(int port, bool flag)
{
    Return<void> ret = mHdmiCecService->enableAudioReturnChannel(port, flag);
    if (!ret.isOk()) {
        ALOGE("Failed to enable/disable ARC.");
    }
}

bool HdmiCecHidlClient::isConnected(int port)
{
    Return<bool> ret = mHdmiCecService->isConnected(port);
    if (!ret.isOk()) {
        ALOGE("Failed to get connection info.");
    }
    return ret;
}

int HdmiCecHidlClient::getVersion(int* version)
{
    Return<int32_t> ret = mHdmiCecService->getCecVersion();
    if (!ret.isOk()) {
        ALOGE("Failed to get cec version.");
        return -1;
    }
    *version= ret;
    return 0;
}

int HdmiCecHidlClient::getVendorId(uint32_t* vendorId)
{
    Return<uint32_t> ret = mHdmiCecService->getVendorId();
    if (!ret.isOk()) {
        ALOGE("Failed to get vendor id.");
        return -1;
    }

    *vendorId= ret;
    return 0;
}

int HdmiCecHidlClient::getPhysicalAddress(uint16_t* addr)
{
    Result result;
    uint16_t hidlAddr;
    Return<void> ret = mHdmiCecService->getPhysicalAddress([&result, &hidlAddr](Result res, uint16_t paddr) {
            result = res;
            hidlAddr = paddr;
        });
    if (!ret.isOk()) {
        ALOGE("Failed to get physical address.");
        return -1;
    }
    *addr = hidlAddr;
    return 0;

}

int HdmiCecHidlClient::getCecWakePort() {
    Return<int32_t> ret = mHdmiCecService->getCecWakePort();
    if (!ret.isOk()) {
        ALOGE("Failed to get cec wake port.");
        return -1;
    }
    return ret;
}


int HdmiCecHidlClient::sendMessage(const cec_message_t* message, bool isExtend)
{
    CecMessage hidlMsg;
    //change message from hwbinder data structure to needed data structure
    hidlMsg.initiator = static_cast<CecLogicalAddress>(message->initiator);
    hidlMsg.destination = static_cast<CecLogicalAddress>(message->destination);
    hidlMsg.body.resize(message->length);
    for (size_t i = 0; i < message->length; ++i) {
        hidlMsg.body[i] = message->body[i];
    }
    //ALOGE("!!sendMessage:isExtend:%d", isExtend);
    Return<SendMessageResult> ret = mHdmiCecService->sendMessage(hidlMsg, isExtend);
    if (SendMessageResult::SUCCESS == ret)
        return 0;
    else if (SendMessageResult::NACK == ret)
        return 1 ;
    else if (SendMessageResult::BUSY == ret)
        return 2;
    else if (SendMessageResult::FAIL == ret)
        return 3;
    if (!ret.isOk()) {
        ALOGE("Failed to send CEC message.");
        return -1;
    }

    return (SendMessageResult::SUCCESS == ret)?0:-1;
}

sp<IDroidHdmiCEC> HdmiCecHidlClient::getHdmiCecService()
{
    Mutex::Autolock _l(mLock);

#if 1//PLATFORM_SDK_VERSION >= 26
    sp<IDroidHdmiCEC> hdmicec = IDroidHdmiCEC::tryGetService();
    while (hdmicec == nullptr) {
         usleep(200*1000);//sleep 200ms
         hdmicec = IDroidHdmiCEC::tryGetService();
         ALOGE("tryGet hdmicecd daemon Service");
    };
    mDeathRecipient = new HdmiCecDaemonDeathRecipient(this);
    Return<bool> linked = hdmicec->linkToDeath(mDeathRecipient, /*cookie*/ 0);
    if (!linked.isOk()) {
        ALOGE("Transaction error in linking to hdmi cec daemon service death: %s", linked.description().c_str());
    } else if (!linked) {
        ALOGE("Unable to link to hdmi cec daemon service death notifications");
    } else {
        ALOGI("Link to hdmi cec daemon service death notification successful");
    }

#else
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

    if (mHdmiCecService == NULL)
        ALOGW("wrong... can't get hdmi cec service");
    return mHdmiCecService;
#endif

    return hdmicec;
}

void HdmiCecHidlClient::setEventObserver(const sp<HdmiCecEventListener> &eventListener)
{
    mEventListener = eventListener;
}

Return<void> HdmiCecHidlClient::HdmiCecHidlCallback::notifyCallback(const CecEvent& hidlEvent)
{
    ALOGI("notifyCallback event type:%d", hidlEvent.eventType);
    if (cecClient->mEventListener != NULL) {
        hdmi_cec_event_t event;
        event.eventType = hidlEvent.eventType;
        if (HDMI_EVENT_ADD_LOGICAL_ADDRESS == hidlEvent.eventType) {
            event.logicalAddress = hidlEvent.logicalAddress;
        }
        else if (HDMI_EVENT_HOT_PLUG == hidlEvent.eventType) {
            event.hotplug.connected = hidlEvent.hotplug.connected?1:0;
            event.hotplug.port_id = hidlEvent.hotplug.portId;
        }
        //HDMI_EVENT_CEC_MESSAGE or HDMI_EVENT_RECEIVE_MESSAGE
        else if (0 != hidlEvent.eventType) {
            event.cec.initiator = static_cast<cec_logical_address_t>(hidlEvent.cec.initiator);
            event.cec.destination = static_cast<cec_logical_address_t>(hidlEvent.cec.destination);

            event.cec.length = hidlEvent.cec.body.size();
            for (size_t i = 0; i < event.cec.length; i++) {
                 event.cec.body[i] = hidlEvent.cec.body[i];
            }
        }

        cecClient->mEventListener->onEventUpdate(&event);
    }
    return Void();
}

void HdmiCecHidlClient::HdmiCecDaemonDeathRecipient::serviceDied(uint64_t cookie __unused,
        const ::android::wp<::android::hidl::base::V1_0::IBase>& who __unused)
{
    ALOGE("hdmi cec daemon died.");
    Mutex::Autolock _l(mLock);

    usleep(200*1000);//sleep 200ms
    cecClient->reconnect();
}

};//namespace android
