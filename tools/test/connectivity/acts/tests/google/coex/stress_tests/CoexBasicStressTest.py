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

import time

from acts.test_utils.coex.CoexBaseTest import CoexBaseTest
from acts.test_utils.coex.coex_test_utils import toggle_bluetooth
from acts.test_utils.coex.coex_test_utils import device_discoverable


class CoexBasicStressTest(CoexBaseTest):

    def __init__(self, controllers):
        CoexBaseTest.__init__(self, controllers)

    def setup_class(self):
        CoexBaseTest.setup_class(self)
        req_params = ["iterations"]
        self.unpack_userparams(req_params)

    def start_stop_classic_discovery_with_iperf(self):
        """Starts and stop bluetooth discovery for 1000 iterations.

        Steps:
        1. Start Discovery on Primary device.
        2. Stop Discovery on Secondary device.
        3. Repeat step 1 and 2.

        Returns:
            True if successful, False otherwise.
        """
        self.run_iperf_and_get_result()
        for i in range(self.iterations):
            self.log.info("Inquiry iteration {}".format(i))
            if not self.pri_ad.droid.bluetoothStartDiscovery():
                self.log.error("Bluetooth discovery failed.")
                return False
            time.sleep(2)
            if not self.pri_ad.droid.bluetoothCancelDiscovery():
                self.log.error("Bluetooth cancel discovery failed.")
                return False
        return self.teardown_result()

    def check_device_discoverability_with_iperf(self):
        """Checks if primary device is visible from secondary device.

        Steps:
        1. Start discovery on primary device.
        2. Discover primary device from Secondary device.
        3. Repeat step 1 and 2.

        Returns:
            True if successful, False otherwise.
        """
        self.run_iperf_and_get_result()
        for i in range(self.iterations):
            self.log.info("Discovery iteration = {}".format(i))
            if not device_discoverable(self.pri_ad, self.sec_ad):
                self.log.error("Primary device could not be discovered.")
                return False
        return self.teardown_result()

    def toogle_bluetooth_with_iperf(self):
        """Wrapper function to start iperf traffic and toggling bluetooth."""
        self.run_iperf_and_get_result()
        if not toggle_bluetooth(self.pri_ad, self.iterations):
            return False
        return self.teardown_result()

    def test_stress_toggle_bluetooth_with_tcp_ul(self):
        """Stress test toggling bluetooth on and off.

        This test is to start TCP-uplink traffic between host machine
        and android device and test the integrity of toggling bluetooth
        on and off.

        Steps:
        1. Starts TCP-uplink traffic.
        2. Toggle bluetooth state on and off for N iterations.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_001
        """
        if not self.toogle_bluetooth_with_iperf():
            return False
        return True

    def test_stress_toggle_bluetooth_with_tcp_dl(self):
        """Stress test toggling bluetooth on and off.

        This test is to start TCP-downlink traffic between host machine
        and android device and test the integrity of toggling bluetooth
        on and off.

        Steps:
        1. Starts TCP-downlink traffic.
        2. Toggle bluetooth state on and off for N iterations.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_002
        """
        if not self.toogle_bluetooth_with_iperf():
            return False
        return True

    def test_stress_toggle_bluetooth_with_udp_ul(self):
        """Stress test toggling bluetooth on and off.

        This test is to start UDP-uplink traffic between host machine
        and android device and test the integrity of toggling bluetooth
        on and off.

        Steps:
        1. Starts UDP-uplink traffic.
        2. Toggle bluetooth state on and off for N iterations.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_003
        """
        if not self.toogle_bluetooth_with_iperf():
            return False
        return True

    def test_stress_toggle_bluetooth_with_udp_dl(self):
        """Stress test toggling bluetooth on and off.

        This test is to start UDP-downlink traffic between host machine
        and android device and test the integrity of toggling bluetooth
        on and off.

        Steps:
        1. Starts UDP-downlink traffic.
        2. Toggle bluetooth state on and off for N iterations.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_004
        """
        if not self.toogle_bluetooth_with_iperf():
            return False
        return True

    def test_stress_bluetooth_discovery_with_tcp_ul(self):
        """Stress test on bluetooth discovery.

        This test is to start TCP-uplink traffic between host machine
        and android device and test the integrity of start and stop discovery.

        Steps:
        1. Starts TCP-uplink traffic.
        2. Performs start and stop discovery in quick succession.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_005
        """
        if not self.start_stop_classic_discovery_with_iperf():
            return False
        return True

    def test_stress_bluetooth_discovery_with_tcp_dl(self):
        """Stress test on bluetooth discovery.

        This test is to start TCP-downlink traffic between host machine
        and android device and test the integrity of start and stop discovery.

        Steps:
        1. Starts TCP-downlink traffic.
        2. Performs start and stop discovery in quick succession.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_006
        """
        if not self.start_stop_classic_discovery_with_iperf():
            return False
        return True

    def test_stress_bluetooth_discovery_with_udp_ul(self):
        """Stress test on bluetooth discovery.

        This test is to start UDP-uplink traffic between host machine
        and android device and test the integrity of start and stop discovery.

        Steps:
        1. Starts UDP-uplink traffic.
        2. Performs start and stop discovery in quick succession.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_007
        """
        if not self.start_stop_classic_discovery_with_iperf():
            return False
        return True

    def test_stress_bluetooth_discovery_with_udp_dl(self):
        """Stress test on bluetooth discovery.

        This test is to start UDP-downlink traffic between host machine
        and android device and test the integrity of start and stop discovery.

        Steps:
        1. Starts UDP-downlink traffic.
        2. Performs start and stop discovery in quick succession.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_008
        """
        if not self.start_stop_classic_discovery_with_iperf():
            return False
        return True

    def test_stress_primary_device_visibility_with_tcp_ul(self):
        """Stress test on device visibility from secondary device.

        This test is to start TCP-uplink traffic between host machine
        and android device and stress test on device discoverability from
        secondary device

        Steps:
        1. Start TCP-uplink traffic.
        2. Make primary device visible.
        3. Check if primary device name is visible from secondary device.
        4. Repeat step 2 and 3 for n iterations

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_009
        """
        if not self.check_device_discoverability_with_iperf():
            return False
        return True

    def test_stress_primary_device_visibility_with_tcp_dl(self):
        """Stress test on device visibility from secondary device.

        This test is to start TCP-downlink traffic between host machine
        and android device and stress test on device discoverability from
        secondary device

        Steps:
        1. Start TCP-downlink traffic.
        2. Make primary device visible.
        3. Check if primary device name is visible from secondary device.
        4. Repeat step 2 and 3 for n iterations

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_010
        """
        if not self.check_device_discoverability_with_iperf():
            return False
        return True

    def test_stress_primary_device_visibility_with_udp_ul(self):
        """Stress test on device visibility from secondary device.

        This test is to start UDP-uplink traffic between host machine
        and android device and stress test on device discoverability from
        secondary device

        Steps:
        1. Start UDP-uplink traffic.
        2. Make primary device visible.
        3. Check if primary device name is visible from secondary device.
        4. Repeat step 2 and 3 for n iterations

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_011
        """
        if not self.check_device_discoverability_with_iperf():
            return False
        return True

    def test_stress_primary_device_visibility_with_udp_dl(self):
        """Stress test on device visibility from secondary device.

        This test is to start UDP-downlink traffic between host machine
        and android device and stress test on device discoverability from
        secondary device

        Steps:
        1. Start UDP-downlink traffic.
        2. Make primary device visible.
        3. Check if primary device name is visible from secondary device.
        4. Repeat step 2 and 3 for n iterations

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_012
        """
        if not self.check_device_discoverability_with_iperf():
            return False
        return True

    def test_stress_toggle_bluetooth_with_tcp_bidirectional(self):
        """Stress test toggling bluetooth on and off.

        This test is to start TCP-bidirectional traffic between host machine
        and android device and test the integrity of toggling bluetooth
        on and off.

        Steps:
        1. Starts TCP-bidirectional traffic.
        2. Toggle bluetooth state on and off for N iterations.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_051
        """
        if not self.toogle_bluetooth_with_iperf():
            return False
        return True

    def test_stress_toggle_bluetooth_with_udp_bidirectional(self):
        """Stress test toggling bluetooth on and off.

        This test is to start UDP-bidirectional traffic between host machine
        and android device and test the integrity of toggling bluetooth
        on and off.

        Steps:
        1. Starts TCP-bidirectional traffic.
        2. Toggle bluetooth state on and off for N iterations.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_052
        """
        if not self.toogle_bluetooth_with_iperf():
            return False
        return True

    def test_stress_bluetooth_discovery_with_tcp_bidirectional(self):
        """Stress test on bluetooth discovery.

        This test is to start TCP-bidirectional traffic between host machine
        and android device and test the integrity of start and stop discovery.

        Steps:
        1. Starts TCP-bidirectional traffic.
        2. Performs start and stop discovery in quick succession.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_053
        """
        if not self.start_stop_classic_discovery_with_iperf():
            return False
        return True

    def test_stress_bluetooth_discovery_with_udp_bidirectional(self):
        """Stress test on bluetooth discovery.

        This test is to start UDP-bidirectional traffic between host machine
        and android device and test the integrity of start and stop discovery.

        Steps:
        1. Start wlan traffic with UDP-bidirectional.
        2. Start bluetooth discovery for N iterations.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_054
        """
        if not self.start_stop_classic_discovery_with_iperf():
            return False
        return True

    def test_stress_primary_device_visiblity_tcp_bidirectional(self):
        """Stress test on device visibility from secondary device.

        This test is to start TCP-bidirectional traffic between host machine
        and android device and stress test on device discoverability from
        secondary device

        Steps:
        1. Start TCP-bidirectional traffic.
        2. Make primary device visible.
        3. Check if primary device name is visible from secondary device.
        4. Repeat step 2 and 3 for n iterations

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_055
        """
        if not self.check_device_discoverability_with_iperf():
            return False
        return True

    def test_stress_primary_device_visiblity_udp_bidirectional(self):
        """Stress test on device visibility from secondary device.

        This test is to start UDP-bidirectional traffic between host machine
        and android device and stress test on device discoverability from
        secondary device

        Steps:
        1. Start UDP-bidirectional traffic.
        2. Make primary device visible.
        3. Check if primary device name is visible from secondary device.
        4. Repeat step 2 and 3 for n iterations

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_056
        """
        if not self.check_device_discoverability_with_iperf():
            return False
        return True
