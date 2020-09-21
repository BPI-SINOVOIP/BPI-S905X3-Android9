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

#ifndef UTILS_NATIVE_TESTABILITY_CHECKER_H_
#define UTILS_NATIVE_TESTABILITY_CHECKER_H_

#include <set>

#include <android-base/logging.h>
#include <android/hidl/manager/1.0/IServiceManager.h>
#include <vintf/CompatibilityMatrix.h>
#include <vintf/HalManifest.h>

using android::hidl::manager::V1_0::IServiceManager;
using android::vintf::Arch;
using android::vintf::CompatibilityMatrix;
using android::vintf::HalManifest;
using android::vintf::ManifestHal;
using android::vintf::MatrixHal;
using android::vintf::Version;
using std::set;
using std::string;
using std::vector;

namespace android {
namespace vts {

// Library class to decide whether to run a test against given hal based on
// the system compatibility matrix and device manifest files. Also collect the
// instance names for testing if the decision is true.
class VtsTestabilityChecker {
 public:
  VtsTestabilityChecker(const CompatibilityMatrix* framework_comp_matrix,
                        const HalManifest* framework_hal_manifest,
                        const HalManifest* device_hal_manifest,
                        sp<IServiceManager> sm)
      : framework_comp_matrix_(framework_comp_matrix),
        framework_hal_manifest_(framework_hal_manifest),
        device_hal_manifest_(device_hal_manifest),
        sm_(sm) {
    CHECK(framework_comp_matrix_) << "framework_comp_matrix null.";
    CHECK(framework_hal_manifest_) << "framework_hal_manifest null.";
    CHECK(device_hal_manifest_) << "device_hal_manifest null.";
  };

  // Check whether we should run a compliance test against the given hal with
  // the package name, version and interface name. Arch (32 or 64) info is
  // required if the hal is a passthrough hal.
  // Return true to indicate we should run the test, false otherwise.
  // Store the instances name to run the test, instance should be empty set if
  // we determine not to run the test (i,e. return value false).
  bool CheckHalForComplianceTest(const string& hal_package_name,
                                 const Version& hal_version,
                                 const string& hal_interface_name,
                                 const Arch& arch, set<string>* instances);

  // Check whether we should run a non-compliance test against the given hal
  // with the package name, version and interface name. Arch (32 or 64) info is
  // required if the hal is a passthrough hal.
  // Return true to indicate we should run the test, false otherwise.
  // Store the instances name to run the test, instance should be empty set if
  // we determine not to run the test (i,e. return value false).
  bool CheckHalForNonComplianceTest(const string& hal_package_name,
                                    const Version& hal_version,
                                    const string& hal_interface_name,
                                    const Arch& arch, set<string>* instances);
 private:
  // Internal method to check the given hal against the framework compatibility
  // matrix and device manifest.
  // If the hal is required by the framework, return true with the corresponding
  // instance names. If the hal is optional for framework, return true if vendor
  // supports the hal with the corresponding instance names, false otherwise.
  bool CheckFrameworkCompatibleHal(const string& hal_package_name,
                                   const Version& hal_version,
                                   const string& hal_interface_name,
                                   const Arch& arch, set<string>* instances);

  // Internal method to check whether the given hal is supported by vendor
  // (i.e exists in the vendor manifest file). Store the corresponding instance
  // names if supported..
  // Arch (32 or 64) info is required if the hal is a passthrough hal.
  bool CheckVendorManifestHal(const string& hal_package_name,
                              const Version& hal_version,
                              const string& hal_interface_name,
                              const Arch& arch, set<string>* instances);

  // Internal method to check whether the given hal is supported by framework
  // (i.e exists in the framework manifest file). Store the corresponding
  // instance names if supported.
  // Arch (32 or 64) info is required if the hal is a passthrough hal.
  bool CheckFrameworkManifestHal(const string& hal_package_name,
                                 const Version& hal_version,
                                 const string& hal_interface_name,
                                 const Arch& arch, set<string>* instances);

  // Internal method to check whether the given hal is registered with
  // hwservicemanager. Store the corresponding instance names if registered.
  // This is used to check test hals that is not listed in manifest files.
  // Note this could not check for passthrough hals.
  bool CheckTestHalWithHwManager(const string& hal_package_name,
                                 const Version& hal_version,
                                 const string& hal_interface_name,
                                 set<string>* instances);

  // Helper method to check whether the hal_manifest support the
  // package@version::interface and arch (for passthrough hal). Store the
  // corresponding instance names if supported.
  bool CheckManifestHal(const HalManifest* hal_manifest,
                        const string& hal_package_name,
                        const Version& hal_version,
                        const string& hal_interface_name, const Arch& arch,
                        set<string>* instances);

  // Helper method to check whether a passthrough hal support the given arch
  // (32 or 64).
  bool CheckPassthroughManifestArch(const Arch& manifest_arch,
                                    const Arch& arch);

  // Helper method to find matching instances from a list of
  // manifest_instances.
  vector<const vintf::ManifestInstance*> FindInstance(
      const vector<vintf::ManifestInstance>& manifest_instances,
      const vintf::MatrixInstance& matrix_instance);

  // Helper method to find matching interfaces from a list of
  // manifest_instances.
  vector<const vintf::ManifestInstance*> FindInterface(
      const vector<vintf::ManifestInstance>& manifest_instances,
      const vintf::MatrixInstance& matrix_instance);

  const CompatibilityMatrix* framework_comp_matrix_;  // Do not own.
  const HalManifest* framework_hal_manifest_;         // Do not own.
  const HalManifest* device_hal_manifest_;            // Do not own.
  sp<IServiceManager> sm_;

  friend class
      VtsTestabilityCheckerTest_CheckFrameworkCompatibleHalOptional_Test;
  friend class
      VtsTestabilityCheckerTest_CheckFrameworkCompatibleHalRequired_Test;
};

}  // namespace vts
}  // namespace android
#endif  // UTILS_NATIVE_TESTABILITY_CHECKER_H_
