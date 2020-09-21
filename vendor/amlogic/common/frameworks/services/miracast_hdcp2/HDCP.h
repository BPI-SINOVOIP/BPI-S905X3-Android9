/*
 * Copyright (C) 2012 The Android Open Source Project
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
 */

#ifndef VENDOR_AMLOGIC_HDCP_V1_0_HDCP_H
#define VENDOR_AMLOGIC_HDCP_V1_0_HDCP_H

#include <vendor/amlogic/hardware/miracast_hdcp2/1.0/IHDCP.h>
#include <vendor/amlogic/hardware/miracast_hdcp2/1.0/IHDCPObserver.h>
#include <media/hardware/HDCPAPI.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>
#include <utils/Mutex.h>
#include <utils/Log.h>

#define OBSERVER_COOKIE (999)
namespace vendor {
namespace amlogic {
namespace hardware {
namespace miracast_hdcp2 {
namespace V1_0 {
namespace implementation {

using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;
using ::android::Mutex;
using ::android::HDCPModule;
using ::android::hardware::hidl_death_recipient;

struct HDCP : public IHDCP, public hidl_death_recipient {

    HDCP(bool createEncryptionModule);
    ~HDCP();

    // Methods from ::vendor::amlogic::hardware::miracast_hdcp2::V1_0::IHDCPService follow.
    Return<Status> setObserver(const sp<IHDCPObserver> &observer);
    Return<Status> initAsync(const hidl_string& host, unsigned port);
    Return<Status> shutdownAsync();
    Return<void> encrypt(const hidl_vec<uint8_t>& inData, uint32_t streamCTR, encrypt_cb _hidl_cb);
    // Method for decrypt funtion add outAddr to pass secure hardware address or no secure handle(0)
    Return<void> decrypt(const hidl_vec<uint8_t>& inData, uint32_t streamCTR, uint64_t outInputCTR, uint32_t outAddr, decrypt_cb _hidl_cb);
    Return<uint32_t> getCaps();

    virtual void serviceDied(uint64_t cookie, const android::wp<::android::hidl::base::V1_0::IBase>& who) {
        Mutex::Autolock autoLock(mLock);
        (void) who;
        if (cookie == OBSERVER_COOKIE) {
            ALOGE("observer died, ignoring it\n");
            mObserver = NULL;
        }
    }
private:
    Mutex mLock;

    bool mIsEncryptionModule;

    void *mLibHandle;
    HDCPModule *mHDCPModule;
    sp<IHDCPObserver> mObserver;

    static void ObserveWrapper(void *me, int msg, int ext1, int ext2);
    void observe(int msg, int ext1, int ext2);
    HDCP();
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace miracast_hdcp2
}  // namespace hardware
}  // namespace amlogic
}  // namespace vendor

#endif // VENDOR_AMLOGIC_HDCP_V1_0_HDCP_H

