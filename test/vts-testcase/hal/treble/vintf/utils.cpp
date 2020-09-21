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

#include "utils.h"

#include <map>
#include <set>
#include <string>
#include <vector>

#include <android-base/properties.h>

using android::base::GetUintProperty;

namespace android {
namespace vintf {
namespace testing {

// Path to directory on target containing test data.
const string kDataDir = "/data/local/tmp/";

// Name of file containing HAL hashes.
const string kHashFileName = "current.txt";

// Map from package name to package root.
const map<string, string> kPackageRoot = {
    {"android.frameworks", "frameworks/hardware/interfaces/"},
    {"android.hardware", "hardware/interfaces/"},
    {"android.hidl", "system/libhidl/transport/"},
    {"android.system", "system/hardware/interfaces/"},
};

// HALs that are allowed to be passthrough under Treble rules.
const set<string> kPassthroughHals = {
    "android.hardware.graphics.mapper", "android.hardware.renderscript",
    "android.hidl.memory",
};

// kFcm2ApiLevelMap is associated with API level. There can be multiple
// Framework Compatibility Matrix Version (FCM Version) per API level, or
// multiple API levels per FCM version.
// kFcm2ApiLevelMap is defined apart from android::vintf::Level. Level is an
// integer designed to be irrelevant with API level; the O / O_MR1 values are
// historic values for convenience, and should be removed (b/70628538). Hence
// these values are not used here.
// For example:
//    ...
//    // Assume devices launch with Android X must implement FCM version >= 9
//    X = 9,
//    // Assume devices launch with Android Y and Android Z must implement
//    // FCM version >= 11
//    Y = 11,
//    Z = 11
const map<size_t /* Shipping API Level */, Level /* FCM Version */>
    kFcm2ApiLevelMap{{// N. The test runs on devices that launch with N and
                      // become a Treble device when upgrading to O.
                      {25, static_cast<Level>(1)},
                      // O
                      {26, static_cast<Level>(1)},
                      // O MR-1
                      {27, static_cast<Level>(2)},
                      // P
                      {28, static_cast<Level>(3)}}};

// Returns ro.product.first_api_level if it is defined and not 0. Returns
// ro.build.version.sdk otherwise.
uint64_t GetShippingApiLevel() {
  uint64_t api_level =
      GetUintProperty<uint64_t>("ro.product.first_api_level", 0);
  if (api_level != 0) {
    return api_level;
  }
  return GetUintProperty<uint64_t>("ro.build.version.sdk", 0);
}

// For a given interface returns package root if known. Returns empty string
// otherwise.
const string PackageRoot(const FQName &fq_iface_name) {
  for (const auto &package_root : kPackageRoot) {
    if (fq_iface_name.inPackage(package_root.first)) {
      return package_root.second;
    }
  }
  return "";
}

// Returns true iff HAL interface is Google-defined.
bool IsGoogleDefinedIface(const FQName &fq_iface_name) {
  // Package roots are only known for Google-defined packages.
  return !PackageRoot(fq_iface_name).empty();
}

// Returns the set of released hashes for a given HAL interface.
set<string> ReleasedHashes(const FQName &fq_iface_name) {
  set<string> released_hashes{};
  string err = "";

  string file_path = kDataDir + PackageRoot(fq_iface_name) + kHashFileName;
  auto hashes = Hash::lookupHash(file_path, fq_iface_name.string(), &err);
  released_hashes.insert(hashes.begin(), hashes.end());
  return released_hashes;
}

// Returns the partition that a HAL is associated with.
Partition PartitionOfProcess(int32_t pid) {
  auto partition = android::procpartition::getPartition(pid);

  // TODO(b/70033981): remove once ODM and Vendor manifests are distinguished
  if (partition == Partition::ODM) {
    partition = Partition::VENDOR;
  }

  return partition;
}

Partition PartitionOfType(SchemaType type) {
  switch (type) {
    case SchemaType::DEVICE:
      return Partition::VENDOR;
    case SchemaType::FRAMEWORK:
      return Partition::SYSTEM;
  }
  return Partition::UNKNOWN;
}

}  // namespace testing
}  // namespace vintf
}  // namespace android

namespace std {
void PrintTo(const android::vintf::testing::HalManifestPtr &v, ostream *os) {
  if (v == nullptr) {
    *os << "nullptr";
    return;
  }
  *os << to_string(v->type()) << " manifest";
}
void PrintTo(nullptr_t, ostream *os) { *os << "nullptr"; }
}  // namespace std
