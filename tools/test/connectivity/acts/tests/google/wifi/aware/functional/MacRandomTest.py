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
from acts.test_decorators import test_tracker_info
from acts.test_utils.net import connectivity_const as cconsts
from acts.test_utils.wifi.aware import aware_const as aconsts
from acts.test_utils.wifi.aware import aware_test_utils as autils
from acts.test_utils.wifi.aware.AwareBaseTest import AwareBaseTest


class MacRandomTest(AwareBaseTest):
  """Set of tests for Wi-Fi Aware MAC address randomization of NMI (NAN
  management interface) and NDI (NAN data interface)."""

  NUM_ITERATIONS = 10

  # number of second to 'reasonably' wait to make sure that devices synchronize
  # with each other - useful for OOB test cases, where the OOB discovery would
  # take some time
  WAIT_FOR_CLUSTER = 5

  def __init__(self, controllers):
    AwareBaseTest.__init__(self, controllers)

  def request_network(self, dut, ns):
    """Request a Wi-Fi Aware network.

    Args:
      dut: Device
      ns: Network specifier
    Returns: the request key
    """
    network_req = {"TransportType": 5, "NetworkSpecifier": ns}
    return dut.droid.connectivityRequestWifiAwareNetwork(network_req)

  ##########################################################################

  @test_tracker_info(uuid="09964368-146a-48e4-9f33-6a319f9eeadc")
  def test_nmi_ndi_randomization_on_enable(self):
    """Validate randomization of the NMI (NAN management interface) and all NDIs
    (NAN data-interface) on each enable/disable cycle"""
    dut = self.android_devices[0]

    # re-enable randomization interval (since if disabled it may also disable
    # the 'randomize on enable' feature).
    autils.configure_mac_random_interval(dut, 1800)

    # DUT: attach and wait for confirmation & identity 10 times
    mac_addresses = {}
    for i in range(self.NUM_ITERATIONS):
      id = dut.droid.wifiAwareAttach(True)
      autils.wait_for_event(dut, aconsts.EVENT_CB_ON_ATTACHED)
      ident_event = autils.wait_for_event(dut,
                                          aconsts.EVENT_CB_ON_IDENTITY_CHANGED)

      # process NMI
      mac = ident_event["data"]["mac"]
      dut.log.info("NMI=%s", mac)
      if mac in mac_addresses:
        mac_addresses[mac] = mac_addresses[mac] + 1
      else:
        mac_addresses[mac] = 1

      # process NDIs
      time.sleep(5) # wait for NDI creation to complete
      for j in range(dut.aware_capabilities[aconsts.CAP_MAX_NDI_INTERFACES]):
        ndi_interface = "%s%d" % (aconsts.AWARE_NDI_PREFIX, j)
        ndi_mac = autils.get_mac_addr(dut, ndi_interface)
        dut.log.info("NDI %s=%s", ndi_interface, ndi_mac)
        if ndi_mac in mac_addresses:
          mac_addresses[ndi_mac] = mac_addresses[ndi_mac] + 1
        else:
          mac_addresses[ndi_mac] = 1

      dut.droid.wifiAwareDestroy(id)

    # Test for uniqueness
    for mac in mac_addresses.keys():
      if mac_addresses[mac] != 1:
        asserts.fail("MAC address %s repeated %d times (all=%s)" % (mac,
                     mac_addresses[mac], mac_addresses))

    # Verify that infra interface (e.g. wlan0) MAC address is not used for NMI
    infra_mac = autils.get_wifi_mac_address(dut)
    asserts.assert_false(
        infra_mac in mac_addresses,
        "Infrastructure MAC address (%s) is used for Aware NMI (all=%s)" %
        (infra_mac, mac_addresses))

  @test_tracker_info(uuid="0fb0b5d8-d9cb-4e37-b9af-51811be5670d")
  def test_nmi_randomization_on_interval(self):
    """Validate randomization of the NMI (NAN management interface) on a set
    interval. Default value is 30 minutes - change to a small value to allow
    testing in real-time"""
    RANDOM_INTERVAL = 120 # minimal value in current implementation

    dut = self.android_devices[0]

    # set randomization interval to 120 seconds
    autils.configure_mac_random_interval(dut, RANDOM_INTERVAL)

    # attach and wait for first identity
    id = dut.droid.wifiAwareAttach(True)
    autils.wait_for_event(dut, aconsts.EVENT_CB_ON_ATTACHED)
    ident_event = autils.wait_for_event(dut,
                                        aconsts.EVENT_CB_ON_IDENTITY_CHANGED)
    mac1 = ident_event["data"]["mac"]

    # wait for second identity callback
    # Note: exact randomization interval is not critical, just approximate,
    # hence giving a few more seconds.
    ident_event = autils.wait_for_event(dut,
                                        aconsts.EVENT_CB_ON_IDENTITY_CHANGED,
                                        timeout=RANDOM_INTERVAL + 5)
    mac2 = ident_event["data"]["mac"]

    # validate MAC address is randomized
    asserts.assert_false(
        mac1 == mac2,
        "Randomized MAC addresses (%s, %s) should be different" % (mac1, mac2))

    # clean-up
    dut.droid.wifiAwareDestroy(id)
