#!/usr/bin/env python3.4
#
#   Copyright 2017 - Google
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

from acts import asserts
from acts import utils
from acts.base_test import BaseTestClass
from acts.test_utils.wifi import wifi_test_utils as wutils
from acts.test_utils.wifi.aware import aware_const as aconsts
from acts.test_utils.wifi.aware import aware_test_utils as autils


class AwareBaseTest(BaseTestClass):
  def __init__(self, controllers):
    super(AwareBaseTest, self).__init__(controllers)

  # message ID counter to make sure all uses are unique
  msg_id = 0

  # offset (in seconds) to separate the start-up of multiple devices.
  # De-synchronizes the start-up time so that they don't start and stop scanning
  # at the same time - which can lead to very long clustering times.
  device_startup_offset = 2

  def setup_test(self):
    required_params = ("aware_default_power_mode", )
    self.unpack_userparams(required_params)

    for ad in self.android_devices:
      asserts.skip_if(
          not ad.droid.doesDeviceSupportWifiAwareFeature(),
          "Device under test does not support Wi-Fi Aware - skipping test")
      wutils.wifi_toggle_state(ad, True)
      ad.droid.wifiP2pClose()
      utils.set_location_service(ad, True)
      aware_avail = ad.droid.wifiIsAwareAvailable()
      if not aware_avail:
        self.log.info('Aware not available. Waiting ...')
        autils.wait_for_event(ad, aconsts.BROADCAST_WIFI_AWARE_AVAILABLE)
      ad.ed.clear_all_events()
      ad.aware_capabilities = autils.get_aware_capabilities(ad)
      self.reset_device_parameters(ad)
      self.reset_device_statistics(ad)
      self.set_power_mode_parameters(ad)
      ad.droid.wifiSetCountryCode(wutils.WifiEnums.CountryCode.US)
      autils.configure_ndp_allow_any_override(ad, True)
      # set randomization interval to 0 (disable) to reduce likelihood of
      # interference in tests
      autils.configure_mac_random_interval(ad, 0)

  def teardown_test(self):
    for ad in self.android_devices:
      if not ad.droid.doesDeviceSupportWifiAwareFeature():
        return
      ad.droid.wifiP2pClose()
      ad.droid.wifiAwareDestroyAll()
      self.reset_device_parameters(ad)
      autils.validate_forbidden_callbacks(ad)

  def reset_device_parameters(self, ad):
    """Reset device configurations which may have been set by tests. Should be
    done before tests start (in case previous one was killed without tearing
    down) and after they end (to leave device in usable state).

    Args:
      ad: device to be reset
    """
    ad.adb.shell("cmd wifiaware reset")

  def reset_device_statistics(self, ad):
    """Reset device statistics.

    Args:
        ad: device to be reset
    """
    ad.adb.shell("cmd wifiaware native_cb get_cb_count --reset")

  def set_power_mode_parameters(self, ad):
    """Set the power configuration DW parameters for the device based on any
    configuration overrides (if provided)"""
    if self.aware_default_power_mode == "INTERACTIVE":
      autils.config_settings_high_power(ad)
    elif self.aware_default_power_mode == "NON_INTERACTIVE":
      autils.config_settings_low_power(ad)
    else:
      asserts.assert_false(
          "The 'aware_default_power_mode' configuration must be INTERACTIVE or "
          "NON_INTERACTIVE"
      )

  def get_next_msg_id(self):
    """Increment the message ID and returns the new value. Guarantees that
    each call to the method returns a unique value.

    Returns: a new message id value.
    """
    self.msg_id = self.msg_id + 1
    return self.msg_id

  def on_fail(self, test_name, begin_time):
    for ad in self.android_devices:
      ad.take_bug_report(test_name, begin_time)
      ad.cat_adb_log(test_name, begin_time)
