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
from acts.test_utils.net import connectivity_const as cconsts
from acts.test_utils.wifi.aware import aware_const as aconsts
from acts.test_utils.wifi.aware import aware_test_utils as autils
from acts.test_utils.wifi.aware.AwareBaseTest import AwareBaseTest


class CapabilitiesTest(AwareBaseTest):
  """Set of tests for Wi-Fi Aware Capabilities - verifying that the provided
  capabilities are real (i.e. available)."""

  SERVICE_NAME = "GoogleTestXYZ"

  def __init__(self, controllers):
    AwareBaseTest.__init__(self, controllers)

  def create_config(self, dtype, service_name):
    """Create a discovery configuration based on input parameters.

    Args:
      dtype: Publish or Subscribe discovery type
      service_name: Service name.

    Returns:
      Discovery configuration object.
    """
    config = {}
    config[aconsts.DISCOVERY_KEY_DISCOVERY_TYPE] = dtype
    config[aconsts.DISCOVERY_KEY_SERVICE_NAME] = service_name
    return config

  def start_discovery_session(self, dut, session_id, is_publish, dtype,
                              service_name, expect_success):
    """Start a discovery session

    Args:
      dut: Device under test
      session_id: ID of the Aware session in which to start discovery
      is_publish: True for a publish session, False for subscribe session
      dtype: Type of the discovery session
      service_name: Service name to use for the discovery session
      expect_success: True if expect session to be created, False otherwise

    Returns:
      Discovery session ID.
    """
    config = {}
    config[aconsts.DISCOVERY_KEY_DISCOVERY_TYPE] = dtype
    config[aconsts.DISCOVERY_KEY_SERVICE_NAME] = service_name

    if is_publish:
      disc_id = dut.droid.wifiAwarePublish(session_id, config)
      event_name = aconsts.SESSION_CB_ON_PUBLISH_STARTED
    else:
      disc_id = dut.droid.wifiAwareSubscribe(session_id, config)
      event_name = aconsts.SESSION_CB_ON_SUBSCRIBE_STARTED

    if expect_success:
      autils.wait_for_event(dut, event_name)
    else:
      autils.wait_for_event(dut, aconsts.SESSION_CB_ON_SESSION_CONFIG_FAILED)

    return disc_id

  ###############################

  @test_tracker_info(uuid="45da8a41-6c02-4434-9eb9-aa0a36ff9f65")
  def test_max_discovery_sessions(self):
    """Validate that the device can create as many discovery sessions as are
    indicated in the device capabilities
    """
    dut = self.android_devices[0]

    # attach
    session_id = dut.droid.wifiAwareAttach(True)
    autils.wait_for_event(dut, aconsts.EVENT_CB_ON_ATTACHED)

    service_name_template = 'GoogleTestService-%s-%d'

    # start the max number of publish sessions
    for i in range(dut.aware_capabilities[aconsts.CAP_MAX_PUBLISHES]):
      # create publish discovery session of both types
      pub_disc_id = self.start_discovery_session(
          dut, session_id, True, aconsts.PUBLISH_TYPE_UNSOLICITED
          if i % 2 == 0 else aconsts.PUBLISH_TYPE_SOLICITED,
          service_name_template % ('pub', i), True)

    # start the max number of subscribe sessions
    for i in range(dut.aware_capabilities[aconsts.CAP_MAX_SUBSCRIBES]):
      # create publish discovery session of both types
      sub_disc_id = self.start_discovery_session(
          dut, session_id, False, aconsts.SUBSCRIBE_TYPE_PASSIVE
          if i % 2 == 0 else aconsts.SUBSCRIBE_TYPE_ACTIVE,
          service_name_template % ('sub', i), True)

    # start another publish & subscribe and expect failure
    self.start_discovery_session(dut, session_id, True,
                                 aconsts.PUBLISH_TYPE_UNSOLICITED,
                                 service_name_template % ('pub', 900), False)
    self.start_discovery_session(dut, session_id, False,
                                 aconsts.SUBSCRIBE_TYPE_ACTIVE,
                                 service_name_template % ('pub', 901), False)

    # delete one of the publishes and try again (see if can create subscribe
    # instead - should not)
    dut.droid.wifiAwareDestroyDiscoverySession(pub_disc_id)
    self.start_discovery_session(dut, session_id, False,
                                 aconsts.SUBSCRIBE_TYPE_ACTIVE,
                                 service_name_template % ('pub', 902), False)
    self.start_discovery_session(dut, session_id, True,
                                 aconsts.PUBLISH_TYPE_UNSOLICITED,
                                 service_name_template % ('pub', 903), True)

    # delete one of the subscribes and try again (see if can create publish
    # instead - should not)
    dut.droid.wifiAwareDestroyDiscoverySession(sub_disc_id)
    self.start_discovery_session(dut, session_id, True,
                                 aconsts.PUBLISH_TYPE_UNSOLICITED,
                                 service_name_template % ('pub', 904), False)
    self.start_discovery_session(dut, session_id, False,
                                 aconsts.SUBSCRIBE_TYPE_ACTIVE,
                                 service_name_template % ('pub', 905), True)

  def test_max_ndp(self):
    """Validate that the device can create as many NDPs as are specified
    by its capabilities.

    Mechanics:
    - Publisher on DUT (first device)
    - Subscribers on all other devices
    - On discovery set up NDP

    Note: the test requires MAX_NDP + 2 devices to be validated. If these are
    not available the test will fail.
    """
    dut = self.android_devices[0]

    # get max NDP: using first available device (assumes all devices are the
    # same)
    max_ndp = dut.aware_capabilities[aconsts.CAP_MAX_NDP_SESSIONS]

    # get number of attached devices: needs to be max_ndp+2 to allow for max_ndp
    # NDPs + an additional one expected to fail.
    # However, will run the test with max_ndp+1 devices to verify that at least
    # that many NDPs can be created. Will still fail at the end to indicate that
    # full test was not run.
    num_peer_devices = min(len(self.android_devices) - 1, max_ndp + 1)
    asserts.assert_true(
        num_peer_devices >= max_ndp,
        'A minimum of %d devices is needed to run the test, have %d' %
        (max_ndp + 1, len(self.android_devices)))

    # attach
    session_id = dut.droid.wifiAwareAttach()
    autils.wait_for_event(dut, aconsts.EVENT_CB_ON_ATTACHED)

    # start publisher
    p_disc_id = self.start_discovery_session(
        dut,
        session_id,
        is_publish=True,
        dtype=aconsts.PUBLISH_TYPE_UNSOLICITED,
        service_name=self.SERVICE_NAME,
        expect_success=True)

    # loop over other DUTs
    for i in range(num_peer_devices):
      other_dut = self.android_devices[i + 1]

      # attach
      other_session_id = other_dut.droid.wifiAwareAttach()
      autils.wait_for_event(other_dut, aconsts.EVENT_CB_ON_ATTACHED)

      # start subscriber
      s_disc_id = self.start_discovery_session(
          other_dut,
          other_session_id,
          is_publish=False,
          dtype=aconsts.SUBSCRIBE_TYPE_PASSIVE,
          service_name=self.SERVICE_NAME,
          expect_success=True)

      discovery_event = autils.wait_for_event(
          other_dut, aconsts.SESSION_CB_ON_SERVICE_DISCOVERED)
      peer_id_on_sub = discovery_event['data'][aconsts.SESSION_CB_KEY_PEER_ID]

      # Subscriber: send message to peer (Publisher - so it knows our address)
      other_dut.droid.wifiAwareSendMessage(
          s_disc_id, peer_id_on_sub,
          self.get_next_msg_id(), "ping", aconsts.MAX_TX_RETRIES)
      autils.wait_for_event(other_dut, aconsts.SESSION_CB_ON_MESSAGE_SENT)

      # Publisher: wait for received message
      pub_rx_msg_event = autils.wait_for_event(
          dut, aconsts.SESSION_CB_ON_MESSAGE_RECEIVED)
      peer_id_on_pub = pub_rx_msg_event['data'][aconsts.SESSION_CB_KEY_PEER_ID]

      # publisher (responder): request network
      p_req_key = autils.request_network(
          dut,
          dut.droid.wifiAwareCreateNetworkSpecifier(p_disc_id, peer_id_on_pub))

      # subscriber (initiator): request network
      s_req_key = autils.request_network(
          other_dut,
          other_dut.droid.wifiAwareCreateNetworkSpecifier(
              s_disc_id, peer_id_on_sub))

      # wait for network (or not - on the last iteration)
      if i != max_ndp:
        p_net_event = autils.wait_for_event_with_keys(
            dut, cconsts.EVENT_NETWORK_CALLBACK, autils.EVENT_NDP_TIMEOUT,
            (cconsts.NETWORK_CB_KEY_EVENT,
             cconsts.NETWORK_CB_LINK_PROPERTIES_CHANGED),
            (cconsts.NETWORK_CB_KEY_ID, p_req_key))
        s_net_event = autils.wait_for_event_with_keys(
            other_dut, cconsts.EVENT_NETWORK_CALLBACK, autils.EVENT_NDP_TIMEOUT,
            (cconsts.NETWORK_CB_KEY_EVENT,
             cconsts.NETWORK_CB_LINK_PROPERTIES_CHANGED),
            (cconsts.NETWORK_CB_KEY_ID, s_req_key))

        p_aware_if = p_net_event['data'][cconsts.NETWORK_CB_KEY_INTERFACE_NAME]
        s_aware_if = s_net_event['data'][cconsts.NETWORK_CB_KEY_INTERFACE_NAME]
        self.log.info('Interface names: p=%s, s=%s', p_aware_if, s_aware_if)

        p_ipv6 = dut.droid.connectivityGetLinkLocalIpv6Address(
            p_aware_if).split('%')[0]
        s_ipv6 = other_dut.droid.connectivityGetLinkLocalIpv6Address(
            s_aware_if).split('%')[0]
        self.log.info('Interface addresses (IPv6): p=%s, s=%s', p_ipv6, s_ipv6)
      else:
        autils.fail_on_event_with_keys(
            dut, cconsts.EVENT_NETWORK_CALLBACK, autils.EVENT_NDP_TIMEOUT,
            (cconsts.NETWORK_CB_KEY_EVENT,
             cconsts.NETWORK_CB_LINK_PROPERTIES_CHANGED),
            (cconsts.NETWORK_CB_KEY_ID, p_req_key))
        autils.fail_on_event_with_keys(
            other_dut, cconsts.EVENT_NETWORK_CALLBACK, autils.EVENT_NDP_TIMEOUT,
            (cconsts.NETWORK_CB_KEY_EVENT,
             cconsts.NETWORK_CB_LINK_PROPERTIES_CHANGED),
            (cconsts.NETWORK_CB_KEY_ID, s_req_key))

    asserts.assert_true(num_peer_devices > max_ndp,
                        'Needed %d devices to run the test, have %d' %
                        (max_ndp + 2, len(self.android_devices)))
