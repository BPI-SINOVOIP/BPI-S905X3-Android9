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

import string
import time

from acts import asserts
from acts.test_decorators import test_tracker_info
from acts.test_utils.wifi.aware import aware_const as aconsts
from acts.test_utils.wifi.aware import aware_test_utils as autils
from acts.test_utils.wifi.aware.AwareBaseTest import AwareBaseTest


class DiscoveryTest(AwareBaseTest):
  """Set of tests for Wi-Fi Aware discovery."""

  # configuration parameters used by tests
  PAYLOAD_SIZE_MIN = 0
  PAYLOAD_SIZE_TYPICAL = 1
  PAYLOAD_SIZE_MAX = 2

  # message strings
  query_msg = "How are you doing? 你好嗎？"
  response_msg = "Doing ok - thanks! 做的不錯 - 謝謝！"

  # message re-transmit counter (increases reliability in open-environment)
  # Note: reliability of message transmission is tested elsewhere
  msg_retx_count = 5  # hard-coded max value, internal API

  def __init__(self, controllers):
    AwareBaseTest.__init__(self, controllers)

  def create_base_config(self, caps, is_publish, ptype, stype, payload_size,
                         ttl, term_ind_on, null_match):
    """Create a base configuration based on input parameters.

    Args:
      caps: device capability dictionary
      is_publish: True if a publish config, else False
      ptype: unsolicited or solicited (used if is_publish is True)
      stype: passive or active (used if is_publish is False)
      payload_size: min, typical, max (PAYLOAD_SIZE_xx)
      ttl: time-to-live configuration (0 - forever)
      term_ind_on: is termination indication enabled
      null_match: null-out the middle match filter
    Returns:
      publish discovery configuration object.
    """
    config = {}
    if is_publish:
      config[aconsts.DISCOVERY_KEY_DISCOVERY_TYPE] = ptype
    else:
      config[aconsts.DISCOVERY_KEY_DISCOVERY_TYPE] = stype
    config[aconsts.DISCOVERY_KEY_TTL] = ttl
    config[aconsts.DISCOVERY_KEY_TERM_CB_ENABLED] = term_ind_on
    if payload_size == self.PAYLOAD_SIZE_MIN:
      config[aconsts.DISCOVERY_KEY_SERVICE_NAME] = "a"
      config[aconsts.DISCOVERY_KEY_SSI] = None
      config[aconsts.DISCOVERY_KEY_MATCH_FILTER_LIST] = []
    elif payload_size == self.PAYLOAD_SIZE_TYPICAL:
      config[aconsts.DISCOVERY_KEY_SERVICE_NAME] = "GoogleTestServiceX"
      if is_publish:
        config[aconsts.DISCOVERY_KEY_SSI] = string.ascii_letters
      else:
        config[aconsts.DISCOVERY_KEY_SSI] = string.ascii_letters[::
                                                                 -1]  # reverse
      config[aconsts.DISCOVERY_KEY_MATCH_FILTER_LIST] = autils.encode_list(
          [(10).to_bytes(1, byteorder="big"), "hello there string"
          if not null_match else None,
           bytes(range(40))])
    else: # PAYLOAD_SIZE_MAX
      config[aconsts.DISCOVERY_KEY_SERVICE_NAME] = "VeryLong" + "X" * (
        caps[aconsts.CAP_MAX_SERVICE_NAME_LEN] - 8)
      config[aconsts.DISCOVERY_KEY_SSI] = ("P" if is_publish else "S") * caps[
        aconsts.CAP_MAX_SERVICE_SPECIFIC_INFO_LEN]
      mf = autils.construct_max_match_filter(
          caps[aconsts.CAP_MAX_MATCH_FILTER_LEN])
      if null_match:
        mf[2] = None
      config[aconsts.DISCOVERY_KEY_MATCH_FILTER_LIST] = autils.encode_list(mf)

    return config

  def create_publish_config(self, caps, ptype, payload_size, ttl, term_ind_on,
                            null_match):
    """Create a publish configuration based on input parameters.

    Args:
      caps: device capability dictionary
      ptype: unsolicited or solicited
      payload_size: min, typical, max (PAYLOAD_SIZE_xx)
      ttl: time-to-live configuration (0 - forever)
      term_ind_on: is termination indication enabled
      null_match: null-out the middle match filter
    Returns:
      publish discovery configuration object.
    """
    return self.create_base_config(caps, True, ptype, None, payload_size, ttl,
                                   term_ind_on, null_match)

  def create_subscribe_config(self, caps, stype, payload_size, ttl, term_ind_on,
                              null_match):
    """Create a subscribe configuration based on input parameters.

    Args:
      caps: device capability dictionary
      stype: passive or active
      payload_size: min, typical, max (PAYLOAD_SIZE_xx)
      ttl: time-to-live configuration (0 - forever)
      term_ind_on: is termination indication enabled
      null_match: null-out the middle match filter
    Returns:
      subscribe discovery configuration object.
    """
    return self.create_base_config(caps, False, None, stype, payload_size, ttl,
                                   term_ind_on, null_match)

  def positive_discovery_test_utility(self, ptype, stype, payload_size):
    """Utility which runs a positive discovery test:
    - Discovery (publish/subscribe) with TTL=0 (non-self-terminating)
    - Exchange messages
    - Update publish/subscribe
    - Terminate

    Args:
      ptype: Publish discovery type
      stype: Subscribe discovery type
      payload_size: One of PAYLOAD_SIZE_* constants - MIN, TYPICAL, MAX
    """
    p_dut = self.android_devices[0]
    p_dut.pretty_name = "Publisher"
    s_dut = self.android_devices[1]
    s_dut.pretty_name = "Subscriber"

    # Publisher+Subscriber: attach and wait for confirmation
    p_id = p_dut.droid.wifiAwareAttach(False)
    autils.wait_for_event(p_dut, aconsts.EVENT_CB_ON_ATTACHED)
    time.sleep(self.device_startup_offset)
    s_id = s_dut.droid.wifiAwareAttach(False)
    autils.wait_for_event(s_dut, aconsts.EVENT_CB_ON_ATTACHED)

    # Publisher: start publish and wait for confirmation
    p_config = self.create_publish_config(
        p_dut.aware_capabilities,
        ptype,
        payload_size,
        ttl=0,
        term_ind_on=False,
        null_match=False)
    p_disc_id = p_dut.droid.wifiAwarePublish(p_id, p_config)
    autils.wait_for_event(p_dut, aconsts.SESSION_CB_ON_PUBLISH_STARTED)

    # Subscriber: start subscribe and wait for confirmation
    s_config = self.create_subscribe_config(
        s_dut.aware_capabilities,
        stype,
        payload_size,
        ttl=0,
        term_ind_on=False,
        null_match=True)
    s_disc_id = s_dut.droid.wifiAwareSubscribe(s_id, s_config)
    autils.wait_for_event(s_dut, aconsts.SESSION_CB_ON_SUBSCRIBE_STARTED)

    # Subscriber: wait for service discovery
    discovery_event = autils.wait_for_event(
        s_dut, aconsts.SESSION_CB_ON_SERVICE_DISCOVERED)
    peer_id_on_sub = discovery_event["data"][aconsts.SESSION_CB_KEY_PEER_ID]

    # Subscriber: validate contents of discovery:
    # - SSI: publisher's
    # - Match filter: UNSOLICITED - publisher, SOLICITED - subscriber
    autils.assert_equal_strings(
        bytes(discovery_event["data"][
            aconsts.SESSION_CB_KEY_SERVICE_SPECIFIC_INFO]).decode("utf-8"),
        p_config[aconsts.DISCOVERY_KEY_SSI],
        "Discovery mismatch: service specific info (SSI)")
    asserts.assert_equal(
        autils.decode_list(
            discovery_event["data"][aconsts.SESSION_CB_KEY_MATCH_FILTER_LIST]),
        autils.decode_list(p_config[aconsts.DISCOVERY_KEY_MATCH_FILTER_LIST]
                           if ptype == aconsts.PUBLISH_TYPE_UNSOLICITED else
                           s_config[aconsts.DISCOVERY_KEY_MATCH_FILTER_LIST]),
        "Discovery mismatch: match filter")

    # Subscriber: send message to peer (Publisher)
    s_dut.droid.wifiAwareSendMessage(s_disc_id, peer_id_on_sub,
                                     self.get_next_msg_id(), self.query_msg,
                                     self.msg_retx_count)
    autils.wait_for_event(s_dut, aconsts.SESSION_CB_ON_MESSAGE_SENT)

    # Publisher: wait for received message
    pub_rx_msg_event = autils.wait_for_event(
        p_dut, aconsts.SESSION_CB_ON_MESSAGE_RECEIVED)
    peer_id_on_pub = pub_rx_msg_event["data"][aconsts.SESSION_CB_KEY_PEER_ID]

    # Publisher: validate contents of message
    asserts.assert_equal(
        pub_rx_msg_event["data"][aconsts.SESSION_CB_KEY_MESSAGE_AS_STRING],
        self.query_msg, "Subscriber -> Publisher message corrupted")

    # Publisher: send message to peer (Subscriber)
    p_dut.droid.wifiAwareSendMessage(p_disc_id, peer_id_on_pub,
                                     self.get_next_msg_id(), self.response_msg,
                                     self.msg_retx_count)
    autils.wait_for_event(p_dut, aconsts.SESSION_CB_ON_MESSAGE_SENT)

    # Subscriber: wait for received message
    sub_rx_msg_event = autils.wait_for_event(
        s_dut, aconsts.SESSION_CB_ON_MESSAGE_RECEIVED)

    # Subscriber: validate contents of message
    asserts.assert_equal(
        sub_rx_msg_event["data"][aconsts.SESSION_CB_KEY_PEER_ID],
        peer_id_on_sub,
        "Subscriber received message from different peer ID then discovery!?")
    autils.assert_equal_strings(
        sub_rx_msg_event["data"][aconsts.SESSION_CB_KEY_MESSAGE_AS_STRING],
        self.response_msg, "Publisher -> Subscriber message corrupted")

    # Subscriber: validate that we're not getting another Service Discovery
    autils.fail_on_event(s_dut, aconsts.SESSION_CB_ON_SERVICE_DISCOVERED)

    # Publisher: update publish and wait for confirmation
    p_config[aconsts.DISCOVERY_KEY_SSI] = "something else"
    p_dut.droid.wifiAwareUpdatePublish(p_disc_id, p_config)
    autils.wait_for_event(p_dut, aconsts.SESSION_CB_ON_SESSION_CONFIG_UPDATED)

    # Subscriber: expect a new service discovery
    discovery_event = autils.wait_for_event(
        s_dut, aconsts.SESSION_CB_ON_SERVICE_DISCOVERED)

    # Subscriber: validate contents of discovery
    autils.assert_equal_strings(
        bytes(discovery_event["data"][
            aconsts.SESSION_CB_KEY_SERVICE_SPECIFIC_INFO]).decode("utf-8"),
        p_config[aconsts.DISCOVERY_KEY_SSI],
        "Discovery mismatch (after pub update): service specific info (SSI)")
    asserts.assert_equal(
        autils.decode_list(
            discovery_event["data"][aconsts.SESSION_CB_KEY_MATCH_FILTER_LIST]),
        autils.decode_list(p_config[aconsts.DISCOVERY_KEY_MATCH_FILTER_LIST]
                           if ptype == aconsts.PUBLISH_TYPE_UNSOLICITED else
                           s_config[aconsts.DISCOVERY_KEY_MATCH_FILTER_LIST]),
        "Discovery mismatch: match filter")

    # Subscribe: update subscribe and wait for confirmation
    s_config = self.create_subscribe_config(
        s_dut.aware_capabilities,
        stype,
        payload_size,
        ttl=0,
        term_ind_on=False,
        null_match=False)
    s_dut.droid.wifiAwareUpdateSubscribe(s_disc_id, s_config)
    autils.wait_for_event(s_dut, aconsts.SESSION_CB_ON_SESSION_CONFIG_UPDATED)

    # Publisher+Subscriber: Terminate sessions
    p_dut.droid.wifiAwareDestroyDiscoverySession(p_disc_id)
    s_dut.droid.wifiAwareDestroyDiscoverySession(s_disc_id)

    # sleep for timeout period and then verify all 'fail_on_event' together
    time.sleep(autils.EVENT_TIMEOUT)

    # verify that there were no other events
    autils.verify_no_more_events(p_dut, timeout=0)
    autils.verify_no_more_events(s_dut, timeout=0)

    # verify that forbidden callbacks aren't called
    autils.validate_forbidden_callbacks(p_dut, {aconsts.CB_EV_MATCH: 0})

  def verify_discovery_session_term(self, dut, disc_id, config, is_publish,
                                    term_ind_on):
    """Utility to verify that the specified discovery session has terminated (by
    waiting for the TTL and then attempting to reconfigure).

    Args:
      dut: device under test
      disc_id: discovery id for the existing session
      config: configuration of the existing session
      is_publish: True if the configuration was publish, False if subscribe
      term_ind_on: True if a termination indication is expected, False otherwise
    """
    # Wait for session termination
    if term_ind_on:
      autils.wait_for_event(
          dut,
          autils.decorate_event(aconsts.SESSION_CB_ON_SESSION_TERMINATED,
                                disc_id))
    else:
      # can't defer wait to end since in any case have to wait for session to
      # expire
      autils.fail_on_event(
          dut,
          autils.decorate_event(aconsts.SESSION_CB_ON_SESSION_TERMINATED,
                                disc_id))

    # Validate that session expired by trying to configure it (expect failure)
    config[aconsts.DISCOVERY_KEY_SSI] = "something else"
    if is_publish:
      dut.droid.wifiAwareUpdatePublish(disc_id, config)
    else:
      dut.droid.wifiAwareUpdateSubscribe(disc_id, config)

    # The response to update discovery session is:
    # term_ind_on=True: session was cleaned-up so won't get an explicit failure, but won't get a
    #                   success either. Can check for no SESSION_CB_ON_SESSION_CONFIG_UPDATED but
    #                   will defer to the end of the test (no events on queue).
    # term_ind_on=False: session was not cleaned-up (yet). So expect
    #                    SESSION_CB_ON_SESSION_CONFIG_FAILED.
    if not term_ind_on:
      autils.wait_for_event(
          dut,
          autils.decorate_event(aconsts.SESSION_CB_ON_SESSION_CONFIG_FAILED,
                                disc_id))

  def positive_ttl_test_utility(self, is_publish, ptype, stype, term_ind_on):
    """Utility which runs a positive discovery session TTL configuration test

    Iteration 1: Verify session started with TTL
    Iteration 2: Verify session started without TTL and reconfigured with TTL
    Iteration 3: Verify session started with (long) TTL and reconfigured with
                 (short) TTL

    Args:
      is_publish: True if testing publish, False if testing subscribe
      ptype: Publish discovery type (used if is_publish is True)
      stype: Subscribe discovery type (used if is_publish is False)
      term_ind_on: Configuration of termination indication
    """
    SHORT_TTL = 5  # 5 seconds
    LONG_TTL = 100  # 100 seconds
    dut = self.android_devices[0]

    # Attach and wait for confirmation
    id = dut.droid.wifiAwareAttach(False)
    autils.wait_for_event(dut, aconsts.EVENT_CB_ON_ATTACHED)

    # Iteration 1: Start discovery session with TTL
    config = self.create_base_config(dut.aware_capabilities, is_publish, ptype,
                                     stype, self.PAYLOAD_SIZE_TYPICAL,
                                     SHORT_TTL, term_ind_on, False)
    if is_publish:
      disc_id = dut.droid.wifiAwarePublish(id, config, True)
      autils.wait_for_event(dut,
                            autils.decorate_event(
                                aconsts.SESSION_CB_ON_PUBLISH_STARTED, disc_id))
    else:
      disc_id = dut.droid.wifiAwareSubscribe(id, config, True)
      autils.wait_for_event(
          dut,
          autils.decorate_event(aconsts.SESSION_CB_ON_SUBSCRIBE_STARTED,
                                disc_id))

    # Wait for session termination & verify
    self.verify_discovery_session_term(dut, disc_id, config, is_publish,
                                       term_ind_on)

    # Iteration 2: Start a discovery session without TTL
    config = self.create_base_config(dut.aware_capabilities, is_publish, ptype,
                                     stype, self.PAYLOAD_SIZE_TYPICAL, 0,
                                     term_ind_on, False)
    if is_publish:
      disc_id = dut.droid.wifiAwarePublish(id, config, True)
      autils.wait_for_event(dut,
                            autils.decorate_event(
                                aconsts.SESSION_CB_ON_PUBLISH_STARTED, disc_id))
    else:
      disc_id = dut.droid.wifiAwareSubscribe(id, config, True)
      autils.wait_for_event(
          dut,
          autils.decorate_event(aconsts.SESSION_CB_ON_SUBSCRIBE_STARTED,
                                disc_id))

    # Update with a TTL
    config = self.create_base_config(dut.aware_capabilities, is_publish, ptype,
                                     stype, self.PAYLOAD_SIZE_TYPICAL,
                                     SHORT_TTL, term_ind_on, False)
    if is_publish:
      dut.droid.wifiAwareUpdatePublish(disc_id, config)
    else:
      dut.droid.wifiAwareUpdateSubscribe(disc_id, config)
    autils.wait_for_event(
        dut,
        autils.decorate_event(aconsts.SESSION_CB_ON_SESSION_CONFIG_UPDATED,
                              disc_id))

    # Wait for session termination & verify
    self.verify_discovery_session_term(dut, disc_id, config, is_publish,
                                       term_ind_on)

    # Iteration 3: Start a discovery session with (long) TTL
    config = self.create_base_config(dut.aware_capabilities, is_publish, ptype,
                                     stype, self.PAYLOAD_SIZE_TYPICAL, LONG_TTL,
                                     term_ind_on, False)
    if is_publish:
      disc_id = dut.droid.wifiAwarePublish(id, config, True)
      autils.wait_for_event(dut,
                            autils.decorate_event(
                                aconsts.SESSION_CB_ON_PUBLISH_STARTED, disc_id))
    else:
      disc_id = dut.droid.wifiAwareSubscribe(id, config, True)
      autils.wait_for_event(
          dut,
          autils.decorate_event(aconsts.SESSION_CB_ON_SUBSCRIBE_STARTED,
                                disc_id))

    # Update with a TTL
    config = self.create_base_config(dut.aware_capabilities, is_publish, ptype,
                                     stype, self.PAYLOAD_SIZE_TYPICAL,
                                     SHORT_TTL, term_ind_on, False)
    if is_publish:
      dut.droid.wifiAwareUpdatePublish(disc_id, config)
    else:
      dut.droid.wifiAwareUpdateSubscribe(disc_id, config)
    autils.wait_for_event(
        dut,
        autils.decorate_event(aconsts.SESSION_CB_ON_SESSION_CONFIG_UPDATED,
                              disc_id))

    # Wait for session termination & verify
    self.verify_discovery_session_term(dut, disc_id, config, is_publish,
                                       term_ind_on)

    # verify that there were no other events
    autils.verify_no_more_events(dut)

    # verify that forbidden callbacks aren't called
    if not term_ind_on:
      autils.validate_forbidden_callbacks(dut, {
          aconsts.CB_EV_PUBLISH_TERMINATED: 0,
          aconsts.CB_EV_SUBSCRIBE_TERMINATED: 0
      })

  def discovery_mismatch_test_utility(self,
                                      is_expected_to_pass,
                                      p_type,
                                      s_type,
                                      p_service_name=None,
                                      s_service_name=None,
                                      p_mf_1=None,
                                      s_mf_1=None):
    """Utility which runs the negative discovery test for mismatched service
    configs.

    Args:
      is_expected_to_pass: True if positive test, False if negative
      p_type: Publish discovery type
      s_type: Subscribe discovery type
      p_service_name: Publish service name (or None to leave unchanged)
      s_service_name: Subscribe service name (or None to leave unchanged)
      p_mf_1: Publish match filter element [1] (or None to leave unchanged)
      s_mf_1: Subscribe match filter element [1] (or None to leave unchanged)
    """
    p_dut = self.android_devices[0]
    p_dut.pretty_name = "Publisher"
    s_dut = self.android_devices[1]
    s_dut.pretty_name = "Subscriber"

    # create configurations
    p_config = self.create_publish_config(
        p_dut.aware_capabilities,
        p_type,
        self.PAYLOAD_SIZE_TYPICAL,
        ttl=0,
        term_ind_on=False,
        null_match=False)
    if p_service_name is not None:
      p_config[aconsts.DISCOVERY_KEY_SERVICE_NAME] = p_service_name
    if p_mf_1 is not None:
      p_config[aconsts.DISCOVERY_KEY_MATCH_FILTER_LIST] = autils.encode_list(
          [(10).to_bytes(1, byteorder="big"),
           p_mf_1,
           bytes(range(40))])
    s_config = self.create_publish_config(
        s_dut.aware_capabilities,
        s_type,
        self.PAYLOAD_SIZE_TYPICAL,
        ttl=0,
        term_ind_on=False,
        null_match=False)
    if s_service_name is not None:
      s_config[aconsts.DISCOVERY_KEY_SERVICE_NAME] = s_service_name
    if s_mf_1 is not None:
      s_config[aconsts.DISCOVERY_KEY_MATCH_FILTER_LIST] = autils.encode_list(
          [(10).to_bytes(1, byteorder="big"),
           s_mf_1,
           bytes(range(40))])

    p_id = p_dut.droid.wifiAwareAttach(False)
    autils.wait_for_event(p_dut, aconsts.EVENT_CB_ON_ATTACHED)
    time.sleep(self.device_startup_offset)
    s_id = s_dut.droid.wifiAwareAttach(False)
    autils.wait_for_event(s_dut, aconsts.EVENT_CB_ON_ATTACHED)

    # Publisher: start publish and wait for confirmation
    p_disc_id = p_dut.droid.wifiAwarePublish(p_id, p_config)
    autils.wait_for_event(p_dut, aconsts.SESSION_CB_ON_PUBLISH_STARTED)

    # Subscriber: start subscribe and wait for confirmation
    s_disc_id = s_dut.droid.wifiAwareSubscribe(s_id, s_config)
    autils.wait_for_event(s_dut, aconsts.SESSION_CB_ON_SUBSCRIBE_STARTED)

    # Subscriber: fail on service discovery
    if is_expected_to_pass:
      autils.wait_for_event(s_dut, aconsts.SESSION_CB_ON_SERVICE_DISCOVERED)
    else:
      autils.fail_on_event(s_dut, aconsts.SESSION_CB_ON_SERVICE_DISCOVERED)

    # Publisher+Subscriber: Terminate sessions
    p_dut.droid.wifiAwareDestroyDiscoverySession(p_disc_id)
    s_dut.droid.wifiAwareDestroyDiscoverySession(s_disc_id)

    # verify that there were no other events (including terminations)
    time.sleep(autils.EVENT_TIMEOUT)
    autils.verify_no_more_events(p_dut, timeout=0)
    autils.verify_no_more_events(s_dut, timeout=0)


  #######################################
  # Positive tests key:
  #
  # names is: test_<pub_type>_<sub_type>_<size>
  # where:
  #
  # pub_type: Type of publish discovery session: unsolicited or solicited.
  # sub_type: Type of subscribe discovery session: passive or active.
  # size: Size of payload fields (service name, service specific info, and match
  # filter: typical, max, or min.
  #######################################

  @test_tracker_info(uuid="954ebbde-ed2b-4f04-9e68-88239187d69d")
  def test_positive_unsolicited_passive_typical(self):
    """Functional test case / Discovery test cases / positive test case:
    - Solicited publish + passive subscribe
    - Typical payload fields size

    Verifies that discovery and message exchange succeeds.
    """
    self.positive_discovery_test_utility(
        ptype=aconsts.PUBLISH_TYPE_UNSOLICITED,
        stype=aconsts.SUBSCRIBE_TYPE_PASSIVE,
        payload_size=self.PAYLOAD_SIZE_TYPICAL)

  @test_tracker_info(uuid="67fb22bb-6985-4345-95a4-90b76681a58b")
  def test_positive_unsolicited_passive_min(self):
    """Functional test case / Discovery test cases / positive test case:
    - Solicited publish + passive subscribe
    - Minimal payload fields size

    Verifies that discovery and message exchange succeeds.
    """
    self.positive_discovery_test_utility(
        ptype=aconsts.PUBLISH_TYPE_UNSOLICITED,
        stype=aconsts.SUBSCRIBE_TYPE_PASSIVE,
        payload_size=self.PAYLOAD_SIZE_MIN)

  @test_tracker_info(uuid="a02a47b9-41bb-47bb-883b-921024a2c30d")
  def test_positive_unsolicited_passive_max(self):
    """Functional test case / Discovery test cases / positive test case:
    - Solicited publish + passive subscribe
    - Maximal payload fields size

    Verifies that discovery and message exchange succeeds.
    """
    self.positive_discovery_test_utility(
        ptype=aconsts.PUBLISH_TYPE_UNSOLICITED,
        stype=aconsts.SUBSCRIBE_TYPE_PASSIVE,
        payload_size=self.PAYLOAD_SIZE_MAX)

  @test_tracker_info(uuid="586c657f-2388-4e7a-baee-9bce2f3d1a16")
  def test_positive_solicited_active_typical(self):
    """Functional test case / Discovery test cases / positive test case:
    - Unsolicited publish + active subscribe
    - Typical payload fields size

    Verifies that discovery and message exchange succeeds.
    """
    self.positive_discovery_test_utility(
        ptype=aconsts.PUBLISH_TYPE_SOLICITED,
        stype=aconsts.SUBSCRIBE_TYPE_ACTIVE,
        payload_size=self.PAYLOAD_SIZE_TYPICAL)

  @test_tracker_info(uuid="5369e4ff-f406-48c5-b41a-df38ec340146")
  def test_positive_solicited_active_min(self):
    """Functional test case / Discovery test cases / positive test case:
    - Unsolicited publish + active subscribe
    - Minimal payload fields size

    Verifies that discovery and message exchange succeeds.
    """
    self.positive_discovery_test_utility(
        ptype=aconsts.PUBLISH_TYPE_SOLICITED,
        stype=aconsts.SUBSCRIBE_TYPE_ACTIVE,
        payload_size=self.PAYLOAD_SIZE_MIN)

  @test_tracker_info(uuid="634c6eb8-2c4f-42bd-9bbb-d874d0ec22f3")
  def test_positive_solicited_active_max(self):
    """Functional test case / Discovery test cases / positive test case:
    - Unsolicited publish + active subscribe
    - Maximal payload fields size

    Verifies that discovery and message exchange succeeds.
    """
    self.positive_discovery_test_utility(
        ptype=aconsts.PUBLISH_TYPE_SOLICITED,
        stype=aconsts.SUBSCRIBE_TYPE_ACTIVE,
        payload_size=self.PAYLOAD_SIZE_MAX)

  #######################################
  # TTL tests key:
  #
  # names is: test_ttl_<pub_type|sub_type>_<term_ind>
  # where:
  #
  # pub_type: Type of publish discovery session: unsolicited or solicited.
  # sub_type: Type of subscribe discovery session: passive or active.
  # term_ind: ind_on or ind_off
  #######################################

  @test_tracker_info(uuid="9d7e758e-e0e2-4550-bcee-bfb6a2bff63e")
  def test_ttl_unsolicited_ind_on(self):
    """Functional test case / Discovery test cases / TTL test case:
    - Unsolicited publish
    - Termination indication enabled
    """
    self.positive_ttl_test_utility(
        is_publish=True,
        ptype=aconsts.PUBLISH_TYPE_UNSOLICITED,
        stype=None,
        term_ind_on=True)

  @test_tracker_info(uuid="48fd69bc-cc2a-4f65-a0a1-63d7c1720702")
  def test_ttl_unsolicited_ind_off(self):
    """Functional test case / Discovery test cases / TTL test case:
    - Unsolicited publish
    - Termination indication disabled
    """
    self.positive_ttl_test_utility(
        is_publish=True,
        ptype=aconsts.PUBLISH_TYPE_UNSOLICITED,
        stype=None,
        term_ind_on=False)

  @test_tracker_info(uuid="afb75fc1-9ba7-446a-b5ed-7cd37ab51b1c")
  def test_ttl_solicited_ind_on(self):
    """Functional test case / Discovery test cases / TTL test case:
    - Solicited publish
    - Termination indication enabled
    """
    self.positive_ttl_test_utility(
        is_publish=True,
        ptype=aconsts.PUBLISH_TYPE_SOLICITED,
        stype=None,
        term_ind_on=True)

  @test_tracker_info(uuid="703311a6-e444-4055-94ee-ea9b9b71799e")
  def test_ttl_solicited_ind_off(self):
    """Functional test case / Discovery test cases / TTL test case:
    - Solicited publish
    - Termination indication disabled
    """
    self.positive_ttl_test_utility(
        is_publish=True,
        ptype=aconsts.PUBLISH_TYPE_SOLICITED,
        stype=None,
        term_ind_on=False)

  @test_tracker_info(uuid="38a541c4-ff55-4387-87b7-4d940489da9d")
  def test_ttl_passive_ind_on(self):
    """Functional test case / Discovery test cases / TTL test case:
    - Passive subscribe
    - Termination indication enabled
    """
    self.positive_ttl_test_utility(
        is_publish=False,
        ptype=None,
        stype=aconsts.SUBSCRIBE_TYPE_PASSIVE,
        term_ind_on=True)

  @test_tracker_info(uuid="ba971e12-b0ca-417c-a1b5-9451598de47d")
  def test_ttl_passive_ind_off(self):
    """Functional test case / Discovery test cases / TTL test case:
    - Passive subscribe
    - Termination indication disabled
    """
    self.positive_ttl_test_utility(
        is_publish=False,
        ptype=None,
        stype=aconsts.SUBSCRIBE_TYPE_PASSIVE,
        term_ind_on=False)

  @test_tracker_info(uuid="7b5d96f2-2415-4b98-9a51-32957f0679a0")
  def test_ttl_active_ind_on(self):
    """Functional test case / Discovery test cases / TTL test case:
    - Active subscribe
    - Termination indication enabled
    """
    self.positive_ttl_test_utility(
        is_publish=False,
        ptype=None,
        stype=aconsts.SUBSCRIBE_TYPE_ACTIVE,
        term_ind_on=True)

  @test_tracker_info(uuid="c9268eca-0a30-42dd-8e6c-b8b0b84697fb")
  def test_ttl_active_ind_off(self):
    """Functional test case / Discovery test cases / TTL test case:
    - Active subscribe
    - Termination indication disabled
    """
    self.positive_ttl_test_utility(
        is_publish=False,
        ptype=None,
        stype=aconsts.SUBSCRIBE_TYPE_ACTIVE,
        term_ind_on=False)

  #######################################
  # Mismatched service name tests key:
  #
  # names is: test_mismatch_service_name_<pub_type>_<sub_type>
  # where:
  #
  # pub_type: Type of publish discovery session: unsolicited or solicited.
  # sub_type: Type of subscribe discovery session: passive or active.
  #######################################

  @test_tracker_info(uuid="175415e9-7d07-40d0-95f0-3a5f91ea4711")
  def test_mismatch_service_name_unsolicited_passive(self):
    """Functional test case / Discovery test cases / Mismatch service name
    - Unsolicited publish
    - Passive subscribe
    """
    self.discovery_mismatch_test_utility(
        is_expected_to_pass=False,
        p_type=aconsts.PUBLISH_TYPE_UNSOLICITED,
        s_type=aconsts.SUBSCRIBE_TYPE_PASSIVE,
        p_service_name="GoogleTestServiceXXX",
        s_service_name="GoogleTestServiceYYY")

  @test_tracker_info(uuid="c22a54ce-9e46-47a5-ac44-831faf93d317")
  def test_mismatch_service_name_solicited_active(self):
    """Functional test case / Discovery test cases / Mismatch service name
    - Solicited publish
    - Active subscribe
    """
    self.discovery_mismatch_test_utility(
        is_expected_to_pass=False,
        p_type=aconsts.PUBLISH_TYPE_SOLICITED,
        s_type=aconsts.SUBSCRIBE_TYPE_ACTIVE,
        p_service_name="GoogleTestServiceXXX",
        s_service_name="GoogleTestServiceYYY")

  #######################################
  # Mismatched discovery session type tests key:
  #
  # names is: test_mismatch_service_type_<pub_type>_<sub_type>
  # where:
  #
  # pub_type: Type of publish discovery session: unsolicited or solicited.
  # sub_type: Type of subscribe discovery session: passive or active.
  #######################################

  @test_tracker_info(uuid="4806f631-d9eb-45fd-9e75-24674962770f")
  def test_mismatch_service_type_unsolicited_active(self):
    """Functional test case / Discovery test cases / Mismatch service name
    - Unsolicited publish
    - Active subscribe
    """
    self.discovery_mismatch_test_utility(
        is_expected_to_pass=True,
        p_type=aconsts.PUBLISH_TYPE_UNSOLICITED,
        s_type=aconsts.SUBSCRIBE_TYPE_ACTIVE)

  @test_tracker_info(uuid="12d648fd-b8fa-4c0f-9467-95e2366047de")
  def test_mismatch_service_type_solicited_passive(self):
    """Functional test case / Discovery test cases / Mismatch service name
    - Unsolicited publish
    - Active subscribe
    """
    self.discovery_mismatch_test_utility(
        is_expected_to_pass=False,
        p_type=aconsts.PUBLISH_TYPE_SOLICITED,
        s_type=aconsts.SUBSCRIBE_TYPE_PASSIVE)

  #######################################
  # Mismatched discovery match filter tests key:
  #
  # names is: test_mismatch_match_filter_<pub_type>_<sub_type>
  # where:
  #
  # pub_type: Type of publish discovery session: unsolicited or solicited.
  # sub_type: Type of subscribe discovery session: passive or active.
  #######################################

  @test_tracker_info(uuid="d98454cb-64af-4266-8fed-f0b545a2d7c4")
  def test_mismatch_match_filter_unsolicited_passive(self):
    """Functional test case / Discovery test cases / Mismatch match filter
    - Unsolicited publish
    - Passive subscribe
    """
    self.discovery_mismatch_test_utility(
        is_expected_to_pass=False,
        p_type=aconsts.PUBLISH_TYPE_UNSOLICITED,
        s_type=aconsts.SUBSCRIBE_TYPE_PASSIVE,
        p_mf_1="hello there string",
        s_mf_1="goodbye there string")

  @test_tracker_info(uuid="663c1008-ae11-4e1a-87c7-c311d83f481c")
  def test_mismatch_match_filter_solicited_active(self):
    """Functional test case / Discovery test cases / Mismatch match filter
    - Solicited publish
    - Active subscribe
    """
    self.discovery_mismatch_test_utility(
        is_expected_to_pass=False,
        p_type=aconsts.PUBLISH_TYPE_SOLICITED,
        s_type=aconsts.SUBSCRIBE_TYPE_ACTIVE,
        p_mf_1="hello there string",
        s_mf_1="goodbye there string")

  #######################################
  # Multiple concurrent services
  #######################################

  def run_multiple_concurrent_services(self, type_x, type_y):
    """Validate multiple identical discovery services running on both devices:
    - DUT1 & DUT2 running Publish for X
    - DUT1 & DUT2 running Publish for Y
    - DUT1 Subscribes for X
    - DUT2 Subscribes for Y
    Message exchanges.

    Note: test requires that devices support 2 publish sessions concurrently.
    The test will be skipped if the devices are not capable.

    Args:
      type_x, type_y: A list of [ptype, stype] of the publish and subscribe
                      types for services X and Y respectively.
    """
    dut1 = self.android_devices[0]
    dut2 = self.android_devices[1]

    X_SERVICE_NAME = "ServiceXXX"
    Y_SERVICE_NAME = "ServiceYYY"

    asserts.skip_if(dut1.aware_capabilities[aconsts.CAP_MAX_PUBLISHES] < 2 or
                    dut2.aware_capabilities[aconsts.CAP_MAX_PUBLISHES] < 2,
                    "Devices do not support 2 publish sessions")

    # attach and wait for confirmation
    id1 = dut1.droid.wifiAwareAttach(False)
    autils.wait_for_event(dut1, aconsts.EVENT_CB_ON_ATTACHED)
    time.sleep(self.device_startup_offset)
    id2 = dut2.droid.wifiAwareAttach(False)
    autils.wait_for_event(dut2, aconsts.EVENT_CB_ON_ATTACHED)

    # DUT1 & DUT2: start publishing both X & Y services and wait for
    # confirmations
    dut1_x_pid = dut1.droid.wifiAwarePublish(id1,
                                             autils.create_discovery_config(
                                               X_SERVICE_NAME, type_x[0]))
    event = autils.wait_for_event(dut1, aconsts.SESSION_CB_ON_PUBLISH_STARTED)
    asserts.assert_equal(event["data"][aconsts.SESSION_CB_KEY_SESSION_ID],
                         dut1_x_pid,
                         "Unexpected DUT1 X publish session discovery ID")

    dut1_y_pid = dut1.droid.wifiAwarePublish(id1,
                                             autils.create_discovery_config(
                                               Y_SERVICE_NAME, type_y[0]))
    event = autils.wait_for_event(dut1, aconsts.SESSION_CB_ON_PUBLISH_STARTED)
    asserts.assert_equal(event["data"][aconsts.SESSION_CB_KEY_SESSION_ID],
                         dut1_y_pid,
                         "Unexpected DUT1 Y publish session discovery ID")

    dut2_x_pid = dut2.droid.wifiAwarePublish(id2,
                                             autils.create_discovery_config(
                                                 X_SERVICE_NAME, type_x[0]))
    event = autils.wait_for_event(dut2, aconsts.SESSION_CB_ON_PUBLISH_STARTED)
    asserts.assert_equal(event["data"][aconsts.SESSION_CB_KEY_SESSION_ID],
                         dut2_x_pid,
                         "Unexpected DUT2 X publish session discovery ID")

    dut2_y_pid = dut2.droid.wifiAwarePublish(id2,
                                             autils.create_discovery_config(
                                                 Y_SERVICE_NAME, type_y[0]))
    event = autils.wait_for_event(dut2, aconsts.SESSION_CB_ON_PUBLISH_STARTED)
    asserts.assert_equal(event["data"][aconsts.SESSION_CB_KEY_SESSION_ID],
                         dut2_y_pid,
                         "Unexpected DUT2 Y publish session discovery ID")

    # DUT1: start subscribing for X
    dut1_x_sid = dut1.droid.wifiAwareSubscribe(id1,
                                               autils.create_discovery_config(
                                                   X_SERVICE_NAME, type_x[1]))
    autils.wait_for_event(dut1, aconsts.SESSION_CB_ON_SUBSCRIBE_STARTED)

    # DUT2: start subscribing for Y
    dut2_y_sid = dut2.droid.wifiAwareSubscribe(id2,
                                               autils.create_discovery_config(
                                                   Y_SERVICE_NAME, type_y[1]))
    autils.wait_for_event(dut2, aconsts.SESSION_CB_ON_SUBSCRIBE_STARTED)

    # DUT1 & DUT2: wait for service discovery
    event = autils.wait_for_event(dut1,
                                  aconsts.SESSION_CB_ON_SERVICE_DISCOVERED)
    asserts.assert_equal(event["data"][aconsts.SESSION_CB_KEY_SESSION_ID],
                         dut1_x_sid,
                         "Unexpected DUT1 X subscribe session discovery ID")
    dut1_peer_id_for_dut2_x = event["data"][aconsts.SESSION_CB_KEY_PEER_ID]

    event = autils.wait_for_event(dut2,
                                  aconsts.SESSION_CB_ON_SERVICE_DISCOVERED)
    asserts.assert_equal(event["data"][aconsts.SESSION_CB_KEY_SESSION_ID],
                         dut2_y_sid,
                         "Unexpected DUT2 Y subscribe session discovery ID")
    dut2_peer_id_for_dut1_y = event["data"][aconsts.SESSION_CB_KEY_PEER_ID]

    # DUT1.X send message to DUT2
    x_msg = "Hello X on DUT2!"
    dut1.droid.wifiAwareSendMessage(dut1_x_sid, dut1_peer_id_for_dut2_x,
                                     self.get_next_msg_id(), x_msg,
                                     self.msg_retx_count)
    autils.wait_for_event(dut1, aconsts.SESSION_CB_ON_MESSAGE_SENT)
    event = autils.wait_for_event(dut2, aconsts.SESSION_CB_ON_MESSAGE_RECEIVED)
    asserts.assert_equal(event["data"][aconsts.SESSION_CB_KEY_SESSION_ID],
                         dut2_x_pid,
                        "Unexpected publish session ID on DUT2 for meesage "
                        "received on service X")
    asserts.assert_equal(
        event["data"][aconsts.SESSION_CB_KEY_MESSAGE_AS_STRING], x_msg,
        "Message on service X from DUT1 to DUT2 not received correctly")

    # DUT2.Y send message to DUT1
    y_msg = "Hello Y on DUT1!"
    dut2.droid.wifiAwareSendMessage(dut2_y_sid, dut2_peer_id_for_dut1_y,
                                    self.get_next_msg_id(), y_msg,
                                    self.msg_retx_count)
    autils.wait_for_event(dut2, aconsts.SESSION_CB_ON_MESSAGE_SENT)
    event = autils.wait_for_event(dut1, aconsts.SESSION_CB_ON_MESSAGE_RECEIVED)
    asserts.assert_equal(event["data"][aconsts.SESSION_CB_KEY_SESSION_ID],
                         dut1_y_pid,
                         "Unexpected publish session ID on DUT1 for meesage "
                         "received on service Y")
    asserts.assert_equal(
        event["data"][aconsts.SESSION_CB_KEY_MESSAGE_AS_STRING], y_msg,
        "Message on service Y from DUT2 to DUT1 not received correctly")

  @test_tracker_info(uuid="eef80cf3-1fd2-4526-969b-6af2dce785d7")
  def test_multiple_concurrent_services_both_unsolicited_passive(self):
    """Validate multiple concurrent discovery sessions running on both devices.
    - DUT1 & DUT2 running Publish for X
    - DUT1 & DUT2 running Publish for Y
    - DUT1 Subscribes for X
    - DUT2 Subscribes for Y
    Message exchanges.

    Both sessions are Unsolicited/Passive.

    Note: test requires that devices support 2 publish sessions concurrently.
    The test will be skipped if the devices are not capable.
    """
    self.run_multiple_concurrent_services(
      type_x=[aconsts.PUBLISH_TYPE_UNSOLICITED, aconsts.SUBSCRIBE_TYPE_PASSIVE],
      type_y=[aconsts.PUBLISH_TYPE_UNSOLICITED, aconsts.SUBSCRIBE_TYPE_PASSIVE])

  @test_tracker_info(uuid="46739f04-ab2b-4556-b1a4-9aa2774869b5")
  def test_multiple_concurrent_services_both_solicited_active(self):
    """Validate multiple concurrent discovery sessions running on both devices.
    - DUT1 & DUT2 running Publish for X
    - DUT1 & DUT2 running Publish for Y
    - DUT1 Subscribes for X
    - DUT2 Subscribes for Y
    Message exchanges.

    Both sessions are Solicited/Active.

    Note: test requires that devices support 2 publish sessions concurrently.
    The test will be skipped if the devices are not capable.
    """
    self.run_multiple_concurrent_services(
      type_x=[aconsts.PUBLISH_TYPE_SOLICITED, aconsts.SUBSCRIBE_TYPE_ACTIVE],
      type_y=[aconsts.PUBLISH_TYPE_SOLICITED, aconsts.SUBSCRIBE_TYPE_ACTIVE])

  @test_tracker_info(uuid="5f8f7fd2-4a0e-4cca-8cbb-6d54353f2baa")
  def test_multiple_concurrent_services_mix_unsolicited_solicited(self):
    """Validate multiple concurrent discovery sessions running on both devices.
    - DUT1 & DUT2 running Publish for X
    - DUT1 & DUT2 running Publish for Y
    - DUT1 Subscribes for X
    - DUT2 Subscribes for Y
    Message exchanges.

    Session A is Unsolicited/Passive.
    Session B is Solicited/Active.

    Note: test requires that devices support 2 publish sessions concurrently.
    The test will be skipped if the devices are not capable.
    """
    self.run_multiple_concurrent_services(
      type_x=[aconsts.PUBLISH_TYPE_UNSOLICITED, aconsts.SUBSCRIBE_TYPE_PASSIVE],
      type_y=[aconsts.PUBLISH_TYPE_SOLICITED, aconsts.SUBSCRIBE_TYPE_ACTIVE])

  #########################################################

  @test_tracker_info(uuid="908ec896-fc7a-4ee4-b633-a2f042b74448")
  def test_upper_lower_service_name_equivalence(self):
    """Validate that Service Name is case-insensitive. Publish a service name
    with mixed case, subscribe to the same service name with alternative case
    and verify that discovery happens."""
    p_dut = self.android_devices[0]
    s_dut = self.android_devices[1]

    pub_service_name = "GoogleAbCdEf"
    sub_service_name = "GoogleaBcDeF"

    autils.create_discovery_pair(p_dut, s_dut,
                               p_config=autils.create_discovery_config(
                                 pub_service_name,
                                 aconsts.PUBLISH_TYPE_UNSOLICITED),
                               s_config=autils.create_discovery_config(
                                 sub_service_name,
                                 aconsts.SUBSCRIBE_TYPE_PASSIVE),
                               device_startup_offset=self.device_startup_offset)
