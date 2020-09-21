#!/usr/bin/env python3.4
#
#   Copyright 2016 - The Android Open Source Project
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
from acts.test_decorators import test_tracker_info
from acts.test_utils.wifi.WifiBaseTest import WifiBaseTest

WifiEnums = wutils.WifiEnums
# Default timeout used for reboot, toggle WiFi and Airplane mode,
# for the system to settle down after the operation.
DEFAULT_TIMEOUT = 10
BAND_2GHZ = 0
BAND_5GHZ = 1


class WifiManagerTest(WifiBaseTest):
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
            "open_network", "reference_networks", "iperf_server_address"
        ]
        self.unpack_userparams(
            req_param_names=req_params, opt_param_names=opt_param)

        if "AccessPoint" in self.user_params:
            self.legacy_configure_ap_and_start()

        asserts.assert_true(
            len(self.reference_networks) > 0,
            "Need at least one reference network with psk.")
        wutils.wifi_toggle_state(self.dut, True)
        if "iperf_server_address" in self.user_params:
            self.iperf_server = self.iperf_servers[0]
        self.wpapsk_2g = self.reference_networks[0]["2g"]
        self.wpapsk_5g = self.reference_networks[0]["5g"]
        self.open_network = self.open_network[0]["2g"]
        if hasattr(self, 'iperf_server'):
            self.iperf_server.start()

    def setup_test(self):
        self.dut.droid.wakeLockAcquireBright()
        self.dut.droid.wakeUpNow()
        wutils.wifi_toggle_state(self.dut, True)

    def teardown_test(self):
        self.dut.droid.wakeLockRelease()
        self.dut.droid.goToSleepNow()
        self.turn_location_off_and_scan_toggle_off()
        wutils.reset_wifi(self.dut)

    def teardown_class(self):
        if hasattr(self, 'iperf_server'):
            self.iperf_server.stop()

    def on_fail(self, test_name, begin_time):
        self.dut.take_bug_report(test_name, begin_time)
        self.dut.cat_adb_log(test_name, begin_time)

    def teardown_class(self):
        if "AccessPoint" in self.user_params:
            del self.user_params["reference_networks"]
            del self.user_params["open_network"]

    """Helper Functions"""

    def connect_to_wifi_network(self, params):
        """Connection logic for open and psk wifi networks.

        Args:
            params: A tuple of network info and AndroidDevice object.
        """
        network, ad = params
        droid = ad.droid
        ed = ad.ed
        SSID = network[WifiEnums.SSID_KEY]
        wutils.start_wifi_connection_scan_and_ensure_network_found(
            ad, SSID);
        wutils.wifi_connect(ad, network, num_of_tries=3)

    def get_connection_data(self, dut, network):
        """Get network id and ssid info from connection data.

        Args:
            dut: The Android device object under test.
            network: dict representing the network to connect to.

        Returns:
            A convenience dict with the connected network's ID and SSID.

        """
        params = (network, dut)
        self.connect_to_wifi_network(params)
        connect_data = dut.droid.wifiGetConnectionInfo()
        ssid_id_dict = dict()
        ssid_id_dict[WifiEnums.NETID_KEY] = connect_data[WifiEnums.NETID_KEY]
        ssid_id_dict[WifiEnums.SSID_KEY] = connect_data[WifiEnums.SSID_KEY]
        return ssid_id_dict

    def connect_multiple_networks(self, dut):
        """Connect to one 2.4GHz and one 5Ghz network.

        Args:
            dut: The Android device object under test.

        Returns:
            A list with the connection details for the 2GHz and 5GHz networks.

        """
        network_list = list()
        connect_2g_data = self.get_connection_data(dut, self.wpapsk_2g)
        network_list.append(connect_2g_data)
        connect_5g_data = self.get_connection_data(dut, self.wpapsk_5g)
        network_list.append(connect_5g_data)
        return network_list

    def get_enabled_network(self, network1, network2):
        """Check network status and return currently unconnected network.

        Args:
            network1: dict representing a network.
            network2: dict representing a network.

        Return:
            Network dict of the unconnected network.

        """
        wifi_info = self.dut.droid.wifiGetConnectionInfo()
        enabled = network1
        if wifi_info[WifiEnums.SSID_KEY] == network1[WifiEnums.SSID_KEY]:
            enabled = network2
        return enabled

    def check_configstore_networks(self, networks):
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
                "Length of configured networks before and after reboot don't"
                " match. \nBefore reboot = %s \n After reboot = %s" %
                (networks, network_info))
            raise signals.TestFailure(msg)
        current_count = 0
        # For each network, check if it exists in configured list after reboot
        for network in networks:
            exists = wutils.match_networks({
                WifiEnums.SSID_KEY: network[WifiEnums.SSID_KEY]
            }, network_info)
            if not len(exists):
                raise signals.TestFailure("%s network is not present in the"
                                          " configured list after reboot" %
                                          network[WifiEnums.SSID_KEY])
            # Get the new network id for each network after reboot.
            network[WifiEnums.NETID_KEY] = exists[0]['networkId']
            if exists[0]['status'] == 'CURRENT':
                current_count += 1
                # At any given point, there can only be one currently active
                # network, defined with 'status':'CURRENT'
                if current_count > 1:
                    raise signals.TestFailure("More than one network showing"
                                              "as 'CURRENT' after reboot")

    def connect_to_wifi_network_with_id(self, network_id, network_ssid):
        """Connect to the given network using network id and verify SSID.

        Args:
            network_id: int Network Id of the network.
            network_ssid: string SSID of the network.

        Returns: True if connect using network id was successful;
                 False otherwise.

        """
        wutils.start_wifi_connection_scan_and_ensure_network_found(
            self.dut, network_ssid);
        wutils.wifi_connect_by_id(self.dut, network_id)
        connect_data = self.dut.droid.wifiGetConnectionInfo()
        connect_ssid = connect_data[WifiEnums.SSID_KEY]
        self.log.debug("Expected SSID = %s Connected SSID = %s" %
                       (network_ssid, connect_ssid))
        if connect_ssid != network_ssid:
            return False
        return True

    def run_iperf_client(self, params):
        """Run iperf traffic after connection.

        Args:
            params: A tuple of network info and AndroidDevice object.
        """
        if "iperf_server_address" in self.user_params:
            wait_time = 5
            network, ad = params
            SSID = network[WifiEnums.SSID_KEY]
            self.log.info("Starting iperf traffic through {}".format(SSID))
            time.sleep(wait_time)
            port_arg = "-p {}".format(self.iperf_server.port)
            success, data = ad.run_iperf_client(self.iperf_server_address,
                                                port_arg)
            self.log.debug(pprint.pformat(data))
            asserts.assert_true(success, "Error occurred in iPerf traffic.")

    def connect_to_wifi_network_toggle_wifi_and_run_iperf(self, params):
        """ Connect to the provided network and then toggle wifi mode and wait
        for reconnection to the provided network.

        Logic steps are
        1. Connect to the network.
        2. Turn wifi off.
        3. Turn wifi on.
        4. Wait for connection to the network.
        5. Run iperf traffic.

        Args:
            params: A tuple of network info and AndroidDevice object.
       """
        network, ad = params
        self.connect_to_wifi_network(params)
        wutils.toggle_wifi_and_wait_for_reconnection(
            ad, network, num_of_tries=5)
        self.run_iperf_client(params)

    def run_iperf(self, iperf_args):
        if "iperf_server_address" not in self.user_params:
            self.log.error(("Missing iperf_server_address. "
                            "Provide one in config."))
        else:
            iperf_addr = self.user_params["iperf_server_address"]
            self.log.info("Running iperf client.")
            result, data = self.dut.run_iperf_client(iperf_addr, iperf_args)
            self.log.debug(data)

    def run_iperf_rx_tx(self, time, omit=10):
        args = "-p {} -t {} -O 10".format(self.iperf_server.port, time, omit)
        self.log.info("Running iperf client {}".format(args))
        self.run_iperf(args)
        args = "-p {} -t {} -O 10 -R".format(self.iperf_server.port, time,
                                             omit)
        self.log.info("Running iperf client {}".format(args))
        self.run_iperf(args)

    def get_energy_info(self):
        """ Steps:
            1. Check that the WiFi energy info reporting support on this device
               is as expected (support or not).
            2. If the device does not support energy info reporting as
               expected, skip the test.
            3. Call API to get WiFi energy info.
            4. Verify the values of "ControllerEnergyUsed" and
               "ControllerIdleTimeMillis" in energy info don't decrease.
            5. Repeat from Step 3 for 10 times.
        """
        # Check if dut supports energy info reporting.
        actual_support = self.dut.droid.wifiIsEnhancedPowerReportingSupported()
        model = self.dut.model
        if not actual_support:
            asserts.skip(
                ("Device %s does not support energy info reporting as "
                 "expected.") % model)
        # Verify reported values don't decrease.
        self.log.info(("Device %s supports energy info reporting, verify that "
                       "the reported values don't decrease.") % model)
        energy = 0
        idle_time = 0
        for i in range(10):
            info = self.dut.droid.wifiGetControllerActivityEnergyInfo()
            self.log.debug("Iteration %d, got energy info: %s" % (i, info))
            new_energy = info["ControllerEnergyUsed"]
            new_idle_time = info["ControllerIdleTimeMillis"]
            asserts.assert_true(new_energy >= energy,
                                "Energy value decreased: previous %d, now %d" %
                                (energy, new_energy))
            energy = new_energy
            asserts.assert_true(new_idle_time >= idle_time,
                                "Idle time decreased: previous %d, now %d" % (
                                    idle_time, new_idle_time))
            idle_time = new_idle_time
            wutils.start_wifi_connection_scan(self.dut)

    def turn_location_on_and_scan_toggle_on(self):
        """ Turns on wifi location scans.
        """
        acts.utils.set_location_service(self.dut, True)
        self.dut.droid.wifiScannerToggleAlwaysAvailable(True)
        msg = "Failed to turn on location service's scan."
        asserts.assert_true(self.dut.droid.wifiScannerIsAlwaysAvailable(), msg)

    def turn_location_off_and_scan_toggle_off(self):
        """ Turns off wifi location scans.
        """
        acts.utils.set_location_service(self.dut, False)
        self.dut.droid.wifiScannerToggleAlwaysAvailable(False)
        msg = "Failed to turn off location service's scan."
        asserts.assert_true(not self.dut.droid.wifiScannerIsAlwaysAvailable(), msg)

    def turn_location_on_and_scan_toggle_off(self):
        """ Turns off wifi location scans, but keeps location on.
        """
        acts.utils.set_location_service(self.dut, True)
        self.dut.droid.wifiScannerToggleAlwaysAvailable(False)
        msg = "Failed to turn off location service's scan."
        asserts.assert_true(not self.dut.droid.wifiScannerIsAlwaysAvailable(), msg)

    def helper_reconnect_toggle_wifi(self):
        """Connect to multiple networks, turn off/on wifi, then reconnect to
           a previously connected network.

        Steps:
        1. Connect to a 2GHz network.
        2. Connect to a 5GHz network.
        3. Turn WiFi OFF/ON.
        4. Reconnect to the non-current network.

        """
        connect_2g_data = self.get_connection_data(self.dut, self.wpapsk_2g)
        connect_5g_data = self.get_connection_data(self.dut, self.wpapsk_5g)
        wutils.toggle_wifi_off_and_on(self.dut)
        reconnect_to = self.get_enabled_network(connect_2g_data,
                                                connect_5g_data)
        reconnect = self.connect_to_wifi_network_with_id(
            reconnect_to[WifiEnums.NETID_KEY],
            reconnect_to[WifiEnums.SSID_KEY])
        if not reconnect:
            raise signals.TestFailure("Device did not connect to the correct"
                                      " network after toggling WiFi.")

    def helper_reconnect_toggle_airplane(self):
        """Connect to multiple networks, turn on/off Airplane moce, then
           reconnect a previously connected network.

        Steps:
        1. Connect to a 2GHz network.
        2. Connect to a 5GHz network.
        3. Turn ON/OFF Airplane mode.
        4. Reconnect to the non-current network.

        """
        connect_2g_data = self.get_connection_data(self.dut, self.wpapsk_2g)
        connect_5g_data = self.get_connection_data(self.dut, self.wpapsk_5g)
        wutils.toggle_airplane_mode_on_and_off(self.dut)
        reconnect_to = self.get_enabled_network(connect_2g_data,
                                                connect_5g_data)
        reconnect = self.connect_to_wifi_network_with_id(
            reconnect_to[WifiEnums.NETID_KEY],
            reconnect_to[WifiEnums.SSID_KEY])
        if not reconnect:
            raise signals.TestFailure("Device did not connect to the correct"
                                      " network after toggling Airplane mode.")

    def helper_reboot_configstore_reconnect(self):
        """Connect to multiple networks, reboot then reconnect to previously
           connected network.

        Steps:
        1. Connect to a 2GHz network.
        2. Connect to a 5GHz network.
        3. Reboot device.
        4. Verify all networks are persistent after reboot.
        5. Reconnect to the non-current network.

        """
        network_list = self.connect_multiple_networks(self.dut)
        self.dut.reboot()
        time.sleep(DEFAULT_TIMEOUT)
        self.check_configstore_networks(network_list)

        reconnect_to = self.get_enabled_network(network_list[BAND_2GHZ],
                                                network_list[BAND_5GHZ])

        reconnect = self.connect_to_wifi_network_with_id(
            reconnect_to[WifiEnums.NETID_KEY],
            reconnect_to[WifiEnums.SSID_KEY])
        if not reconnect:
            raise signals.TestFailure(
                "Device failed to reconnect to the correct"
                " network after reboot.")

    def helper_toggle_wifi_reboot_configstore_reconnect(self):
        """Connect to multiple networks, disable WiFi, reboot, then
           reconnect to previously connected network.

        Steps:
        1. Connect to a 2GHz network.
        2. Connect to a 5GHz network.
        3. Turn WiFi OFF.
        4. Reboot device.
        5. Turn WiFi ON.
        4. Verify all networks are persistent after reboot.
        5. Reconnect to the non-current network.

        """
        network_list = self.connect_multiple_networks(self.dut)
        self.log.debug("Toggling wifi OFF")
        wutils.wifi_toggle_state(self.dut, False)
        time.sleep(DEFAULT_TIMEOUT)
        self.dut.reboot()
        time.sleep(DEFAULT_TIMEOUT)
        self.log.debug("Toggling wifi ON")
        wutils.wifi_toggle_state(self.dut, True)
        time.sleep(DEFAULT_TIMEOUT)
        self.check_configstore_networks(network_list)
        reconnect_to = self.get_enabled_network(network_list[BAND_2GHZ],
                                                network_list[BAND_5GHZ])
        reconnect = self.connect_to_wifi_network_with_id(
            reconnect_to[WifiEnums.NETID_KEY],
            reconnect_to[WifiEnums.SSID_KEY])
        if not reconnect:
            msg = ("Device failed to reconnect to the correct network after"
                   " toggling WiFi and rebooting.")
            raise signals.TestFailure(msg)

    def helper_toggle_airplane_reboot_configstore_reconnect(self):
        """Connect to multiple networks, enable Airplane mode, reboot, then
           reconnect to previously connected network.

        Steps:
        1. Connect to a 2GHz network.
        2. Connect to a 5GHz network.
        3. Toggle Airplane mode ON.
        4. Reboot device.
        5. Toggle Airplane mode OFF.
        4. Verify all networks are persistent after reboot.
        5. Reconnect to the non-current network.

        """
        network_list = self.connect_multiple_networks(self.dut)
        self.log.debug("Toggling Airplane mode ON")
        asserts.assert_true(
            acts.utils.force_airplane_mode(self.dut, True),
            "Can not turn on airplane mode on: %s" % self.dut.serial)
        time.sleep(DEFAULT_TIMEOUT)
        self.dut.reboot()
        time.sleep(DEFAULT_TIMEOUT)
        self.log.debug("Toggling Airplane mode OFF")
        asserts.assert_true(
            acts.utils.force_airplane_mode(self.dut, False),
            "Can not turn on airplane mode on: %s" % self.dut.serial)
        time.sleep(DEFAULT_TIMEOUT)
        self.check_configstore_networks(network_list)
        reconnect_to = self.get_enabled_network(network_list[BAND_2GHZ],
                                                network_list[BAND_5GHZ])
        reconnect = self.connect_to_wifi_network_with_id(
            reconnect_to[WifiEnums.NETID_KEY],
            reconnect_to[WifiEnums.SSID_KEY])
        if not reconnect:
            msg = ("Device failed to reconnect to the correct network after"
                   " toggling Airplane mode and rebooting.")
            raise signals.TestFailure(msg)

    """Tests"""

    @test_tracker_info(uuid="525fc5e3-afba-4bfd-9a02-5834119e3c66")
    def test_toggle_wifi_state_and_get_startupTime(self):
        """Test toggling wifi"""
        self.log.debug("Going from on to off.")
        wutils.wifi_toggle_state(self.dut, False)
        self.log.debug("Going from off to on.")
        startTime = time.time()
        wutils.wifi_toggle_state(self.dut, True)
        startup_time = time.time() - startTime
        self.log.debug("WiFi was enabled on the device in %s s." % startup_time)

    @test_tracker_info(uuid="e9d11563-2bbe-4c96-87eb-ec919b51435b")
    def test_toggle_with_screen(self):
        """Test toggling wifi with screen on/off"""
        wait_time = 5
        self.log.debug("Screen from off to on.")
        self.dut.droid.wakeLockAcquireBright()
        self.dut.droid.wakeUpNow()
        time.sleep(wait_time)
        self.log.debug("Going from on to off.")
        try:
            wutils.wifi_toggle_state(self.dut, False)
            time.sleep(wait_time)
            self.log.debug("Going from off to on.")
            wutils.wifi_toggle_state(self.dut, True)
        finally:
            self.dut.droid.wakeLockRelease()
            time.sleep(wait_time)
            self.dut.droid.goToSleepNow()

    @test_tracker_info(uuid="71556e06-7fb1-4e2b-9338-b01f1f8e286e")
    def test_scan(self):
        """Test wifi connection scan can start and find expected networks."""
        ssid = self.open_network[WifiEnums.SSID_KEY]
        wutils.start_wifi_connection_scan_and_ensure_network_found(
            self.dut, ssid);

    @test_tracker_info(uuid="3ea09efb-6921-429e-afb1-705ef5a09afa")
    def test_scan_with_wifi_off_and_location_scan_on(self):
        """Put wifi in scan only mode"""
        self.turn_location_on_and_scan_toggle_on()
        wutils.wifi_toggle_state(self.dut, False)

        """Test wifi connection scan can start and find expected networks."""
        ssid = self.open_network[WifiEnums.SSID_KEY]
        wutils.start_wifi_connection_scan_and_ensure_network_found(
            self.dut, ssid);

    @test_tracker_info(uuid="770caebe-bcb1-43ac-95b6-5dd52dd90e80")
    def test_scan_with_wifi_off_and_location_scan_off(self):
        """Turn off wifi and location scan"""
        self.turn_location_on_and_scan_toggle_off()
        wutils.wifi_toggle_state(self.dut, False)

        """Test wifi connection scan should fail."""
        self.dut.droid.wifiStartScan()
        try:
            self.dut.ed.pop_event("WifiManagerScanResultsAvailable", 60)
        except queue.Empty:
            self.log.debug("Wifi scan results not received.")
        else:
            asserts.fail("Wi-Fi scan results received")

    @test_tracker_info(uuid="a4ad9930-a8fa-4868-81ed-a79c7483e502")
    def test_add_network(self):
        """Test wifi connection scan."""
        ssid = self.open_network[WifiEnums.SSID_KEY]
        nId = self.dut.droid.wifiAddNetwork(self.open_network)
        asserts.assert_true(nId > -1, "Failed to add network.")
        configured_networks = self.dut.droid.wifiGetConfiguredNetworks()
        self.log.debug(
            ("Configured networks after adding: %s" % configured_networks))
        wutils.assert_network_in_list({
            WifiEnums.SSID_KEY: ssid
        }, configured_networks)

    @test_tracker_info(uuid="aca85551-10ba-4007-90d9-08bcdeb16a60")
    def test_forget_network(self):
        ssid = self.open_network[WifiEnums.SSID_KEY]
        nId = self.dut.droid.wifiAddNetwork(self.open_network)
        asserts.assert_true(nId > -1, "Failed to add network.")
        configured_networks = self.dut.droid.wifiGetConfiguredNetworks()
        self.log.debug(
            ("Configured networks after adding: %s" % configured_networks))
        wutils.assert_network_in_list({
            WifiEnums.SSID_KEY: ssid
        }, configured_networks)
        wutils.wifi_forget_network(self.dut, ssid)
        configured_networks = self.dut.droid.wifiGetConfiguredNetworks()
        for nw in configured_networks:
            asserts.assert_true(
                nw[WifiEnums.BSSID_KEY] != ssid,
                "Found forgotten network %s in configured networks." % ssid)

    @test_tracker_info(uuid="b306d65c-6df3-4eb5-a178-6278bdc76c3e")
    def test_reconnect_to_connected_network(self):
        """Connect to a network and immediately issue reconnect.

        Steps:
        1. Connect to a 2GHz network.
        2. Reconnect to the network using its network id.
        3. Connect to a 5GHz network.
        4. Reconnect to the network using its network id.

        """
        connect_2g_data = self.get_connection_data(self.dut, self.wpapsk_2g)
        reconnect_2g = self.connect_to_wifi_network_with_id(
            connect_2g_data[WifiEnums.NETID_KEY],
            connect_2g_data[WifiEnums.SSID_KEY])
        if not reconnect_2g:
            raise signals.TestFailure("Device did not connect to the correct"
                                      " 2GHz network.")
        connect_5g_data = self.get_connection_data(self.dut, self.wpapsk_5g)
        reconnect_5g = self.connect_to_wifi_network_with_id(
            connect_5g_data[WifiEnums.NETID_KEY],
            connect_5g_data[WifiEnums.SSID_KEY])
        if not reconnect_5g:
            raise signals.TestFailure("Device did not connect to the correct"
                                      " 5GHz network.")

    @test_tracker_info(uuid="3cff17f6-b684-4a95-a438-8272c2ad441d")
    def test_reconnect_to_previously_connected(self):
        """Connect to multiple networks and reconnect to the previous network.

        Steps:
        1. Connect to a 2GHz network.
        2. Connect to a 5GHz network.
        3. Reconnect to the 2GHz network using its network id.
        4. Reconnect to the 5GHz network using its network id.

        """
        connect_2g_data = self.get_connection_data(self.dut, self.wpapsk_2g)
        connect_5g_data = self.get_connection_data(self.dut, self.wpapsk_5g)
        reconnect_2g = self.connect_to_wifi_network_with_id(
            connect_2g_data[WifiEnums.NETID_KEY],
            connect_2g_data[WifiEnums.SSID_KEY])
        if not reconnect_2g:
            raise signals.TestFailure("Device did not connect to the correct"
                                      " 2GHz network.")
        reconnect_5g = self.connect_to_wifi_network_with_id(
            connect_5g_data[WifiEnums.NETID_KEY],
            connect_5g_data[WifiEnums.SSID_KEY])
        if not reconnect_5g:
            raise signals.TestFailure("Device did not connect to the correct"
                                      " 5GHz network.")

    @test_tracker_info(uuid="334175c3-d26a-4c87-a8ab-8eb220b2d80f")
    def test_reconnect_toggle_wifi(self):
        """Connect to multiple networks, turn off/on wifi, then reconnect to
           a previously connected network.

        Steps:
        1. Connect to a 2GHz network.
        2. Connect to a 5GHz network.
        3. Turn WiFi OFF/ON.
        4. Reconnect to the non-current network.

        """
        self.helper_reconnect_toggle_wifi()

    @test_tracker_info(uuid="bd2cec9e-7f17-44ef-8a0c-4da92a9b55ae")
    def test_reconnect_toggle_wifi_with_location_scan_on(self):
        """Connect to multiple networks, turn off/on wifi, then reconnect to
           a previously connected network.

        Steps:
        1. Turn on location scans.
        2. Connect to a 2GHz network.
        3. Connect to a 5GHz network.
        4. Turn WiFi OFF/ON.
        5. Reconnect to the non-current network.

        """
        self.turn_location_on_and_scan_toggle_on()
        self.helper_reconnect_toggle_wifi()

    @test_tracker_info(uuid="8e6e6c21-fefb-4fe8-9fb1-f09b1182b76d")
    def test_reconnect_toggle_airplane(self):
        """Connect to multiple networks, turn on/off Airplane moce, then
           reconnect a previously connected network.

        Steps:
        1. Connect to a 2GHz network.
        2. Connect to a 5GHz network.
        3. Turn ON/OFF Airplane mode.
        4. Reconnect to the non-current network.

        """
        self.helper_reconnect_toggle_airplane()

    @test_tracker_info(uuid="28562f13-8a0a-492e-932c-e587560db5f2")
    def test_reconnect_toggle_airplane_with_location_scan_on(self):
        """Connect to multiple networks, turn on/off Airplane moce, then
           reconnect a previously connected network.

        Steps:
        1. Turn on location scans.
        2. Connect to a 2GHz network.
        3. Connect to a 5GHz network.
        4. Turn ON/OFF Airplane mode.
        5. Reconnect to the non-current network.

        """
        self.turn_location_on_and_scan_toggle_on()
        self.helper_reconnect_toggle_airplane()

    @test_tracker_info(uuid="3d041c12-05e2-46a7-ab9b-e3f60cc735db")
    def test_reboot_configstore_reconnect(self):
        """Connect to multiple networks, reboot then reconnect to previously
           connected network.

        Steps:
        1. Connect to a 2GHz network.
        2. Connect to a 5GHz network.
        3. Reboot device.
        4. Verify all networks are persistent after reboot.
        5. Reconnect to the non-current network.

        """
        self.helper_reboot_configstore_reconnect()

    @test_tracker_info(uuid="a70d5853-67b5-4d48-bdf7-08ee51fafd21")
    def test_reboot_configstore_reconnect_with_location_scan_on(self):
        """Connect to multiple networks, reboot then reconnect to previously
           connected network.

        Steps:
        1. Turn on location scans.
        2. Connect to a 2GHz network.
        3. Connect to a 5GHz network.
        4. Reboot device.
        5. Verify all networks are persistent after reboot.
        6. Reconnect to the non-current network.

        """
        self.turn_location_on_and_scan_toggle_on()
        self.helper_reboot_configstore_reconnect()

    @test_tracker_info(uuid="26d94dfa-1349-4c8b-aea0-475eb73bb521")
    def test_toggle_wifi_reboot_configstore_reconnect(self):
        """Connect to multiple networks, disable WiFi, reboot, then
           reconnect to previously connected network.

        Steps:
        1. Connect to a 2GHz network.
        2. Connect to a 5GHz network.
        3. Turn WiFi OFF.
        4. Reboot device.
        5. Turn WiFi ON.
        4. Verify all networks are persistent after reboot.
        5. Reconnect to the non-current network.

        """
        self.helper_toggle_wifi_reboot_configstore_reconnect()

    @test_tracker_info(uuid="7c004a3b-c1c6-4371-9124-0f34650be915")
    def test_toggle_wifi_reboot_configstore_reconnect_with_location_scan_on(self):
        """Connect to multiple networks, disable WiFi, reboot, then
           reconnect to previously connected network.

        Steps:
        1. Turn on location scans.
        2. Connect to a 2GHz network.
        3. Connect to a 5GHz network.
        4. Turn WiFi OFF.
        5. Reboot device.
        6. Turn WiFi ON.
        7. Verify all networks are persistent after reboot.
        8. Reconnect to the non-current network.

        """
        self.turn_location_on_and_scan_toggle_on()
        self.helper_toggle_wifi_reboot_configstore_reconnect()

    @test_tracker_info(uuid="4fce017b-b443-40dc-a598-51d59d3bb38f")
    def test_toggle_airplane_reboot_configstore_reconnect(self):
        """Connect to multiple networks, enable Airplane mode, reboot, then
           reconnect to previously connected network.

        Steps:
        1. Connect to a 2GHz network.
        2. Connect to a 5GHz network.
        3. Toggle Airplane mode ON.
        4. Reboot device.
        5. Toggle Airplane mode OFF.
        4. Verify all networks are persistent after reboot.
        5. Reconnect to the non-current network.

        """
        self.helper_toggle_airplane_reboot_configstore_reconnect()

    @test_tracker_info(uuid="7f0810f9-2338-4158-95f5-057f5a1905b6")
    def test_toggle_airplane_reboot_configstore_reconnect_with_location_scan_on(self):
        """Connect to multiple networks, enable Airplane mode, reboot, then
           reconnect to previously connected network.

        Steps:
        1. Turn on location scans.
        2. Connect to a 2GHz network.
        3. Connect to a 5GHz network.
        4. Toggle Airplane mode ON.
        5. Reboot device.
        6. Toggle Airplane mode OFF.
        7. Verify all networks are persistent after reboot.
        8. Reconnect to the non-current network.

        """
        self.turn_location_on_and_scan_toggle_on()
        self.helper_toggle_airplane_reboot_configstore_reconnect()

    @test_tracker_info(uuid="81eb7527-4c92-4422-897a-6b5f6445e84a")
    def test_config_store_with_wpapsk_2g(self):
        self.connect_to_wifi_network_toggle_wifi_and_run_iperf(
            (self.wpapsk_2g, self.dut))

    @test_tracker_info(uuid="8457903d-cb7e-4c89-bcea-7f59585ea6e0")
    def test_config_store_with_wpapsk_5g(self):
        self.connect_to_wifi_network_toggle_wifi_and_run_iperf(
            (self.wpapsk_5g, self.dut))

    @test_tracker_info(uuid="b9fbc13a-47b4-4f64-bd2c-e5a3cb24ab2f")
    def test_tdls_supported(self):
        model = self.dut.model
        self.log.debug("Model is %s" % model)
        if not model.startswith("volantis"):
            asserts.assert_true(self.dut.droid.wifiIsTdlsSupported(), (
                "TDLS should be supported on %s, but device is "
                "reporting not supported.") % model)
        else:
            asserts.assert_false(self.dut.droid.wifiIsTdlsSupported(), (
                "TDLS should not be supported on %s, but device "
                "is reporting supported.") % model)

    @test_tracker_info(uuid="50637d40-ea59-4f4b-9fc1-e6641d64074c")
    def test_energy_info(self):
        """Verify the WiFi energy info reporting feature """
        self.get_energy_info()

    @test_tracker_info(uuid="1f1cf549-53eb-4f36-9f33-ce06c9158efc")
    def test_energy_info_connected(self):
        """Verify the WiFi energy info reporting feature when connected.

        Connect to a wifi network, then the same as test_energy_info.
        """
        wutils.wifi_connect(self.dut, self.open_network)
        self.get_energy_info()
