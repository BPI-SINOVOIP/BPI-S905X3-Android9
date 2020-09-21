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
#define LOG_TAG "VtsTestabilityChecker"

#include "VtsTestabilityChecker.h"

#include <algorithm>
#include <iostream>
#include <set>

#include <android-base/strings.h>
#include <vintf/parse_string.h>

using android::base::Join;
using android::vintf::Arch;
using android::vintf::CompatibilityMatrix;
using android::vintf::gArchStrings;
using android::vintf::HalManifest;
using android::vintf::ManifestHal;
using android::vintf::ManifestInstance;
using android::vintf::MatrixHal;
using android::vintf::MatrixInstance;
using android::vintf::toFQNameString;
using android::vintf::Transport;
using android::vintf::Version;
using android::vintf::operator<<;
using std::set;
using std::string;
using std::vector;

namespace android {
namespace vts {

bool VtsTestabilityChecker::CheckHalForComplianceTest(
    const string& hal_package_name, const Version& hal_version,
    const string& hal_interface_name, const Arch& arch,
    set<string>* instances) {
  CHECK(instances) << "instances set should not be NULL.";
  set<string> famework_hal_instances;
  set<string> vendor_hal_instances;
  bool check_framework_hal = CheckFrameworkManifestHal(
      hal_package_name, hal_version, hal_interface_name, arch,
      &famework_hal_instances);
  bool check_vendor_hal =
      CheckVendorManifestHal(hal_package_name, hal_version, hal_interface_name,
                             arch, &vendor_hal_instances);
  set_union(famework_hal_instances.begin(), famework_hal_instances.end(),
            vendor_hal_instances.begin(), vendor_hal_instances.end(),
            std::inserter(*instances, instances->begin()));
  return check_framework_hal || check_vendor_hal;
}

bool VtsTestabilityChecker::CheckHalForNonComplianceTest(
    const string& hal_package_name, const Version& hal_version,
    const string& hal_interface_name, const Arch& arch,
    set<string>* instances) {
  CHECK(instances) << "instances set should not be NULL.";
  set<string> vendor_hal_instances;
  set<string> test_hal_instances;
  bool check_vendor_hal =
      CheckVendorManifestHal(hal_package_name, hal_version, hal_interface_name,
                             arch, &vendor_hal_instances);

  bool check_test_hal = CheckTestHalWithHwManager(
      hal_package_name, hal_version, hal_interface_name, &test_hal_instances);

  set_union(vendor_hal_instances.begin(), vendor_hal_instances.end(),
            test_hal_instances.begin(), test_hal_instances.end(),
            std::inserter(*instances, instances->begin()));
  return check_vendor_hal || check_test_hal;
}

vector<const ManifestInstance*> VtsTestabilityChecker::FindInstance(
    const vector<ManifestInstance>& manifest_instances,
    const MatrixInstance& matrix_instance) {
  vector<const ManifestInstance*> ret;
  for (const auto& e : manifest_instances) {
    if (matrix_instance.matchInstance(e.instance())) {
      ret.push_back(&e);
    }
  }
  return ret;
}

vector<const ManifestInstance*> VtsTestabilityChecker::FindInterface(
    const vector<ManifestInstance>& manifest_instances,
    const MatrixInstance& matrix_instance) {
  vector<const ManifestInstance*> ret;
  for (const auto& e : manifest_instances) {
    if (e.interface() == matrix_instance.interface()) {
      ret.push_back(&e);
    }
  }
  return ret;
}

bool VtsTestabilityChecker::CheckFrameworkCompatibleHal(
    const string& hal_package_name, const Version& hal_version,
    const string& hal_interface_name, const Arch& arch,
    set<string>* instances) {
  CHECK(instances) << "instances set should not be NULL.";

  auto matrix_instances = framework_comp_matrix_->getFqInstances(
      hal_package_name, hal_version, hal_interface_name);
  auto manifest_instances = device_hal_manifest_->getFqInstances(
      hal_package_name, hal_version, hal_interface_name);

  bool testable = false;

  for (const auto& matrix_instance : matrix_instances) {
    const auto& matched_instances =
        FindInstance(manifest_instances, matrix_instance);
    if (!matrix_instance.optional() && matched_instances.empty()) {
      // In matrix but not in manifest.
      // The test should still run, but expect the test
      // to fail (due to incompatible vendor and framework HAL).
      LOG(ERROR) << "Compatibility error. Hal " << hal_package_name
                 << " is required by framework but not supported by vendor";
      if (!hal_interface_name.empty()) {
        if (!matrix_instance.isRegex()) {
          instances->insert(matrix_instance.exactInstance());
        } else {
          LOG(ERROR) << "Ignore regex-instance '"
                     << matrix_instance.regexPattern();
        }
      }
      testable |= true;
      continue;
    }

    if (hal_interface_name.empty()) {
      testable |= !matched_instances.empty();
      continue;
    }

    auto get_testable_instances =
        [&](const vector<const vintf::ManifestInstance*>& manifest_instances) {
          vector<string> ret;
          for (const auto& manifest_instance : manifest_instances) {
            if ((manifest_instance->transport() == Transport::PASSTHROUGH &&
                 CheckPassthroughManifestArch(manifest_instance->arch(),
                                              arch)) ||
                manifest_instance->transport() == Transport::HWBINDER) {
              ret.push_back(manifest_instance->instance());
            }
          }
          return ret;
        };

    auto testable_instances = get_testable_instances(matched_instances);
    if (!testable_instances.empty()) {
      instances->insert(testable_instances.begin(), testable_instances.end());
      testable |= true;
      continue;
    }

    // Special case: if a.h.foo@1.0::IFoo/default is in matrix but /custom
    // is in manifest, the interface is still testable, but /default should
    // not be added to instances.
    const auto& matched_interface_instances =
        FindInterface(manifest_instances, matrix_instance);
    auto testable_interfaces =
        get_testable_instances(matched_interface_instances);
    if (!testable_interfaces.empty()) {
      testable |= true;
      continue;
    }
  }
  if (instances->empty()) {
    LOG(ERROR) << "Hal "
               << toFQNameString(hal_package_name, hal_version,
                                 hal_interface_name)
               << " has no testable instance";
  }
  return testable;
}

bool VtsTestabilityChecker::CheckPassthroughManifestArch(
    const Arch& manifest_arch, const Arch& arch) {
  switch (arch) {
    case Arch::ARCH_32: {
      if (android::vintf::has32(manifest_arch)) {
        return true;
      }
      break;
    }
    case Arch::ARCH_64: {
      if (android::vintf::has64(manifest_arch)) {
        return true;
      }
      break;
    }
    default: {
      LOG(ERROR) << "Unexpected arch to check: " << arch;
      break;
    }
  }
  return false;
}

bool VtsTestabilityChecker::CheckFrameworkManifestHal(
    const string& hal_package_name, const Version& hal_version,
    const string& hal_interface_name, const Arch& arch,
    set<string>* instances) {
  return CheckManifestHal(framework_hal_manifest_, hal_package_name,
                          hal_version, hal_interface_name, arch, instances);
}

bool VtsTestabilityChecker::CheckVendorManifestHal(
    const string& hal_package_name, const Version& hal_version,
    const string& hal_interface_name, const Arch& arch,
    set<string>* instances) {
  return CheckManifestHal(device_hal_manifest_, hal_package_name, hal_version,
                          hal_interface_name, arch, instances);
}

bool VtsTestabilityChecker::CheckManifestHal(const HalManifest* hal_manifest,
                                             const string& hal_package_name,
                                             const Version& hal_version,
                                             const string& hal_interface_name,
                                             const Arch& arch,
                                             set<string>* instances) {
  CHECK(instances) << "instances set should not be NULL.";

  const auto& manifest_instances = hal_manifest->getFqInstances(
      hal_package_name, hal_version, hal_interface_name);

  const auto& fq_instance_name =
      toFQNameString(hal_package_name, hal_version, hal_interface_name);

  if (manifest_instances.empty()) {
    LOG(DEBUG) << "Does not find instances for " << fq_instance_name
               << " in manifest file";
    return false;
  }

  bool testable = false;
  for (const auto& manifest_instance : manifest_instances) {
    if (manifest_instance.transport() == Transport::PASSTHROUGH &&
        !CheckPassthroughManifestArch(manifest_instance.arch(), arch)) {
      LOG(DEBUG) << "Manifest HAL " << fq_instance_name
                 << " is passthrough and does not support arch " << arch;
      continue;  // skip this instance
    }
    if (!hal_interface_name.empty()) {
      instances->insert(manifest_instance.instance());
    }
    testable = true;
  }
  return testable;
}

bool VtsTestabilityChecker::CheckTestHalWithHwManager(
    const string& hal_package_name, const Version& hal_version,
    const string& hal_interface_name, set<string>* instances) {
  CHECK(instances) << "instances set should not be NULL.";

  string fqName =
      toFQNameString(hal_package_name, hal_version, hal_interface_name);
  bool registered = false;
  hardware::Return<void> res;
  if (!hal_interface_name.empty()) {
    res = sm_->listByInterface(fqName, [&](const auto& registered_instances) {
      for (const string& instance : registered_instances) {
        registered = true;
        instances->insert(instance);
      }
    });
  } else {  // handle legacy data without interface info.
    res = sm_->list([&](const auto& services) {
      for (const string& service : services) {
        if (service.find(fqName) == 0) {
          registered = true;
          break;
        }
      }
    });
  }
  if (!res.isOk()) {
    LOG(ERROR) << "failed to check services: " << res.description();
    return false;
  }
  return registered;
}

}  // namespace vts
}  // namespace android
