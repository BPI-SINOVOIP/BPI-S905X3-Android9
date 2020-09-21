#!/usr/bin/env python3.4
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

import itertools
import pprint
import queue
import time

import acts.base_test
import acts.signals
import acts.test_utils.wifi.wifi_test_utils as wutils
import acts.utils

from acts import asserts
from acts.test_decorators import test_tracker_info
from acts.test_utils.wifi.WifiBaseTest import WifiBaseTest


class WifiHiddenSSIDTest(WifiBaseTest):
    """Tests for APIs in Android's WifiManager class.

    Test Bed Requirement:
    * One Android device
    * Several Wi-Fi networks visible to the device, including an open Wi-Fi
      network.
    """

    def __init__(self, controllers):
        WifiBaseTest.__init__(self, controllers)

    def setup_class(self):
        self.dut = self.android_devices[0]
        wutils.wifi_test_device_init(self.dut)
        req_params = []
        opt_param = [
            "open_network", "reference_networks"]
        self.unpack_userparams(
            req_param_names=req_params, opt_param_names=opt_param)

        if "AccessPoint" in self.user_params:
            self.legacy_configure_ap_and_start(hidden=True)

        asserts.assert_true(
            len(self.reference_networks) > 0,
            "Need at least one reference network with psk.")
        self.open_hidden_2g = self.open_network[0]["2g"]
        self.open_hidden_5g = self.open_network[0]["5g"]
        self.wpa_hidden_2g = self.reference_networks[0]["2g"]
        self.wpa_hidden_5g = self.reference_networks[0]["5g"]

    def setup_test(self):
        self.dut.droid.wakeLockAcquireBright()
        self.dut.droid.wakeUpNow()

    def teardown_test(self):
        self.dut.droid.wakeLockRelease()
        self.dut.droid.goToSleepNow()

    def on_fail(self, test_name, begin_time):
        self.dut.take_bug_report(test_name, begin_time)
        self.dut.cat_adb_log(test_name, begin_time)

    def teardown_class(self):
        wutils.reset_wifi(self.dut)
        if "AccessPoint" in self.user_params:
            del self.user_params["reference_networks"]
            del self.user_params["open_network"]

    """Helper Functions"""

    def check_hiddenSSID_in_scan(self, ap_ssid, max_tries=2):
        """Check if the ap started by wifi tethering is seen in scan results.

        Args:
            ap_ssid: SSID of the ap we are looking for.
            max_tries: Number of scans to try.
        Returns:
            True: if ap_ssid is found in scan results.
            False: if ap_ssid is not found in scan results.
        """
        for num_tries in range(max_tries):
            wutils.start_wifi_connection_scan(self.dut)
            scan_results = self.dut.droid.wifiGetScanResults()
            match_results = wutils.match_networks(
                {wutils.WifiEnums.SSID_KEY: ap_ssid}, scan_results)
            if len(match_results) > 0:
                return True
        return False

    def add_hiddenSSID_and_connect(self, hidden_network):
        """Add the hidden network and connect to it.

        Args:
            hidden_network: The hidden network config to connect to.

        """
        ret = self.dut.droid.wifiAddNetwork(hidden_network)
        asserts.assert_true(ret != -1, "Add network %r failed" % hidden_network)
        self.dut.droid.wifiEnableNetwork(ret, 0)
        wutils.connect_to_wifi_network(self.dut, hidden_network)
        if not wutils.validate_connection(self.dut):
            raise signals.TestFailure("Fail to connect to internet on %s" %
                                       hidden_network)

    """Tests"""

    @test_tracker_info(uuid="d0871f98-6049-4937-a288-ec4a2746c771")
    def test_connect_to_wpa_hidden_2g(self):
        """Connect to a WPA, 2G network.

        Steps:
        1. Add a WPA, 2G hidden network.
        2. Ensure the network is visible in scan.
        3. Connect and run ping.

        """
        self.add_hiddenSSID_and_connect(self.wpa_hidden_2g)

    @test_tracker_info(uuid="c558b31a-549a-4012-9052-275623992187")
    def test_connect_to_wpa_hidden_5g(self):
        """Connect to a WPA, 5G hidden  network.

        Steps:
        1. Add a WPA, 5G hidden network.
        2. Ensure the network is visible in scan.
        3. Connect and run ping.

        """
        self.add_hiddenSSID_and_connect(self.wpa_hidden_5g)

    @test_tracker_info(uuid="cdfce76f-6374-439d-aa1d-e920508269d2")
    def test_connect_to_open_hidden_2g(self):
        """Connect to a Open, 2G hidden  network.

        Steps:
        1. Add a Open, 2G hidden network.
        2. Ensure the network is visible in scan.
        3. Connect and run ping.

        """
        self.add_hiddenSSID_and_connect(self.open_hidden_2g)

    @test_tracker_info(uuid="29ccbae4-4382-4df8-8fc5-00e3104230d0")
    def test_connect_to_open_hidden_5g(self):
        """Connect to a Open, 5G hidden  network.

        Steps:
        1. Add a Open, 5G hidden network.
        2. Ensure the network is visible in scan.
        3. Connect and run ping.

        """
        self.add_hiddenSSID_and_connect(self.open_hidden_5g)
