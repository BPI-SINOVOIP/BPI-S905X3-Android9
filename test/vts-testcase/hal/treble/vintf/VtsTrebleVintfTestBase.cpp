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

#include "VtsTrebleVintfTestBase.h"

#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <iostream>
#include <map>
#include <mutex>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <android-base/logging.h>
#include <android-base/strings.h>
#include <android/hidl/manager/1.0/IServiceManager.h>
#include <gtest/gtest.h>
#include <hidl-hash/Hash.h>
#include <hidl-util/FQName.h>
#include <hidl-util/FqInstance.h>
#include <hidl/HidlTransportUtils.h>
#include <hidl/ServiceManagement.h>
#include <procpartition/procpartition.h>
#include <vintf/HalManifest.h>
#include <vintf/VintfObject.h>
#include <vintf/parse_string.h>

#include "SingleManifestTest.h"
#include "utils.h"

namespace android {
namespace vintf {
namespace testing {

using android::FqInstance;
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
using android::vintf::Transport;
using android::vintf::Version;
using android::vintf::VintfObject;
using android::vintf::operator<<;
using android::vintf::to_string;
using android::vintf::toFQNameString;

using std::cout;
using std::endl;
using std::map;
using std::set;
using std::string;
using std::vector;

void VtsTrebleVintfTestBase::SetUp() {
  default_manager_ = ::android::hardware::defaultServiceManager();
  ASSERT_NE(default_manager_, nullptr)
      << "Failed to get default service manager." << endl;
}

void VtsTrebleVintfTestBase::ForEachHalInstance(const HalManifestPtr &manifest,
                                                HalVerifyFn fn) {
  manifest->forEachInstance([manifest, fn](const auto &manifest_instance) {
    const FQName fq_name{manifest_instance.package(),
                         to_string(manifest_instance.version()),
                         manifest_instance.interface()};
    const Transport transport = manifest_instance.transport();
    const std::string instance_name = manifest_instance.instance();

    auto future_result =
        std::async([&]() { fn(fq_name, instance_name, transport); });
    auto timeout = std::chrono::seconds(1);
    std::future_status status = future_result.wait_for(timeout);
    if (status != std::future_status::ready) {
      cout << "Timed out on: " << fq_name.string() << " " << instance_name
           << endl;
    }
    return true;  // continue to next instance
  });
}

sp<IBase> VtsTrebleVintfTestBase::GetHalService(const FQName &fq_name,
                                                const string &instance_name,
                                                Transport transport, bool log) {
  return GetHalService(fq_name.string(), instance_name, transport, log);
}

sp<IBase> VtsTrebleVintfTestBase::GetHalService(const string &fq_name,
                                                const string &instance_name,
                                                Transport transport, bool log) {
  using android::hardware::details::getRawServiceInternal;

  if (log) {
    cout << "Getting: " << fq_name << "/" << instance_name << endl;
  }

  // getService blocks until a service is available. In 100% of other cases
  // where getService is used, it should be called directly. However, this test
  // enforces that various services are actually available when they are
  // declared, it must make a couple of precautions in case the service isn't
  // actually available so that the proper failure can be reported.

  auto task = std::packaged_task<sp<IBase>()>([fq_name, instance_name]() {
    return getRawServiceInternal(fq_name, instance_name, true /* retry */,
                                 false /* getStub */);
  });

  std::future<sp<IBase>> future = task.get_future();
  std::thread(std::move(task)).detach();
  auto status = future.wait_for(std::chrono::milliseconds(500));

  if (status != std::future_status::ready) return nullptr;

  sp<IBase> base = future.get();
  if (base == nullptr) return nullptr;

  bool wantRemote = transport == Transport::HWBINDER;
  if (base->isRemote() != wantRemote) return nullptr;

  return base;
}

vector<string> VtsTrebleVintfTestBase::GetInstanceNames(
    const sp<IServiceManager> &manager, const FQName &fq_name) {
  vector<string> ret;
  auto status =
      manager->listByInterface(fq_name.string(), [&](const auto &out) {
        for (const auto &e : out) ret.push_back(e);
      });
  EXPECT_TRUE(status.isOk()) << status.description();
  return ret;
}

vector<string> VtsTrebleVintfTestBase::GetInterfaceChain(
    const sp<IBase> &service) {
  vector<string> iface_chain{};
  service->interfaceChain([&iface_chain](const hidl_vec<hidl_string> &chain) {
    for (const auto &iface_name : chain) {
      iface_chain.push_back(iface_name);
    }
  });
  return iface_chain;
}

Partition VtsTrebleVintfTestBase::GetPartition(sp<IBase> hal_service) {
  Partition partition = Partition::UNKNOWN;
  auto ret = hal_service->getDebugInfo(
      [&](const auto &info) { partition = PartitionOfProcess(info.pid); });
  EXPECT_TRUE(ret.isOk());
  return partition;
}

set<string> VtsTrebleVintfTestBase::GetPassthroughHals(
    HalManifestPtr manifest) {
  std::set<std::string> manifest_passthrough_hals_;

  auto add_manifest_hals = [&manifest_passthrough_hals_](
                               const FQName &fq_name,
                               const string &instance_name,
                               Transport transport) {
    if (transport == Transport::HWBINDER) {
      // ignore
    } else if (transport == Transport::PASSTHROUGH) {
      // 1.n in manifest => 1.0, 1.1, ... 1.n are all served (if they exist)
      FQName fq = fq_name;
      while (true) {
        manifest_passthrough_hals_.insert(fq.string() + "/" + instance_name);
        if (fq.getPackageMinorVersion() <= 0) break;
        fq = fq.downRev();
      }
    } else {
      ADD_FAILURE() << "Unrecognized transport: " << transport;
    }
  };
  ForEachHalInstance(manifest, add_manifest_hals);
  return manifest_passthrough_hals_;
}

set<string> VtsTrebleVintfTestBase::GetHwbinderHals(HalManifestPtr manifest) {
  std::set<std::string> manifest_hwbinder_hals_;

  auto add_manifest_hals = [&manifest_hwbinder_hals_](
                               const FQName &fq_name,
                               const string &instance_name,
                               Transport transport) {
    if (transport == Transport::HWBINDER) {
      // 1.n in manifest => 1.0, 1.1, ... 1.n are all served (if they exist)
      FQName fq = fq_name;
      while (true) {
        manifest_hwbinder_hals_.insert(fq.string() + "/" + instance_name);
        if (fq.getPackageMinorVersion() <= 0) break;
        fq = fq.downRev();
      }
    } else if (transport == Transport::PASSTHROUGH) {
      // ignore
    } else {
      ADD_FAILURE() << "Unrecognized transport: " << transport;
    }
  };
  ForEachHalInstance(manifest, add_manifest_hals);
  return manifest_hwbinder_hals_;
}

}  // namespace testing
}  // namespace vintf
}  // namespace android
