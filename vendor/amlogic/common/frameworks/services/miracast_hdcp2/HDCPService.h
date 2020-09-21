/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: C++ file
 */
#ifndef VENDOR_AMLOGIC_HDCPSERVICE_V1_0_HDCPSERVICE_H
#define VENDOR_AMLOGIC_HDCPSERVICE_V1_0_HDCPSERVICE_H

#include <vendor/amlogic/hardware/miracast_hdcp2/1.0/IHDCPService.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>
#include "HDCP.h"

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

struct HDCPService : public IHDCPService {
    // Methods from ::vendor::amlogic::hardware::miracast_hdcp2::V1_0::IHDCPService follow.
    Return<sp<IBase>> makeHDCP(bool createEncryptionModule) {
        return new HDCP(createEncryptionModule);
    };
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace miracast_hdcp2
}  // namespace hardware
}  // namespace amlogic
}  // namespace vendor

#endif  // ANDROID_HARDWARE_AUTHSECRET_V1_0_AUTHSECRET_H
