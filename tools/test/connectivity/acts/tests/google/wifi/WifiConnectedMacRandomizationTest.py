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
import acts.signals as signals
from acts.test_utils.tel.tel_test_utils import WIFI_CONFIG_APBAND_2G
import acts.test_utils.wifi.wifi_test_utils as wutils
import acts.utils as utils

from acts import asserts
from acts.test_decorators import test_tracker_info
from acts.test_utils.wifi.WifiBaseTest import WifiBaseTest

WifiEnums = wutils.WifiEnums
# Default timeout used for reboot, toggle WiFi and Airplane mode,
# for the system to settle down after the operation.
DEFAULT_TIMEOUT = 10
GET_MAC_ADDRESS= ("ip addr show wlan0"
                  "| grep 'link/ether'"
                  "| cut -d ' ' -f6")
MAC_SETTING = "wifi_connected_mac_randomization_enabled"
GET_MAC_RANDOMIZATION_STATUS = "settings get global {}".format(MAC_SETTING)
TURN_ON_MAC_RANDOMIZATION = "settings put global {} 1".format(MAC_SETTING)
TURN_OFF_MAC_RANDOMIZATION = "settings put global {} 0".format(MAC_SETTING)
LOG_CLEAR = "logcat -c"
LOG_GREP = "logcat -d | grep {}"

