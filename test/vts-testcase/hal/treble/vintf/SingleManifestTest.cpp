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

#include "SingleManifestTest.h"

#include <hidl-util/FqInstance.h>
#include <hidl/HidlTransportUtils.h>
#include <vintf/parse_string.h>

#include "utils.h"

namespace android {
namespace vintf {
namespace testing {

using android::FqInstance;
using android::vintf::toFQNameString;

// For devices that launched <= Android O-MR1, systems/hals/implementations
// were delivered to companies which either don't start up on device boot.
bool LegacyAndExempt(const FQName &fq_name) {
  return GetShippingApiLevel() <= 27 && !IsGoogleDefinedIface(fq_name);
}

void FailureHalMissing(const FQName &fq_name) {
  if (LegacyAndExempt(fq_name)) {
    cout << "[  WARNING ] " << fq_name.string()
         << " not available but is exempted because it is legacy. It is still "
            "recommended to fix this."
         << endl;
  } else {
    ADD_FAILURE() << fq_name.string() << " not available.";
  }
}

void FailureHashMissing(const FQName &fq_name) {
  if (LegacyAndExempt(fq_name)) {
    cout << "[  WARNING ] " << fq_name.string()
         << " has an empty hash but is exempted because it is legacy. It is "
            "still recommended to fix this. This is because it was compiled "
            "without being frozen in a corresponding current.txt file."
         << endl;
  } else {
    ADD_FAILURE()
        << fq_name.string()
        << " has an empty hash. This is because it was compiled "
           "without being frozen in a corresponding current.txt file.";
  }
}

// Tests that no HAL outside of the allowed set is specified as passthrough in
// VINTF.
TEST_P(SingleManifestTest, HalsAreBinderized) {
  // Verifies that HAL is binderized unless it's allowed to be passthrough.
  HalVerifyFn is_binderized = [](const FQName &fq_name,
                                 const string & /* instance_name */,
                                 Transport transport) {
    cout << "Verifying transport method of: " << fq_name.string() << endl;
    string hal_name = fq_name.package();
    Version version{fq_name.getPackageMajorVersion(),
                    fq_name.getPackageMinorVersion()};
    string iface_name = fq_name.name();

    EXPECT_NE(transport, Transport::EMPTY)
        << hal_name << " has no transport specified in VINTF.";

    if (transport == Transport::PASSTHROUGH) {
      EXPECT_NE(kPassthroughHals.find(hal_name), kPassthroughHals.end())
          << hal_name << " can't be passthrough under Treble rules.";
    }
  };

  ForEachHalInstance(GetParam(), is_binderized);
}

// Tests that all HALs specified in the VINTF are available through service
// manager.
// This tests (HAL in manifest) => (HAL is served)
TEST_P(SingleManifestTest, HalsAreServed) {
  // Returns a function that verifies that HAL is available through service
  // manager and is served from a specific set of partitions.
  auto is_available_from = [this](Partition expected_partition) -> HalVerifyFn {
    return [this, expected_partition](const FQName &fq_name,
                                      const string &instance_name,
                                      Transport transport) {
      sp<IBase> hal_service;

      if (transport == Transport::PASSTHROUGH) {
        using android::hardware::details::canCastInterface;

        // Passthrough services all start with minor version 0.
        // there are only three of them listed above. They are looked
        // up based on their binary location. For instance,
        // V1_0::IFoo::getService() might correspond to looking up
        // android.hardware.foo@1.0-impl for the symbol
        // HIDL_FETCH_IFoo. For @1.1::IFoo to continue to work with
        // 1.0 clients, it must also be present in a library that is
        // called the 1.0 name. Clients can say:
        //     mFoo1_0 = V1_0::IFoo::getService();
        //     mFoo1_1 = V1_1::IFoo::castFrom(mFoo1_0);
        // This is the standard pattern for making a service work
        // for both versions (mFoo1_1 != nullptr => you have 1.1)
        // and a 1.0 client still works with the 1.1 interface.

        if (!IsGoogleDefinedIface(fq_name)) {
          // This isn't the case for extensions of core Google interfaces.
          return;
        }

        const FQName lowest_name =
            fq_name.withVersion(fq_name.getPackageMajorVersion(), 0);
        hal_service = GetHalService(lowest_name, instance_name, transport);
        EXPECT_TRUE(
            canCastInterface(hal_service.get(), fq_name.string().c_str()))
            << fq_name.string() << " is not on the device.";
      } else {
        hal_service = GetHalService(fq_name, instance_name, transport);
      }

      if (hal_service == nullptr) {
        FailureHalMissing(fq_name);
        return;
      }

      EXPECT_EQ(transport == Transport::HWBINDER, hal_service->isRemote())
          << "transport is " << transport << "but HAL service is "
          << (hal_service->isRemote() ? "" : "not") << " remote.";
      EXPECT_EQ(transport == Transport::PASSTHROUGH, !hal_service->isRemote())
          << "transport is " << transport << "but HAL service is "
          << (hal_service->isRemote() ? "" : "not") << " remote.";

      if (!hal_service->isRemote()) return;

      Partition partition = GetPartition(hal_service);
      if (partition == Partition::UNKNOWN) return;
      EXPECT_EQ(expected_partition, partition)
          << fq_name.string() << " is in partition " << partition
          << " but is expected to be in " << expected_partition;
    };
  };

  auto manifest = GetParam();
  ForEachHalInstance(manifest,
                     is_available_from(PartitionOfType(manifest->type())));
}

// Tests that all HALs which are served are specified in the VINTF
// This tests (HAL is served) => (HAL in manifest)
TEST_P(SingleManifestTest, ServedHwbinderHalsAreInManifest) {
  auto manifest = GetParam();
  auto expected_partition = PartitionOfType(manifest->type());
  std::set<std::string> manifest_hwbinder_hals_ = GetHwbinderHals(manifest);

  Return<void> ret = default_manager_->list([&](const auto &list) {
    for (const auto &name : list) {
      if (std::string(name).find(IBase::descriptor) == 0) continue;

      FqInstance fqInstanceName;
      EXPECT_TRUE(fqInstanceName.setTo(name));

      auto service =
          GetHalService(toFQNameString(fqInstanceName.getPackage(),
                                       fqInstanceName.getVersion(),
                                       fqInstanceName.getInterface()),
                        fqInstanceName.getInstance(), Transport::HWBINDER);
      ASSERT_NE(service, nullptr);

      Partition partition = GetPartition(service);
      if (partition == Partition::UNKNOWN) {
        // Caught by SystemVendorTest.ServedHwbinderHalsAreInManifest
        // if that test is run.
        return;
      }
      if (partition == expected_partition) {
        EXPECT_NE(manifest_hwbinder_hals_.find(name),
                  manifest_hwbinder_hals_.end())
            << name << " is being served, but it is not in a manifest.";
      }
    }
  });
  EXPECT_TRUE(ret.isOk());
}

TEST_P(SingleManifestTest, ServedPassthroughHalsAreInManifest) {
  auto manifest = GetParam();
  std::set<std::string> manifest_passthrough_hals_ =
      GetPassthroughHals(manifest);

  auto passthrough_interfaces_declared = [&manifest_passthrough_hals_](
                                             const FQName &fq_name,
                                             const string &instance_name,
                                             Transport transport) {
    if (transport != Transport::PASSTHROUGH) return;

    // See HalsAreServed. These are always retrieved through the base interface
    // and if it is not a google defined interface, it must be an extension of
    // one.
    if (!IsGoogleDefinedIface(fq_name)) return;

    const FQName lowest_name =
        fq_name.withVersion(fq_name.getPackageMajorVersion(), 0);
    sp<IBase> hal_service =
        GetHalService(lowest_name, instance_name, transport);
    if (hal_service == nullptr) {
      ADD_FAILURE() << "Could not get service " << fq_name.string() << "/"
                    << instance_name;
      return;
    }

    Return<void> ret = hal_service->interfaceChain(
        [&manifest_passthrough_hals_, &instance_name](const auto &interfaces) {
          for (const auto &interface : interfaces) {
            if (std::string(interface) == IBase::descriptor) continue;

            const std::string instance =
                std::string(interface) + "/" + instance_name;
            EXPECT_NE(manifest_passthrough_hals_.find(instance),
                      manifest_passthrough_hals_.end())
                << "Instance missing from manifest: " << instance;
          }
        });
    EXPECT_TRUE(ret.isOk());
  };
  ForEachHalInstance(manifest, passthrough_interfaces_declared);
}

// Tests that HAL interfaces are officially released.
TEST_P(SingleManifestTest, InterfacesAreReleased) {
  // Verifies that HAL are released by fetching the hash of the interface and
  // comparing it to the set of known hashes of released interfaces.
  HalVerifyFn is_released = [](const FQName &fq_name,
                               const string &instance_name,
                               Transport transport) {
    // See HalsAreServed. These are always retrieved through the base interface
    // and if it is not a google defined interface, it must be an extension of
    // one.
    if (transport == Transport::PASSTHROUGH &&
        (!IsGoogleDefinedIface(fq_name) ||
         fq_name.getPackageMinorVersion() != 0)) {
      return;
    }

    sp<IBase> hal_service = GetHalService(fq_name, instance_name, transport);

    if (hal_service == nullptr) {
      FailureHalMissing(fq_name);
      return;
    }

    vector<string> iface_chain = GetInterfaceChain(hal_service);

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

      if (hash == Hash::hexString(Hash::kEmptyHash)) {
        FailureHashMissing(fq_iface_name);
      }

      if (IsGoogleDefinedIface(fq_iface_name)) {
        set<string> released_hashes = ReleasedHashes(fq_iface_name);
        EXPECT_NE(released_hashes.find(hash), released_hashes.end())
            << "Hash not found. This interface was not released." << endl
            << "Interface name: " << fq_iface_name.string() << endl
            << "Hash: " << hash << endl;
      }
    }
  };

  ForEachHalInstance(GetParam(), is_released);
}

}  // namespace testing
}  // namespace vintf
}  // namespace android
