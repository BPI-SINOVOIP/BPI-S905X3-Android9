# /usr/bin/env python3.4
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
from acts.test_utils.coex.coex_test_utils import perform_classic_discovery


class CoexBasicPerformanceTest(CoexBaseTest):

    def __init__(self, controllers):
        CoexBaseTest.__init__(self, controllers)

    def run_iperf_and_perform_discovery(self):
        """Starts iperf client on host machine and bluetooth discovery
        simultaneously.

        Returns:
            True if successful, False otherwise.
        """
        self.run_iperf_and_get_result()
        if not perform_classic_discovery(self.pri_ad):
            return False
        return self.teardown_result()

    def test_performance_with_bt_on_tcp_ul(self):
        """Check throughput when bluetooth on.

        This test is to start TCP-Uplink traffic between host machine and
        android device and check the throughput when bluetooth is on.

        Steps:
        1. Start TCP-uplink traffic when bluetooth is on.

        Test Id: Bt_CoEx_kpi_005
        """
        self.run_iperf_and_get_result()
        self.teardown_result()

    def test_performance_with_bt_on_tcp_dl(self):
        """Check throughput when bluetooth on.

        This test is to start TCP-downlink traffic between host machine and
        android device and check the throughput when bluetooth is on.

        Steps:
        1. Start TCP-downlink traffic when bluetooth is on.

        Test Id: Bt_CoEx_kpi_006
        """
        self.run_iperf_and_get_result()
        self.teardown_result()

    def test_performance_with_bt_on_udp_ul(self):
        """Check throughput when bluetooth on.

        This test is to start UDP-uplink traffic between host machine and
        android device and check the throughput when bluetooth is on.

        Steps:
        1. Start UDP-uplink traffic when bluetooth is on.

        Test Id: Bt_CoEx_kpi_007
        """
        self.run_iperf_and_get_result()
        self.teardown_result()

    def test_performance_with_bt_on_udp_dl(self):
        """Check throughput when bluetooth on.

        This test is to start UDP-downlink traffic between host machine and
        android device and check the throughput when bluetooth is on.

        Steps:
        1. Start UDP-downlink traffic when bluetooth is on.

        Test Id: Bt_CoEx_kpi_008
        """
        self.run_iperf_and_get_result()
        self.teardown_result()

    def test_performance_with_bluetooth_discovery_tcp_ul(self):
        """Check throughput when bluetooth discovery is ongoing.

        This test is to start TCP-uplink traffic between host machine and
        android device and bluetooth discovery and checks throughput.

        Steps:
        1. Start TCP-uplink traffic and bluetooth discovery parallelly.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_kpi_009
        """
        if not self.run_iperf_and_perform_discovery():
            return False
        return True

    def test_performance_with_bluetooth_discovery_tcp_dl(self):
        """Check throughput when bluetooth discovery is ongoing.

        This test is to start TCP-downlink traffic between host machine and
        android device and bluetooth discovery and checks throughput.

        Steps:
        1. Start TCP-downlink traffic and bluetooth discovery parallelly.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_kpi_010
        """
        if not self.run_iperf_and_perform_discovery():
            return False
        return True

    def test_performance_with_bluetooth_discovery_udp_ul(self):
        """Check throughput when bluetooth discovery is ongoing.

        This test is to start UDP-uplink traffic between host machine and
        android device and bluetooth discovery and checks throughput.

        Steps:
        1. Start UDP-uplink traffic and bluetooth discovery parallelly.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_kpi_011
        """
        if not self.run_iperf_and_perform_discovery():
            return False
        return True

    def test_performance_with_bluetooth_discovery_udp_dl(self):
        """Check throughput when bluetooth discovery is ongoing.

        This test is to start UDP-downlink traffic between host machine and
        android device and bluetooth discovery and checks throughput.

        Steps:
        1. Start UDP-downlink traffic and bluetooth discovery parallelly.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_kpi_012
        """
        if not self.run_iperf_and_perform_discovery():
            return False
        return True
