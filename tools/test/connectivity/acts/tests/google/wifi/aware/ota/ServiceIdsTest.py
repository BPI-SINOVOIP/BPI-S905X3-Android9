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

import time

from acts import asserts
from acts.test_utils.wifi.aware import aware_const as aconsts
from acts.test_utils.wifi.aware import aware_test_utils as autils
from acts.test_utils.wifi.aware.AwareBaseTest import AwareBaseTest


class ServiceIdsTest(AwareBaseTest):
  """Set of tests for Wi-Fi Aware to verify that beacons include service IDs
  for discovery.

  Note: this test is an OTA (over-the-air) and requires a Sniffer.
  """

  def __init__(self, controllers):
    AwareBaseTest.__init__(self, controllers)

  def start_discovery_session(self, dut, session_id, is_publish, dtype,
                              service_name):
    """Start a discovery session

    Args:
      dut: Device under test
      session_id: ID of the Aware session in which to start discovery
      is_publish: True for a publish session, False for subscribe session
      dtype: Type of the discovery session
      service_name: Service name to use for the discovery session

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

    autils.wait_for_event(dut, event_name)
    return disc_id

  ####################################################################

  def test_service_ids_in_beacon(self):
    """Verify that beacons include service IDs for both publish and subscribe
    sessions of all types: solicited/unsolicited/active/passive."""
    dut = self.android_devices[0]

    self.log.info("Reminder: start a sniffer before running test")

    # attach
    session_id = dut.droid.wifiAwareAttach(True)
    autils.wait_for_event(dut, aconsts.EVENT_CB_ON_ATTACHED)
    ident_event = autils.wait_for_event(dut,
                                        aconsts.EVENT_CB_ON_IDENTITY_CHANGED)
    mac = ident_event["data"]["mac"]
    self.log.info("Source MAC Address of 'interesting' packets = %s", mac)
    self.log.info("Wireshark filter = 'wlan.ta == %s:%s:%s:%s:%s:%s'", mac[0:2],
                  mac[2:4], mac[4:6], mac[6:8], mac[8:10], mac[10:12])

    time.sleep(5)  # get some samples pre-discovery

    # start 4 discovery session (one of each type)
    self.start_discovery_session(dut, session_id, True,
                                 aconsts.PUBLISH_TYPE_UNSOLICITED,
                                 "GoogleTestService-Pub-Unsolicited")
    self.start_discovery_session(dut, session_id, True,
                                 aconsts.PUBLISH_TYPE_SOLICITED,
                                 "GoogleTestService-Pub-Solicited")
    self.start_discovery_session(dut, session_id, False,
                                 aconsts.SUBSCRIBE_TYPE_ACTIVE,
                                 "GoogleTestService-Sub-Active")
    self.start_discovery_session(dut, session_id, False,
                                 aconsts.SUBSCRIBE_TYPE_PASSIVE,
                                 "GoogleTestService-Sub-Passive")

    time.sleep(15)  # get some samples while discovery is alive

    self.log.info("Reminder: stop sniffer")
