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


class MessageTest(AwareBaseTest):
  """Set of tests for Wi-Fi Aware L2 (layer 2) message exchanges."""

  # configuration parameters used by tests
  PAYLOAD_SIZE_MIN = 0
  PAYLOAD_SIZE_TYPICAL = 1
  PAYLOAD_SIZE_MAX = 2

  NUM_MSGS_NO_QUEUE = 10
  NUM_MSGS_QUEUE_DEPTH_MULT = 2  # number of messages = mult * queue depth

  def __init__(self, controllers):
    AwareBaseTest.__init__(self, controllers)

  def create_msg(self, caps, payload_size, id):
    """Creates a message string of the specified size containing the input id.

    Args:
      caps: Device capabilities.
      payload_size: The size of the message to create - min (null or empty
                    message), typical, max (based on device capabilities). Use
                    the PAYLOAD_SIZE_xx constants.
      id: Information to include in the generated message (or None).

    Returns: A string of the requested size, optionally containing the id.
    """
    if payload_size == self.PAYLOAD_SIZE_MIN:
      # arbitrarily return a None or an empty string (equivalent messages)
      return None if id % 2 == 0 else ""
    elif payload_size == self.PAYLOAD_SIZE_TYPICAL:
      return "*** ID=%d ***" % id + string.ascii_uppercase
    else:  # PAYLOAD_SIZE_MAX
      return "*** ID=%4d ***" % id + "M" * (
          caps[aconsts.CAP_MAX_SERVICE_SPECIFIC_INFO_LEN] - 15)

  def create_config(self, is_publish, extra_diff=None):
    """Create a base configuration based on input parameters.

    Args:
      is_publish: True for publish, False for subscribe sessions.
      extra_diff: String to add to service name: allows differentiating
                  discovery sessions.

    Returns:
      publish discovery configuration object.
    """
    config = {}
    if is_publish:
      config[aconsts.
             DISCOVERY_KEY_DISCOVERY_TYPE] = aconsts.PUBLISH_TYPE_UNSOLICITED
    else:
      config[
          aconsts.DISCOVERY_KEY_DISCOVERY_TYPE] = aconsts.SUBSCRIBE_TYPE_PASSIVE
    config[aconsts.DISCOVERY_KEY_SERVICE_NAME] = "GoogleTestServiceX" + (
        extra_diff if extra_diff is not None else "")
    return config

  def prep_message_exchange(self, extra_diff=None):
    """Creates a discovery session (publish and subscribe), and waits for
    service discovery - at that point the sessions are ready for message
    exchange.

    Args:
      extra_diff: String to add to service name: allows differentiating
                  discovery sessions.
    """
    p_dut = self.android_devices[0]
    p_dut.pretty_name = "Publisher"
    s_dut = self.android_devices[1]
    s_dut.pretty_name = "Subscriber"

    # if differentiating (multiple) sessions then should decorate events with id
    use_id = extra_diff is not None

    # Publisher+Subscriber: attach and wait for confirmation
    p_id = p_dut.droid.wifiAwareAttach(False, None, use_id)
    autils.wait_for_event(p_dut, aconsts.EVENT_CB_ON_ATTACHED
                          if not use_id else autils.decorate_event(
                              aconsts.EVENT_CB_ON_ATTACHED, p_id))
    time.sleep(self.device_startup_offset)
    s_id = s_dut.droid.wifiAwareAttach(False, None, use_id)
    autils.wait_for_event(s_dut, aconsts.EVENT_CB_ON_ATTACHED
                          if not use_id else autils.decorate_event(
                              aconsts.EVENT_CB_ON_ATTACHED, s_id))

    # Publisher: start publish and wait for confirmation
    p_disc_id = p_dut.droid.wifiAwarePublish(p_id,
                                             self.create_config(
                                                 True, extra_diff=extra_diff),
                                             use_id)
    autils.wait_for_event(p_dut, aconsts.SESSION_CB_ON_PUBLISH_STARTED
                          if not use_id else autils.decorate_event(
                              aconsts.SESSION_CB_ON_PUBLISH_STARTED, p_disc_id))

    # Subscriber: start subscribe and wait for confirmation
    s_disc_id = s_dut.droid.wifiAwareSubscribe(
        s_id, self.create_config(False, extra_diff=extra_diff), use_id)
    autils.wait_for_event(s_dut, aconsts.SESSION_CB_ON_SUBSCRIBE_STARTED
                          if not use_id else autils.decorate_event(
                              aconsts.SESSION_CB_ON_SUBSCRIBE_STARTED,
                              s_disc_id))

    # Subscriber: wait for service discovery
    discovery_event = autils.wait_for_event(
        s_dut, aconsts.SESSION_CB_ON_SERVICE_DISCOVERED
        if not use_id else autils.decorate_event(
            aconsts.SESSION_CB_ON_SERVICE_DISCOVERED, s_disc_id))
    peer_id_on_sub = discovery_event["data"][aconsts.SESSION_CB_KEY_PEER_ID]

    return {
        "p_dut": p_dut,
        "s_dut": s_dut,
        "p_id": p_id,
        "s_id": s_id,
        "p_disc_id": p_disc_id,
        "s_disc_id": s_disc_id,
        "peer_id_on_sub": peer_id_on_sub
    }

  def run_message_no_queue(self, payload_size):
    """Validate L2 message exchange between publisher & subscriber with no
    queueing - i.e. wait for an ACK on each message before sending the next
    message.

    Args:
      payload_size: min, typical, or max (PAYLOAD_SIZE_xx).
    """
    discovery_info = self.prep_message_exchange()
    p_dut = discovery_info["p_dut"]
    s_dut = discovery_info["s_dut"]
    p_disc_id = discovery_info["p_disc_id"]
    s_disc_id = discovery_info["s_disc_id"]
    peer_id_on_sub = discovery_info["peer_id_on_sub"]

    for i in range(self.NUM_MSGS_NO_QUEUE):
      msg = self.create_msg(s_dut.aware_capabilities, payload_size, i)
      msg_id = self.get_next_msg_id()
      s_dut.droid.wifiAwareSendMessage(s_disc_id, peer_id_on_sub, msg_id, msg,
                                       0)
      tx_event = autils.wait_for_event(s_dut,
                                       aconsts.SESSION_CB_ON_MESSAGE_SENT)
      rx_event = autils.wait_for_event(p_dut,
                                       aconsts.SESSION_CB_ON_MESSAGE_RECEIVED)
      asserts.assert_equal(msg_id,
                           tx_event["data"][aconsts.SESSION_CB_KEY_MESSAGE_ID],
                           "Subscriber -> Publisher message ID corrupted")
      autils.assert_equal_strings(
          msg, rx_event["data"][aconsts.SESSION_CB_KEY_MESSAGE_AS_STRING],
          "Subscriber -> Publisher message %d corrupted" % i)

    peer_id_on_pub = rx_event["data"][aconsts.SESSION_CB_KEY_PEER_ID]
    for i in range(self.NUM_MSGS_NO_QUEUE):
      msg = self.create_msg(s_dut.aware_capabilities, payload_size, 1000 + i)
      msg_id = self.get_next_msg_id()
      p_dut.droid.wifiAwareSendMessage(p_disc_id, peer_id_on_pub, msg_id, msg,
                                       0)
      tx_event = autils.wait_for_event(p_dut,
                                       aconsts.SESSION_CB_ON_MESSAGE_SENT)
      rx_event = autils.wait_for_event(s_dut,
                                       aconsts.SESSION_CB_ON_MESSAGE_RECEIVED)
      asserts.assert_equal(msg_id,
                           tx_event["data"][aconsts.SESSION_CB_KEY_MESSAGE_ID],
                           "Publisher -> Subscriber message ID corrupted")
      autils.assert_equal_strings(
          msg, rx_event["data"][aconsts.SESSION_CB_KEY_MESSAGE_AS_STRING],
          "Publisher -> Subscriber message %d corrupted" % i)

    # verify there are no more events
    time.sleep(autils.EVENT_TIMEOUT)
    autils.verify_no_more_events(p_dut, timeout=0)
    autils.verify_no_more_events(s_dut, timeout=0)

  def wait_for_messages(self, tx_msgs, tx_msg_ids, tx_disc_id, rx_disc_id,
                        tx_dut, rx_dut, are_msgs_empty=False):
    """Validate that all expected messages are transmitted correctly and
    received as expected. Method is called after the messages are sent into
    the transmission queue.

    Note: that message can be transmitted and received out-of-order (which is
    acceptable and the method handles that correctly).

    Args:
      tx_msgs: dictionary of transmitted messages
      tx_msg_ids: dictionary of transmitted message ids
      tx_disc_id: transmitter discovery session id (None for no decoration)
      rx_disc_id: receiver discovery session id (None for no decoration)
      tx_dut: transmitter device
      rx_dut: receiver device
      are_msgs_empty: True if the messages are None or empty (changes dup detection)

    Returns: the peer ID from any of the received messages
    """
    # peer id on receiver
    peer_id_on_rx = None

    # wait for all messages to be transmitted
    still_to_be_tx = len(tx_msg_ids)
    while still_to_be_tx != 0:
      tx_event = autils.wait_for_event(
          tx_dut, aconsts.SESSION_CB_ON_MESSAGE_SENT
          if tx_disc_id is None else autils.decorate_event(
              aconsts.SESSION_CB_ON_MESSAGE_SENT, tx_disc_id))
      tx_msg_id = tx_event["data"][aconsts.SESSION_CB_KEY_MESSAGE_ID]
      tx_msg_ids[tx_msg_id] = tx_msg_ids[tx_msg_id] + 1
      if tx_msg_ids[tx_msg_id] == 1:
        still_to_be_tx = still_to_be_tx - 1

    # check for any duplicate transmit notifications
    asserts.assert_equal(
        len(tx_msg_ids),
        sum(tx_msg_ids.values()),
        "Duplicate transmit message IDs: %s" % tx_msg_ids)

    # wait for all messages to be received
    still_to_be_rx = len(tx_msg_ids)
    while still_to_be_rx != 0:
      rx_event = autils.wait_for_event(
          rx_dut, aconsts.SESSION_CB_ON_MESSAGE_RECEIVED
          if rx_disc_id is None else autils.decorate_event(
              aconsts.SESSION_CB_ON_MESSAGE_RECEIVED, rx_disc_id))
      peer_id_on_rx = rx_event["data"][aconsts.SESSION_CB_KEY_PEER_ID]
      if are_msgs_empty:
        still_to_be_rx = still_to_be_rx - 1
      else:
        rx_msg = rx_event["data"][aconsts.SESSION_CB_KEY_MESSAGE_AS_STRING]
        asserts.assert_true(
            rx_msg in tx_msgs,
            "Received a message we did not send!? -- '%s'" % rx_msg)
        tx_msgs[rx_msg] = tx_msgs[rx_msg] + 1
        if tx_msgs[rx_msg] == 1:
          still_to_be_rx = still_to_be_rx - 1

    # check for any duplicate received messages
    if not are_msgs_empty:
      asserts.assert_equal(
          len(tx_msgs),
          sum(tx_msgs.values()), "Duplicate transmit messages: %s" % tx_msgs)

    return peer_id_on_rx

  def run_message_with_queue(self, payload_size):
    """Validate L2 message exchange between publisher & subscriber with
    queueing - i.e. transmit all messages and then wait for ACKs.

    Args:
      payload_size: min, typical, or max (PAYLOAD_SIZE_xx).
    """
    discovery_info = self.prep_message_exchange()
    p_dut = discovery_info["p_dut"]
    s_dut = discovery_info["s_dut"]
    p_disc_id = discovery_info["p_disc_id"]
    s_disc_id = discovery_info["s_disc_id"]
    peer_id_on_sub = discovery_info["peer_id_on_sub"]

    msgs = {}
    msg_ids = {}
    for i in range(
        self.NUM_MSGS_QUEUE_DEPTH_MULT *
        s_dut.aware_capabilities[aconsts.CAP_MAX_QUEUED_TRANSMIT_MESSAGES]):
      msg = self.create_msg(s_dut.aware_capabilities, payload_size, i)
      msg_id = self.get_next_msg_id()
      msgs[msg] = 0
      msg_ids[msg_id] = 0
      s_dut.droid.wifiAwareSendMessage(s_disc_id, peer_id_on_sub, msg_id, msg,
                                       0)
    peer_id_on_pub = self.wait_for_messages(
        msgs, msg_ids, None, None, s_dut, p_dut,
        payload_size == self.PAYLOAD_SIZE_MIN)

    msgs = {}
    msg_ids = {}
    for i in range(
            self.NUM_MSGS_QUEUE_DEPTH_MULT *
            p_dut.aware_capabilities[aconsts.CAP_MAX_QUEUED_TRANSMIT_MESSAGES]):
      msg = self.create_msg(p_dut.aware_capabilities, payload_size, 1000 + i)
      msg_id = self.get_next_msg_id()
      msgs[msg] = 0
      msg_ids[msg_id] = 0
      p_dut.droid.wifiAwareSendMessage(p_disc_id, peer_id_on_pub, msg_id, msg,
                                       0)
    self.wait_for_messages(msgs, msg_ids, None, None, p_dut, s_dut,
                           payload_size == self.PAYLOAD_SIZE_MIN)

    # verify there are no more events
    time.sleep(autils.EVENT_TIMEOUT)
    autils.verify_no_more_events(p_dut, timeout=0)
    autils.verify_no_more_events(s_dut, timeout=0)

  def run_message_multi_session_with_queue(self, payload_size):
    """Validate L2 message exchange between publishers & subscribers with
    queueing - i.e. transmit all messages and then wait for ACKs. Uses 2
    discovery sessions running concurrently and validates that messages
    arrive at the correct destination.

    Args:
      payload_size: min, typical, or max (PAYLOAD_SIZE_xx)
    """
    discovery_info1 = self.prep_message_exchange(extra_diff="-111")
    p_dut = discovery_info1["p_dut"] # same for both sessions
    s_dut = discovery_info1["s_dut"] # same for both sessions
    p_disc_id1 = discovery_info1["p_disc_id"]
    s_disc_id1 = discovery_info1["s_disc_id"]
    peer_id_on_sub1 = discovery_info1["peer_id_on_sub"]

    discovery_info2 = self.prep_message_exchange(extra_diff="-222")
    p_disc_id2 = discovery_info2["p_disc_id"]
    s_disc_id2 = discovery_info2["s_disc_id"]
    peer_id_on_sub2 = discovery_info2["peer_id_on_sub"]

    msgs1 = {}
    msg_ids1 = {}
    msgs2 = {}
    msg_ids2 = {}
    for i in range(
            self.NUM_MSGS_QUEUE_DEPTH_MULT *
            s_dut.aware_capabilities[aconsts.CAP_MAX_QUEUED_TRANSMIT_MESSAGES]):
      msg1 = self.create_msg(s_dut.aware_capabilities, payload_size, i)
      msg_id1 = self.get_next_msg_id()
      msgs1[msg1] = 0
      msg_ids1[msg_id1] = 0
      s_dut.droid.wifiAwareSendMessage(s_disc_id1, peer_id_on_sub1, msg_id1,
                                       msg1, 0)
      msg2 = self.create_msg(s_dut.aware_capabilities, payload_size, 100 + i)
      msg_id2 = self.get_next_msg_id()
      msgs2[msg2] = 0
      msg_ids2[msg_id2] = 0
      s_dut.droid.wifiAwareSendMessage(s_disc_id2, peer_id_on_sub2, msg_id2,
                                       msg2, 0)

    peer_id_on_pub1 = self.wait_for_messages(
        msgs1, msg_ids1, s_disc_id1, p_disc_id1, s_dut, p_dut,
        payload_size == self.PAYLOAD_SIZE_MIN)
    peer_id_on_pub2 = self.wait_for_messages(
        msgs2, msg_ids2, s_disc_id2, p_disc_id2, s_dut, p_dut,
        payload_size == self.PAYLOAD_SIZE_MIN)

    msgs1 = {}
    msg_ids1 = {}
    msgs2 = {}
    msg_ids2 = {}
    for i in range(
            self.NUM_MSGS_QUEUE_DEPTH_MULT *
            p_dut.aware_capabilities[aconsts.CAP_MAX_QUEUED_TRANSMIT_MESSAGES]):
      msg1 = self.create_msg(p_dut.aware_capabilities, payload_size, 1000 + i)
      msg_id1 = self.get_next_msg_id()
      msgs1[msg1] = 0
      msg_ids1[msg_id1] = 0
      p_dut.droid.wifiAwareSendMessage(p_disc_id1, peer_id_on_pub1, msg_id1,
                                       msg1, 0)
      msg2 = self.create_msg(p_dut.aware_capabilities, payload_size, 1100 + i)
      msg_id2 = self.get_next_msg_id()
      msgs2[msg2] = 0
      msg_ids2[msg_id2] = 0
      p_dut.droid.wifiAwareSendMessage(p_disc_id2, peer_id_on_pub2, msg_id2,
                                       msg2, 0)

    self.wait_for_messages(msgs1, msg_ids1, p_disc_id1, s_disc_id1, p_dut,
                           s_dut, payload_size == self.PAYLOAD_SIZE_MIN)
    self.wait_for_messages(msgs2, msg_ids2, p_disc_id2, s_disc_id2, p_dut,
                           s_dut, payload_size == self.PAYLOAD_SIZE_MIN)

    # verify there are no more events
    time.sleep(autils.EVENT_TIMEOUT)
    autils.verify_no_more_events(p_dut, timeout=0)
    autils.verify_no_more_events(s_dut, timeout=0)

  ############################################################################

  @test_tracker_info(uuid="a8cd0512-b279-425f-93cf-949ddba22c7a")
  def test_message_no_queue_min(self):
    """Functional / Message / No queue
    - Minimal payload size (None or "")
    """
    self.run_message_no_queue(self.PAYLOAD_SIZE_MIN)

  @test_tracker_info(uuid="2c26170a-5d0a-4cf4-b0b9-56ef03f5dcf4")
  def test_message_no_queue_typical(self):
    """Functional / Message / No queue
    - Typical payload size
    """
    self.run_message_no_queue(self.PAYLOAD_SIZE_TYPICAL)

  @test_tracker_info(uuid="c984860c-b62d-4d9b-8bce-4d894ea3bfbe")
  def test_message_no_queue_max(self):
    """Functional / Message / No queue
    - Max payload size (based on device capabilities)
    """
    self.run_message_no_queue(self.PAYLOAD_SIZE_MAX)

  @test_tracker_info(uuid="3f06de73-31ab-4e0c-bc6f-59abdaf87f4f")
  def test_message_with_queue_min(self):
    """Functional / Message / With queue
    - Minimal payload size (none or "")
    """
    self.run_message_with_queue(self.PAYLOAD_SIZE_MIN)

  @test_tracker_info(uuid="9b7f5bd8-b0b1-479e-8e4b-9db0bb56767b")
  def test_message_with_queue_typical(self):
    """Functional / Message / With queue
    - Typical payload size
    """
    self.run_message_with_queue(self.PAYLOAD_SIZE_TYPICAL)

  @test_tracker_info(uuid="4f9a6dce-3050-4e6a-a143-53592c6c7c28")
  def test_message_with_queue_max(self):
    """Functional / Message / With queue
    - Max payload size (based on device capabilities)
    """
    self.run_message_with_queue(self.PAYLOAD_SIZE_MAX)

  @test_tracker_info(uuid="4cece232-0983-4d6b-90a9-1bb9314b64f0")
  def test_message_with_multiple_discovery_sessions_typical(self):
    """Functional / Message / Multiple sessions

     Sets up 2 discovery sessions on 2 devices. Sends a message in each
     direction on each discovery session and verifies that reaches expected
     destination.
    """
    self.run_message_multi_session_with_queue(self.PAYLOAD_SIZE_TYPICAL)
