/*
 * Copyright (C) 2018 The Android Open Source Project
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

#ifndef VTS_TREBLE_VINTF_TEST_UTILS_H_
#define VTS_TREBLE_VINTF_TEST_UTILS_H_
#include <map>
#include <set>
#include <string>
#include <vector>

#include <android/hidl/manager/1.0/IServiceManager.h>
#include <hidl-hash/Hash.h>
#include <hidl-util/FQName.h>
#include <hidl/HidlSupport.h>
#include <procpartition/procpartition.h>
#include <vintf/VintfObject.h>
#include <vintf/parse_string.h>

namespace android {
namespace vintf {
namespace testing {

using android::FQName;
using android::Hash;
using android::sp;
using android::hardware::hidl_array;
using android::hardware::hidl_string;
using android::hardware::hidl_vec;
using android::hardware::Return;
using android::hidl::base::V1_0::IBase;
using android::hidl::manager::V1_0::IServiceManager;
using android::procpartition::Partition;
using android::vintf::HalManifest;
using android::vintf::Level;
using android::vintf::ManifestHal;
using android::vintf::SchemaType;
using android::vintf::to_string;
using android::vintf::Transport;
using android::vintf::Version;
using android::vintf::VintfObject;

using std::cout;
using std::endl;
using std::map;
using std::set;
using std::string;

using std::vector;

using HalVerifyFn = std::function<void(const FQName& fq_name,
                                       const string& instance_name, Transport)>;
using HashCharArray = hidl_array<unsigned char, 32>;
using HalManifestPtr = std::shared_ptr<const HalManifest>;

// Path to directory on target containing test data.
extern const string kDataDir;
// Name of file containing HAL hashes.
extern const string kHashFileName;
// Map from package name to package root.
extern const map<string, string> kPackageRoot;
// HALs that are allowed to be passthrough under Treble rules.
extern const set<string> kPassthroughHals;

extern const map<size_t /* Shipping API Level */, Level /* FCM Version */>
    kFcm2ApiLevelMap;

// Returns ro.product.first_api_level if it is defined and not 0. Returns
// ro.build.version.sdk otherwise.
uint64_t GetShippingApiLevel();

// For a given interface returns package root if known. Returns empty string
// otherwise.
const string PackageRoot(const FQName& fq_iface_name);

// Returns true iff HAL interface is Google-defined.
bool IsGoogleDefinedIface(const FQName& fq_iface_name);

// Returns the set of released hashes for a given HAL interface.
set<string> ReleasedHashes(const FQName& fq_iface_name);

// Returns the partition that a HAL is associated with.
Partition PartitionOfProcess(int32_t pid);

// Returns SYSTEM for FRAMEWORK, VENDOR for DEVICE.
Partition PartitionOfType(SchemaType type);

}  // namespace testing
}  // namespace vintf

// Allows GTest to print pointers with a human readable string.
template <typename T>
void PrintTo(const sp<T>& v, std::ostream* os) {
  *os << android::hardware::details::toHexString<uintptr_t>(
      reinterpret_cast<uintptr_t>(&*v), true /* prefix */);
}
template <typename T>
void PrintTo(const T* v, std::ostream* os) {
  *os << android::hardware::details::toHexString<uintptr_t>(
      reinterpret_cast<uintptr_t>(v), true /* prefix */);
}

}  // namespace android

// Allows GTest to print pointers with a human readable string.
namespace std {
void PrintTo(const android::vintf::testing::HalManifestPtr& v, ostream* os);
void PrintTo(nullptr_t, ostream* os);
template <typename T>
void PrintTo(const T* v, ostream* os) {
  *os << android::hardware::details::toHexString<uintptr_t>(
      reinterpret_cast<uintptr_t>(v), true /* prefix */);
}
}  // namespace std

#endif  // VTS_TREBLE_VINTF_TEST_UTILS_H_
