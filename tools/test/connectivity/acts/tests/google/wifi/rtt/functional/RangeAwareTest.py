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

import queue
import time

from acts import asserts
from acts.test_decorators import test_tracker_info
from acts.test_utils.wifi.aware import aware_const as aconsts
from acts.test_utils.wifi.aware import aware_test_utils as autils
from acts.test_utils.wifi.aware.AwareBaseTest import AwareBaseTest
from acts.test_utils.wifi.rtt import rtt_const as rconsts
from acts.test_utils.wifi.rtt import rtt_test_utils as rutils
from acts.test_utils.wifi.rtt.RttBaseTest import RttBaseTest


class RangeAwareTest(AwareBaseTest, RttBaseTest):
  """Test class for RTT ranging to Wi-Fi Aware peers"""
  SERVICE_NAME = "GoogleTestServiceXY"

  # Number of RTT iterations
  NUM_ITER = 10

  # Time gap (in seconds) between iterations
  TIME_BETWEEN_ITERATIONS = 0

  # Time gap (in seconds) when switching between Initiator and Responder
  TIME_BETWEEN_ROLES = 4

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

  def run_rtt_ib_discovery_set(self, do_both_directions, iter_count,
      time_between_iterations, time_between_roles):
    """Perform a set of RTT measurements, using in-band (Aware) discovery.

    Args:
      do_both_directions: False - perform all measurements in one direction,
                          True - perform 2 measurements one in both directions.
      iter_count: Number of measurements to perform.
      time_between_iterations: Number of seconds to wait between iterations.
      time_between_roles: Number of seconds to wait when switching between
                          Initiator and Responder roles (only matters if
                          do_both_directions=True).

    Returns: a list of the events containing the RTT results (or None for a
    failed measurement). If both directions are tested then returns a list of
    2 elements: one set for each direction.
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

    resultsPS = []
    resultsSP = []
    for i in range(iter_count):
      if i != 0 and time_between_iterations != 0:
        time.sleep(time_between_iterations)

      # perform RTT from pub -> sub
      resultsPS.append(
        self.run_rtt_discovery(p_dut, resp_peer_id=peer_id_on_pub))

      if do_both_directions:
        if time_between_roles != 0:
          time.sleep(time_between_roles)

        # perform RTT from sub -> pub
        resultsSP.append(
          self.run_rtt_discovery(s_dut, resp_peer_id=peer_id_on_sub))

    return resultsPS if not do_both_directions else [resultsPS, resultsSP]

  def run_rtt_oob_discovery_set(self, do_both_directions, iter_count,
      time_between_iterations, time_between_roles):
    """Perform a set of RTT measurements, using out-of-band discovery.

    Args:
      do_both_directions: False - perform all measurements in one direction,
                          True - perform 2 measurements one in both directions.
      iter_count: Number of measurements to perform.
      time_between_iterations: Number of seconds to wait between iterations.
      time_between_roles: Number of seconds to wait when switching between
                          Initiator and Responder roles (only matters if
                          do_both_directions=True).
      enable_ranging: True to enable Ranging, False to disable.

    Returns: a list of the events containing the RTT results (or None for a
    failed measurement). If both directions are tested then returns a list of
    2 elements: one set for each direction.
    """
    dut0 = self.android_devices[0]
    dut1 = self.android_devices[1]

    id0, mac0 = autils.attach_with_identity(dut0)
    id1, mac1 = autils.attach_with_identity(dut1)

    # wait for for devices to synchronize with each other - there are no other
    # mechanisms to make sure this happens for OOB discovery (except retrying
    # to execute the data-path request)
    time.sleep(autils.WAIT_FOR_CLUSTER)

    # start publisher(s) on the Responder(s) with ranging enabled
    p_config = autils.add_ranging_to_pub(
      autils.create_discovery_config(self.SERVICE_NAME,
                                     aconsts.PUBLISH_TYPE_UNSOLICITED),
      enable_ranging=True)
    dut1.droid.wifiAwarePublish(id1, p_config)
    autils.wait_for_event(dut1, aconsts.SESSION_CB_ON_PUBLISH_STARTED)
    if do_both_directions:
      dut0.droid.wifiAwarePublish(id0, p_config)
      autils.wait_for_event(dut0, aconsts.SESSION_CB_ON_PUBLISH_STARTED)

    results01 = []
    results10 = []
    for i in range(iter_count):
      if i != 0 and time_between_iterations != 0:
        time.sleep(time_between_iterations)

      # perform RTT from dut0 -> dut1
      results01.append(
          self.run_rtt_discovery(dut0, resp_mac=mac1))

      if do_both_directions:
        if time_between_roles != 0:
          time.sleep(time_between_roles)

        # perform RTT from dut1 -> dut0
        results10.append(
            self.run_rtt_discovery(dut1, resp_mac=mac0))

    return results01 if not do_both_directions else [results01, results10]

  def verify_results(self, results, results_reverse_direction=None):
    """Verifies the results of the RTT experiment.

    Args:
      results: List of RTT results.
      results_reverse_direction: List of RTT results executed in the
                                reverse direction. Optional.
    """
    stats = rutils.extract_stats(results, self.rtt_reference_distance_mm,
                                 self.rtt_reference_distance_margin_mm,
                                 self.rtt_min_expected_rssi_dbm)
    stats_reverse_direction = None
    if results_reverse_direction is not None:
      stats_reverse_direction = rutils.extract_stats(results_reverse_direction,
          self.rtt_reference_distance_mm, self.rtt_reference_distance_margin_mm,
          self.rtt_min_expected_rssi_dbm)
    self.log.debug("Stats: %s", stats)
    if stats_reverse_direction is not None:
      self.log.debug("Stats in reverse direction: %s", stats_reverse_direction)

    extras = stats if stats_reverse_direction is None else {
      "forward": stats,
      "reverse": stats_reverse_direction}

    asserts.assert_true(stats['num_no_results'] == 0,
                        "Missing (timed-out) results", extras=extras)
    asserts.assert_false(stats['any_lci_mismatch'],
                         "LCI mismatch", extras=extras)
    asserts.assert_false(stats['any_lcr_mismatch'],
                         "LCR mismatch", extras=extras)
    asserts.assert_false(stats['invalid_num_attempted'],
                         "Invalid (0) number of attempts", extras=stats)
    asserts.assert_false(stats['invalid_num_successful'],
                         "Invalid (0) number of successes", extras=stats)
    asserts.assert_equal(stats['num_invalid_rssi'], 0, "Invalid RSSI",
                         extras=extras)
    asserts.assert_true(
        stats['num_failures'] <=
          self.rtt_max_failure_rate_two_sided_rtt_percentage
          * stats['num_results'] / 100,
        "Failure rate is too high", extras=extras)
    asserts.assert_true(
        stats['num_range_out_of_margin']
          <= self.rtt_max_margin_exceeded_rate_two_sided_rtt_percentage
             * stats['num_success_results'] / 100,
        "Results exceeding error margin rate is too high", extras=extras)

    if stats_reverse_direction is not None:
      asserts.assert_true(stats_reverse_direction['num_no_results'] == 0,
                          "Missing (timed-out) results",
                          extras=extras)
      asserts.assert_false(stats['any_lci_mismatch'],
                           "LCI mismatch", extras=extras)
      asserts.assert_false(stats['any_lcr_mismatch'],
                           "LCR mismatch", extras=extras)
      asserts.assert_equal(stats['num_invalid_rssi'], 0, "Invalid RSSI",
                           extras=extras)
      asserts.assert_true(
          stats_reverse_direction['num_failures']
            <= self.rtt_max_failure_rate_two_sided_rtt_percentage
                * stats['num_results'] / 100,
          "Failure rate is too high", extras=extras)
      asserts.assert_true(
          stats_reverse_direction['num_range_out_of_margin']
            <= self.rtt_max_margin_exceeded_rate_two_sided_rtt_percentage
                * stats['num_success_results'] / 100,
          "Results exceeding error margin rate is too high",
          extras=extras)

    asserts.explicit_pass("RTT Aware test done", extras=extras)

  #############################################################################

  @test_tracker_info(uuid="9e4e7ab4-2254-498c-9788-21e15ed9a370")
  def test_rtt_oob_discovery_one_way(self):
    """Perform RTT between 2 Wi-Fi Aware devices. Use out-of-band discovery
    to communicate the MAC addresses to the peer. Test one-direction RTT only.
    """
    rtt_results = self.run_rtt_oob_discovery_set(do_both_directions=False,
          iter_count=self.NUM_ITER,
          time_between_iterations=self.TIME_BETWEEN_ITERATIONS,
          time_between_roles=self.TIME_BETWEEN_ROLES)
    self.verify_results(rtt_results)

  @test_tracker_info(uuid="22edba77-eeb2-43ee-875a-84437550ad84")
  def test_rtt_oob_discovery_both_ways(self):
    """Perform RTT between 2 Wi-Fi Aware devices. Use out-of-band discovery
    to communicate the MAC addresses to the peer. Test RTT both-ways:
    switching rapidly between Initiator and Responder.
    """
    rtt_results1, rtt_results2 = self.run_rtt_oob_discovery_set(
        do_both_directions=True, iter_count=self.NUM_ITER,
        time_between_iterations=self.TIME_BETWEEN_ITERATIONS,
        time_between_roles=self.TIME_BETWEEN_ROLES)
    self.verify_results(rtt_results1, rtt_results2)

  @test_tracker_info(uuid="18cef4be-95b4-4f7d-a140-5165874e7d1c")
  def test_rtt_ib_discovery_one_way(self):
    """Perform RTT between 2 Wi-Fi Aware devices. Use in-band (Aware) discovery
    to communicate the MAC addresses to the peer. Test one-direction RTT only.
    """
    rtt_results = self.run_rtt_ib_discovery_set(do_both_directions=False,
           iter_count=self.NUM_ITER,
           time_between_iterations=self.TIME_BETWEEN_ITERATIONS,
           time_between_roles=self.TIME_BETWEEN_ROLES)
    self.verify_results(rtt_results)

  @test_tracker_info(uuid="c67c8e70-c417-42d9-9bca-af3a89f1ddd9")
  def test_rtt_ib_discovery_both_ways(self):
    """Perform RTT between 2 Wi-Fi Aware devices. Use in-band (Aware) discovery
    to communicate the MAC addresses to the peer. Test RTT both-ways:
    switching rapidly between Initiator and Responder.
    """
    rtt_results1, rtt_results2 = self.run_rtt_ib_discovery_set(
        do_both_directions=True, iter_count=self.NUM_ITER,
        time_between_iterations=self.TIME_BETWEEN_ITERATIONS,
        time_between_roles=self.TIME_BETWEEN_ROLES)
    self.verify_results(rtt_results1, rtt_results2)

  @test_tracker_info(uuid="54f9693d-45e5-4979-adbb-1b875d217c0c")
  def test_rtt_without_initiator_aware(self):
    """Try to perform RTT operation when there is no local Aware session (on the
    Initiator). The Responder is configured normally: Aware on and a Publisher
    with Ranging enable. Should FAIL."""
    init_dut = self.android_devices[0]
    resp_dut = self.android_devices[1]

    # Enable a Responder and start a Publisher
    resp_id = resp_dut.droid.wifiAwareAttach(True)
    autils.wait_for_event(resp_dut, aconsts.EVENT_CB_ON_ATTACHED)
    resp_ident_event = autils.wait_for_event(resp_dut,
                                         aconsts.EVENT_CB_ON_IDENTITY_CHANGED)
    resp_mac = resp_ident_event['data']['mac']

    resp_config = autils.add_ranging_to_pub(
        autils.create_discovery_config(self.SERVICE_NAME,
                                       aconsts.PUBLISH_TYPE_UNSOLICITED),
        enable_ranging=True)
    resp_dut.droid.wifiAwarePublish(resp_id, resp_config)
    autils.wait_for_event(resp_dut, aconsts.SESSION_CB_ON_PUBLISH_STARTED)

    # Initiate an RTT to Responder (no Aware started on Initiator!)
    results = []
    num_no_responses = 0
    num_successes = 0
    for i in range(self.NUM_ITER):
      result = self.run_rtt_discovery(init_dut, resp_mac=resp_mac)
      self.log.debug("result: %s", result)
      results.append(result)
      if result is None:
        num_no_responses = num_no_responses + 1
      elif (result[rconsts.EVENT_CB_RANGING_KEY_STATUS]
            == rconsts.EVENT_CB_RANGING_STATUS_SUCCESS):
        num_successes = num_successes + 1

    asserts.assert_equal(num_no_responses, 0, "No RTT response?",
                         extras={"data":results})
    asserts.assert_equal(num_successes, 0, "Aware RTT w/o Aware should FAIL!",
                         extras={"data":results})
    asserts.explicit_pass("RTT Aware test done", extras={"data":results})

  @test_tracker_info(uuid="87a69053-8261-4928-8ec1-c93aac7f3a8d")
  def test_rtt_without_responder_aware(self):
    """Try to perform RTT operation when there is no peer Aware session (on the
    Responder). Should FAIL."""
    init_dut = self.android_devices[0]
    resp_dut = self.android_devices[1]

    # Enable a Responder and start a Publisher
    resp_id = resp_dut.droid.wifiAwareAttach(True)
    autils.wait_for_event(resp_dut, aconsts.EVENT_CB_ON_ATTACHED)
    resp_ident_event = autils.wait_for_event(resp_dut,
                                             aconsts.EVENT_CB_ON_IDENTITY_CHANGED)
    resp_mac = resp_ident_event['data']['mac']

    resp_config = autils.add_ranging_to_pub(
        autils.create_discovery_config(self.SERVICE_NAME,
                                       aconsts.PUBLISH_TYPE_UNSOLICITED),
        enable_ranging=True)
    resp_dut.droid.wifiAwarePublish(resp_id, resp_config)
    autils.wait_for_event(resp_dut, aconsts.SESSION_CB_ON_PUBLISH_STARTED)

    # Disable Responder
    resp_dut.droid.wifiAwareDestroy(resp_id)

    # Enable the Initiator
    init_id = init_dut.droid.wifiAwareAttach()
    autils.wait_for_event(init_dut, aconsts.EVENT_CB_ON_ATTACHED)

    # Initiate an RTT to Responder (no Aware started on Initiator!)
    results = []
    num_no_responses = 0
    num_successes = 0
    for i in range(self.NUM_ITER):
      result = self.run_rtt_discovery(init_dut, resp_mac=resp_mac)
      self.log.debug("result: %s", result)
      results.append(result)
      if result is None:
        num_no_responses = num_no_responses + 1
      elif (result[rconsts.EVENT_CB_RANGING_KEY_STATUS]
            == rconsts.EVENT_CB_RANGING_STATUS_SUCCESS):
        num_successes = num_successes + 1

    asserts.assert_equal(num_no_responses, 0, "No RTT response?",
                         extras={"data":results})
    asserts.assert_equal(num_successes, 0, "Aware RTT w/o Aware should FAIL!",
                         extras={"data":results})
    asserts.explicit_pass("RTT Aware test done", extras={"data":results})
