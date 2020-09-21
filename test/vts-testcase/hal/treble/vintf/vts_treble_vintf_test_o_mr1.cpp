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

#include <chrono>
#include <functional>
#include <future>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <android/hidl/manager/1.0/IServiceManager.h>
#include <gtest/gtest.h>
#include <hidl-hash/Hash.h>
#include <hidl-util/FQName.h>
#include <hidl/ServiceManagement.h>
#include <vintf/HalManifest.h>
#include <vintf/VintfObject.h>
#include <vintf/parse_string.h>

#include "VtsTrebleVintfTestBase.h"

namespace android {
namespace vintf {
namespace testing {
namespace legacy {

using android::FQName;
using android::Hash;
using android::sp;
using android::hardware::hidl_array;
using android::hardware::hidl_string;
using android::hardware::hidl_vec;
using android::hidl::manager::V1_0::IServiceManager;
using android::vintf::HalManifest;
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
using HalVerifyFn =
    std::function<void(const FQName &fq_name, const string &instance_name)>;

// Returns true iff HAL interface is exempt from following rules:
// 1. If an interface is declared in VINTF, it has to be served on the device.
static bool IsExempt(const FQName &fq_iface_name) {
  static const set<string> exempt_hals_ = {};
  string hal_name = fq_iface_name.package();
  // Radio-releated and non-Google HAL interfaces are given exemptions.
  return exempt_hals_.find(hal_name) != exempt_hals_.end() ||
         !IsGoogleDefinedIface(fq_iface_name);
}

class VtsTrebleVintfTest : public ::testing::Test {
 public:
  virtual void SetUp() override {
    default_manager_ = ::android::hardware::defaultServiceManager();
    ASSERT_NE(default_manager_, nullptr)
        << "Failed to get default service manager." << endl;

    passthrough_manager_ = ::android::hardware::getPassthroughServiceManager();
    ASSERT_NE(passthrough_manager_, nullptr)
        << "Failed to get passthrough service manager." << endl;

    vendor_manifest_ = VintfObject::GetDeviceHalManifest();
    ASSERT_NE(vendor_manifest_, nullptr)
        << "Failed to get vendor HAL manifest." << endl;
  }

  // Applies given function to each HAL instance in VINTF.
  void ForEachHalInstance(HalVerifyFn);
  // Retrieves an existing HAL service.
  sp<android::hidl::base::V1_0::IBase> GetHalService(
      const FQName &fq_name, const string &instance_name);

