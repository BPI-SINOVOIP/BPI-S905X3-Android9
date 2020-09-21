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
from acts.test_decorators import test_tracker_info
from acts.test_utils.wifi.WifiBaseTest import WifiBaseTest
from acts.test_utils.wifi.rtt import rtt_const as rconsts
from acts.test_utils.wifi.rtt import rtt_test_utils as rutils
from acts.test_utils.wifi.rtt.RttBaseTest import RttBaseTest


class RangeApNonSupporting11McTest(WifiBaseTest, RttBaseTest):
  """Test class for RTT ranging to Access Points which do not support IEEE
  802.11mc"""

  # Number of RTT iterations
  NUM_ITER = 10

  # Time gap (in seconds) between iterations
  TIME_BETWEEN_ITERATIONS = 0

  def __init__(self, controllers):
    WifiBaseTest.__init__(self, controllers)
    RttBaseTest.__init__(self, controllers)
    if "AccessPoint" in self.user_params:
      self.legacy_configure_ap_and_start()

  #############################################################################

  @test_tracker_info(uuid="cde756e9-11f3-43da-b9ae-9edf85764f82")
  def test_rtt_non_80211mc_supporting_aps(self):
    """Scan for APs and perform RTT on non-IEEE 802.11mc supporting APs"""
    dut = self.android_devices[0]
    non_rtt_aps = rutils.select_best_scan_results(
      rutils.scan_with_rtt_support_constraint(dut, False), select_count=1)
    dut.log.debug("Visible non-IEEE 802.11mc APs=%s", non_rtt_aps)
    asserts.assert_true(len(non_rtt_aps) > 0, "Need at least one AP!")
    events = rutils.run_ranging(dut, non_rtt_aps, self.NUM_ITER,
                                self.TIME_BETWEEN_ITERATIONS)
    stats = rutils.analyze_results(events, self.rtt_reference_distance_mm,
                                   self.rtt_reference_distance_margin_mm,
                                   self.rtt_min_expected_rssi_dbm,
                                   self.lci_reference, self.lcr_reference)
    dut.log.debug("Stats=%s", stats)

    for bssid, stat in stats.items():
      asserts.assert_true(stat['num_no_results'] == 0,
                          "Missing (timed-out) results", extras=stats)
      asserts.assert_false(stat['any_lci_mismatch'],
                           "LCI mismatch", extras=stats)
      asserts.assert_false(stat['any_lcr_mismatch'],
                           "LCR mismatch", extras=stats)
      asserts.assert_equal(stat['num_invalid_rssi'], 0, "Invalid RSSI",
                          extras=stats)
      asserts.assert_true(stat['num_failures'] <=
                          self.rtt_max_failure_rate_one_sided_rtt_percentage
                          * stat['num_results'] / 100,
                          "Failure rate is too high", extras=stats)
      asserts.assert_true(stat['num_range_out_of_margin'] <=
                self.rtt_max_margin_exceeded_rate_one_sided_rtt_percentage
                          * stat['num_success_results'] / 100,
                "Results exceeding error margin rate is too high",
                extras=stats)
    asserts.explicit_pass("RTT test done", extras=stats)

  @test_tracker_info(uuid="c9e22185-16d4-4fe6-894f-5823587b3288")
  def test_rtt_non_80211mc_supporting_aps_wo_privilege(self):
    """Scan for APs and perform RTT on non-IEEE 802.11mc supporting APs with the
    device not having privilege access (expect failures)."""
    dut = self.android_devices[0]
    rutils.config_privilege_override(dut, True)
    non_rtt_aps = rutils.select_best_scan_results(
      rutils.scan_with_rtt_support_constraint(dut, False), select_count=1)
    dut.log.debug("Visible non-IEEE 802.11mc APs=%s", non_rtt_aps)
    asserts.assert_true(len(non_rtt_aps) > 0, "Need at least one AP!")
    events = rutils.run_ranging(dut, non_rtt_aps, self.NUM_ITER,
                                self.TIME_BETWEEN_ITERATIONS)
    stats = rutils.analyze_results(events, self.rtt_reference_distance_mm,
                                   self.rtt_reference_distance_margin_mm,
                                   self.rtt_min_expected_rssi_dbm,
                                   self.lci_reference, self.lcr_reference)
    dut.log.debug("Stats=%s", stats)

    for bssid, stat in stats.items():
      asserts.assert_true(stat['num_no_results'] == 0,
                          "Missing (timed-out) results", extras=stats)
      asserts.assert_true(stat['num_failures'] == self.NUM_ITER,
        "All one-sided RTT requests must fail when executed without privilege",
                          extras=stats)
      for code in stat['status_codes']:
        asserts.assert_true(code ==
        rconsts.EVENT_CB_RANGING_STATUS_RESPONDER_DOES_NOT_SUPPORT_IEEE80211MC,
                            "Expected non-support error code", extras=stats)
    asserts.explicit_pass("RTT test done", extras=stats)

  @test_tracker_info(uuid="e117af56-bd3f-40ae-a2fd-4175f0daa7fa")
  def test_rtt_non_80211mc_supporting_ap_faked_as_supporting(self):
    """Scan for APs which do not support IEEE 802.11mc, maliciously modify the
    Responder config to indicate support and pass-through to service. Verify
    that get an error result.
    """
    dut = self.android_devices[0]
    non_rtt_aps = rutils.select_best_scan_results(
      rutils.scan_with_rtt_support_constraint(dut, False), select_count=1)
    dut.log.debug("Visible non-IEEE 802.11mc APs=%s", non_rtt_aps)
    asserts.assert_true(len(non_rtt_aps) > 0, "Need at least one AP!")
    non_rtt_aps = non_rtt_aps[0:1] # pick first
    non_rtt_aps[0][rconsts.SCAN_RESULT_KEY_RTT_RESPONDER] = True # falsify
    dut.log.debug("Visible non-IEEE 802.11mc APs=%s", non_rtt_aps)
    events = rutils.run_ranging(dut, non_rtt_aps, self.NUM_ITER,
                                self.TIME_BETWEEN_ITERATIONS)
    stats = rutils.analyze_results(events, self.rtt_reference_distance_mm,
                                   self.rtt_reference_distance_margin_mm,
                                   self.rtt_min_expected_rssi_dbm,
                                   self.lci_reference, self.lcr_reference)
    dut.log.debug("Stats=%s", stats)

    for bssid, stat in stats.items():
      asserts.assert_true(stat['num_no_results'] == 0,
                          "Missing (timed-out) results", extras=stats)
      asserts.assert_true(stat['num_failures'] == self.NUM_ITER,
                          "Failures expected for falsified responder config",
                          extras=stats)
    asserts.explicit_pass("RTT test done", extras=stats)
