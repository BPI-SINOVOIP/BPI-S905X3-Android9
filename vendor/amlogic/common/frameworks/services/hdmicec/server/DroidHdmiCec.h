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
 *  - 1 droidlogic hdmi cec hwbinder service
 */

#ifndef ANDROID_DROIDLOGIC_HDMI_CEC_V1_0_H
#define ANDROID_DROIDLOGIC_HDMI_CEC_V1_0_H

#include <binder/IBinder.h>
#include <utils/Mutex.h>
#include <vector>
#include <map>
#include <HdmiCecControl.h>
#include <vendor/amlogic/hardware/hdmicec/1.0/IDroidHdmiCEC.h>

namespace vendor {
namespace amlogic {
namespace hardware {
namespace hdmicec {
namespace V1_0 {
namespace implementation {

using ::vendor::amlogic::hardware::hdmicec::V1_0::IDroidHdmiCEC;
using ::vendor::amlogic::hardware::hdmicec::V1_0::IDroidHdmiCecCallback;
using ::vendor::amlogic::hardware::hdmicec::V1_0::ConnectType;
using ::vendor::amlogic::hardware::hdmicec::V1_0::Result;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

using namespace android;

class DroidHdmiCec : public IDroidHdmiCEC, public HdmiCecEventListener {
public:
    DroidHdmiCec();
    virtual ~DroidHdmiCec();

    Return<void> openCecDevice(openCecDevice_cb _hidl_cb) override;
    Return<void> closeCecDevice() override;
    Return<int32_t> getCecVersion() override;
    Return<uint32_t> getVendorId() override;
    Return<void> getPhysicalAddress(getPhysicalAddress_cb _hidl_cb) override;
    Return<SendMessageResult> sendMessage(const CecMessage& message, bool isExtend) override;

    Return<void> getPortInfo(getPortInfo_cb _hidl_cb) override;
    Return<Result> addLogicalAddress(CecLogicalAddress addr) override;
    Return<void> clearLogicalAddress() override;
    Return<void> setOption(OptionKey key, bool value) override;
    Return<void> enableAudioReturnChannel(int32_t portId, bool enable) override;
    Return<bool> isConnected(int32_t portId) override;

    Return<void> setCallback(const sp<IDroidHdmiCecCallback>& callback, ConnectType type) override;
    Return<int32_t> getCecWakePort() override;

    virtual void onEventUpdate(const hdmi_cec_event_t* event);

    static void instantiate();

private:
    const char* getEventTypeStr(int eventType);
    const char* getConnectTypeStr(ConnectType type);

    // Handle the case where the callback registered for the given type dies
    void handleServiceDeath(uint32_t type);

    bool mDebug = false;
    HdmiCecControl *mHdmiCecControl;
    //std::vector<sp<IDroidHdmiCecCallback>> mClients;
    std::map<uint32_t, sp<IDroidHdmiCecCallback>> mClients;

    mutable Mutex mLock;

    class DeathRecipient : public android::hardware::hidl_death_recipient  {
    public:
        DeathRecipient(sp<DroidHdmiCec> cec);

        // hidl_death_recipient interface
        virtual void serviceDied(uint64_t cookie,
            const ::android::wp<::android::hidl::base::V1_0::IBase>& who) override;

    private:
        sp<DroidHdmiCec> mDroidHdmiCec;
    };

    sp<DeathRecipient> mDeathRecipient;
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace hdmicec
}  // namespace hardware
}  // namespace amlogic
}  // namespace vendor
#endif /* ANDROID_DROIDLOGIC_HDMI_CEC_V1_0_H */
