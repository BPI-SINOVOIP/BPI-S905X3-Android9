// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_POLICY_DEVICE_POLICY_IMPL_H_
#define LIBBRILLO_POLICY_DEVICE_POLICY_IMPL_H_

#include <set>
#include <string>
#include <vector>

#include <base/files/file_path.h>
#include <base/macros.h>

#include "bindings/chrome_device_policy.pb.h"
#include "bindings/device_management_backend.pb.h"
#include "policy/device_policy.h"

#pragma GCC visibility push(default)

namespace policy {

// This class holds device settings that are to be enforced across all users.
//
// Before serving it to the users this class verifies that the policy is valid
// against its signature and the owner's key and also that the policy files
// are owned by root.
class DevicePolicyImpl : public DevicePolicy {
 public:
  DevicePolicyImpl();
  ~DevicePolicyImpl() override;

  bool LoadPolicy() override;
  bool GetPolicyRefreshRate(int* rate) const override;
  bool GetUserWhitelist(
      std::vector<std::string>* user_whitelist) const override;
  bool GetGuestModeEnabled(bool* guest_mode_enabled) const override;
  bool GetCameraEnabled(bool* camera_enabled) const override;
  bool GetShowUserNames(bool* show_user_names) const override;
  bool GetDataRoamingEnabled(bool* data_roaming_enabled) const override;
  bool GetAllowNewUsers(bool* allow_new_users) const override;
  bool GetMetricsEnabled(bool* metrics_enabled) const override;
  bool GetReportVersionInfo(bool* report_version_info) const override;
  bool GetReportActivityTimes(bool* report_activity_times) const override;
  bool GetReportBootMode(bool* report_boot_mode) const override;
  bool GetEphemeralUsersEnabled(bool* ephemeral_users_enabled) const override;
  bool GetReleaseChannel(std::string* release_channel) const override;
  bool GetReleaseChannelDelegated(
      bool* release_channel_delegated) const override;
  bool GetUpdateDisabled(bool* update_disabled) const override;
  bool GetTargetVersionPrefix(
      std::string* target_version_prefix) const override;
  bool GetScatterFactorInSeconds(
      int64_t* scatter_factor_in_seconds) const override;
  bool GetAllowedConnectionTypesForUpdate(
      std::set<std::string>* connection_types) const override;
  bool GetOpenNetworkConfiguration(
      std::string* open_network_configuration) const override;
  bool GetOwner(std::string* owner) const override;
  bool GetHttpDownloadsEnabled(bool* http_downloads_enabled) const override;
  bool GetAuP2PEnabled(bool* au_p2p_enabled) const override;
  bool GetAllowKioskAppControlChromeVersion(
      bool* allow_kiosk_app_control_chrome_version) const override;
  bool GetUsbDetachableWhitelist(
      std::vector<UsbDeviceId>* usb_whitelist) const override;
  bool GetAutoLaunchedKioskAppId(std::string* app_id_out) const override;

  // Methods that can be used only for testing.
  void set_policy_data_for_testing(
      const enterprise_management::PolicyData& policy_data) {
    policy_data_ = policy_data;
  }
  void set_verify_root_ownership_for_testing(bool verify_root_ownership) {
    verify_root_ownership_ = verify_root_ownership;
  }
  void set_install_attributes_for_testing(
      std::unique_ptr<InstallAttributesReader> install_attributes_reader) {
    install_attributes_reader_ = std::move(install_attributes_reader);
  }
  void set_policy_path_for_testing(const base::FilePath& policy_path) {
    policy_path_ = policy_path;
  }
  void set_key_file_path_for_testing(const base::FilePath& keyfile_path) {
    keyfile_path_ = keyfile_path;
  }

 private:
  // Verifies that both the policy file and the signature file exist and are
  // owned by the root. Does nothing when |verify_root_ownership_| is set to
  // false.
  bool VerifyPolicyFile(const base::FilePath& policy_path);

  // Verifies that the policy signature is correct.
  bool VerifyPolicySignature() override;

  // Loads the signed policy off of disk from |policy_path| into |policy_|.
  // Returns true if the |policy_path| is present on disk and loading it is
  // successful.
  bool LoadPolicyFromFile(const base::FilePath& policy_path);

  base::FilePath policy_path_;
  base::FilePath keyfile_path_;
  std::unique_ptr<InstallAttributesReader> install_attributes_reader_;
  enterprise_management::PolicyFetchResponse policy_;
  enterprise_management::PolicyData policy_data_;
  enterprise_management::ChromeDeviceSettingsProto device_policy_;

  // If true, verify that policy files are owned by root. True in production
  // but can be set to false by tests.
  bool verify_root_ownership_;

  DISALLOW_COPY_AND_ASSIGN(DevicePolicyImpl);
};
}  // namespace policy

#pragma GCC visibility pop

#endif  // LIBBRILLO_POLICY_DEVICE_POLICY_IMPL_H_
