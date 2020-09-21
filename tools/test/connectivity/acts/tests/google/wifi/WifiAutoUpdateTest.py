#!/usr/bin/env python3.4
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

import itertools
import pprint
import queue
import time

import acts.base_test
import acts.signals as signals
import acts.test_utils.wifi.wifi_test_utils as wutils
import acts.utils

from acts import asserts
from acts.libs.ota import ota_updater
from acts.test_decorators import test_tracker_info
from acts.test_utils.wifi.WifiBaseTest import WifiBaseTest

WifiEnums = wutils.WifiEnums
# Default timeout used for reboot, toggle WiFi and Airplane mode,
# for the system to settle down after the operation.
DEFAULT_TIMEOUT = 10
BAND_2GHZ = 0
BAND_5GHZ = 1


class WifiAutoUpdateTest(WifiBaseTest):
    """Tests for APIs in Android's WifiManager class.

    Test Bed Requirement:
    * One Android device
    * Several Wi-Fi networks visible to the device, including an open Wi-Fi
      network.
    """

    def __init__(self, controllers):
        WifiBaseTest.__init__(self, controllers)
        self.tests = (
            "test_check_wifi_state_after_au",
            "test_verify_networks_after_au",
            "test_all_networks_connectable_after_au",
            "test_connection_to_new_networks",
            "test_check_wifi_toggling_after_au",
            "test_reset_wifi_after_au")

    def setup_class(self):
        super(WifiAutoUpdateTest, self).setup_class()
        ota_updater.initialize(self.user_params, self.android_devices)
        self.dut = self.android_devices[0]
        wutils.wifi_test_device_init(self.dut)
        req_params = []
        opt_param = [
            "open_network", "reference_networks", "iperf_server_address"
        ]
        self.unpack_userparams(
            req_param_names=req_params, opt_param_names=opt_param)

        if "AccessPoint" in self.user_params:
            self.legacy_configure_ap_and_start()

        asserts.assert_true(
            len(self.reference_networks) > 0,
            "Need at least two reference network with psk.")
        asserts.assert_true(
            len(self.open_network) > 0,
            "Need at least two open network with psk.")
        wutils.wifi_toggle_state(self.dut, True)

        self.wifi_config_list = []

        # Setup WiFi and add few open and wpa networks before OTA.
        self.add_network_and_enable(self.open_network[0]['2g'])
        self.add_network_and_enable(self.reference_networks[0]['5g'])

        # Add few dummy networks to the list.
        self.add_and_enable_dummy_networks()

        # Run OTA below, if ota fails then abort all tests.
        try:
            ota_updater.update(self.dut)
        except Exception as err:
            raise signals.TestSkipClass(
                "Failed up apply OTA update. Aborting tests")

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
        if "AccessPoint" in self.user_params:
            del self.user_params["reference_networks"]
            del self.user_params["open_network"]

    """Helper Functions"""

    def add_network_and_enable(self, network):
        """Add a network and enable it.

        Args:
            network : Network details for the network to be added.

        """
        ret = self.dut.droid.wifiAddNetwork(network)
        asserts.assert_true(ret != -1, "Add network %r failed" % network)
        self.wifi_config_list.append({
                WifiEnums.SSID_KEY: network[WifiEnums.SSID_KEY],
                WifiEnums.NETID_KEY: ret})
        self.dut.droid.wifiEnableNetwork(ret, 0)

    def add_and_enable_dummy_networks(self, num_networks=5):
        """Add some dummy networks to the device and enable them.

        Args:
            num_networks: Number of networks to add.
        """
        ssid_name_base = "dummy_network_"
        for i in range(0, num_networks):
            network = {}
            network[WifiEnums.SSID_KEY] = ssid_name_base + str(i)
            network[WifiEnums.PWD_KEY] = "dummynet_password"
            self.add_network_and_enable(network)

    def check_networks_after_autoupdate(self, networks):
        """Verify that all previously configured networks are presistent after
           reboot.

        Args:
            networks: List of network dicts.

        Return:
            None. Raises TestFailure.

        """
        network_info = self.dut.droid.wifiGetConfiguredNetworks()
        if len(network_info) != len(networks):
            msg = (
                "Number of configured networks before and after Auto-update "
                "don't match. \nBefore reboot = %s \n After reboot = %s" %
                (networks, network_info))
            raise signals.TestFailure(msg)
        current_count = 0
        # For each network, check if it exists in configured list after Auto-
        # update.
        for network in networks:
            exists = wutils.match_networks({
                WifiEnums.SSID_KEY: network[WifiEnums.SSID_KEY]
            }, network_info)
            if not len(exists):
                raise signals.TestFailure("%s network is not present in the"
                                          " configured list after Auto-update" %
                                          network[WifiEnums.SSID_KEY])
            # Get the new network id for each network after reboot.
            network[WifiEnums.NETID_KEY] = exists[0]['networkId']

    """Tests"""

    @test_tracker_info(uuid="9ff1f01e-e5ff-408b-9a95-29e87a2df2d8")
    def test_check_wifi_state_after_au(self):
        """Check if the state of WiFi is enabled after Auto-update."""
        if not self.dut.droid.wifiCheckState():
            raise signals.TestFailure("WiFi is disabled after Auto-update!!!")

    @test_tracker_info(uuid="e3ebdbba-71dd-4281-aef8-5b3d42b88770")
    def test_verify_networks_after_au(self):
        """Check if the previously added networks are intact.

           Steps:
               Number of networs should be the same and match each network.

        """
        self.check_networks_after_autoupdate(self.wifi_config_list)

    @test_tracker_info(uuid="b8e47a4f-62fe-4a0e-b999-27ae1ebf4d19")
    def test_connection_to_new_networks(self):
        """Check if we can connect to new networks after Auto-update.

           Steps:
               1. Connect to a PSK network.
               2. Connect to an open network.
               3. Forget ntworks added in 1 & 2.
               TODO: (@bmahadev) Add WEP network once it's ready.

        """
        wutils.connect_to_wifi_network((self.open_network[0]['5g'], self.dut))
        wutils.connect_to_wifi_network((self.reference_networks[0]['2g'],
                self.dut))
        wutils.wifi_forget_network(self.dut,
                self.reference_networks[0]['2g'][WifiEnums.SSID_KEY])
        wutils.wifi_forget_network(self.dut,
                self.open_network[0]['5g'][WifiEnums.SSID_KEY])

    @test_tracker_info(uuid="1d8309e4-d5a2-4f48-ba3b-895a58c9bf3a")
    def test_all_networks_connectable_after_au(self):
        """Check if previously added networks are connectable.

           Steps:
               1. Connect to previously added PSK network using network id.
               2. Connect to previously added open network using network id.
               TODO: (@bmahadev) Add WEP network once it's ready.

        """
        for network in self.wifi_config_list:
            if 'dummy' not in network[WifiEnums.SSID_KEY]:
                if not wutils.connect_to_wifi_network_with_id(self.dut,
                        network[WifiEnums.NETID_KEY],
                        network[WifiEnums.SSID_KEY]):
                    raise signals.TestFailure("Failed to connect to %s after \
                            Auto-update" % network[WifiEnums.SSID_KEY])

    @test_tracker_info(uuid="05671859-38b1-4dbf-930c-18048971d075")
    def test_check_wifi_toggling_after_au(self):
        """Check if WiFi can be toggled ON/OFF after auto-update."""
        self.log.debug("Going from on to off.")
        wutils.wifi_toggle_state(self.dut, False)
        self.log.debug("Going from off to on.")
        wutils.wifi_toggle_state(self.dut, True)

    @test_tracker_info(uuid="440edf32-4b00-42b0-9811-9f2bc4a83efb")
    def test_reset_wifi_after_au(self):
        """"Check if WiFi can be reset after auto-update."""
        wutils.reset_wifi(self.dut)
