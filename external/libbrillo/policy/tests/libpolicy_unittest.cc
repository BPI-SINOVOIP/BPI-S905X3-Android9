// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "policy/libpolicy.h"

#include <openssl/err.h>
#include <openssl/ssl.h>

#include <base/files/file_path.h>
#include <base/logging.h>
#include <gtest/gtest.h>

#include "policy/device_policy_impl.h"

namespace policy {

static const char kPolicyFileAllSet[] = "policy/tests/whitelist/policy_all";
static const char kPolicyFileNoneSet[] = "policy/tests/whitelist/policy_none";
static const char kKeyFile[] = "policy/tests/whitelist/owner.key";

// Creates the DevicePolicyImpl with given parameters for test.
std::unique_ptr<DevicePolicyImpl> CreateDevicePolicyImpl(
    std::unique_ptr<InstallAttributesReader> install_attributes_reader,
    const base::FilePath& policy_path,
    const base::FilePath& keyfile_path,
    bool verify_files) {
  std::unique_ptr<DevicePolicyImpl> device_policy(new DevicePolicyImpl());
  device_policy->set_install_attributes_for_testing(
      std::move(install_attributes_reader));
  device_policy->set_policy_path_for_testing(policy_path);
  device_policy->set_key_file_path_for_testing(keyfile_path);
  device_policy->set_verify_root_ownership_for_testing(verify_files);

  return device_policy;
}

// Test that a policy file can be verified and parsed correctly. The file
// contains all possible fields, so reading should succeed for all.
TEST(PolicyTest, DevicePolicyAllSetTest) {
  base::FilePath policy_file(kPolicyFileAllSet);
  base::FilePath key_file(kKeyFile);
  PolicyProvider provider(
      CreateDevicePolicyImpl(std::make_unique<MockInstallAttributesReader>(
                                 cryptohome::SerializedInstallAttributes()),
                             policy_file, key_file, false));
  provider.Reload();

  // Ensure we successfully loaded the device policy file.
  ASSERT_TRUE(provider.device_policy_is_loaded());

  const DevicePolicy& policy = provider.GetDevicePolicy();

  // Check that we can read out all fields of the sample protobuf.
  int int_value = -1;
  ASSERT_TRUE(policy.GetPolicyRefreshRate(&int_value));
  ASSERT_EQ(100, int_value);

  std::vector<std::string> list_value;
  ASSERT_TRUE(policy.GetUserWhitelist(&list_value));
  ASSERT_EQ(3, list_value.size());
  ASSERT_EQ("me@here.com", list_value[0]);
  ASSERT_EQ("you@there.com", list_value[1]);
  ASSERT_EQ("*@monsters.com", list_value[2]);

  bool bool_value = true;
  ASSERT_TRUE(policy.GetGuestModeEnabled(&bool_value));
  ASSERT_FALSE(bool_value);

  bool_value = true;
  ASSERT_TRUE(policy.GetCameraEnabled(&bool_value));
  ASSERT_FALSE(bool_value);

  bool_value = true;
  ASSERT_TRUE(policy.GetShowUserNames(&bool_value));
  ASSERT_FALSE(bool_value);

  bool_value = true;
  ASSERT_TRUE(policy.GetDataRoamingEnabled(&bool_value));
  ASSERT_FALSE(bool_value);

  bool_value = true;
  ASSERT_TRUE(policy.GetAllowNewUsers(&bool_value));
  ASSERT_FALSE(bool_value);

  bool_value = true;
  ASSERT_TRUE(policy.GetMetricsEnabled(&bool_value));
  ASSERT_FALSE(bool_value);

  bool_value = true;
  ASSERT_TRUE(policy.GetReportVersionInfo(&bool_value));
  ASSERT_FALSE(bool_value);

  bool_value = true;
  ASSERT_TRUE(policy.GetReportActivityTimes(&bool_value));
  ASSERT_FALSE(bool_value);

  bool_value = true;
  ASSERT_TRUE(policy.GetReportBootMode(&bool_value));
  ASSERT_FALSE(bool_value);

  bool_value = true;
  ASSERT_TRUE(policy.GetEphemeralUsersEnabled(&bool_value));
  ASSERT_FALSE(bool_value);

  std::string string_value;
  ASSERT_TRUE(policy.GetReleaseChannel(&string_value));
  ASSERT_EQ("stable-channel", string_value);

  bool_value = false;
  ASSERT_TRUE(policy.GetReleaseChannelDelegated(&bool_value));
  ASSERT_TRUE(bool_value);

  bool_value = true;
  ASSERT_TRUE(policy.GetUpdateDisabled(&bool_value));
  ASSERT_FALSE(bool_value);

  int64_t int64_value = -1LL;
  ASSERT_TRUE(policy.GetScatterFactorInSeconds(&int64_value));
  ASSERT_EQ(17LL, int64_value);

  ASSERT_TRUE(policy.GetTargetVersionPrefix(&string_value));
  ASSERT_EQ("42.0.", string_value);

  std::set<std::string> types;
  ASSERT_TRUE(policy.GetAllowedConnectionTypesForUpdate(&types));
  ASSERT_TRUE(types.end() != types.find("ethernet"));
  ASSERT_TRUE(types.end() != types.find("wifi"));
  ASSERT_EQ(2, types.size());

  ASSERT_TRUE(policy.GetOpenNetworkConfiguration(&string_value));
  ASSERT_EQ("{}", string_value);

  ASSERT_TRUE(policy.GetOwner(&string_value));
  ASSERT_EQ("", string_value);

  bool_value = true;
  ASSERT_TRUE(policy.GetHttpDownloadsEnabled(&bool_value));
  ASSERT_FALSE(bool_value);

  bool_value = true;
  ASSERT_TRUE(policy.GetAuP2PEnabled(&bool_value));
  ASSERT_FALSE(bool_value);

  bool_value = true;
  ASSERT_TRUE(policy.GetAllowKioskAppControlChromeVersion(&bool_value));
  ASSERT_FALSE(bool_value);

  std::vector<DevicePolicy::UsbDeviceId> list_device;
  ASSERT_TRUE(policy.GetUsbDetachableWhitelist(&list_device));
  ASSERT_EQ(2, list_device.size());
  ASSERT_EQ(0x413c, list_device[0].vendor_id);
  ASSERT_EQ(0x2105, list_device[0].product_id);
  ASSERT_EQ(0x0403, list_device[1].vendor_id);
  ASSERT_EQ(0x6001, list_device[1].product_id);

  ASSERT_TRUE(policy.GetAutoLaunchedKioskAppId(&string_value));
  ASSERT_EQ("my_kiosk_app", string_value);

  // Reloading the protobuf should succeed.
  ASSERT_TRUE(provider.Reload());
}

// Test that a policy file can be verified and parsed correctly. The file
// contains none of the possible fields, so reading should fail for all.
TEST(PolicyTest, DevicePolicyNoneSetTest) {
  base::FilePath policy_file(kPolicyFileNoneSet);
  base::FilePath key_file(kKeyFile);

  PolicyProvider provider(
      CreateDevicePolicyImpl(std::make_unique<MockInstallAttributesReader>(
                                 cryptohome::SerializedInstallAttributes()),
                             policy_file, key_file, false));
  provider.Reload();

  // Ensure we successfully loaded the device policy file.
  ASSERT_TRUE(provider.device_policy_is_loaded());

  const DevicePolicy& policy = provider.GetDevicePolicy();

  // Check that we cannot read any fields out of the sample protobuf.
  int int_value;
  int64_t int64_value;
  std::vector<std::string> list_value;
  bool bool_value;
  std::string string_value;
  std::vector<DevicePolicy::UsbDeviceId> list_device;

  ASSERT_FALSE(policy.GetPolicyRefreshRate(&int_value));
  ASSERT_FALSE(policy.GetUserWhitelist(&list_value));
  ASSERT_FALSE(policy.GetGuestModeEnabled(&bool_value));
  ASSERT_FALSE(policy.GetCameraEnabled(&bool_value));
  ASSERT_FALSE(policy.GetShowUserNames(&bool_value));
  ASSERT_FALSE(policy.GetDataRoamingEnabled(&bool_value));
  ASSERT_FALSE(policy.GetAllowNewUsers(&bool_value));
  ASSERT_FALSE(policy.GetMetricsEnabled(&bool_value));
  ASSERT_FALSE(policy.GetReportVersionInfo(&bool_value));
  ASSERT_FALSE(policy.GetReportActivityTimes(&bool_value));
  ASSERT_FALSE(policy.GetReportBootMode(&bool_value));
  ASSERT_FALSE(policy.GetEphemeralUsersEnabled(&bool_value));
  ASSERT_FALSE(policy.GetReleaseChannel(&string_value));
  ASSERT_FALSE(policy.GetUpdateDisabled(&bool_value));
  ASSERT_FALSE(policy.GetTargetVersionPrefix(&string_value));
  ASSERT_FALSE(policy.GetScatterFactorInSeconds(&int64_value));
  ASSERT_FALSE(policy.GetOpenNetworkConfiguration(&string_value));
  ASSERT_FALSE(policy.GetHttpDownloadsEnabled(&bool_value));
  ASSERT_FALSE(policy.GetAuP2PEnabled(&bool_value));
  ASSERT_FALSE(policy.GetAllowKioskAppControlChromeVersion(&bool_value));
  ASSERT_FALSE(policy.GetUsbDetachableWhitelist(&list_device));
}

// Verify that the library will correctly recognize and signal missing files.
TEST(PolicyTest, DevicePolicyFailure) {
  LOG(INFO) << "Errors expected.";
  // Try loading non-existing protobuf should fail.
  base::FilePath non_existing("this_file_is_doof");
  PolicyProvider provider(
      CreateDevicePolicyImpl(std::make_unique<MockInstallAttributesReader>(
                                 cryptohome::SerializedInstallAttributes()),
                             non_existing,
                             non_existing,
                             true));

  // Even after reload the policy should still be not loaded.
  ASSERT_FALSE(provider.Reload());
  ASSERT_FALSE(provider.device_policy_is_loaded());
}

}  // namespace policy

int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
