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
import acts.test_utils.wifi.wifi_test_utils as wutils


import WifiManagerTest
from acts import asserts
from acts import signals
from acts.test_decorators import test_tracker_info
from acts.utils import force_airplane_mode

WifiEnums = wutils.WifiEnums

DEFAULT_TIMEOUT = 10
GLOBAL_RE = 0
BOINGO = 1
UNKNOWN_FQDN = "@#@@!00fffffx"

class WifiPasspointTest(acts.base_test.BaseTestClass):
    """Tests for APIs in Android's WifiManager class.

    Test Bed Requirement:
    * One Android device
    * Several Wi-Fi networks visible to the device, including an open Wi-Fi
      network.
    """

    def setup_class(self):
        self.dut = self.android_devices[0]
        wutils.wifi_test_device_init(self.dut)
        req_params = ("passpoint_networks",)
        self.unpack_userparams(req_params)
        asserts.assert_true(
            len(self.passpoint_networks) > 0,
            "Need at least one Passpoint network.")
        wutils.wifi_toggle_state(self.dut, True)
        self.unknown_fqdn = UNKNOWN_FQDN


    def setup_test(self):
        self.dut.droid.wakeLockAcquireBright()
        self.dut.droid.wakeUpNow()


    def teardown_test(self):
        self.dut.droid.wakeLockRelease()
        self.dut.droid.goToSleepNow()
        wutils.reset_wifi(self.dut)


    def on_fail(self, test_name, begin_time):
        self.dut.take_bug_report(test_name, begin_time)


    """Helper Functions"""


    def install_passpoint_profile(self, passpoint_config):
        """Install the Passpoint network Profile.

        Args:
            passpoint_config: A JSON dict of the Passpoint configuration.

        """
        asserts.assert_true(WifiEnums.SSID_KEY in passpoint_config,
                "Key '%s' must be present in network definition." %
                WifiEnums.SSID_KEY)
        # Install the Passpoint profile.
        self.dut.droid.addUpdatePasspointConfig(passpoint_config)


    def check_passpoint_connection(self, passpoint_network):
        """Verify the device is automatically able to connect to the Passpoint
           network.

           Args:
               passpoint_network: SSID of the Passpoint network.

        """
        ad = self.dut
        ad.ed.clear_all_events()
        wutils.start_wifi_connection_scan(ad)
        scan_results = ad.droid.wifiGetScanResults()
        # Wait for scan to complete.
        time.sleep(5)
        ssid = passpoint_network
        wutils.assert_network_in_list({WifiEnums.SSID_KEY: ssid}, scan_results)
        # Passpoint network takes longer time to connect than normal networks.
        # Every try comes with a timeout of 30s. Setting total timeout to 120s.
        wutils.wifi_passpoint_connect(self.dut, passpoint_network, num_of_tries=4)
        # Re-verify we are connected to the correct network.
        network_info = self.dut.droid.wifiGetConnectionInfo()
        if network_info[WifiEnums.SSID_KEY] != passpoint_network:
            raise signals.TestFailure("Device did not connect to the passpoint"
                                      " network.")


    def get_configured_passpoint_and_delete(self):
        """Get configured Passpoint network and delete using its FQDN."""
        passpoint_config = self.dut.droid.getPasspointConfigs()
        if not len(passpoint_config):
            raise signals.TestFailure("Failed to fetch the list of configured"
                                      "passpoint networks.")
        if not wutils.delete_passpoint(self.dut, passpoint_config[0]):
            raise signals.TestFailure("Failed to delete Passpoint configuration"
                                      " with FQDN = %s" % passpoint_config[0])

    """Tests"""

    @test_tracker_info(uuid="b0bc0153-77bb-4594-8f19-cea2c6bd2f43")
    def test_add_passpoint_network(self):
        """Add a Passpoint network and verify device connects to it.

        Steps:
            1. Install a Passpoint Profile.
            2. Verify the device connects to the required Passpoint SSID.
            3. Get the Passpoint configuration added above.
            4. Delete Passpoint configuration using its FQDN.
            5. Verify that we are disconnected from the Passpoint network.

        """
        passpoint_config = self.passpoint_networks[BOINGO]
        self.install_passpoint_profile(passpoint_config)
        ssid = passpoint_config[WifiEnums.SSID_KEY]
        self.check_passpoint_connection(ssid)
        self.get_configured_passpoint_and_delete()
        wutils.wait_for_disconnect(self.dut)


    @test_tracker_info(uuid="eb29d6e2-a755-4c9c-9e4e-63ea2277a64a")
    def test_update_passpoint_network(self):
        """Update a previous Passpoint network and verify device still connects
           to it.

        1. Install a Passpoint Profile.
        2. Verify the device connects to the required Passpoint SSID.
        3. Update the Passpoint Profile.
        4. Verify device is still connected to the Passpoint SSID.
        5. Get the Passpoint configuration added above.
        6. Delete Passpoint configuration using its FQDN.

        """
        passpoint_config = self.passpoint_networks[BOINGO]
        self.install_passpoint_profile(passpoint_config)
        ssid = passpoint_config[WifiEnums.SSID_KEY]
        self.check_passpoint_connection(ssid)

        # Update passpoint configuration using the original profile because we
        # do not have real profile with updated credentials to use.
        self.install_passpoint_profile(passpoint_config)

        # Wait for a Disconnect event from the supplicant.
        wutils.wait_for_disconnect(self.dut)

        # Now check if we are again connected with the updated profile.
        self.check_passpoint_connection(ssid)

        self.get_configured_passpoint_and_delete()
        wutils.wait_for_disconnect(self.dut)


    @test_tracker_info(uuid="b6e8068d-faa1-49f2-b421-c60defaed5f0")
    def test_add_delete_list_of_passpoint_network(self):
        """Add multiple passpoint networks, list them and delete one by one.

        1. Install Passpoint Profile A.
        2. Install Passpoint Profile B.
        3. Get all the Passpoint configurations added above and verify.
        6. Ensure all Passpoint configurations can be deleted.

        """
        for passpoint_config in self.passpoint_networks:
            self.install_passpoint_profile(passpoint_config)
            time.sleep(DEFAULT_TIMEOUT)
        configs = self.dut.droid.getPasspointConfigs()
        if not len(configs) or len(configs) != len(self.passpoint_networks):
            raise signals.TestFailure("Failed to fetch some or all of the"
                                      " configured passpoint networks.")
        for config in configs:
            if not wutils.delete_passpoint(self.dut, config):
                raise signals.TestFailure("Failed to delete Passpoint"
                                          " configuration with FQDN = %s" %
                                          config)


    @test_tracker_info(uuid="a53251be-7aaf-41fc-a5f3-63984269d224")
    def test_delete_unknown_fqdn(self):
        """Negative test to delete Passpoint profile using an unknown FQDN.

        1. Pass an unknown FQDN for removal.
        2. Verify that it was not successful.

        """
        if wutils.delete_passpoint(self.dut, self.unknown_fqdn):
            raise signals.TestFailure("Failed because an unknown FQDN"
                                      " was successfully deleted.")


    @test_tracker_info(uuid="bf03c03a-e649-4e2b-a557-1f791bd98951")
    def test_passpoint_failover(self):
        """Add a pair of passpoint networks and test failover when one of the"
           profiles is removed.

        1. Install a Passpoint Profile A and B.
        2. Verify device connects to a Passpoint network and get SSID.
        3. Delete the current Passpoint profile using its FQDN.
        4. Verify device fails over and connects to the other Passpoint SSID.
        5. Delete Passpoint configuration using its FQDN.

        """
        # Install both Passpoint profiles on the device.
        passpoint_ssid = list()
        for passpoint_config in self.passpoint_networks:
            passpoint_ssid.append(passpoint_config[WifiEnums.SSID_KEY])
            self.install_passpoint_profile(passpoint_config)
            time.sleep(DEFAULT_TIMEOUT)

        # Get the current network and the failover network.
        wutils.wait_for_connect(self.dut)
        current_passpoint = self.dut.droid.wifiGetConnectionInfo()
        current_ssid = current_passpoint[WifiEnums.SSID_KEY]
        if current_ssid not in passpoint_ssid:
           raise signals.TestFailure("Device did not connect to any of the "
                                     "configured Passpoint networks.")

        expected_ssid =  self.passpoint_networks[0][WifiEnums.SSID_KEY]
        if current_ssid == expected_ssid:
            expected_ssid = self.passpoint_networks[1][WifiEnums.SSID_KEY]

        # Remove the current Passpoint profile.
        for network in self.passpoint_networks:
            if network[WifiEnums.SSID_KEY] == current_ssid:
                if not wutils.delete_passpoint(self.dut, network["fqdn"]):
                    raise signals.TestFailure("Failed to delete Passpoint"
                                              " configuration with FQDN = %s" %
                                              network["fqdn"])
        # Verify device fails over and connects to the other passpoint network.
        time.sleep(DEFAULT_TIMEOUT)

        current_passpoint = self.dut.droid.wifiGetConnectionInfo()
        if current_passpoint[WifiEnums.SSID_KEY] != expected_ssid:
            raise signals.TestFailure("Device did not failover to the %s"
                                      " passpoint network" % expected_ssid)

        # Delete the remaining Passpoint profile.
        self.get_configured_passpoint_and_delete()
        wutils.wait_for_disconnect(self.dut)
