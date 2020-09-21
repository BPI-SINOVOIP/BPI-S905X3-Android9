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

import base64
import time
import queue

from acts import asserts
from acts.test_decorators import test_tracker_info
from acts.test_utils.wifi.aware import aware_const as aconsts
from acts.test_utils.wifi.aware import aware_test_utils as autils
from acts.test_utils.wifi.aware.AwareBaseTest import AwareBaseTest


class MatchFilterTest(AwareBaseTest):
  """Set of tests for Wi-Fi Aware Discovery Match Filter behavior. These all
  use examples from Appendix H of the Wi-Fi Aware standard."""

  SERVICE_NAME = "GoogleTestServiceMFMFMF"

  MF_NNNNN = bytes([0x0, 0x0, 0x0, 0x0, 0x0])
  MF_12345 = bytes([0x1, 0x1, 0x1, 0x2, 0x1, 0x3, 0x1, 0x4, 0x1, 0x5])
  MF_12145 = bytes([0x1, 0x1, 0x1, 0x2, 0x1, 0x1, 0x1, 0x4, 0x1, 0x5])
  MF_1N3N5 = bytes([0x1, 0x1, 0x0, 0x1, 0x3, 0x0, 0x1, 0x5])
  MF_N23N5 = bytes([0x0, 0x1, 0x2, 0x1, 0x3, 0x0, 0x1, 0x5])
  MF_N2N4 = bytes([0x0, 0x1, 0x2, 0x0, 0x1, 0x4])
  MF_1N3N = bytes([0x1, 0x1, 0x0, 0x1, 0x3, 0x0])

  # Set of sample match filters from the spec. There is a set of matched
  # filters:
  # - Filter 1
  # - Filter 2
  # - Expected to match if the Subscriber uses Filter 1 as Tx and the Publisher
  #   uses Filter 2 as Rx (implies Solicited/Active)
  # - (the reverse) Expected to match if the Publisher uses Filter 1 as Tx and
  #   the Subscriber uses Filter 2 as Rx (implies Unsolicited/Passive)
  match_filters = [
    [None, None, True, True],
    [None, MF_NNNNN, True, True],
    [MF_NNNNN, None, True, True],
    [None, MF_12345, True, False],
    [MF_12345, None, False, True],
    [MF_NNNNN, MF_12345, True, True],
    [MF_12345, MF_NNNNN, True, True],
    [MF_12345, MF_12345, True, True],
    [MF_12345, MF_12145, False, False],
    [MF_1N3N5, MF_12345, True,True],
    [MF_12345, MF_N23N5, True, True],
    [MF_N2N4, MF_12345, True, False],
    [MF_12345, MF_1N3N, False, True]
  ]

  def __init__(self, controllers):
    AwareBaseTest.__init__(self, controllers)

  def run_discovery(self, p_dut, s_dut, p_mf, s_mf, do_unsolicited_passive,
      expect_discovery):
    """Creates a discovery session (publish and subscribe) with the specified
    configuration.

    Args:
      p_dut: Device to use as publisher.
      s_dut: Device to use as subscriber.
      p_mf: Publish's match filter.
      s_mf: Subscriber's match filter.
      do_unsolicited_passive: True to use an Unsolicited/Passive discovery,
                              False for a Solicited/Active discovery session.
      expect_discovery: True if service should be discovered, False otherwise.
    Returns: True on success, False on failure (based on expect_discovery arg)
    """
    # Encode the match filters
    p_mf = base64.b64encode(p_mf).decode("utf-8") if p_mf is not None else None
    s_mf = base64.b64encode(s_mf).decode("utf-8") if s_mf is not None else None

    # Publisher+Subscriber: attach and wait for confirmation
    p_id = p_dut.droid.wifiAwareAttach()
    autils.wait_for_event(p_dut, aconsts.EVENT_CB_ON_ATTACHED)
    time.sleep(self.device_startup_offset)
    s_id = s_dut.droid.wifiAwareAttach()
    autils.wait_for_event(s_dut, aconsts.EVENT_CB_ON_ATTACHED)

    # Publisher: start publish and wait for confirmation
    p_dut.droid.wifiAwarePublish(p_id,
                                 autils.create_discovery_config(
                                     self.SERVICE_NAME,
                                     d_type=aconsts.PUBLISH_TYPE_UNSOLICITED
                                     if do_unsolicited_passive else
                                     aconsts.PUBLISH_TYPE_SOLICITED,
                                     match_filter=p_mf))
    autils.wait_for_event(p_dut, aconsts.SESSION_CB_ON_PUBLISH_STARTED)

    # Subscriber: start subscribe and wait for confirmation
    s_dut.droid.wifiAwareSubscribe(s_id,
                                   autils.create_discovery_config(
                                       self.SERVICE_NAME,
                                       d_type=aconsts.SUBSCRIBE_TYPE_PASSIVE
                                       if do_unsolicited_passive else
                                       aconsts.SUBSCRIBE_TYPE_ACTIVE,
                                       match_filter=s_mf))
    autils.wait_for_event(s_dut, aconsts.SESSION_CB_ON_SUBSCRIBE_STARTED)

    # Subscriber: wait or fail on service discovery
    event = None
    try:
      event = s_dut.ed.pop_event(aconsts.SESSION_CB_ON_SERVICE_DISCOVERED,
                                 autils.EVENT_TIMEOUT)
      s_dut.log.info("[Subscriber] SESSION_CB_ON_SERVICE_DISCOVERED: %s", event)
    except queue.Empty:
      s_dut.log.info("[Subscriber] No SESSION_CB_ON_SERVICE_DISCOVERED")

    # clean-up
    p_dut.droid.wifiAwareDestroy(p_id)
    s_dut.droid.wifiAwareDestroy(s_id)

    if expect_discovery:
      return event is not None
    else:
      return event is None

  def run_match_filters_per_spec(self, do_unsolicited_passive):
    """Validate all the match filter combinations in the Wi-Fi Aware spec,
    Appendix H.

    Args:
      do_unsolicited_passive: True to run the Unsolicited/Passive tests, False
                              to run the Solicited/Active tests.
    """
    p_dut = self.android_devices[0]
    p_dut.pretty_name = "Publisher"
    s_dut = self.android_devices[1]
    s_dut.pretty_name = "Subscriber"

    fails = []
    for i in range(len(self.match_filters)):
      test_info = self.match_filters[i]
      if do_unsolicited_passive:
        pub_type = "Unsolicited"
        sub_type = "Passive"
        pub_mf = test_info[0]
        sub_mf = test_info[1]
        expect_discovery = test_info[3]
      else:
        pub_type = "Solicited"
        sub_type = "Active"
        pub_mf = test_info[1]
        sub_mf = test_info[0]
        expect_discovery = test_info[2]

      self.log.info("Test #%d: %s Pub MF=%s, %s Sub MF=%s: Discovery %s", i,
                    pub_type, pub_mf, sub_type, sub_mf, "EXPECTED"
                    if test_info[2] else "UNEXPECTED")
      result = self.run_discovery(
          p_dut,
          s_dut,
          p_mf=pub_mf,
          s_mf=sub_mf,
          do_unsolicited_passive=do_unsolicited_passive,
          expect_discovery=expect_discovery)
      self.log.info("Test #%d %s Pub/%s Sub %s", i, pub_type, sub_type, "PASS"
                    if result else "FAIL")
      if not result:
        fails.append(i)

    asserts.assert_true(
        len(fails) == 0, "Some match filter tests are failing", extras=fails)

  ###############################################################

  @test_tracker_info(uuid="bd734f8c-895a-4cf9-820f-ec5060517fe9")
  def test_match_filters_per_spec_unsolicited_passive(self):
    """Validate all the match filter combinations in the Wi-Fi Aware spec,
    Appendix H for Unsolicited Publish (tx filter) Passive Subscribe (rx
    filter)"""
    self.run_match_filters_per_spec(do_unsolicited_passive=True)

  @test_tracker_info(uuid="6560124d-69e5-49ff-a7e5-3cb305983723")
  def test_match_filters_per_spec_solicited_active(self):
    """Validate all the match filter combinations in the Wi-Fi Aware spec,
    Appendix H for Solicited Publish (rx filter) Active Subscribe (tx
    filter)"""
    self.run_match_filters_per_spec(do_unsolicited_passive=False)
