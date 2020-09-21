//
// Copyright (C) 2017 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "update_engine/update_manager/enterprise_device_policy_impl.h"

#include "update_engine/common/utils.h"

using std::string;

namespace chromeos_update_manager {

// Check to see if Enterprise-managed (has DevicePolicy) and/or Kiosk-mode.  If
// so, then defer to those settings.
EvalStatus EnterpriseDevicePolicyImpl::UpdateCheckAllowed(
    EvaluationContext* ec,
    State* state,
    std::string* error,
    UpdateCheckParams* result) const {
  DevicePolicyProvider* const dp_provider = state->device_policy_provider();
  SystemProvider* const system_provider = state->system_provider();

  const bool* device_policy_is_loaded_p =
      ec->GetValue(dp_provider->var_device_policy_is_loaded());
  if (device_policy_is_loaded_p && *device_policy_is_loaded_p) {
    bool kiosk_app_control_chrome_version = false;

    // Check whether updates are disabled by policy.
    const bool* update_disabled_p =
        ec->GetValue(dp_provider->var_update_disabled());
    if (update_disabled_p && *update_disabled_p) {
      // Check whether allow kiosk app to control chrome version policy. This
      // policy is only effective when AU is disabled by admin.
      const bool* allow_kiosk_app_control_chrome_version_p = ec->GetValue(
          dp_provider->var_allow_kiosk_app_control_chrome_version());
      kiosk_app_control_chrome_version =
          allow_kiosk_app_control_chrome_version_p &&
          *allow_kiosk_app_control_chrome_version_p;
      if (!kiosk_app_control_chrome_version) {
        // No kiosk pin chrome version policy. AU is really disabled.
        LOG(INFO) << "Updates disabled by policy, blocking update checks.";
        return EvalStatus::kAskMeAgainLater;
      }
    }

    if (kiosk_app_control_chrome_version) {
      // Get the required platform version from Chrome.
      const string* kiosk_required_platform_version_p =
          ec->GetValue(system_provider->var_kiosk_required_platform_version());
      if (!kiosk_required_platform_version_p) {
        LOG(INFO) << "Kiosk app required platform version is not fetched, "
                     "blocking update checks";
        return EvalStatus::kAskMeAgainLater;
      }

      result->target_version_prefix = *kiosk_required_platform_version_p;
      LOG(INFO) << "Allow kiosk app to control Chrome version policy is set,"
                << ", target version is "
                << (kiosk_required_platform_version_p
                        ? *kiosk_required_platform_version_p
                        : std::string("latest"));
    } else {
      // Determine whether a target version prefix is dictated by policy.
      const string* target_version_prefix_p =
          ec->GetValue(dp_provider->var_target_version_prefix());
      if (target_version_prefix_p)
        result->target_version_prefix = *target_version_prefix_p;
    }

    // Determine whether a target channel is dictated by policy.
    const bool* release_channel_delegated_p =
        ec->GetValue(dp_provider->var_release_channel_delegated());
    if (release_channel_delegated_p && !(*release_channel_delegated_p)) {
      const string* release_channel_p =
          ec->GetValue(dp_provider->var_release_channel());
      if (release_channel_p)
        result->target_channel = *release_channel_p;
    }
  }
  return EvalStatus::kContinue;
}

}  // namespace chromeos_update_manager
