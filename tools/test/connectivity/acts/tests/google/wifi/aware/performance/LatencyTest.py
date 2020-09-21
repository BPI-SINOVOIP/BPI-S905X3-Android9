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
from acts.test_utils.net import connectivity_const as cconsts
from acts.test_utils.wifi.aware import aware_const as aconsts
from acts.test_utils.wifi.aware import aware_test_utils as autils
from acts.test_utils.wifi.aware.AwareBaseTest import AwareBaseTest


class LatencyTest(AwareBaseTest):
  """Set of tests for Wi-Fi Aware to measure latency of Aware operations."""
  SERVICE_NAME = "GoogleTestServiceXY"

  # number of second to 'reasonably' wait to make sure that devices synchronize
  # with each other - useful for OOB test cases, where the OOB discovery would
  # take some time
  WAIT_FOR_CLUSTER = 5

  def __init__(self, controllers):
    AwareBaseTest.__init__(self, controllers)

  def start_discovery_session(self, dut, session_id, is_publish, dtype):
    """Start a discovery session

    Args:
      dut: Device under test
      session_id: ID of the Aware session in which to start discovery
      is_publish: True for a publish session, False for subscribe session
      dtype: Type of the discovery session

    Returns:
      Discovery session started event.
    """
    config = {}
    config[aconsts.DISCOVERY_KEY_DISCOVERY_TYPE] = dtype
    config[aconsts.DISCOVERY_KEY_SERVICE_NAME] = "GoogleTestServiceXY"

    if is_publish:
      disc_id = dut.droid.wifiAwarePublish(session_id, config)
      event_name = aconsts.SESSION_CB_ON_PUBLISH_STARTED
    else:
      disc_id = dut.droid.wifiAwareSubscribe(session_id, config)
      event_name = aconsts.SESSION_CB_ON_SUBSCRIBE_STARTED

    event = autils.wait_for_event(dut, event_name)
    return disc_id, event

  def run_synchronization_latency(self, results, do_unsolicited_passive,
                                  dw_24ghz, dw_5ghz, num_iterations,
                                  startup_offset, timeout_period):
    """Run the synchronization latency test with the specified DW intervals.
    There is no direct measure of synchronization. Instead starts a discovery
    session as soon as possible and measures both probability of discovery
    within a timeout period and the actual discovery time (not necessarily
    accurate).

    Args:
      results: Result array to be populated - will add results (not erase it)
      do_unsolicited_passive: True for unsolicited/passive, False for
                              solicited/active.
      dw_24ghz: DW interval in the 2.4GHz band.
      dw_5ghz: DW interval in the 5GHz band.
      startup_offset: The start-up gap (in seconds) between the two devices
      timeout_period: Time period over which to measure synchronization
    """
    key = "%s_dw24_%d_dw5_%d_offset_%d" % (
        "unsolicited_passive" if do_unsolicited_passive else "solicited_active",
        dw_24ghz, dw_5ghz, startup_offset)
    results[key] = {}
    results[key]["num_iterations"] = num_iterations

    p_dut = self.android_devices[0]
    p_dut.pretty_name = "Publisher"
    s_dut = self.android_devices[1]
    s_dut.pretty_name = "Subscriber"

    # override the default DW configuration
    autils.config_power_settings(p_dut, dw_24ghz, dw_5ghz)
    autils.config_power_settings(s_dut, dw_24ghz, dw_5ghz)

    latencies = []
    failed_discoveries = 0
    for i in range(num_iterations):
      # Publisher+Subscriber: attach and wait for confirmation
      p_id = p_dut.droid.wifiAwareAttach(False)
      autils.wait_for_event(p_dut, aconsts.EVENT_CB_ON_ATTACHED)
      time.sleep(startup_offset)
      s_id = s_dut.droid.wifiAwareAttach(False)
      autils.wait_for_event(s_dut, aconsts.EVENT_CB_ON_ATTACHED)

      # start publish
      p_disc_id, p_disc_event = self.start_discovery_session(
          p_dut, p_id, True, aconsts.PUBLISH_TYPE_UNSOLICITED
          if do_unsolicited_passive else aconsts.PUBLISH_TYPE_SOLICITED)

      # start subscribe
      s_disc_id, s_session_event = self.start_discovery_session(
          s_dut, s_id, False, aconsts.SUBSCRIBE_TYPE_PASSIVE
          if do_unsolicited_passive else aconsts.SUBSCRIBE_TYPE_ACTIVE)

      # wait for discovery (allow for failures here since running lots of
      # samples and would like to get the partial data even in the presence of
      # errors)
      try:
        discovery_event = s_dut.ed.pop_event(
            aconsts.SESSION_CB_ON_SERVICE_DISCOVERED, timeout_period)
        s_dut.log.info("[Subscriber] SESSION_CB_ON_SERVICE_DISCOVERED: %s",
                       discovery_event["data"])
      except queue.Empty:
        s_dut.log.info("[Subscriber] Timed out while waiting for "
                       "SESSION_CB_ON_SERVICE_DISCOVERED")
        failed_discoveries = failed_discoveries + 1
        continue
      finally:
        # destroy sessions
        p_dut.droid.wifiAwareDestroyDiscoverySession(p_disc_id)
        s_dut.droid.wifiAwareDestroyDiscoverySession(s_disc_id)
        p_dut.droid.wifiAwareDestroy(p_id)
        s_dut.droid.wifiAwareDestroy(s_id)

      # collect latency information
      latencies.append(
          discovery_event["data"][aconsts.SESSION_CB_KEY_TIMESTAMP_MS] -
          s_session_event["data"][aconsts.SESSION_CB_KEY_TIMESTAMP_MS])
      self.log.info("Latency #%d = %d" % (i, latencies[-1]))

    autils.extract_stats(
        s_dut,
        data=latencies,
        results=results[key],
        key_prefix="",
        log_prefix="Subscribe Session Sync/Discovery (%s, dw24=%d, dw5=%d)" %
        ("Unsolicited/Passive"
         if do_unsolicited_passive else "Solicited/Active", dw_24ghz, dw_5ghz))
    results[key]["num_failed_discovery"] = failed_discoveries

  def run_discovery_latency(self, results, do_unsolicited_passive, dw_24ghz,
                            dw_5ghz, num_iterations):
    """Run the service discovery latency test with the specified DW intervals.

    Args:
      results: Result array to be populated - will add results (not erase it)
      do_unsolicited_passive: True for unsolicited/passive, False for
                              solicited/active.
      dw_24ghz: DW interval in the 2.4GHz band.
      dw_5ghz: DW interval in the 5GHz band.
    """
    key = "%s_dw24_%d_dw5_%d" % (
        "unsolicited_passive"
        if do_unsolicited_passive else "solicited_active", dw_24ghz, dw_5ghz)
    results[key] = {}
    results[key]["num_iterations"] = num_iterations

    p_dut = self.android_devices[0]
    p_dut.pretty_name = "Publisher"
    s_dut = self.android_devices[1]
    s_dut.pretty_name = "Subscriber"

    # override the default DW configuration
    autils.config_power_settings(p_dut, dw_24ghz, dw_5ghz)
    autils.config_power_settings(s_dut, dw_24ghz, dw_5ghz)

    # Publisher+Subscriber: attach and wait for confirmation
    p_id = p_dut.droid.wifiAwareAttach(False)
    autils.wait_for_event(p_dut, aconsts.EVENT_CB_ON_ATTACHED)
    time.sleep(self.device_startup_offset)
    s_id = s_dut.droid.wifiAwareAttach(False)
    autils.wait_for_event(s_dut, aconsts.EVENT_CB_ON_ATTACHED)

    # start publish
    p_disc_event = self.start_discovery_session(
        p_dut, p_id, True, aconsts.PUBLISH_TYPE_UNSOLICITED
        if do_unsolicited_passive else aconsts.PUBLISH_TYPE_SOLICITED)

    # wait for for devices to synchronize with each other - used so that first
    # discovery isn't biased by synchronization.
    time.sleep(self.WAIT_FOR_CLUSTER)

    # loop, perform discovery, and collect latency information
    latencies = []
    failed_discoveries = 0
    for i in range(num_iterations):
      # start subscribe
      s_disc_id, s_session_event = self.start_discovery_session(
          s_dut, s_id, False, aconsts.SUBSCRIBE_TYPE_PASSIVE
          if do_unsolicited_passive else aconsts.SUBSCRIBE_TYPE_ACTIVE)

      # wait for discovery (allow for failures here since running lots of
      # samples and would like to get the partial data even in the presence of
      # errors)
      try:
        discovery_event = s_dut.ed.pop_event(
            aconsts.SESSION_CB_ON_SERVICE_DISCOVERED, autils.EVENT_TIMEOUT)
      except queue.Empty:
        s_dut.log.info("[Subscriber] Timed out while waiting for "
                       "SESSION_CB_ON_SERVICE_DISCOVERED")
        failed_discoveries = failed_discoveries + 1
        continue
      finally:
        # destroy subscribe
        s_dut.droid.wifiAwareDestroyDiscoverySession(s_disc_id)

      # collect latency information
      latencies.append(
          discovery_event["data"][aconsts.SESSION_CB_KEY_TIMESTAMP_MS] -
          s_session_event["data"][aconsts.SESSION_CB_KEY_TIMESTAMP_MS])
      self.log.info("Latency #%d = %d" % (i, latencies[-1]))

    autils.extract_stats(
        s_dut,
        data=latencies,
        results=results[key],
        key_prefix="",
        log_prefix="Subscribe Session Discovery (%s, dw24=%d, dw5=%d)" %
        ("Unsolicited/Passive"
         if do_unsolicited_passive else "Solicited/Active", dw_24ghz, dw_5ghz))
    results[key]["num_failed_discovery"] = failed_discoveries

    # clean up
    p_dut.droid.wifiAwareDestroyAll()
    s_dut.droid.wifiAwareDestroyAll()

  def run_message_latency(self, results, dw_24ghz, dw_5ghz, num_iterations):
    """Run the message tx latency test with the specified DW intervals.

    Args:
      results: Result array to be populated - will add results (not erase it)
      dw_24ghz: DW interval in the 2.4GHz band.
      dw_5ghz: DW interval in the 5GHz band.
    """
    key = "dw24_%d_dw5_%d" % (dw_24ghz, dw_5ghz)
    results[key] = {}
    results[key]["num_iterations"] = num_iterations

    p_dut = self.android_devices[0]
    s_dut = self.android_devices[1]

    # override the default DW configuration
    autils.config_power_settings(p_dut, dw_24ghz, dw_5ghz)
    autils.config_power_settings(s_dut, dw_24ghz, dw_5ghz)

    # Start up a discovery session
    (p_id, s_id, p_disc_id, s_disc_id,
     peer_id_on_sub) = autils.create_discovery_pair(
         p_dut,
         s_dut,
         p_config=autils.create_discovery_config(
             self.SERVICE_NAME, aconsts.PUBLISH_TYPE_UNSOLICITED),
         s_config=autils.create_discovery_config(
             self.SERVICE_NAME, aconsts.SUBSCRIBE_TYPE_PASSIVE),
         device_startup_offset=self.device_startup_offset)

    latencies = []
    failed_tx = 0
    messages_rx = 0
    missing_rx = 0
    corrupted_rx = 0
    for i in range(num_iterations):
      # send message
      msg_s2p = "Message Subscriber -> Publisher #%d" % i
      next_msg_id = self.get_next_msg_id()
      s_dut.droid.wifiAwareSendMessage(s_disc_id, peer_id_on_sub, next_msg_id,
                                       msg_s2p, 0)

      # wait for Tx confirmation
      try:
        sub_tx_msg_event = s_dut.ed.pop_event(
            aconsts.SESSION_CB_ON_MESSAGE_SENT, 2 * autils.EVENT_TIMEOUT)
        latencies.append(
            sub_tx_msg_event["data"][aconsts.SESSION_CB_KEY_LATENCY_MS])
      except queue.Empty:
        s_dut.log.info("[Subscriber] Timed out while waiting for "
                       "SESSION_CB_ON_MESSAGE_SENT")
        failed_tx = failed_tx + 1
        continue

      # wait for Rx confirmation (and validate contents)
      try:
        pub_rx_msg_event = p_dut.ed.pop_event(
            aconsts.SESSION_CB_ON_MESSAGE_RECEIVED, 2 * autils.EVENT_TIMEOUT)
        messages_rx = messages_rx + 1
        if (pub_rx_msg_event["data"][aconsts.SESSION_CB_KEY_MESSAGE_AS_STRING]
            != msg_s2p):
          corrupted_rx = corrupted_rx + 1
      except queue.Empty:
        s_dut.log.info("[Publisher] Timed out while waiting for "
                       "SESSION_CB_ON_MESSAGE_RECEIVED")
        missing_rx = missing_rx + 1
        continue

    autils.extract_stats(
        s_dut,
        data=latencies,
        results=results[key],
        key_prefix="",
        log_prefix="Subscribe Session Discovery (dw24=%d, dw5=%d)" %
                   (dw_24ghz, dw_5ghz))
    results[key]["failed_tx"] = failed_tx
    results[key]["messages_rx"] = messages_rx
    results[key]["missing_rx"] = missing_rx
    results[key]["corrupted_rx"] = corrupted_rx

    # clean up
    p_dut.droid.wifiAwareDestroyAll()
    s_dut.droid.wifiAwareDestroyAll()

  def run_ndp_oob_latency(self, results, dw_24ghz, dw_5ghz, num_iterations):
    """Runs the NDP setup with OOB (out-of-band) discovery latency test.

    Args:
      results: Result array to be populated - will add results (not erase it)
      dw_24ghz: DW interval in the 2.4GHz band.
      dw_5ghz: DW interval in the 5GHz band.
    """
    key_avail = "on_avail_dw24_%d_dw5_%d" % (dw_24ghz, dw_5ghz)
    key_link_props = "link_props_dw24_%d_dw5_%d" % (dw_24ghz, dw_5ghz)
    results[key_avail] = {}
    results[key_link_props] = {}
    results[key_avail]["num_iterations"] = num_iterations

    init_dut = self.android_devices[0]
    init_dut.pretty_name = 'Initiator'
    resp_dut = self.android_devices[1]
    resp_dut.pretty_name = 'Responder'

    # override the default DW configuration
    autils.config_power_settings(init_dut, dw_24ghz, dw_5ghz)
    autils.config_power_settings(resp_dut, dw_24ghz, dw_5ghz)

    # Initiator+Responder: attach and wait for confirmation & identity
    init_id = init_dut.droid.wifiAwareAttach(True)
    autils.wait_for_event(init_dut, aconsts.EVENT_CB_ON_ATTACHED)
    init_ident_event = autils.wait_for_event(init_dut,
                                      aconsts.EVENT_CB_ON_IDENTITY_CHANGED)
    init_mac = init_ident_event['data']['mac']
    time.sleep(self.device_startup_offset)
    resp_id = resp_dut.droid.wifiAwareAttach(True)
    autils.wait_for_event(resp_dut, aconsts.EVENT_CB_ON_ATTACHED)
    resp_ident_event = autils.wait_for_event(resp_dut,
                                      aconsts.EVENT_CB_ON_IDENTITY_CHANGED)
    resp_mac = resp_ident_event['data']['mac']

    # wait for for devices to synchronize with each other - there are no other
    # mechanisms to make sure this happens for OOB discovery (except retrying
    # to execute the data-path request)
    time.sleep(autils.WAIT_FOR_CLUSTER)

    on_available_latencies = []
    link_props_latencies = []
    ndp_setup_failures = 0
    for i in range(num_iterations):
      # Responder: request network
      resp_req_key = autils.request_network(
          resp_dut,
          resp_dut.droid.wifiAwareCreateNetworkSpecifierOob(
              resp_id, aconsts.DATA_PATH_RESPONDER, init_mac, None))

      # Initiator: request network
      init_req_key = autils.request_network(
          init_dut,
          init_dut.droid.wifiAwareCreateNetworkSpecifierOob(
              init_id, aconsts.DATA_PATH_INITIATOR, resp_mac, None))

      # Initiator & Responder: wait for network formation
      got_on_available = False
      got_on_link_props = False
      while not got_on_available or not got_on_link_props:
        try:
          nc_event = init_dut.ed.pop_event(cconsts.EVENT_NETWORK_CALLBACK,
                                           autils.EVENT_NDP_TIMEOUT)
          if nc_event["data"][
              cconsts.NETWORK_CB_KEY_EVENT] == cconsts.NETWORK_CB_AVAILABLE:
            got_on_available = True
            on_available_latencies.append(
                nc_event["data"][cconsts.NETWORK_CB_KEY_CURRENT_TS] -
                nc_event["data"][cconsts.NETWORK_CB_KEY_CREATE_TS])
          elif (nc_event["data"][cconsts.NETWORK_CB_KEY_EVENT] ==
                cconsts.NETWORK_CB_LINK_PROPERTIES_CHANGED):
            got_on_link_props = True
            link_props_latencies.append(
                nc_event["data"][cconsts.NETWORK_CB_KEY_CURRENT_TS] -
                nc_event["data"][cconsts.NETWORK_CB_KEY_CREATE_TS])
        except queue.Empty:
          ndp_setup_failures = ndp_setup_failures + 1
          init_dut.log.info("[Initiator] Timed out while waiting for "
                         "EVENT_NETWORK_CALLBACK")
          break

      # clean-up
      init_dut.droid.connectivityUnregisterNetworkCallback(init_req_key)
      resp_dut.droid.connectivityUnregisterNetworkCallback(resp_req_key)

      # wait to make sure previous NDP terminated, otherwise its termination
      # time will be counted in the setup latency!
      time.sleep(2)

    autils.extract_stats(
        init_dut,
        data=on_available_latencies,
        results=results[key_avail],
        key_prefix="",
        log_prefix="NDP setup OnAvailable(dw24=%d, dw5=%d)" % (dw_24ghz,
                                                               dw_5ghz))
    autils.extract_stats(
        init_dut,
        data=link_props_latencies,
        results=results[key_link_props],
        key_prefix="",
        log_prefix="NDP setup OnLinkProperties (dw24=%d, dw5=%d)" % (dw_24ghz,
                                                                     dw_5ghz))
    results[key_avail]["ndp_setup_failures"] = ndp_setup_failures

  def run_end_to_end_latency(self, results, dw_24ghz, dw_5ghz, num_iterations,
      startup_offset, include_setup):
    """Measure the latency for end-to-end communication link setup:
    - Start Aware
    - Discovery
    - Message from Sub -> Pub
    - Message from Pub -> Sub
    - NDP setup

    Args:
      results: Result array to be populated - will add results (not erase it)
      dw_24ghz: DW interval in the 2.4GHz band.
      dw_5ghz: DW interval in the 5GHz band.
      startup_offset: The start-up gap (in seconds) between the two devices
      include_setup: True to include the cluster setup in the latency
                    measurements.
    """
    key = "dw24_%d_dw5_%d" % (dw_24ghz, dw_5ghz)
    results[key] = {}
    results[key]["num_iterations"] = num_iterations

    p_dut = self.android_devices[0]
    p_dut.pretty_name = "Publisher"
    s_dut = self.android_devices[1]
    s_dut.pretty_name = "Subscriber"

    # override the default DW configuration
    autils.config_power_settings(p_dut, dw_24ghz, dw_5ghz)
    autils.config_power_settings(s_dut, dw_24ghz, dw_5ghz)

    latencies = []

    # allow for failures here since running lots of samples and would like to
    # get the partial data even in the presence of errors
    failures = 0

    if not include_setup:
      # Publisher+Subscriber: attach and wait for confirmation
      p_id = p_dut.droid.wifiAwareAttach(False)
      autils.wait_for_event(p_dut, aconsts.EVENT_CB_ON_ATTACHED)
      time.sleep(startup_offset)
      s_id = s_dut.droid.wifiAwareAttach(False)
      autils.wait_for_event(s_dut, aconsts.EVENT_CB_ON_ATTACHED)

    for i in range(num_iterations):
      while (True): # for pseudo-goto/finalize
        timestamp_start = time.perf_counter()

        if include_setup:
          # Publisher+Subscriber: attach and wait for confirmation
          p_id = p_dut.droid.wifiAwareAttach(False)
          autils.wait_for_event(p_dut, aconsts.EVENT_CB_ON_ATTACHED)
          time.sleep(startup_offset)
          s_id = s_dut.droid.wifiAwareAttach(False)
          autils.wait_for_event(s_dut, aconsts.EVENT_CB_ON_ATTACHED)

        # start publish
        p_disc_id, p_disc_event = self.start_discovery_session(
            p_dut, p_id, True, aconsts.PUBLISH_TYPE_UNSOLICITED)

        # start subscribe
        s_disc_id, s_session_event = self.start_discovery_session(
            s_dut, s_id, False, aconsts.SUBSCRIBE_TYPE_PASSIVE)

        # wait for discovery (allow for failures here since running lots of
        # samples and would like to get the partial data even in the presence of
        # errors)
        try:
          event = s_dut.ed.pop_event(aconsts.SESSION_CB_ON_SERVICE_DISCOVERED,
                                     autils.EVENT_TIMEOUT)
          s_dut.log.info("[Subscriber] SESSION_CB_ON_SERVICE_DISCOVERED: %s",
                         event["data"])
          peer_id_on_sub = event['data'][aconsts.SESSION_CB_KEY_PEER_ID]
        except queue.Empty:
          s_dut.log.info("[Subscriber] Timed out while waiting for "
                         "SESSION_CB_ON_SERVICE_DISCOVERED")
          failures = failures + 1
          break

        # message from Sub -> Pub
        msg_s2p = "Message Subscriber -> Publisher #%d" % i
        next_msg_id = self.get_next_msg_id()
        s_dut.droid.wifiAwareSendMessage(s_disc_id, peer_id_on_sub, next_msg_id,
                                         msg_s2p, 0)

        # wait for Tx confirmation
        try:
          s_dut.ed.pop_event(aconsts.SESSION_CB_ON_MESSAGE_SENT,
                             autils.EVENT_TIMEOUT)
        except queue.Empty:
          s_dut.log.info("[Subscriber] Timed out while waiting for "
                         "SESSION_CB_ON_MESSAGE_SENT")
          failures = failures + 1
          break

        # wait for Rx confirmation (and validate contents)
        try:
          event = p_dut.ed.pop_event(aconsts.SESSION_CB_ON_MESSAGE_RECEIVED,
                                     autils.EVENT_TIMEOUT)
          peer_id_on_pub = event['data'][aconsts.SESSION_CB_KEY_PEER_ID]
          if (event["data"][
            aconsts.SESSION_CB_KEY_MESSAGE_AS_STRING] != msg_s2p):
            p_dut.log.info("[Publisher] Corrupted input message - %s", event)
            failures = failures + 1
            break
        except queue.Empty:
          p_dut.log.info("[Publisher] Timed out while waiting for "
                         "SESSION_CB_ON_MESSAGE_RECEIVED")
          failures = failures + 1
          break

        # message from Pub -> Sub
        msg_p2s = "Message Publisher -> Subscriber #%d" % i
        next_msg_id = self.get_next_msg_id()
        p_dut.droid.wifiAwareSendMessage(p_disc_id, peer_id_on_pub, next_msg_id,
                                         msg_p2s, 0)

        # wait for Tx confirmation
        try:
          p_dut.ed.pop_event(aconsts.SESSION_CB_ON_MESSAGE_SENT,
                             autils.EVENT_TIMEOUT)
        except queue.Empty:
          p_dut.log.info("[Publisher] Timed out while waiting for "
                         "SESSION_CB_ON_MESSAGE_SENT")
          failures = failures + 1
          break

        # wait for Rx confirmation (and validate contents)
        try:
          event = s_dut.ed.pop_event(aconsts.SESSION_CB_ON_MESSAGE_RECEIVED,
                                     autils.EVENT_TIMEOUT)
          if (event["data"][
            aconsts.SESSION_CB_KEY_MESSAGE_AS_STRING] != msg_p2s):
            s_dut.log.info("[Subscriber] Corrupted input message - %s", event)
            failures = failures + 1
            break
        except queue.Empty:
          s_dut.log.info("[Subscriber] Timed out while waiting for "
                         "SESSION_CB_ON_MESSAGE_RECEIVED")
          failures = failures + 1
          break

        # create NDP

        # Publisher: request network
        p_req_key = autils.request_network(
            p_dut,
            p_dut.droid.wifiAwareCreateNetworkSpecifier(p_disc_id,
                                                        peer_id_on_pub, None))

        # Subscriber: request network
        s_req_key = autils.request_network(
            s_dut,
            s_dut.droid.wifiAwareCreateNetworkSpecifier(s_disc_id,
                                                        peer_id_on_sub, None))

        # Publisher & Subscriber: wait for network formation
        try:
          p_net_event = autils.wait_for_event_with_keys(
              p_dut, cconsts.EVENT_NETWORK_CALLBACK, autils.EVENT_TIMEOUT, (
              cconsts.NETWORK_CB_KEY_EVENT,
              cconsts.NETWORK_CB_LINK_PROPERTIES_CHANGED),
              (cconsts.NETWORK_CB_KEY_ID, p_req_key))
          s_net_event = autils.wait_for_event_with_keys(
              s_dut, cconsts.EVENT_NETWORK_CALLBACK, autils.EVENT_TIMEOUT, (
              cconsts.NETWORK_CB_KEY_EVENT,
              cconsts.NETWORK_CB_LINK_PROPERTIES_CHANGED),
              (cconsts.NETWORK_CB_KEY_ID, s_req_key))
        except:
          failures = failures + 1
          break

        p_aware_if = p_net_event["data"][cconsts.NETWORK_CB_KEY_INTERFACE_NAME]
        s_aware_if = s_net_event["data"][cconsts.NETWORK_CB_KEY_INTERFACE_NAME]

        p_ipv6 = \
        p_dut.droid.connectivityGetLinkLocalIpv6Address(p_aware_if).split("%")[
          0]
        s_ipv6 = \
        s_dut.droid.connectivityGetLinkLocalIpv6Address(s_aware_if).split("%")[
          0]

        p_dut.log.info("[Publisher] IF=%s, IPv6=%s", p_aware_if, p_ipv6)
        s_dut.log.info("[Subscriber] IF=%s, IPv6=%s", s_aware_if, s_ipv6)

        latencies.append(time.perf_counter() - timestamp_start)
        break

      # destroy sessions
      p_dut.droid.wifiAwareDestroyDiscoverySession(p_disc_id)
      s_dut.droid.wifiAwareDestroyDiscoverySession(s_disc_id)
      if include_setup:
        p_dut.droid.wifiAwareDestroy(p_id)
        s_dut.droid.wifiAwareDestroy(s_id)

    autils.extract_stats(
        p_dut,
        data=latencies,
        results=results[key],
        key_prefix="",
        log_prefix="End-to-End(dw24=%d, dw5=%d)" % (dw_24ghz, dw_5ghz))
    results[key]["failures"] = failures


  ########################################################################

  def test_synchronization_default_dws(self):
    """Measure the device synchronization for default dws. Loop over values
    from 0 to 4 seconds."""
    results = {}
    for startup_offset in range(5):
      self.run_synchronization_latency(
          results=results,
          do_unsolicited_passive=True,
          dw_24ghz=aconsts.POWER_DW_24_INTERACTIVE,
          dw_5ghz=aconsts.POWER_DW_5_INTERACTIVE,
          num_iterations=10,
          startup_offset=startup_offset,
          timeout_period=20)
    asserts.explicit_pass(
        "test_synchronization_default_dws finished", extras=results)

  def test_synchronization_non_interactive_dws(self):
    """Measure the device synchronization for non-interactive dws. Loop over
    values from 0 to 4 seconds."""
    results = {}
    for startup_offset in range(5):
      self.run_synchronization_latency(
          results=results,
          do_unsolicited_passive=True,
          dw_24ghz=aconsts.POWER_DW_24_NON_INTERACTIVE,
          dw_5ghz=aconsts.POWER_DW_5_NON_INTERACTIVE,
          num_iterations=10,
          startup_offset=startup_offset,
          timeout_period=20)
    asserts.explicit_pass(
        "test_synchronization_non_interactive_dws finished", extras=results)

  def test_discovery_latency_default_dws(self):
    """Measure the service discovery latency with the default DW configuration.
    """
    results = {}
    self.run_discovery_latency(
        results=results,
        do_unsolicited_passive=True,
        dw_24ghz=aconsts.POWER_DW_24_INTERACTIVE,
        dw_5ghz=aconsts.POWER_DW_5_INTERACTIVE,
        num_iterations=100)
    asserts.explicit_pass(
        "test_discovery_latency_default_parameters finished", extras=results)

  def test_discovery_latency_non_interactive_dws(self):
    """Measure the service discovery latency with the DW configuration for non
    -interactive mode (lower power)."""
    results = {}
    self.run_discovery_latency(
        results=results,
        do_unsolicited_passive=True,
        dw_24ghz=aconsts.POWER_DW_24_NON_INTERACTIVE,
        dw_5ghz=aconsts.POWER_DW_5_NON_INTERACTIVE,
        num_iterations=100)
    asserts.explicit_pass(
        "test_discovery_latency_non_interactive_dws finished", extras=results)

  def test_discovery_latency_all_dws(self):
    """Measure the service discovery latency with all DW combinations (low
    iteration count)"""
    results = {}
    for dw24 in range(1, 6):  # permitted values: 1-5
      for dw5 in range(0, 6): # permitted values: 0, 1-5
        self.run_discovery_latency(
            results=results,
            do_unsolicited_passive=True,
            dw_24ghz=dw24,
            dw_5ghz=dw5,
            num_iterations=10)
    asserts.explicit_pass(
        "test_discovery_latency_all_dws finished", extras=results)

  def test_message_latency_default_dws(self):
    """Measure the send message latency with the default DW configuration. Test
    performed on non-queued message transmission - i.e. waiting for confirmation
    of reception (ACK) before sending the next message."""
    results = {}
    self.run_message_latency(
        results=results,
        dw_24ghz=aconsts.POWER_DW_24_INTERACTIVE,
        dw_5ghz=aconsts.POWER_DW_5_INTERACTIVE,
        num_iterations=100)
    asserts.explicit_pass(
        "test_message_latency_default_dws finished", extras=results)

  def test_message_latency_non_interactive_dws(self):
    """Measure the send message latency with the DW configuration for
    non-interactive mode. Test performed on non-queued message transmission -
    i.e. waiting for confirmation of reception (ACK) before sending the next
    message."""
    results = {}
    self.run_message_latency(
        results=results,
        dw_24ghz=aconsts.POWER_DW_24_NON_INTERACTIVE,
        dw_5ghz=aconsts.POWER_DW_5_NON_INTERACTIVE,
        num_iterations=100)
    asserts.explicit_pass(
        "test_message_latency_non_interactive_dws finished", extras=results)

  def test_oob_ndp_setup_latency_default_dws(self):
    """Measure the NDP setup latency with the default DW configuration. The
    NDP is setup with OOB (out-of-band) configuration."""
    results = {}
    self.run_ndp_oob_latency(
        results=results,
        dw_24ghz=aconsts.POWER_DW_24_INTERACTIVE,
        dw_5ghz=aconsts.POWER_DW_5_INTERACTIVE,
        num_iterations=100)
    asserts.explicit_pass(
        "test_ndp_setup_latency_default_dws finished", extras=results)

  def test_oob_ndp_setup_latency_non_interactive_dws(self):
    """Measure the NDP setup latency with the DW configuration for
    non-interactive mode. The NDP is setup with OOB (out-of-band)
    configuration"""
    results = {}
    self.run_ndp_oob_latency(
        results=results,
        dw_24ghz=aconsts.POWER_DW_24_NON_INTERACTIVE,
        dw_5ghz=aconsts.POWER_DW_5_NON_INTERACTIVE,
        num_iterations=100)
    asserts.explicit_pass(
        "test_ndp_setup_latency_non_interactive_dws finished", extras=results)

  def test_end_to_end_latency_default_dws(self):
    """Measure the latency for end-to-end communication link setup:
      - Start Aware
      - Discovery
      - Message from Sub -> Pub
      - Message from Pub -> Sub
      - NDP setup
    """
    results = {}
    self.run_end_to_end_latency(
        results,
        dw_24ghz=aconsts.POWER_DW_24_INTERACTIVE,
        dw_5ghz=aconsts.POWER_DW_5_INTERACTIVE,
        num_iterations=10,
        startup_offset=0,
        include_setup=True)
    asserts.explicit_pass(
        "test_end_to_end_latency_default_dws finished", extras=results)

  def test_end_to_end_latency_post_attach_default_dws(self):
    """Measure the latency for end-to-end communication link setup without
    the initial synchronization:
      - Start Aware & synchronize initially
      - Loop:
        - Discovery
        - Message from Sub -> Pub
        - Message from Pub -> Sub
        - NDP setup
    """
    results = {}
    self.run_end_to_end_latency(
        results,
        dw_24ghz=aconsts.POWER_DW_24_INTERACTIVE,
        dw_5ghz=aconsts.POWER_DW_5_INTERACTIVE,
        num_iterations=10,
        startup_offset=0,
        include_setup=False)
    asserts.explicit_pass(
      "test_end_to_end_latency_post_attach_default_dws finished",
      extras=results)
