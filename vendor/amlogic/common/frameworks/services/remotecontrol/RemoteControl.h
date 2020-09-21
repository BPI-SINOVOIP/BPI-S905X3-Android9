/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC
 */

#ifndef VENDOR_AMLOGIC_HARDWARE_REMOTECONTROL_V1_0_REMOTECONTROL_H
#define VENDOR_AMLOGIC_HARDWARE_REMOTECONTROL_V1_0_REMOTECONTROL_H

#include <vendor/amlogic/hardware/remotecontrol/1.0/IRemoteControl.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

namespace vendor {
namespace amlogic {
namespace hardware {
namespace remotecontrol {
namespace V1_0 {
namespace implementation {

using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;
using namespace android;

class RemoteControl : public IRemoteControl {
public:
    class Callback {
      public:
        virtual ~Callback() {}
        virtual void onSetMicEnable(int flag) = 0;
        virtual void onDeviceStatusChanged(int flag) = 0;
    };
public:
    RemoteControl(Callback& callback);
    virtual ~RemoteControl();

    // Methods from IRemoteControl follow.
    Return<void> setMicEnable(int32_t flag) override;
    Return<void> onDeviceChanged(int32_t flag) override;

    // Methods from ::android::hidl::base::V1_0::IBase follow.

protected:
    // Handle the case where the callback registered for the given type dies
    void handleServiceDeath(uint32_t type);

private:
    class DeathRecipient : public android::hardware::hidl_death_recipient  {
    public:
        DeathRecipient(sp<RemoteControl> rc);

        // hidl_death_recipient interface
        virtual void serviceDied(uint64_t cookie,
            const ::android::wp<::android::hidl::base::V1_0::IBase>& who) override;

    private:
        sp<RemoteControl> mRemoteControl;
    };
    Callback& mCallback;
    sp<DeathRecipient> mDeathRecipient;

};

// FIXME: most likely delete, this is only for passthrough implementations
// extern "C" IRemoteControl* HIDL_FETCH_IRemoteControl(const char* name);

}  // namespace implementation
}  // namespace V1_0
}  // namespace remotecontrol
}  // namespace hardware
}  // namespace amlogic
}  // namespace vendor

#endif  // VENDOR_AMLOGIC_HARDWARE_REMOTECONTROL_V1_0_REMOTECONTROL_H

