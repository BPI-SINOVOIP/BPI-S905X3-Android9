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

from acts import asserts
from acts.test_decorators import test_tracker_info
from acts.test_utils.wifi.aware import aware_const as aconsts
from acts.test_utils.wifi.aware import aware_test_utils as autils
from acts.test_utils.wifi.aware.AwareBaseTest import AwareBaseTest

KEY_ID = "id"
KEY_TX_OK_COUNT = "tx_ok_count"
KEY_TX_FAIL_COUNT = "tx_fail_count"
KEY_RX_COUNT = "rx_count"


class MessagesStressTest(AwareBaseTest):
  """Set of stress tests for Wi-Fi Aware L2 (layer 2) message exchanges."""

  # Number of iterations in the stress test (number of messages)
  NUM_ITERATIONS = 100

  # Maximum permitted percentage of messages which fail to be transmitted
  # correctly
  MAX_TX_FAILURE_PERCENTAGE = 2

  # Maximum permitted percentage of messages which are received more than once
  # (indicating, most likely, that the ACK wasn't received and the message was
  # retransmitted)
  MAX_DUPLICATE_RX_PERCENTAGE = 2

  SERVICE_NAME = "GoogleTestServiceXY"

  def __init__(self, controllers):
    AwareBaseTest.__init__(self, controllers)

  def init_info(self, msg, id, messages_by_msg, messages_by_id):
    """Initialize the message data structures.

    Args:
      msg: message text
      id: message id
      messages_by_msg: {text -> {id, tx_ok_count, tx_fail_count, rx_count}}
      messages_by_id: {id -> text}
    """
    messages_by_msg[msg] = {}
    messages_by_msg[msg][KEY_ID] = id
    messages_by_msg[msg][KEY_TX_OK_COUNT] = 0
    messages_by_msg[msg][KEY_TX_FAIL_COUNT] = 0
    messages_by_msg[msg][KEY_RX_COUNT] = 0
    messages_by_id[id] = msg

  def wait_for_tx_events(self, dut, num_msgs, messages_by_msg, messages_by_id):
    """Wait for messages to be transmitted and update data structures.

    Args:
      dut: device under test
      num_msgs: number of expected message tx
      messages_by_msg: {text -> {id, tx_ok_count, tx_fail_count, rx_count}}
      messages_by_id: {id -> text}
    """
    num_ok_tx_confirmations = 0
    num_fail_tx_confirmations = 0
    num_unexpected_ids = 0
    tx_events_regex = "%s|%s" % (aconsts.SESSION_CB_ON_MESSAGE_SEND_FAILED,
                                 aconsts.SESSION_CB_ON_MESSAGE_SENT)
    while num_ok_tx_confirmations + num_fail_tx_confirmations < num_msgs:
      try:
        events = dut.ed.pop_events(tx_events_regex, autils.EVENT_TIMEOUT)
        for event in events:
          if (event["name"] != aconsts.SESSION_CB_ON_MESSAGE_SENT and
              event["name"] != aconsts.SESSION_CB_ON_MESSAGE_SEND_FAILED):
            asserts.fail("Unexpected event: %s" % event)
          is_tx_ok = event["name"] == aconsts.SESSION_CB_ON_MESSAGE_SENT

          id = event["data"][aconsts.SESSION_CB_KEY_MESSAGE_ID]
          if id in messages_by_id:
            msg = messages_by_id[id]
            if is_tx_ok:
              messages_by_msg[msg][
                  KEY_TX_OK_COUNT] = messages_by_msg[msg][KEY_TX_OK_COUNT] + 1
              if messages_by_msg[msg][KEY_TX_OK_COUNT] == 1:
                num_ok_tx_confirmations = num_ok_tx_confirmations + 1
            else:
              messages_by_msg[msg][KEY_TX_FAIL_COUNT] = (
                  messages_by_msg[msg][KEY_TX_FAIL_COUNT] + 1)
              if messages_by_msg[msg][KEY_TX_FAIL_COUNT] == 1:
                num_fail_tx_confirmations = num_fail_tx_confirmations + 1
          else:
            self.log.warning(
                "Tx confirmation of unknown message ID received: %s", event)
            num_unexpected_ids = num_unexpected_ids + 1
      except queue.Empty:
        self.log.warning("[%s] Timed out waiting for any MESSAGE_SEND* event - "
                         "assuming the rest are not coming", dut.pretty_name)
        break

    return (num_ok_tx_confirmations, num_fail_tx_confirmations,
            num_unexpected_ids)

  def wait_for_rx_events(self, dut, num_msgs, messages_by_msg):
    """Wait for messages to be received and update data structures

    Args:
      dut: device under test
      num_msgs: number of expected messages to receive
      messages_by_msg: {text -> {id, tx_ok_count, tx_fail_count, rx_count}}
    """
    num_rx_msgs = 0
    while num_rx_msgs < num_msgs:
      try:
        event = dut.ed.pop_event(aconsts.SESSION_CB_ON_MESSAGE_RECEIVED,
                                 autils.EVENT_TIMEOUT)
        msg = event["data"][aconsts.SESSION_CB_KEY_MESSAGE_AS_STRING]
        if msg not in messages_by_msg:
          messages_by_msg[msg] = {}
          messages_by_msg[msg][KEY_ID] = -1
          messages_by_msg[msg][KEY_TX_OK_COUNT] = 0
          messages_by_msg[msg][KEY_TX_FAIL_COUNT] = 0
          messages_by_msg[msg][KEY_RX_COUNT] = 1

        messages_by_msg[msg][
            KEY_RX_COUNT] = messages_by_msg[msg][KEY_RX_COUNT] + 1
        if messages_by_msg[msg][KEY_RX_COUNT] == 1:
          num_rx_msgs = num_rx_msgs + 1
      except queue.Empty:
        self.log.warning(
            "[%s] Timed out waiting for ON_MESSAGE_RECEIVED event - "
            "assuming the rest are not coming", dut.pretty_name)
        break

  def analyze_results(self, results, messages_by_msg):
    """Analyze the results of the stress message test and add to the results
    dictionary

    Args:
      results: result dictionary into which to add data
      messages_by_msg: {text -> {id, tx_ok_count, tx_fail_count, rx_count}}
    """
    results["raw_data"] = messages_by_msg
    results["tx_count_success"] = 0
    results["tx_count_duplicate_success"] = 0
    results["tx_count_fail"] = 0
    results["tx_count_duplicate_fail"] = 0
    results["tx_count_neither"] = 0
    results["tx_count_tx_ok_but_no_rx"] = 0
    results["rx_count"] = 0
    results["rx_count_duplicate"] = 0
    results["rx_count_no_ok_tx_indication"] = 0
    results["rx_count_fail_tx_indication"] = 0
    results["rx_count_no_tx_message"] = 0

    for msg, data in messages_by_msg.items():
      if data[KEY_TX_OK_COUNT] > 0:
        results["tx_count_success"] = results["tx_count_success"] + 1
      if data[KEY_TX_OK_COUNT] > 1:
        results["tx_count_duplicate_success"] = (
            results["tx_count_duplicate_success"] + 1)
      if data[KEY_TX_FAIL_COUNT] > 0:
        results["tx_count_fail"] = results["tx_count_fail"] + 1
      if data[KEY_TX_FAIL_COUNT] > 1:
        results[
            "tx_count_duplicate_fail"] = results["tx_count_duplicate_fail"] + 1
      if (data[KEY_TX_OK_COUNT] == 0 and data[KEY_TX_FAIL_COUNT] == 0 and
          data[KEY_ID] != -1):
        results["tx_count_neither"] = results["tx_count_neither"] + 1
      if data[KEY_TX_OK_COUNT] > 0 and data[KEY_RX_COUNT] == 0:
        results["tx_count_tx_ok_but_no_rx"] = (
            results["tx_count_tx_ok_but_no_rx"] + 1)
      if data[KEY_RX_COUNT] > 0:
        results["rx_count"] = results["rx_count"] + 1
      if data[KEY_RX_COUNT] > 1:
        results["rx_count_duplicate"] = results["rx_count_duplicate"] + 1
      if data[KEY_RX_COUNT] > 0 and data[KEY_TX_OK_COUNT] == 0:
        results["rx_count_no_ok_tx_indication"] = (
            results["rx_count_no_ok_tx_indication"] + 1)
      if data[KEY_RX_COUNT] > 0 and data[KEY_TX_FAIL_COUNT] > 0:
        results["rx_count_fail_tx_indication"] = (
            results["rx_count_fail_tx_indication"] + 1)
      if data[KEY_RX_COUNT] > 0 and data[KEY_ID] == -1:
        results[
            "rx_count_no_tx_message"] = results["rx_count_no_tx_message"] + 1

  #######################################################################

  @test_tracker_info(uuid="e88c060f-4ca7-41c1-935a-d3d62878ec0b")
  def test_stress_message(self):
    """Stress test for bi-directional message transmission and reception."""
    p_dut = self.android_devices[0]
    s_dut = self.android_devices[1]

    # Start up a discovery session
    discovery_data = autils.create_discovery_pair(
        p_dut,
        s_dut,
        p_config=autils.create_discovery_config(
            self.SERVICE_NAME, aconsts.PUBLISH_TYPE_UNSOLICITED),
        s_config=autils.create_discovery_config(self.SERVICE_NAME,
                                                aconsts.SUBSCRIBE_TYPE_PASSIVE),
        device_startup_offset=self.device_startup_offset,
        msg_id=self.get_next_msg_id())
    p_id = discovery_data[0]
    s_id = discovery_data[1]
    p_disc_id = discovery_data[2]
    s_disc_id = discovery_data[3]
    peer_id_on_sub = discovery_data[4]
    peer_id_on_pub = discovery_data[5]

    # Store information on Tx & Rx messages
    messages_by_msg = {}  # keyed by message text
    # {text -> {id, tx_ok_count, tx_fail_count, rx_count}}
    messages_by_id = {}  # keyed by message ID {id -> text}

    # send all messages at once (one in each direction)
    for i in range(self.NUM_ITERATIONS):
      msg_p2s = "Message Publisher -> Subscriber #%d" % i
      next_msg_id = self.get_next_msg_id()
      self.init_info(msg_p2s, next_msg_id, messages_by_msg, messages_by_id)
      p_dut.droid.wifiAwareSendMessage(p_disc_id, peer_id_on_pub, next_msg_id,
                                       msg_p2s, 0)

      msg_s2p = "Message Subscriber -> Publisher #%d" % i
      next_msg_id = self.get_next_msg_id()
      self.init_info(msg_s2p, next_msg_id, messages_by_msg, messages_by_id)
      s_dut.droid.wifiAwareSendMessage(s_disc_id, peer_id_on_sub, next_msg_id,
                                       msg_s2p, 0)

    # wait for message tx confirmation
    (p_tx_ok_count, p_tx_fail_count, p_tx_unknown_id) = self.wait_for_tx_events(
        p_dut, self.NUM_ITERATIONS, messages_by_msg, messages_by_id)
    (s_tx_ok_count, s_tx_fail_count, s_tx_unknown_id) = self.wait_for_tx_events(
        s_dut, self.NUM_ITERATIONS, messages_by_msg, messages_by_id)
    self.log.info("Transmission done: pub=%d, sub=%d transmitted successfully",
                  p_tx_ok_count, s_tx_ok_count)

    # wait for message rx confirmation (giving it the total number of messages
    # transmitted rather than just those transmitted correctly since sometimes
    # the Tx doesn't get that information correctly. I.e. a message the Tx
    # thought was not transmitted correctly is actually received - missing ACK?
    # bug?)
    self.wait_for_rx_events(p_dut, self.NUM_ITERATIONS, messages_by_msg)
    self.wait_for_rx_events(s_dut, self.NUM_ITERATIONS, messages_by_msg)

    # analyze results
    results = {}
    results["tx_count"] = 2 * self.NUM_ITERATIONS
    results["tx_unknown_ids"] = p_tx_unknown_id + s_tx_unknown_id
    self.analyze_results(results, messages_by_msg)

    # clear errors
    asserts.assert_equal(results["tx_unknown_ids"], 0, "Message ID corruption",
                         results)
    asserts.assert_equal(results["tx_count_neither"], 0,
                         "Tx message with no success or fail indication",
                         results)
    asserts.assert_equal(results["tx_count_duplicate_fail"], 0,
                         "Duplicate Tx fail messages", results)
    asserts.assert_equal(results["tx_count_duplicate_success"], 0,
                         "Duplicate Tx success messages", results)
    asserts.assert_equal(results["rx_count_no_tx_message"], 0,
                         "Rx message which wasn't sent - message corruption?",
                         results)
    asserts.assert_equal(results["tx_count_tx_ok_but_no_rx"], 0,
                         "Tx got ACK but Rx didn't get message", results)

    # possibly ok - but flag since most frequently a bug
    asserts.assert_equal(results["rx_count_no_ok_tx_indication"], 0,
                         "Message received but Tx didn't get ACK", results)
    asserts.assert_equal(results["rx_count_fail_tx_indication"], 0,
                         "Message received but Tx didn't get ACK", results)

    # permissible failures based on thresholds
    asserts.assert_true(results["tx_count_fail"] <= (
          self.MAX_TX_FAILURE_PERCENTAGE * self.NUM_ITERATIONS / 100),
                        "Number of Tx failures exceeds threshold",
                        extras=results)
    asserts.assert_true(results["rx_count_duplicate"] <= (
        self.MAX_DUPLICATE_RX_PERCENTAGE * self.NUM_ITERATIONS / 100),
                        "Number of duplicate Rx exceeds threshold",
                        extras=results)

    asserts.explicit_pass("test_stress_message done", extras=results)