class WifiConnectedMacRandomizationTest(WifiBaseTest):
    """Tests for Connected MAC Randomization.

    Test Bed Requirement:
    * Two Android devices with the first one supporting MAC randomization.
    * At least two Wi-Fi networks to connect to.
    """

    def __init__(self, controllers):
        WifiBaseTest.__init__(self, controllers)

    def setup_class(self):
        self.dut = self.android_devices[0]
        self.dut_softap = self.android_devices[1]
        wutils.wifi_test_device_init(self.dut)
        wutils.wifi_test_device_init(self.dut_softap)

        self.reset_mac_address_to_factory_mac()
        self.dut.adb.shell(TURN_ON_MAC_RANDOMIZATION)
        asserts.assert_equal(
            self.dut.adb.shell(GET_MAC_RANDOMIZATION_STATUS), "1",
            "Failed to enable Connected MAC Randomization on dut.")

        req_params = ["reference_networks"]
        opt_param = []
        self.unpack_userparams(
            req_param_names=req_params, opt_param_names=opt_param)

        if "AccessPoint" in self.user_params:
            self.legacy_configure_ap_and_start()

        asserts.assert_true(
            self.reference_networks[0]["2g"],
            "Need at least 1 2.4Ghz reference network with psk.")
        asserts.assert_true(
            self.reference_networks[0]["5g"],
            "Need at least 1 5Ghz reference network with psk.")
        self.wpapsk_2g = self.reference_networks[0]["2g"]
        self.wpapsk_5g = self.reference_networks[0]["5g"]

    def setup_test(self):
        self.dut.droid.wakeLockAcquireBright()
        self.dut.droid.wakeUpNow()
        wutils.wifi_toggle_state(self.dut, True)
        wutils.wifi_toggle_state(self.dut_softap, False)

    def teardown_test(self):
        self.dut.droid.wakeLockRelease()
        self.dut.droid.goToSleepNow()
        wutils.reset_wifi(self.dut)
        wutils.reset_wifi(self.dut_softap)

    def on_fail(self, test_name, begin_time):
        self.dut.take_bug_report(test_name, begin_time)
        self.dut.cat_adb_log(test_name, begin_time)

    def teardown_class(self):
        wutils.stop_wifi_tethering(self.dut_softap)
        self.reset_mac_address_to_factory_mac()
        if "AccessPoint" in self.user_params:
            del self.user_params["reference_networks"]
            del self.user_params["open_network"]

    """Helper Functions"""
    def get_current_mac_address(self, ad):
        """Get the device's wlan0 MAC address.

        Args:
            ad: AndroidDevice to get MAC address of.

        Returns:
            A MAC address string in the format of "12:34:56:78:90:12".
        """
        return ad.adb.shell(GET_MAC_ADDRESS)

    def is_valid_randomized_mac_address(self, mac):
        """Check if the given MAC address is a valid randomized MAC address.

        Args:
            mac: MAC address to check in the format of "12:34:56:78:90:12".
        """
        asserts.assert_true(
            mac != self.dut_factory_mac,
            "Randomized MAC address is same as factory MAC address.")
        first_byte = int(mac[:2], 16)
        asserts.assert_equal(first_byte & 1, 0, "MAC address is not unicast.")
        asserts.assert_equal(first_byte & 2, 2, "MAC address is not local.")

    def reset_mac_address_to_factory_mac(self):
        """Reset dut to and store factory MAC address by turning off
        Connected MAC Randomization and rebooting dut.
        """
        self.dut.adb.shell(TURN_OFF_MAC_RANDOMIZATION)
        asserts.assert_equal(
            self.dut.adb.shell(GET_MAC_RANDOMIZATION_STATUS), "0",
            "Failed to disable Connected MAC Randomization on dut.")
        self.dut.reboot()
        time.sleep(DEFAULT_TIMEOUT)
        self.dut_factory_mac = self.get_current_mac_address(self.dut)

    def get_connection_data(self, ad, network):
        """Connect and get network id and ssid info from connection data.

        Args:
            ad: AndroidDevice to use for connection
            network: network info of the network to connect to

        Returns:
            A convenience dict with the connected network's ID and SSID.
        """
        wutils.connect_to_wifi_network(ad, network)
        connect_data = ad.droid.wifiGetConnectionInfo()
        ssid_id_dict = dict()
        ssid_id_dict[WifiEnums.NETID_KEY] = connect_data[WifiEnums.NETID_KEY]
        ssid_id_dict[WifiEnums.SSID_KEY] = connect_data[WifiEnums.SSID_KEY]
        return ssid_id_dict

    """Tests"""
    @test_tracker_info(uuid="")
    def test_wifi_connection_2G_with_mac_randomization(self):
        """Tests connection to 2G network with Connected MAC Randomization.
        """
        wutils.connect_to_wifi_network(self.dut, self.wpapsk_2g)
        mac = self.get_current_mac_address(self.dut)
        self.is_valid_randomized_mac_address(mac)

    @test_tracker_info(uuid="")
    def test_wifi_connection_5G_with_mac_randomization(self):
        """Tests connection to 5G network with Connected MAC Randomization.
        """
        wutils.connect_to_wifi_network(self.dut, self.wpapsk_5g)
        mac = self.get_current_mac_address(self.dut)
        self.is_valid_randomized_mac_address(mac)

    @test_tracker_info(uuid="")
    def test_randomized_mac_persistent_between_connections(self):
        """Tests that randomized MAC address assigned to each network is
        persistent between connections.

        Steps:
        1. Connect to a 2GHz network.
        2. Connect to a 5GHz network.
        3. Reconnect to the 2GHz network using its network id.
        4. Verify that MAC addresses in Steps 1 and 3 are equal.
        5. Reconnect to the 5GHz network using its network id.
        6. Verify that MAC addresses in Steps 2 and 5 are equal.
        """
        connect_data_2g = self.get_connection_data(self.dut, self.wpapsk_2g)
        old_mac_2g = self.get_current_mac_address(self.dut)
        self.is_valid_randomized_mac_address(old_mac_2g)

        connect_data_5g = self.get_connection_data(self.dut, self.wpapsk_5g)
        old_mac_5g = self.get_current_mac_address(self.dut)
        self.is_valid_randomized_mac_address(old_mac_5g)

        asserts.assert_true(
            old_mac_2g != old_mac_5g,
            "Randomized MAC addresses for 2G and 5G networks are equal.")

        reconnect_2g = wutils.connect_to_wifi_network_with_id(
            self.dut,
            connect_data_2g[WifiEnums.NETID_KEY],
            connect_data_2g[WifiEnums.SSID_KEY])
        if not reconnect_2g:
            raise signals.TestFailure("Device did not connect to the correct"
                                      " 2GHz network.")
        new_mac_2g = self.get_current_mac_address(self.dut)
        asserts.assert_equal(
            old_mac_2g,
            new_mac_2g,
            "Randomized MAC for 2G is not persistent between connections.")

        reconnect_5g = wutils.connect_to_wifi_network_with_id(
            self.dut,
            connect_data_5g[WifiEnums.NETID_KEY],
            connect_data_5g[WifiEnums.SSID_KEY])
        if not reconnect_5g:
            raise signals.TestFailure("Device did not connect to the correct"
                                      " 5GHz network.")
        new_mac_5g = self.get_current_mac_address(self.dut)
        asserts.assert_equal(
            old_mac_5g,
            new_mac_5g,
            "Randomized MAC for 5G is not persistent between connections.")

    @test_tracker_info(uuid="")
    def test_randomized_mac_used_during_connection(self):
        """Verify that the randomized MAC address and not the factory
        MAC address is used during connection by checking the softap logs.

        Steps:
        1. Set up softAP on dut_softap.
        2. Have dut connect to the softAp.
        3. Verify that only randomized MAC appears in softAp logs.
        """
        self.dut_softap.adb.shell(LOG_CLEAR)
        config = wutils.create_softap_config()
        wutils.start_wifi_tethering(self.dut_softap,
                                    config[wutils.WifiEnums.SSID_KEY],
                                    config[wutils.WifiEnums.PWD_KEY],
                                    WIFI_CONFIG_APBAND_2G)

        # Internet validation fails when dut_softap does not have a valid sim
        # supporting softap. Since this test is not checking for internet
        # validation, we suppress failure signals.
        wutils.connect_to_wifi_network(self.dut, config, assert_on_fail=False)
        mac = self.get_current_mac_address(self.dut)
        wutils.stop_wifi_tethering(self.dut_softap)

        self.is_valid_randomized_mac_address(mac)
        log = self.dut_softap.adb.shell(LOG_GREP.format(mac))
        asserts.assert_true(len(log) > 0, "Randomized MAC not in log.")
        log = self.dut_softap.adb.shell(LOG_GREP.format(self.dut_factory_mac))
        asserts.assert_true(len(log) == 0, "Factory MAC is in log.")
