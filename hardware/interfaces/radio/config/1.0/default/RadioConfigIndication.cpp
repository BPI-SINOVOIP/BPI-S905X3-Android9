/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.1 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.1
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "RadioConfigIndication.h"

namespace android {
namespace hardware {
namespace radio {
namespace config {
namespace V1_0 {
namespace implementation {

using namespace ::android::hardware::radio::V1_0;

// Methods from ::android::hardware::radio::config::V1_0::IRadioConfigIndication follow.
Return<void> RadioConfigIndication::simSlotsStatusChanged(
    RadioIndicationType /* type */, const hidl_vec<SimSlotStatus>& /* slotStatus */) {
    // TODO implement
    return Void();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace config
}  // namespace radio
}  // namespace hardware
}  // namespace android
