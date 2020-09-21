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
from acts.test_utils.coex.coex_test_utils import toggle_bluetooth
from acts.test_utils.coex.coex_test_utils import start_fping


class CoexBasicFunctionalityTest(CoexBaseTest):

    def __init__(self, controllers):
        CoexBaseTest.__init__(self, controllers)

    def setup_class(self):
        CoexBaseTest.setup_class(self)
        req_params = ["iterations"]
        self.unpack_userparams(req_params)

    def toogle_bluetooth_with_iperf(self):
        """Wrapper function to start iperf traffic and toggling bluetooth."""
        self.run_iperf_and_get_result()
        if not toggle_bluetooth(self.pri_ad, self.iterations):
            return False
        return self.teardown_result()

    def start_discovery_with_iperf(self):
        """Wrapper function to starts iperf traffic and bluetooth discovery,
         gets all the devices discovered, stops discovery.
        """
        self.run_iperf_and_get_result()
        for i in range(self.iterations):
            self.log.info("Bluetooth inquiry iteration : {}".format(i))
            if not perform_classic_discovery(self.pri_ad):
                return False
        return self.teardown_result()

    def test_toogle_bluetooth_with_tcp_ul(self):
        """Starts TCP-uplink traffic, when toggling bluetooth.

        This test is to start TCP-uplink traffic between host machine and
        android device when toggling bluetooth.

        Steps:
        1. Start TCP-uplink traffic at background.
        2. Enable bluetooth.
        3. Disable bluetooth.
        4. Repeat steps 3 and 4 for n iterations

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_001
        """
        if not self.toogle_bluetooth_with_iperf():
            return False
        return True

    def test_toogle_bluetooth_with_tcp_dl(self):
        """Starts TCP-downlink traffic, when toggling bluetooth.

        This test is to start TCP-downlink traffic between host machine and
        android device when toggling bluetooth.

        Steps:
        1. Start TCP-downlink traffic at background.
        2. Enable bluetooth.
        3. Disable bluetooth.
        4. Repeat steps 3 and 4 for n iterations

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_002
        """
        if not self.toogle_bluetooth_with_iperf():
            return False
        return True

    def test_toogle_bluetooth_with_udp_ul(self):
        """Starts UDP-uplink traffic, when toggling bluetooth.

        This test is to start UDP-uplink traffic between host machine and
        android device when toggling bluetooth.

        Steps:
        1. Start UDP-uplink traffic at background.
        2. Enable bluetooth.
        3. Disable bluetooth.
        4. Repeat steps 3 and 4 for n iterations

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_003
        """
        if not self.toogle_bluetooth_with_iperf():
            return False
        return True

    def test_toogle_bluetooth_with_udp_dl(self):
        """Starts UDP-downlink traffic, when toggling bluetooth.

        This test is to start UDP-downlink traffic between host machine and
        android device when toggling bluetooth.

        Steps:
        1. Start UDP-downlink traffic at background.
        2. Enable bluetooth.
        3. Disable bluetooth.
        4. Repeat steps 3 and 4 for n iterations

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_004
        """
        if not self.toogle_bluetooth_with_iperf():
            return False
        return True

    def test_bluetooth_discovery_with_tcp_ul(self):
        """Starts TCP-uplink traffic, along with bluetooth discovery.

        This test is to start TCP-uplink traffic between host machine and
        android device test the functional behaviour of bluetooth discovery.

        Steps:
        1. Run TCP-uplink traffic at background.
        2. Enable bluetooth
        3. Start bluetooth discovery.
        4. List all discovered devices.
        5. Repeat step 3 and 4 for n iterations.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_005
        """
        if not self.start_discovery_with_iperf():
            return False
        return True

    def test_bluetooth_discovery_with_tcp_dl(self):
        """Starts TCP-downlink traffic, along with bluetooth discovery.

        This test is to start TCP-downlink traffic between host machine and
        android device test the functional behaviour of bluetooth discovery.

        Steps:
        1. Run TCP-downlink traffic at background.
        2. Enable bluetooth
        3. Start bluetooth discovery.
        4. List all discovered devices.
        5. Repeat step 3 and 4 for n iterations.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_006
        """
        if not self.start_discovery_with_iperf():
            return False
        return True

    def test_bluetooth_discovery_with_udp_ul(self):
        """Starts UDP-uplink traffic, along with bluetooth discovery.

        This test is to start UDP-uplink traffic between host machine and
        android device test the functional behaviour of bluetooth discovery.

        Steps:
        1. Run UDP-uplink traffic at background.
        2. Enable bluetooth
        3. Start bluetooth discovery.
        4. List all discovered devices.
        5. Repeat step 3 and 4 for n iterations.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_007
        """
        if not self.start_discovery_with_iperf():
            return False
        return True

    def test_bluetooth_discovery_with_udp_dl(self):
        """Starts UDP-downlink traffic, along with bluetooth discovery.

        This test is to start UDP-downlink traffic between host machine and
        android device and test the functional behaviour of  bluetooth discovery.

        Steps:
        1. Run UDP-downlink traffic at background.
        2. Enable bluetooth.
        3. Start bluetooth discovery.
        4. List all discovered devices.
        5. Repeat step 3 and 4 for n iterations.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_008
        """
        if not self.start_discovery_with_iperf():
            return False
        return True

    def test_toogle_bluetooth_with_fping(self):
        """Starts fping, while toggling bluetooth.

        This test is to start fping between host machine and android device
        while toggling bluetooth.

        Steps:
        1. Start fping on background.
        2. Enable bluetooth.
        3. Disable bluetooth.
        4. Repeat steps 3 and 4 for n iterations.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_070
        """
        args = [lambda: start_fping(self.pri_ad, self.iperf["duration"])]
        self.run_thread(args)
        if not self.toogle_bluetooth_with_iperf():
            return False
        return self.teardown_thread()

    def test_bluetooth_discovery_with_fping(self):
        """Starts fping, along with bluetooth discovery.

        This test is to start fping between host machine and android device
        and test functional behaviour of bluetooth discovery.

        Steps:
        1. Start fping on background.
        2. Enable bluetooth.
        3. Start bluetooth discovery.
        4. Repeat step 3 for n iterations.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_071
        """
        args = [lambda: start_fping(self.pri_ad, self.iperf["duration"])]
        self.run_thread(args)
        if not self.start_discovery_with_iperf():
            return False
        return self.teardown_thread()
