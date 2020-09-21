#/usr/bin/env python3.4
#
# Copyright (C) 2018 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.

from acts.test_utils.coex.CoexBaseTest import CoexBaseTest
from acts.test_utils.bt.bt_test_utils import disable_bluetooth


class WlanStandalonePerformanceTest(CoexBaseTest):

    def __init__(self, controllers):
        CoexBaseTest.__init__(self, controllers)

    def setup_test(self):
        CoexBaseTest.setup_test(self)
        if not disable_bluetooth(self.pri_ad.droid):
            self.log.info("Failed to disable bluetooth")
            return False

    def test_performance_wlan_standalone_tcp_ul(self):
        """Check throughout for wlan standalone.

        This test is to start TCP-uplink traffic between host machine and
        android device for wlan-standalone.

        Steps:
        1. Start TCP-uplink traffic.

        Test Id: Bt_CoEx_kpi_001
        """
        self.run_iperf_and_get_result()
        return self.teardown_result()

    def test_performance_wlan_standalone_tcp_dl(self):
        """Check throughout for wlan standalone.

        This test is to start TCP-downlink traffic between host machine and
        android device for wlan-standalone.

        Steps:
        1. Start TCP-downlink traffic.

        Test Id: Bt_CoEx_kpi_002
        """
        self.run_iperf_and_get_result()
        return self.teardown_result()

    def test_performance_wlan_standalone_udp_ul(self):
        """Check throughout for wlan standalone.

        This test is to start UDP-uplink traffic between host machine and
        android device for wlan-standalone.

        Steps:
        1. Start UDP-uplink traffic.

        Test Id: Bt_CoEx_kpi_003
        """
        self.run_iperf_and_get_result()
        return self.teardown_result()

    def test_performance_wlan_standalone_udp_dl(self):
        """Check throughout for wlan standalone.

        This test is to start UDP-downlink traffic between host machine and
        android device for wlan-standalone.

        Steps:
        1. Start UDP-downlink traffic.

        Test Id: Bt_CoEx_kpi_004
        """
        self.run_iperf_and_get_result()
        return self.teardown_result()
