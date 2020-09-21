#!/usr/bin/python3.4
#
#   Copyright 2017 - The Android Open Source Project
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
from acts.test_decorators import test_tracker_info
from acts.test_utils.wifi.rtt import rtt_const as rconsts
from acts.test_utils.wifi.rtt import rtt_test_utils as rutils
from acts.test_utils.wifi.rtt.RttBaseTest import RttBaseTest
from acts.test_utils.wifi.WifiBaseTest import WifiBaseTest


class RttDisableTest(WifiBaseTest, RttBaseTest):
  """Test class for RTT ranging enable/disable flows."""

  MODE_DISABLE_WIFI = 0
  MODE_ENABLE_DOZE = 1
  MODE_DISABLE_LOCATIONING = 2

  def __init__(self, controllers):
    WifiBaseTest.__init__(self, controllers)
    RttBaseTest.__init__(self, controllers)
    if "AccessPoint" in self.user_params:
      self.legacy_configure_ap_and_start()

  def run_disable_rtt(self, disable_mode):
    """Validate the RTT disabled flows: whether by disabling Wi-Fi or entering
    doze mode.

    Args:
      disable_mode: The particular mechanism in which RTT is disabled. One of
                    the MODE_* constants.
    """
    dut = self.android_devices[0]

    # validate start-up conditions
    asserts.assert_true(dut.droid.wifiIsRttAvailable(), "RTT is not available")

    # scan to get some APs to be used later
    all_aps = rutils.select_best_scan_results(rutils.scan_networks(dut),
                                              select_count=1)
    asserts.assert_true(len(all_aps) > 0, "Need at least one visible AP!")

    # disable RTT and validate broadcast & API
    if disable_mode == self.MODE_DISABLE_WIFI:
      # disabling Wi-Fi is not sufficient: since scan mode (and hence RTT) will
      # remain enabled - we need to disable the Wi-Fi chip aka Airplane Mode
      asserts.assert_true(utils.force_airplane_mode(dut, True),
                          "Can not turn on airplane mode on: %s" % dut.serial)
    elif disable_mode == self.MODE_ENABLE_DOZE:
      asserts.assert_true(utils.enable_doze(dut), "Can't enable doze")
    elif disable_mode == self.MODE_DISABLE_LOCATIONING:
      utils.set_location_service(dut, False)

    rutils.wait_for_event(dut, rconsts.BROADCAST_WIFI_RTT_NOT_AVAILABLE)
    asserts.assert_false(dut.droid.wifiIsRttAvailable(), "RTT is available")

    # request a range and validate error
    id = dut.droid.wifiRttStartRangingToAccessPoints(all_aps[0:1])
    event = rutils.wait_for_event(dut, rutils.decorate_event(
        rconsts.EVENT_CB_RANGING_ON_FAIL, id))
    asserts.assert_equal(event["data"][rconsts.EVENT_CB_RANGING_KEY_STATUS],
                         rconsts.RANGING_FAIL_CODE_RTT_NOT_AVAILABLE,
                         "Invalid error code")

    # enable RTT and validate broadcast & API
    if disable_mode == self.MODE_DISABLE_WIFI:
      asserts.assert_true(utils.force_airplane_mode(dut, False),
                          "Can not turn off airplane mode on: %s" % dut.serial)
    elif disable_mode == self.MODE_ENABLE_DOZE:
      asserts.assert_true(utils.disable_doze(dut), "Can't disable doze")
    elif disable_mode == self.MODE_DISABLE_LOCATIONING:
      utils.set_location_service(dut, True)

    rutils.wait_for_event(dut, rconsts.BROADCAST_WIFI_RTT_AVAILABLE)
    asserts.assert_true(dut.droid.wifiIsRttAvailable(), "RTT is not available")

  ############################################################################

  @test_tracker_info(uuid="498c49ab-a188-4612-998d-c47b35ff285e")
  def test_disable_wifi(self):
    """Validate that getting expected broadcast when Wi-Fi is disabled and that
    any range requests are rejected."""
    self.run_disable_rtt(self.MODE_DISABLE_WIFI)

  @test_tracker_info(uuid="f71f731f-4aaf-402b-8595-db94b625b544")
  def test_enable_doze(self):
    """Validate that getting expected broadcast when RTT is disabled due to doze
    mode and that any range requests are rejected."""
    self.run_disable_rtt(self.MODE_ENABLE_DOZE)

  @test_tracker_info(uuid="6a1c83a8-9eaf-49db-b547-5131cba0eafe")
  def test_disable_location(self):
    """Validate that getting expected broadcast when locationing is disabled and
    that any range requests are rejected."""
    self.run_disable_rtt(self.MODE_DISABLE_LOCATIONING)
