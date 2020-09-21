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

// SystemVendorTest test cases that runs on P+ vendor.

#include "SystemVendorTest.h"

#include <android-base/logging.h>
#include <android-base/strings.h>
#include <vintf/VintfObject.h>
#include <iostream>

#include "SingleManifestTest.h"

namespace android {
namespace vintf {
namespace testing {

using std::endl;

// Tests that vendor and framework are compatible.
TEST_F(SystemVendorTest, VendorFrameworkCompatibility) {
  string error;

  EXPECT_TRUE(vendor_manifest_->checkCompatibility(
      *VintfObject::GetFrameworkCompatibilityMatrix(), &error))
      << error;

  EXPECT_TRUE(fwk_manifest_->checkCompatibility(
      *VintfObject::GetDeviceCompatibilityMatrix(), &error))
      << error;

  // AVB version is not a compliance requirement.
  EXPECT_TRUE(VintfObject::GetRuntimeInfo()->checkCompatibility(
      *VintfObject::GetFrameworkCompatibilityMatrix(), &error,
      ::android::vintf::DISABLE_AVB_CHECK))
      << error;

  EXPECT_EQ(android::vintf::COMPATIBLE,
            VintfObject::CheckCompatibility(
                {}, &error, ::android::vintf::DISABLE_AVB_CHECK))
      << error;
}

template <typename D, typename S>
static void insert(D *d, const S &s) {
  d->insert(s.begin(), s.end());
}

// This needs to be tested besides
// SingleManifestTest.ServedHwbinderHalsAreInManifest because some HALs may
// refuse to provide its PID, and the partition cannot be inferred.
TEST_F(SystemVendorTest, ServedHwbinderHalsAreInManifest) {
  std::set<std::string> manifest_hwbinder_hals_;

  insert(&manifest_hwbinder_hals_, GetHwbinderHals(fwk_manifest_));
  insert(&manifest_hwbinder_hals_, GetHwbinderHals(vendor_manifest_));

  Return<void> ret = default_manager_->list([&](const auto &list) {
    for (const auto &name : list) {
      // TODO(b/73774955): use standardized parsing code for fqinstancename
      if (std::string(name).find(IBase::descriptor) == 0) continue;

      EXPECT_NE(manifest_hwbinder_hals_.find(name),
                manifest_hwbinder_hals_.end())
          << name << " is being served, but it is not in a manifest.";
    }
  });
  EXPECT_TRUE(ret.isOk());
}

// Tests that deprecated HALs are not served, unless a higher, non-deprecated
// minor version is served.
TEST_F(SystemVendorTest, NoDeprecatedHalsOnManager) {
  if (vendor_manifest_->level() == Level::UNSPECIFIED) {
    // On a legacy device, no HALs are deprecated.
    return;
  }

  // Predicate for whether an instance is served through service manager.
  // Return {instance name, highest minor version}
  // where "highest minor version" is the first element in getInterfaceChain()
  // that has the same "package", major version as "version" and "interface"
  // but a higher minor version than "version".
  VintfObject::ListInstances list_instances = [this](const string &package,
                                                     Version version,
                                                     const string &interface,
                                                     const vector<string>
                                                         &instance_hints) {
    vector<std::pair<string, Version>> ret;

    FQName fq_name(package, to_string(version), interface);
    for (auto transport : {Transport::HWBINDER, Transport::PASSTHROUGH}) {
      const vector<string> &instance_names =
          transport == Transport::HWBINDER
              ? GetInstanceNames(default_manager_, fq_name)
              : instance_hints;

      for (auto instance : instance_names) {
        auto service =
            GetHalService(fq_name, instance, transport, false /* log */);
        if (service == nullptr) {
          if (transport == Transport::PASSTHROUGH) {
            CHECK(std::find(instance_hints.begin(), instance_hints.end(),
                            instance) != instance_hints.end())
                << "existing <instance>'s: ["
                << android::base::Join(instance_hints, ",")
                << "] but instance=" << instance;
            continue;  // name is from instance_hints, so ignore
          }
          ADD_FAILURE()
              << fq_name.string() << "/" << instance
              << " is registered to hwservicemanager but cannot be retrieved.";
          continue;
        }

        vector<string> iface_chain = GetInterfaceChain(service);
        bool done = false;
        for (const auto &fq_interface_str : iface_chain) {
          FQName fq_interface;
          if (!FQName::parse(fq_interface_str, &fq_interface)) {
            // Allow CheckDeprecation to proceed with some sensible default
            ADD_FAILURE() << "'" << fq_interface_str
                          << "' (returned by getInterfaceChain())"
                          << "is not a valid fully-qualified name.";
            ret.push_back(std::make_pair(instance, version));
            done = true;
            continue;
          }
          if (fq_interface.package() == package) {
            Version fq_version{fq_interface.getPackageMajorVersion(),
                               fq_interface.getPackageMinorVersion()};
            if (fq_version.minorAtLeast(version)) {
              ret.push_back(std::make_pair(instance, fq_version));
              done = true;
              break;
            }
          }
        }
        if (!done) {
          // Allow CheckDeprecation to proceed with some sensible default
          ADD_FAILURE() << "getInterfaceChain() does not return interface name "
                        << "with at least minor version'" << package << "@"
                        << version << "'; returned values are ["
                        << android::base::Join(iface_chain, ", ") << "]";
          ret.push_back(std::make_pair(instance, version));
        }
      }
    }

    return ret;
  };
  string error;
  EXPECT_EQ(android::vintf::NO_DEPRECATED_HALS,
            VintfObject::CheckDeprecation(list_instances, &error))
      << error;
}

static std::vector<HalManifestPtr> GetTestManifests() {
  return {
      VintfObject::GetFrameworkHalManifest(),
  };
}

INSTANTIATE_TEST_CASE_P(FrameworkManifest, SingleManifestTest,
                        ::testing::ValuesIn(GetTestManifests()));

}  // namespace testing
}  // namespace vintf
}  // namespace android
