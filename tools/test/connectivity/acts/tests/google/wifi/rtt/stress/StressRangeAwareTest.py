#!/usr/bin/python3.4
#
#   Copyright 2018 - The Android Open Source Project
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

import queue
import time

from acts import asserts
from acts.test_utils.wifi.aware import aware_const as aconsts
from acts.test_utils.wifi.aware import aware_test_utils as autils
from acts.test_utils.wifi.aware.AwareBaseTest import AwareBaseTest
from acts.test_utils.wifi.rtt import rtt_const as rconsts
from acts.test_utils.wifi.rtt import rtt_test_utils as rutils
from acts.test_utils.wifi.rtt.RttBaseTest import RttBaseTest


class StressRangeAwareTest(AwareBaseTest, RttBaseTest):
  """Test class for stress testing of RTT ranging to Wi-Fi Aware peers."""
  SERVICE_NAME = "GoogleTestServiceXY"

  def __init__(self, controllers):
    AwareBaseTest.__init__(self, controllers)
    RttBaseTest.__init__(self, controllers)

  def setup_test(self):
    """Manual setup here due to multiple inheritance: explicitly execute the
    setup method from both parents."""
    AwareBaseTest.setup_test(self)
    RttBaseTest.setup_test(self)

  def teardown_test(self):
    """Manual teardown here due to multiple inheritance: explicitly execute the
    teardown method from both parents."""
    AwareBaseTest.teardown_test(self)
    RttBaseTest.teardown_test(self)

  #############################################################################

  def run_rtt_discovery(self, init_dut, resp_mac=None, resp_peer_id=None):
    """Perform single RTT measurement, using Aware, from the Initiator DUT to
    a Responder. The RTT Responder can be specified using its MAC address
    (obtained using out- of-band discovery) or its Peer ID (using Aware
    discovery).

    Args:
      init_dut: RTT Initiator device
      resp_mac: MAC address of the RTT Responder device
      resp_peer_id: Peer ID of the RTT Responder device
    """
    asserts.assert_true(resp_mac is not None or resp_peer_id is not None,
                        "One of the Responder specifications (MAC or Peer ID)"
                        " must be provided!")
    if resp_mac is not None:
      id = init_dut.droid.wifiRttStartRangingToAwarePeerMac(resp_mac)
    else:
      id = init_dut.droid.wifiRttStartRangingToAwarePeerId(resp_peer_id)
    try:
      event = init_dut.ed.pop_event(rutils.decorate_event(
          rconsts.EVENT_CB_RANGING_ON_RESULT, id), rutils.EVENT_TIMEOUT)
      result = event["data"][rconsts.EVENT_CB_RANGING_KEY_RESULTS][0]
      if resp_mac is not None:
        rutils.validate_aware_mac_result(result, resp_mac, "DUT")
      else:
        rutils.validate_aware_peer_id_result(result, resp_peer_id, "DUT")
      return result
    except queue.Empty:
      return None

  def test_stress_rtt_ib_discovery_set(self):
    """Perform a set of RTT measurements, using in-band (Aware) discovery, and
    switching Initiator and Responder roles repeatedly.

    Stress test: repeat ranging operations. Verify rate of success and
    stability of results.
    """
    p_dut = self.android_devices[0]
    s_dut = self.android_devices[1]

    (p_id, s_id, p_disc_id, s_disc_id,
     peer_id_on_sub, peer_id_on_pub) = autils.create_discovery_pair(
        p_dut,
        s_dut,
        p_config=autils.add_ranging_to_pub(autils.create_discovery_config(
            self.SERVICE_NAME, aconsts.PUBLISH_TYPE_UNSOLICITED), True),
        s_config=autils.add_ranging_to_pub(autils.create_discovery_config(
            self.SERVICE_NAME, aconsts.SUBSCRIBE_TYPE_PASSIVE), True),
        device_startup_offset=self.device_startup_offset,
        msg_id=self.get_next_msg_id())

    results = []
    start_clock = time.time()
    iterations_done = 0
    run_time = 0
    while iterations_done < self.stress_test_min_iteration_count or (
            self.stress_test_target_run_time_sec != 0
        and run_time < self.stress_test_target_run_time_sec):
      results.append(self.run_rtt_discovery(p_dut, resp_peer_id=peer_id_on_pub))
      results.append(self.run_rtt_discovery(s_dut, resp_peer_id=peer_id_on_sub))

      iterations_done = iterations_done + 1
      run_time = time.time() - start_clock

    stats = rutils.extract_stats(results, self.rtt_reference_distance_mm,
                                 self.rtt_reference_distance_margin_mm,
                                 self.rtt_min_expected_rssi_dbm,
                                 summary_only=True)
    self.log.debug("Stats: %s", stats)
    asserts.assert_true(stats['num_no_results'] == 0,
                        "Missing (timed-out) results", extras=stats)
    asserts.assert_false(stats['any_lci_mismatch'],
                         "LCI mismatch", extras=stats)
    asserts.assert_false(stats['any_lcr_mismatch'],
                         "LCR mismatch", extras=stats)
    asserts.assert_equal(stats['num_invalid_rssi'], 0, "Invalid RSSI",
                         extras=stats)
    asserts.assert_true(
        stats['num_failures'] <=
        self.rtt_max_failure_rate_two_sided_rtt_percentage
        * stats['num_results'] / 100,
        "Failure rate is too high", extras=stats)
    asserts.assert_true(
        stats['num_range_out_of_margin']
        <= self.rtt_max_margin_exceeded_rate_two_sided_rtt_percentage
        * stats['num_success_results'] / 100,
        "Results exceeding error margin rate is too high", extras=stats)
