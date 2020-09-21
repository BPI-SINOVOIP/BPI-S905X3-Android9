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
import threading

from acts import asserts
from acts.test_utils.wifi import wifi_constants as wconsts
from acts.test_utils.wifi.aware import aware_const as aconsts
from acts.test_utils.wifi.aware import aware_test_utils as autils
from acts.test_utils.wifi.aware.AwareBaseTest import AwareBaseTest


class InfraAssociationStressTest(AwareBaseTest):

  def __init__(self, controllers):
    AwareBaseTest.__init__(self, controllers)

  # Length of test in seconds
  TEST_DURATION_SECONDS = 300

  # Service name
  SERVICE_NAME = "GoogleTestServiceXYXYXY"

  def is_associated(self, dut):
    """Checks whether the device is associated (to any AP).

    Args:
      dut: Device under test.

    Returns: True if associated (to any AP), False otherwise.
    """
    info = dut.droid.wifiGetConnectionInfo()
    return info is not None and info["supplicant_state"] != "disconnected"

  def wait_for_disassociation(self, q, dut):
    """Waits for a disassociation event on the specified DUT for the given
    timeout. Place a result into the queue (False) only if disassociation
    observed.

    Args:
      q: The synchronization queue into which to place the results.
      dut: The device to track.
    """
    try:
      dut.ed.pop_event(wconsts.WIFI_DISCONNECTED, self.TEST_DURATION_SECONDS)
      q.put(True)
    except queue.Empty:
      pass

  def run_infra_assoc_oob_ndp_stress(self, with_ndp_traffic):
    """Validates that Wi-Fi Aware NDP does not interfere with infrastructure
    (AP) association.

    Test assumes (and verifies) that device is already associated to an AP.

    Args:
      with_ndp_traffic: True to run traffic over the NDP.
    """
    init_dut = self.android_devices[0]
    resp_dut = self.android_devices[1]

    # check that associated and start tracking
    init_dut.droid.wifiStartTrackingStateChange()
    resp_dut.droid.wifiStartTrackingStateChange()
    asserts.assert_true(
        self.is_associated(init_dut), "DUT is not associated to an AP!")
    asserts.assert_true(
        self.is_associated(resp_dut), "DUT is not associated to an AP!")

    # set up NDP
    (init_req_key, resp_req_key, init_aware_if, resp_aware_if, init_ipv6,
     resp_ipv6) = autils.create_oob_ndp(init_dut, resp_dut)
    self.log.info("Interface names: I=%s, R=%s", init_aware_if, resp_aware_if)
    self.log.info("Interface addresses (IPv6): I=%s, R=%s", init_ipv6,
                  resp_ipv6)

    # wait for any disassociation change events
    q = queue.Queue()
    init_thread = threading.Thread(
        target=self.wait_for_disassociation, args=(q, init_dut))
    resp_thread = threading.Thread(
        target=self.wait_for_disassociation, args=(q, resp_dut))

    init_thread.start()
    resp_thread.start()

    any_disassociations = False
    try:
      q.get(True, self.TEST_DURATION_SECONDS)
      any_disassociations = True  # only happens on any disassociation
    except queue.Empty:
      pass
    finally:
      # TODO: no way to terminate thread (so even if we fast fail we still have
      # to wait for the full timeout.
      init_dut.droid.wifiStopTrackingStateChange()
      resp_dut.droid.wifiStopTrackingStateChange()

    asserts.assert_false(any_disassociations,
                         "Wi-Fi disassociated during test run")

  ################################################################

  def test_infra_assoc_discovery_stress(self):
    """Validates that Wi-Fi Aware discovery does not interfere with
    infrastructure (AP) association.

    Test assumes (and verifies) that device is already associated to an AP.
    """
    dut = self.android_devices[0]

    # check that associated and start tracking
    dut.droid.wifiStartTrackingStateChange()
    asserts.assert_true(
        self.is_associated(dut), "DUT is not associated to an AP!")

    # attach
    session_id = dut.droid.wifiAwareAttach(True)
    autils.wait_for_event(dut, aconsts.EVENT_CB_ON_ATTACHED)

    # publish
    p_disc_id = dut.droid.wifiAwarePublish(
        session_id,
        autils.create_discovery_config(self.SERVICE_NAME,
                                       aconsts.PUBLISH_TYPE_UNSOLICITED))
    autils.wait_for_event(dut, aconsts.SESSION_CB_ON_PUBLISH_STARTED)

    # wait for any disassociation change events
    any_disassociations = False
    try:
      dut.ed.pop_event(wconsts.WIFI_DISCONNECTED, self.TEST_DURATION_SECONDS)
      any_disassociations = True
    except queue.Empty:
      pass
    finally:
      dut.droid.wifiStopTrackingStateChange()

    asserts.assert_false(any_disassociations,
                         "Wi-Fi disassociated during test run")

  def test_infra_assoc_ndp_no_traffic_stress(self):
    """Validates that Wi-Fi Aware NDP (with no traffic) does not interfere with
    infrastructure (AP) association.

    Test assumes (and verifies) that devices are already associated to an AP.
    """
    self.run_infra_assoc_oob_ndp_stress(with_ndp_traffic=False)
