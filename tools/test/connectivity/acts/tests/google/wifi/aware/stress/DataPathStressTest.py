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
from acts.test_decorators import test_tracker_info
from acts.test_utils.net import connectivity_const as cconsts
from acts.test_utils.wifi.aware import aware_const as aconsts
from acts.test_utils.wifi.aware import aware_test_utils as autils
from acts.test_utils.wifi.aware.AwareBaseTest import AwareBaseTest


class DataPathStressTest(AwareBaseTest):

  # Number of iterations on create/destroy Attach sessions.
  ATTACH_ITERATIONS = 2

  # Number of iterations on create/destroy NDP in each discovery session.
  NDP_ITERATIONS = 50

  # Maximum percentage of NDP setup failures over all iterations
  MAX_FAILURE_PERCENTAGE = 1

  def __init__(self, controllers):
    AwareBaseTest.__init__(self, controllers)

  ################################################################

  def run_oob_ndp_stress(self, attach_iterations, ndp_iterations,
      trigger_failure_on_index=None):
    """Run NDP (NAN data-path) stress test creating and destroying Aware
    attach sessions, discovery sessions, and NDPs.

    Args:
      attach_iterations: Number of attach sessions.
      ndp_iterations: Number of NDP to be attempted per attach session.
      trigger_failure_on_index: Trigger a failure on this NDP iteration (the
                                mechanism is to request NDP on Initiator
                                before issuing the requeest on the Responder).
                                If None then no artificial failure triggered.
    """
    init_dut = self.android_devices[0]
    init_dut.pretty_name = 'Initiator'
    resp_dut = self.android_devices[1]
    resp_dut.pretty_name = 'Responder'

    ndp_init_setup_success = 0
    ndp_init_setup_failures = 0
    ndp_resp_setup_success = 0
    ndp_resp_setup_failures = 0

    for attach_iter in range(attach_iterations):
      init_id = init_dut.droid.wifiAwareAttach(True)
      autils.wait_for_event(init_dut, aconsts.EVENT_CB_ON_ATTACHED)
      init_ident_event = autils.wait_for_event(
          init_dut, aconsts.EVENT_CB_ON_IDENTITY_CHANGED)
      init_mac = init_ident_event['data']['mac']
      time.sleep(self.device_startup_offset)
      resp_id = resp_dut.droid.wifiAwareAttach(True)
      autils.wait_for_event(resp_dut, aconsts.EVENT_CB_ON_ATTACHED)
      resp_ident_event = autils.wait_for_event(
          resp_dut, aconsts.EVENT_CB_ON_IDENTITY_CHANGED)
      resp_mac = resp_ident_event['data']['mac']

      # wait for for devices to synchronize with each other - there are no other
      # mechanisms to make sure this happens for OOB discovery (except retrying
      # to execute the data-path request)
      time.sleep(autils.WAIT_FOR_CLUSTER)

      for ndp_iteration in range(ndp_iterations):
        if trigger_failure_on_index != ndp_iteration:
          # Responder: request network
          resp_req_key = autils.request_network(
              resp_dut,
              resp_dut.droid.wifiAwareCreateNetworkSpecifierOob(
                  resp_id, aconsts.DATA_PATH_RESPONDER, init_mac, None))

          # Wait a minimal amount of time to let the Responder configure itself
          # and be ready for the request. While calling it first may be
          # sufficient there are no guarantees that a glitch may slow the
          # Responder slightly enough to invert the setup order.
          time.sleep(1)

          # Initiator: request network
          init_req_key = autils.request_network(
              init_dut,
              init_dut.droid.wifiAwareCreateNetworkSpecifierOob(
                  init_id, aconsts.DATA_PATH_INITIATOR, resp_mac, None))
        else:
          # Initiator: request network
          init_req_key = autils.request_network(
              init_dut,
              init_dut.droid.wifiAwareCreateNetworkSpecifierOob(
                  init_id, aconsts.DATA_PATH_INITIATOR, resp_mac, None))

          # Wait a minimal amount of time to let the Initiator configure itself
          # to guarantee failure!
          time.sleep(2)

          # Responder: request network
          resp_req_key = autils.request_network(
              resp_dut,
              resp_dut.droid.wifiAwareCreateNetworkSpecifierOob(
                  resp_id, aconsts.DATA_PATH_RESPONDER, init_mac, None))

        # Initiator: wait for network formation
        got_on_available = False
        got_on_link_props = False
        while not got_on_available or not got_on_link_props:
          try:
            nc_event = init_dut.ed.pop_event(cconsts.EVENT_NETWORK_CALLBACK,
                                             autils.EVENT_NDP_TIMEOUT)
            if nc_event['data'][
                cconsts.NETWORK_CB_KEY_EVENT] == cconsts.NETWORK_CB_AVAILABLE:
              got_on_available = True
            elif (nc_event['data'][cconsts.NETWORK_CB_KEY_EVENT] ==
                  cconsts.NETWORK_CB_LINK_PROPERTIES_CHANGED):
              got_on_link_props = True
          except queue.Empty:
            ndp_init_setup_failures = ndp_init_setup_failures + 1
            init_dut.log.info('[Initiator] Timed out while waiting for '
                              'EVENT_NETWORK_CALLBACK')
            break

        if got_on_available and got_on_link_props:
          ndp_init_setup_success = ndp_init_setup_success + 1

        # Responder: wait for network formation
        got_on_available = False
        got_on_link_props = False
        while not got_on_available or not got_on_link_props:
          try:
            nc_event = resp_dut.ed.pop_event(cconsts.EVENT_NETWORK_CALLBACK,
                                             autils.EVENT_NDP_TIMEOUT)
            if nc_event['data'][
                cconsts.NETWORK_CB_KEY_EVENT] == cconsts.NETWORK_CB_AVAILABLE:
              got_on_available = True
            elif (nc_event['data'][cconsts.NETWORK_CB_KEY_EVENT] ==
                  cconsts.NETWORK_CB_LINK_PROPERTIES_CHANGED):
              got_on_link_props = True
          except queue.Empty:
            ndp_resp_setup_failures = ndp_resp_setup_failures + 1
            init_dut.log.info('[Responder] Timed out while waiting for '
                              'EVENT_NETWORK_CALLBACK')
            break

        if got_on_available and got_on_link_props:
          ndp_resp_setup_success = ndp_resp_setup_success + 1

        # clean-up
        init_dut.droid.connectivityUnregisterNetworkCallback(init_req_key)
        resp_dut.droid.connectivityUnregisterNetworkCallback(resp_req_key)

      # clean-up at end of iteration
      init_dut.droid.wifiAwareDestroy(init_id)
      resp_dut.droid.wifiAwareDestroy(resp_id)

    results = {}
    results['ndp_init_setup_success'] = ndp_init_setup_success
    results['ndp_init_setup_failures'] = ndp_init_setup_failures
    results['ndp_resp_setup_success'] = ndp_resp_setup_success
    results['ndp_resp_setup_failures'] = ndp_resp_setup_failures
    max_failures = (
        self.MAX_FAILURE_PERCENTAGE * attach_iterations * ndp_iterations / 100)
    if max_failures == 0:
      max_failures = 1
    if trigger_failure_on_index is not None:
      max_failures = max_failures + 1 # for the triggered failure
    asserts.assert_true(
      (ndp_init_setup_failures + ndp_resp_setup_failures) < (2 * max_failures),
      'NDP setup failure rate exceeds threshold', extras=results)
    asserts.explicit_pass("test_oob_ndp_stress* done", extras=results)

  @test_tracker_info(uuid="a20a96ba-e71f-4d31-b850-b88a75381981")
  def test_oob_ndp_stress(self):
    """Run NDP (NAN data-path) stress test creating and destroying Aware
    attach sessions, discovery sessions, and NDPs."""
    self.run_oob_ndp_stress(self.ATTACH_ITERATIONS, self.NDP_ITERATIONS)

  @test_tracker_info(uuid="1fb4a383-bf1a-411a-a904-489dd9e29c6a")
  def test_oob_ndp_stress_failure_case(self):
    """Run NDP (NAN data-path) stress test creating and destroying Aware
    attach sessions, discovery sessions, and NDPs.

    Verify recovery from failure by triggering an artifical failure and
    verifying that all subsequent iterations succeed.
    """
    self.run_oob_ndp_stress(attach_iterations=1,
                            ndp_iterations=10,
                            trigger_failure_on_index=3)