  // Default service manager.
  sp<IServiceManager> default_manager_;
  // Passthrough service manager.
  sp<IServiceManager> passthrough_manager_;
  // Vendor hal manifest.
  std::shared_ptr<const HalManifest> vendor_manifest_;
};

void VtsTrebleVintfTest::ForEachHalInstance(HalVerifyFn fn) {
  vendor_manifest_->forEachInstance([fn](const auto &manifest_instance) {
    const FQName fq_name{manifest_instance.package(),
                         to_string(manifest_instance.version()),
                         manifest_instance.interface()};
    const std::string instance_name = manifest_instance.instance();

    auto future_result = std::async([&]() { fn(fq_name, instance_name); });
    auto timeout = std::chrono::milliseconds(500);
    std::future_status status = future_result.wait_for(timeout);
    if (status != std::future_status::ready) {
      cout << "Timed out on: " << fq_name.string() << " " << instance_name
           << endl;
    }
    return true;  // continue to next instance
  });
}

sp<android::hidl::base::V1_0::IBase> VtsTrebleVintfTest::GetHalService(
    const FQName &fq_name, const string &instance_name) {
  string hal_name = fq_name.package();
  Version version{fq_name.getPackageMajorVersion(),
                  fq_name.getPackageMinorVersion()};
  string iface_name = fq_name.name();
  string fq_iface_name = fq_name.string();
  cout << "Getting service of: " << fq_iface_name << endl;

  Transport transport = vendor_manifest_->getTransport(
      hal_name, version, iface_name, instance_name);

  return VtsTrebleVintfTestBase::GetHalService(fq_name, instance_name,
                                               transport);
}

// Tests that no HAL outside of the allowed set is specified as passthrough in
// VINTF.
TEST_F(VtsTrebleVintfTest, HalsAreBinderized) {
  // Verifies that HAL is binderized unless it's allowed to be passthrough.
  HalVerifyFn is_binderized = [this](const FQName &fq_name,
                                     const string &instance_name) {
    cout << "Verifying transport method of: " << fq_name.string() << endl;
    string hal_name = fq_name.package();
    Version version{fq_name.getPackageMajorVersion(),
                    fq_name.getPackageMinorVersion()};
    string iface_name = fq_name.name();

    Transport transport = vendor_manifest_->getTransport(
        hal_name, version, iface_name, instance_name);

    EXPECT_NE(transport, Transport::EMPTY)
        << hal_name << " has no transport specified in VINTF.";

    if (transport == Transport::PASSTHROUGH) {
      EXPECT_NE(kPassthroughHals.find(hal_name), kPassthroughHals.end())
          << hal_name << " can't be passthrough under Treble rules.";
    }
  };

  ForEachHalInstance(is_binderized);
}

// Tests that all HALs specified in the VINTF are available through service
// manager.
TEST_F(VtsTrebleVintfTest, VintfHalsAreServed) {
  // Verifies that HAL is available through service manager.
  HalVerifyFn is_available = [this](const FQName &fq_name,
                                    const string &instance_name) {
    if (IsExempt(fq_name)) {
      cout << fq_name.string() << " is exempt for O-MR1 vendor." << endl;
      return;
    }

    sp<android::hidl::base::V1_0::IBase> hal_service =
        GetHalService(fq_name, instance_name);
    EXPECT_NE(hal_service, nullptr)
        << fq_name.string() << " not available." << endl;
  };

  ForEachHalInstance(is_available);
}

// Tests that HAL interfaces are officially released.
TEST_F(VtsTrebleVintfTest, InterfacesAreReleased) {
  // Verifies that HAL are released by fetching the hash of the interface and
  // comparing it to the set of known hashes of released interfaces.
  HalVerifyFn is_released = [this](const FQName &fq_name,
                                   const string &instance_name) {
    sp<android::hidl::base::V1_0::IBase> hal_service =
        GetHalService(fq_name, instance_name);

    if (hal_service == nullptr) {
      if (IsExempt(fq_name)) {
        cout << fq_name.string() << " is exempt for O-MR1 vendor." << endl;
      } else {
        ADD_FAILURE() << fq_name.package() << " not available." << endl;
      }
      return;
    }

    vector<string> iface_chain{};
    hal_service->interfaceChain(
        [&iface_chain](const hidl_vec<hidl_string> &chain) {
          for (const auto &iface_name : chain) {
            iface_chain.push_back(iface_name);
          }
        });

    vector<string> hash_chain{};
    hal_service->getHashChain(
        [&hash_chain](const hidl_vec<HashCharArray> &chain) {
          for (const HashCharArray &hash_array : chain) {
            vector<uint8_t> hash{hash_array.data(),
                                 hash_array.data() + hash_array.size()};
            hash_chain.push_back(Hash::hexString(hash));
          }
        });

    ASSERT_EQ(iface_chain.size(), hash_chain.size());
    for (size_t i = 0; i < iface_chain.size(); ++i) {
      FQName fq_iface_name;
      if (!FQName::parse(iface_chain[i], &fq_iface_name)) {
        ADD_FAILURE() << "Could not parse iface name " << iface_chain[i]
                      << " from interface chain of " << fq_name.string();
        return;
      }
      string hash = hash_chain[i];

      if (IsGoogleDefinedIface(fq_iface_name)) {
        set<string> released_hashes = ReleasedHashes(fq_iface_name);
        EXPECT_NE(released_hashes.find(hash), released_hashes.end())
            << "Hash not found. This interface was not released." << endl
            << "Interface name: " << fq_iface_name.string() << endl
            << "Hash: " << hash << endl;
      }
    }
  };

  ForEachHalInstance(is_released);
}

}  // namespace legacy
}  // namespace testing
}  // namespace vintf
}  // namespace android
