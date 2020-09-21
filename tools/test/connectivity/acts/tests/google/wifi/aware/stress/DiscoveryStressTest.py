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


class DiscoveryStressTest(AwareBaseTest):
  """Stress tests for Discovery sessions"""

  # Number of iterations on create/destroy Attach sessions.
  ATTACH_ITERATIONS = 2

  # Number of iterations on create/destroy Discovery sessions
  DISCOVERY_ITERATIONS = 40

  def __init__(self, controllers):
    AwareBaseTest.__init__(self, controllers)

  ####################################################################

  @test_tracker_info(uuid="783791e5-7726-44e0-ac5b-98c1dbf493cb")
  def test_discovery_stress(self):
    """Create and destroy a random array of discovery sessions, up to the
    limit of capabilities."""
    dut = self.android_devices[0]

    discovery_setup_success = 0
    discovery_setup_fail = 0

    for attach_iter in range(self.ATTACH_ITERATIONS):
      # attach
      session_id = dut.droid.wifiAwareAttach(True)
      autils.wait_for_event(dut, aconsts.EVENT_CB_ON_ATTACHED)

      p_discovery_ids = []
      s_discovery_ids = []
      for discovery_iter in range(self.DISCOVERY_ITERATIONS):
        service_name = 'GoogleTestService-%d-%d' % (attach_iter, discovery_iter)

        p_config = None
        s_config = None

        if discovery_iter % 4 == 0:  # publish/unsolicited
          p_config = autils.create_discovery_config(
              service_name, aconsts.PUBLISH_TYPE_UNSOLICITED)
        elif discovery_iter % 4 == 1:  # publish/solicited
          p_config = autils.create_discovery_config(
              service_name, aconsts.PUBLISH_TYPE_SOLICITED)
        elif discovery_iter % 4 == 2:  # subscribe/passive
          s_config = autils.create_discovery_config(
              service_name, aconsts.SUBSCRIBE_TYPE_PASSIVE)
        elif discovery_iter % 4 == 3:  # subscribe/active
          s_config = autils.create_discovery_config(
              service_name, aconsts.SUBSCRIBE_TYPE_ACTIVE)

        if p_config is not None:
          if len(p_discovery_ids) == dut.aware_capabilities[
              aconsts.CAP_MAX_PUBLISHES]:
            dut.droid.wifiAwareDestroyDiscoverySession(
                p_discovery_ids.pop(dut.aware_capabilities[
                    aconsts.CAP_MAX_PUBLISHES] // 2))
          disc_id = dut.droid.wifiAwarePublish(session_id, p_config)
          event_name = aconsts.SESSION_CB_ON_PUBLISH_STARTED
          p_discovery_ids.append(disc_id)
        else:
          if len(s_discovery_ids) == dut.aware_capabilities[
              aconsts.CAP_MAX_SUBSCRIBES]:
            dut.droid.wifiAwareDestroyDiscoverySession(
                s_discovery_ids.pop(dut.aware_capabilities[
                    aconsts.CAP_MAX_SUBSCRIBES] // 2))
          disc_id = dut.droid.wifiAwareSubscribe(session_id, s_config)
          event_name = aconsts.SESSION_CB_ON_SUBSCRIBE_STARTED
          s_discovery_ids.append(disc_id)

        try:
          dut.ed.pop_event(event_name, autils.EVENT_TIMEOUT)
          discovery_setup_success = discovery_setup_success + 1
        except queue.Empty:
          discovery_setup_fail = discovery_setup_fail + 1

      dut.droid.wifiAwareDestroy(session_id)

    results = {}
    results['discovery_setup_success'] = discovery_setup_success
    results['discovery_setup_fail'] = discovery_setup_fail
    asserts.assert_equal(discovery_setup_fail, 0,
                         'Discovery setup failures', extras=results)
    asserts.explicit_pass('test_discovery_stress done', extras=results)
