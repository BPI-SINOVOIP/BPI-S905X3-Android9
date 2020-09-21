/*
 * Copyright 2017 The Android Open Source Project
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
#include <getopt.h>
#include <json/json.h>
#include <iostream>

#include <hidl-util/FQName.h>
#include <hidl/ServiceManagement.h>
#include <vintf/VintfObject.h>

#include "VtsTestabilityChecker.h"

using android::hidl::manager::V1_0::IServiceManager;
using android::vintf::VintfObject;
using std::cerr;
using std::cout;
using std::endl;

// Example:
// vts_testability_checker --compliance android.hardware.foo@1.0::IFoo
// vts_testability_checker -c -b 32 android.hardware.foo@1.0::IFoo
// vts_testability_checker android.hardware.foo@1.0
void ShowUsage() {
  cout << "Usage: vts_hal_testability_checker [options] <hal name>\n"
          "--compliance:     Whether to check for compliance test.\n"
          "--bitness:        Test bitness.\n"
          "--help:           Show help\n";
  exit(1);
}

const option long_opts[] = {{"help", no_argument, nullptr, 'h'},
                            {"compliance", no_argument, nullptr, 'c'},
                            {"bitness", required_argument, nullptr, 'b'},
                            {nullptr, 0, nullptr, 0}};

int main(int argc, char** argv) {
  string bitness = "32";
  bool is_compliance_test = false;

  while (true) {
    int opt = getopt_long(argc, argv, "hcb:", long_opts, nullptr);
    if (opt == -1) {
      break;
    }

    switch (opt) {
      case 'h':
      case '?':
        ShowUsage();
        return 0;
      case 'c': {
        is_compliance_test = true;
        break;
      }
      case 'b': {
        bitness = string(optarg);
        if (bitness != "32" && bitness != "64") {
          cerr << "Invalid bitness " << bitness << endl;
          return -1;
        }
        break;
      }
      default:
        cerr << "getopt_long returned unexpected value " << opt << endl;
        return -1;
    }
  }

  if (optind != argc - 1) {
    cerr << "Must specify the hal name (see --help)." << endl;
    return -1;
  }

  android::FQName hal_fq_name = android::FQName(argv[optind]);
  if (!hal_fq_name.isValid()) {
    cerr << "Invalid hal name: " << argv[optind] << endl;
    return -1;
  }

  string hal_package_name = hal_fq_name.package();
  size_t hal_major_version = hal_fq_name.getPackageMajorVersion();
  size_t hal_minor_version = hal_fq_name.getPackageMinorVersion();
  string hal_interface_name = hal_fq_name.name();
  Version hal_version(hal_major_version, hal_minor_version);

  Arch arch;
  if (bitness == "32") {
    arch = Arch::ARCH_32;
  } else if (bitness == "64") {
    arch = Arch::ARCH_64;
  }

  auto framework_comp_matrix = VintfObject::GetFrameworkCompatibilityMatrix();
  if (!framework_comp_matrix) {
    cerr << "Failed to get framework compatibility matrix." << endl;
    return -1;
  }

  auto device_hal_manifest = VintfObject::GetDeviceHalManifest();
  if (!device_hal_manifest) {
    cerr << "Failed to get device manifest." << endl;
    return -1;
  }

  auto framework_hal_manifest = VintfObject::GetFrameworkHalManifest();
  if (!framework_hal_manifest) {
    cerr << "Failed to get framework manifest." << endl;
    return -1;
  }

  android::sp<IServiceManager> sm =
      ::android::hardware::defaultServiceManager();
  if (sm == nullptr) {
    cerr << "failed to get IServiceManager" << endl;
    return -1;
  }

  android::vts::VtsTestabilityChecker checker(framework_comp_matrix.get(),
                                              framework_hal_manifest.get(),
                                              device_hal_manifest.get(), sm);
  set<string> instances;
  bool result = false;
  if (is_compliance_test) {
    result = checker.CheckHalForComplianceTest(
        hal_package_name, hal_version, hal_interface_name, arch, &instances);
  } else {
    result = checker.CheckHalForNonComplianceTest(
        hal_package_name, hal_version, hal_interface_name, arch, &instances);
  }

  // Create json output.
  Json::Value root(Json::objectValue);
  root["Testable"] = Json::Value(result);
  root["instances"] = Json::Value(Json::arrayValue);
  for (const string& instance : instances) {
    root["instances"].append(instance);
  }

  Json::FastWriter writer;
  std::string json_output = writer.write(root);

  cout << json_output;
  return 0;
}
