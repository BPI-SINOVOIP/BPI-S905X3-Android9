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
from acts.base_test import BaseTestClass
from acts.test_utils.wifi.rtt import rtt_test_utils as rutils
from acts.test_utils.wifi.rtt.RttBaseTest import RttBaseTest


class StressRangeApTest(RttBaseTest):
  """Test class for stress testing of RTT ranging to Access Points"""

  def __init__(self, controllers):
    BaseTestClass.__init__(self, controllers)

  #############################################################################

  def test_rtt_supporting_ap_only(self):
    """Scan for APs and perform RTT only to those which support 802.11mc.

    Stress test: repeat ranging to the same AP. Verify rate of success and
    stability of results.
    """
    dut = self.android_devices[0]
    rtt_supporting_aps = rutils.scan_with_rtt_support_constraint(dut, True,
                                                                 repeat=10)
    dut.log.debug("RTT Supporting APs=%s", rtt_supporting_aps)

    num_iter = self.stress_test_min_iteration_count

    max_peers = dut.droid.wifiRttMaxPeersInRequest()
    asserts.assert_true(
        len(rtt_supporting_aps) > 0,
        "Need at least one AP which supports 802.11mc!")
    if len(rtt_supporting_aps) > max_peers:
      rtt_supporting_aps = rtt_supporting_aps[0:max_peers]

    events = rutils.run_ranging(dut, rtt_supporting_aps, num_iter, 0,
                                self.stress_test_target_run_time_sec)
    stats = rutils.analyze_results(events, self.rtt_reference_distance_mm,
                                   self.rtt_reference_distance_margin_mm,
                                   self.rtt_min_expected_rssi_dbm,
                                   self.lci_reference, self.lcr_reference,
                                   summary_only=True)
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
                          self.rtt_max_failure_rate_two_sided_rtt_percentage
                          * stat['num_results'] / 100,
                          "Failure rate is too high", extras=stats)
      asserts.assert_true(stat['num_range_out_of_margin'] <=
                    self.rtt_max_margin_exceeded_rate_two_sided_rtt_percentage
                    * stat['num_success_results'] / 100,
                    "Results exceeding error margin rate is too high",
                    extras=stats)
    asserts.explicit_pass("RTT test done", extras=stats)

