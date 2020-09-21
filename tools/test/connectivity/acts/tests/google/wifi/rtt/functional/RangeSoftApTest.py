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

from acts import asserts
from acts.test_decorators import test_tracker_info
from acts.test_utils.tel.tel_test_utils import WIFI_CONFIG_APBAND_5G
from acts.test_utils.wifi import wifi_test_utils as wutils
from acts.test_utils.wifi.rtt import rtt_const as rconsts
from acts.test_utils.wifi.rtt import rtt_test_utils as rutils
from acts.test_utils.wifi.rtt.RttBaseTest import RttBaseTest


class RangeSoftApTest(RttBaseTest):
  """Test class for RTT ranging to an Android Soft AP."""

  # Soft AP SSID
  SOFT_AP_SSID = "RTT_TEST_SSID"

  # Soft AP Password (irrelevant)
  SOFT_AP_PASSWORD = "ABCDEFGH"

  # Number of RTT iterations
  NUM_ITER = 10

  def __init__(self, controllers):
    RttBaseTest.__init__(self, controllers)

  #########################################################################

  @test_tracker_info(uuid="578f0725-31e3-4e60-ad62-0212d93cf5b8")
  def test_rtt_to_soft_ap(self):
    """Set up a Soft AP on one device and try performing an RTT ranging to it
    from another device. The attempt must fail - RTT on Soft AP must be
    disabled."""
    sap = self.android_devices[0]
    sap.pretty_name = "SoftAP"
    client = self.android_devices[1]
    client.pretty_name = "Client"

    # start Soft AP
    wutils.start_wifi_tethering(sap, self.SOFT_AP_SSID, self.SOFT_AP_PASSWORD,
                                band=WIFI_CONFIG_APBAND_5G, hidden=False)

    try:
      # start scanning on the client
      wutils.start_wifi_connection_scan_and_ensure_network_found(client,
                                                             self.SOFT_AP_SSID)
      scans = client.droid.wifiGetScanResults()
      scanned_softap = None
      for scanned_ap in scans:
        if scanned_ap[wutils.WifiEnums.SSID_KEY] == self.SOFT_AP_SSID:
          scanned_softap = scanned_ap
          break

      asserts.assert_false(scanned_softap == None, "Soft AP not found in scan!",
                           extras=scans)

      # validate that Soft AP does not advertise 802.11mc support
      asserts.assert_false(
        rconsts.SCAN_RESULT_KEY_RTT_RESPONDER in scanned_softap and
        scanned_softap[rconsts.SCAN_RESULT_KEY_RTT_RESPONDER],
        "Soft AP advertises itself as supporting 802.11mc!",
        extras=scanned_softap)

      # falsify the SoftAP's support for IEEE 802.11 so we try a 2-sided RTT
      scanned_softap[rconsts.SCAN_RESULT_KEY_RTT_RESPONDER] = True # falsify

      # actually try ranging to the Soft AP
      events = rutils.run_ranging(client, [scanned_softap], self.NUM_ITER, 0)
      stats = rutils.analyze_results(events, self.rtt_reference_distance_mm,
                                     self.rtt_reference_distance_margin_mm,
                                     self.rtt_min_expected_rssi_dbm,
                                     self.lci_reference, self.lcr_reference)

      asserts.assert_equal(
          stats[scanned_ap[wutils.WifiEnums.BSSID_KEY]]['num_failures'],
          self.NUM_ITER, "Some RTT operations to Soft AP succeed!?",
          extras=stats)

      asserts.explicit_pass("SoftAP + RTT validation done", extras=events)
    finally:
      wutils.stop_wifi_tethering(sap)
