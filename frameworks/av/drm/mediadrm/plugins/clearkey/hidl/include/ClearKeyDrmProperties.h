/*
 * Copyright (C) 2017 The Android Open Source Project
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

#ifndef CLEARKEY_DRM_PROPERTIES_H_
#define CLEARKEY_DRM_PROPERTIES_H_

#include <string.h>

namespace android {
namespace hardware {
namespace drm {
namespace V1_1 {
namespace clearkey {

static const std::string kVendorKey("vendor");
static const std::string kVendorValue("Google");
static const std::string kVersionKey("version");
static const std::string kVersionValue("1.1");
static const std::string kPluginDescriptionKey("description");
static const std::string kPluginDescriptionValue("ClearKey CDM");
static const std::string kAlgorithmsKey("algorithms");
static const std::string kAlgorithmsValue("");
static const std::string kListenerTestSupportKey("listenerTestSupport");
static const std::string kListenerTestSupportValue("true");

static const std::string kDeviceIdKey("deviceId");
static const uint8_t kTestDeviceIdData[] =
        {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7,
         0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf};
// TODO stub out metrics for nw
static const std::string kMetricsKey("metrics");
static const uint8_t kMetricsData[] = { 0 };

} // namespace clearkey
} // namespace V1_1
} // namespace drm
} // namespace hardware
} // namespace android

#endif // CLEARKEY_DRM_PROPERTIES_H_

