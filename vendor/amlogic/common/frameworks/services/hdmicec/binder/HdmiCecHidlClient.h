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

#ifndef DROIDLOGIC_HDMICEC_HIDL_CLIENT_H_
#define DROIDLOGIC_HDMICEC_HIDL_CLIENT_H_

#include <binder/IBinder.h>
#include <utils/threads.h>

#include "HdmiCecBase.h"
#include "IHdmiCecCallback.h"
#include "IHdmiCecService.h"

#include <vendor/amlogic/hardware/hdmicec/1.0/IDroidHdmiCEC.h>

namespace android {

using ::vendor::amlogic::hardware::hdmicec::V1_0::IDroidHdmiCEC;
using ::vendor::amlogic::hardware::hdmicec::V1_0::IDroidHdmiCecCallback;
using ::vendor::amlogic::hardware::hdmicec::V1_0::ConnectType;
using ::vendor::amlogic::hardware::hdmicec::V1_0::Result;
using ::vendor::amlogic::hardware::hdmicec::V1_0::CecEvent;
using ::vendor::amlogic::hardware::hdmicec::V1_0::HdmiPortInfo;
using ::vendor::amlogic::hardware::hdmicec::V1_0::CecLogicalAddress;
using ::vendor::amlogic::hardware::hdmicec::V1_0::CecMessage;
using ::vendor::amlogic::hardware::hdmicec::V1_0::SendMessageResult;
using ::vendor::amlogic::hardware::hdmicec::V1_0::OptionKey;

using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::hardware::Return;
using ::android::hardware::Void;

typedef enum {
    CONNECT_TYPE_HAL            = 0,
    CONNECT_TYPE_EXTEND         = 1,
    CONNECT_TYPE_POWER          = 2
} cec_connect_type_t;

class HdmiCecHidlClient {
public:
    HdmiCecHidlClient(cec_connect_type_t type);
    virtual ~HdmiCecHidlClient();

    static HdmiCecHidlClient* connect(cec_connect_type_t type);

    virtual int openCecDevice();
    virtual int closeCecDevice();
    virtual int getVersion(int* version);
    virtual int getVendorId(uint32_t* vendorId);
    virtual int getPhysicalAddress(uint16_t* addr);
    virtual int sendMessage(const cec_message_t* message, bool isExtend = false);

    virtual void getPortInfos(hdmi_port_info_t* list[], int* total);
    virtual int addLogicalAddress(cec_logical_address_t address);
    virtual void clearLogicaladdress();
    virtual void setOption(int flag, int value);
    virtual void setAudioReturnChannel(int port, bool flag);
    virtual bool isConnected(int port);
    virtual int getCecWakePort();

    void setEventObserver(const sp<HdmiCecEventListener> &eventListener);

private:

    static Mutex mLock;
    cec_connect_type_t mType;
    sp<IDroidHdmiCEC> mHdmiCecService;
    sp<HdmiCecEventListener> mEventListener;

    sp<IDroidHdmiCEC> getHdmiCecService();
    void reconnect();
    class HdmiCecHidlCallback : public IDroidHdmiCecCallback {
    public:
        HdmiCecHidlCallback(HdmiCecHidlClient *client): cecClient(client) {};

        Return<void> notifyCallback(const CecEvent& hidlEvent) override;

    private:
        HdmiCecHidlClient *cecClient;
    };

    struct HdmiCecDaemonDeathRecipient : public android::hardware::hidl_death_recipient  {
        HdmiCecDaemonDeathRecipient(HdmiCecHidlClient *client): cecClient(client) {};

        // hidl_death_recipient interface
        virtual void serviceDied(uint64_t cookie,
            const ::android::wp<::android::hidl::base::V1_0::IBase>& who) override;
    private:
        HdmiCecHidlClient *cecClient;
    };
    sp<HdmiCecDaemonDeathRecipient> mDeathRecipient = nullptr;

    sp<HdmiCecHidlCallback> mHdmiCecHidlCallback = nullptr;
    hdmi_port_info_t *mpPortInfo = nullptr;
};

}//namespace android

#endif /* DROIDLOGIC_HDMICEC_HIDL_CLIENT_H_ */
