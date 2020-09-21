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
 */

#define LOG_TAG "dumpstate"

#include "DumpstateDevice.h"

#include <log/log.h>

#include "DumpstateUtil.h"

using android::os::dumpstate::DumpFileToFd;
using android::os::dumpstate::RunCommandToFd;

namespace android {
namespace hardware {
namespace dumpstate {
namespace V1_0 {
namespace implementation {

// Methods from ::android::hardware::dumpstate::V1_0::IDumpstateDevice follow.
Return<void> DumpstateDevice::dumpstateBoard(const hidl_handle& handle) {
    // NOTE: this is just an example on how to use the DumpstateUtil.h functions to implement
    // this interface - since HIDL_FETCH_IDumpstateDevice() is not defined, this function will never
    // be called by dumpstate.

    if (handle == nullptr || handle->numFds < 1) {
        ALOGE("no FDs\n");
        return Void();
    }

    int fd = handle->data[0];
    if (fd < 0) {
        ALOGE("invalid FD: %d\n", handle->data[0]);
        return Void();
    }
    ALOGD("DumpstateDevice::dumpstateBoard() FD: %d\n", fd);
    ALOGI("Dumpstate HIDL not provided by device\n");
    dprintf(fd, "Dumpstate HIDL not provided by device; providing bogus data.\n");

    // Shows some examples on how to use the libdumpstateutil API.
    RunCommandToFd(fd, "DATE", {"/vendor/bin/date"});
    DumpFileToFd(fd, "HOSTS", "/system/etc/hosts");

    return Void();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace dumpstate
}  // namespace hardware
}  // namespace android
