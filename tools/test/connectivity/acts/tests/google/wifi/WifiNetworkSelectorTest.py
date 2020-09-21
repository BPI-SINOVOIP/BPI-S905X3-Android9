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

import logging
import time

import acts.signals as signals

from acts import asserts
from acts import base_test
from acts.controllers import android_device
from acts.controllers import attenuator
from acts.test_decorators import test_tracker_info
from acts.test_utils.wifi import wifi_test_utils as wutils
from acts.test_utils.wifi.WifiBaseTest import WifiBaseTest

WifiEnums = wutils.WifiEnums

AP_1 = 0
AP_2 = 1
AP_1_2G_ATTENUATOR = 0
AP_1_5G_ATTENUATOR = 1
AP_2_2G_ATTENUATOR = 2
AP_2_5G_ATTENUATOR = 3
ATTENUATOR_INITIAL_SETTING = 60
# WifiNetworkSelector imposes a 10 seconds gap between two selections
NETWORK_SELECTION_TIME_GAP = 12


class WifiNetworkSelectorTest(WifiBaseTest):
    """These tests verify the behavior of the Android Wi-Fi Network Selector
    feature.
    """

    def __init__(self, controllers):
        WifiBaseTest.__init__(self, controllers)

    def setup_class(self):
        self.dut = self.android_devices[0]
        wutils.wifi_test_device_init(self.dut)
        req_params = []
        opt_param = ["open_network", "reference_networks"]
        self.unpack_userparams(
            req_param_names=req_params, opt_param_names=opt_param)

        if "AccessPoint" in self.user_params:
            self.legacy_configure_ap_and_start(ap_count=2)

    def setup_test(self):
        #reset and clear all saved networks on the DUT
        wutils.reset_wifi(self.dut)
        #move the APs out of range
        for attenuator in self.attenuators:
            attenuator.set_atten(ATTENUATOR_INITIAL_SETTING)
        #turn on the screen
        self.dut.droid.wakeLockAcquireBright()
        self.dut.droid.wakeUpNow()
        self.dut.ed.clear_all_events()

    def teardown_test(self):
        #turn off the screen
        self.dut.droid.wakeLockRelease()
        self.dut.droid.goToSleepNow()

    def on_fail(self, test_name, begin_time):
        self.dut.take_bug_report(test_name, begin_time)
        self.dut.cat_adb_log(test_name, begin_time)

    def teardown_class(self):
        if "AccessPoint" in self.user_params:
            del self.user_params["reference_networks"]
            del self.user_params["open_network"]

    """ Helper Functions """

    def add_networks(self, ad, networks):
        """Add Wi-Fi networks to an Android device and verify the networks were
        added correctly.

        Args:
            ad: the AndroidDevice object to add networks to.
            networks: a list of dicts, each dict represents a Wi-Fi network.
        """
        for network in networks:
            ret = ad.droid.wifiAddNetwork(network)
            asserts.assert_true(ret != -1, "Failed to add network %s" %
                                network)
            ad.droid.wifiEnableNetwork(ret, 0)
        configured_networks = ad.droid.wifiGetConfiguredNetworks()
        logging.debug("Configured networks: %s", configured_networks)

    def connect_and_verify_connected_bssid(self, expected_bssid):
        """Start a scan to get the DUT connected to an AP and verify the DUT
        is connected to the correct BSSID.

        Args:
            expected_bssid: Network bssid to which connection.

        Returns:
            True if connection to given network happen, else return False.
        """
        #wait for the attenuator to stablize
        time.sleep(10)
        #force start a single scan so we don't have to wait for the
        #WCM scheduled scan.
        wutils.start_wifi_connection_scan(self.dut)
        #wait for connection
        time.sleep(20)
        #verify connection
        actual_network = self.dut.droid.wifiGetConnectionInfo()
        logging.info("Actual network: %s", actual_network)
        try:
            asserts.assert_equal(expected_bssid,
                                 actual_network[WifiEnums.BSSID_KEY])
        except:
           msg = "Device did not connect to any network."
           raise signals.TestFailure(msg)

    """ Tests Begin """

    @test_tracker_info(uuid="ffa5e278-db3f-4e17-af11-6c7a3e7c5cc2")
    def test_network_selector_automatic_connection(self):
        """
            1. Add one saved network to DUT.
            2. Move the DUT in range.
            3. Verify the DUT is connected to the network.
        """
        #add a saved network to DUT
        networks = [self.reference_networks[AP_1]['5g']]
        self.add_networks(self.dut, networks)
        #move the DUT in range
        self.attenuators[AP_1_5G_ATTENUATOR].set_atten(0)
        #verify
        self.connect_and_verify_connected_bssid(self.reference_networks[AP_1][
            '5g']['bssid'])

    @test_tracker_info(uuid="3ea818f2-10d7-4aad-bfab-7d8fb25aae78")
    def test_network_selector_basic_connection_prefer_5g(self):
        """
            1. Add one saved SSID with 2G and 5G BSSIDs of similar RSSI.
            2. Move the DUT in range.
            3. Verify the DUT is connected to the 5G BSSID.
        """
        #add a saved network with both 2G and 5G BSSIDs to DUT
        # TODO: bmahadev Change this to a single SSID once we migrate tests to
        # use dynamic AP.
        networks = [self.reference_networks[AP_1]['2g'],
                    self.reference_networks[AP_1]['5g']]
        self.add_networks(self.dut, networks)
        #move the DUT in range
        self.attenuators[AP_1_2G_ATTENUATOR].set_atten(0)
        self.attenuators[AP_1_5G_ATTENUATOR].set_atten(0)
        #verify
        self.connect_and_verify_connected_bssid(self.reference_networks[AP_1][
            '5g']['bssid'])

    @test_tracker_info(uuid="bebb29ca-4486-4cde-b390-c5f8f2e1580c")
    def test_network_selector_prefer_stronger_rssi(self):
        """
            1. Add two saved SSIDs to DUT, same band, one has stronger RSSI
               than the other.
            2. Move the DUT in range.
            3. Verify the DUT is connected to the SSID with stronger RSSI.
        """
        #add a 2G and a 5G saved network to DUT
        networks = [
            self.reference_networks[AP_1]['2g'], self.reference_networks[AP_2][
                '2g']
        ]
        self.add_networks(self.dut, networks)
        #move the DUT in range
        self.attenuators[AP_1_2G_ATTENUATOR].set_atten(20)
        self.attenuators[AP_2_2G_ATTENUATOR].set_atten(40)
        #verify
        self.connect_and_verify_connected_bssid(self.reference_networks[AP_1][
            '2g']['bssid'])

    @test_tracker_info(uuid="f9f72dc5-034f-4fe2-a27d-df1b6cae76cd")
    def test_network_selector_prefer_secure_over_open_network(self):
        """
            1. Add two saved networks to DUT, same band, similar RSSI, one uses
               WPA2 security, the other is open.
            2. Move the DUT in range.
            3. Verify the DUT is connected to the secure network that uses WPA2.
        """
        #add a open network and a secure saved network to DUT
        networks = [
            self.open_network[AP_1]['5g'], self.reference_networks[AP_1]['5g']
        ]
        self.add_networks(self.dut, networks)
        #move the DUT in range
        #TODO: control open network attenuator
        self.attenuators[AP_1_5G_ATTENUATOR].set_atten(0)
        #verify
        self.connect_and_verify_connected_bssid(self.reference_networks[AP_1][
            '5g']['bssid'])

    @test_tracker_info(uuid="ab2c527c-0f9c-4f09-a13f-e3f461b7da52")
    def test_network_selector_blacklist_by_connection_failure(self):
        """
            1. Add two saved secured networks X and Y to DUT. X has stronger
               RSSI than Y. X has wrong password configured.
            2. Move the DUT in range.
            3. Verify the DUT is connected to network Y.
        """
        #add two saved networks to DUT, and one of them is configured with incorrect password
        wrong_passwd_network = self.reference_networks[AP_1]['5g'].copy()
        wrong_passwd_network['password'] += 'haha'
        networks = [wrong_passwd_network, self.reference_networks[AP_2]['5g']]
        self.add_networks(self.dut, networks)
        #make both AP_1 5G and AP_2 5G in range, and AP_1 5G has stronger RSSI than AP_2 5G
        self.attenuators[AP_1_5G_ATTENUATOR].set_atten(0)
        self.attenuators[AP_2_5G_ATTENUATOR].set_atten(10)
        #start 3 scans to get AP_1 5G blacklisted because of the incorrect password
        count = 0
        while count < 3:
            wutils.start_wifi_connection_scan(self.dut)
            time.sleep(NETWORK_SELECTION_TIME_GAP)
            count += 1
        #verify
        self.connect_and_verify_connected_bssid(self.reference_networks[AP_2][
            '5g']['bssid'])

    @test_tracker_info(uuid="71d88fcf-c7b8-4fd2-a7cb-84ac4a130ecf")
    def test_network_selector_2g_to_5g_prefer_same_SSID(self):
        """
            1. Add SSID_A and SSID_B to DUT. Both SSIDs have both 2G and 5G
               BSSIDs.
            2. Attenuate the networks so that the DUT is connected to SSID_A's
               2G in the beginning.
            3. Increase the RSSI of both SSID_A's 5G and SSID_B's 5G.
            4. Verify the DUT switches to SSID_A's 5G.
        """
        #add two saved networks to DUT
        networks = [
            self.reference_networks[AP_1]['2g'], self.reference_networks[AP_2][
                '2g']
        ]
        self.add_networks(self.dut, networks)
        #make AP_1 2G in range
        self.attenuators[AP_1_2G_ATTENUATOR].set_atten(0)
        #verify
        self.connect_and_verify_connected_bssid(self.reference_networks[AP_1][
            '2g']['bssid'])
        #make both AP_1 and AP_2 5G in range with similar RSSI
        self.attenuators[AP_1_5G_ATTENUATOR].set_atten(0)
        self.attenuators[AP_2_5G_ATTENUATOR].set_atten(0)
        #ensure the time gap between two network selections
        time.sleep(NETWORK_SELECTION_TIME_GAP)
        #verify
        self.connect_and_verify_connected_bssid(self.reference_networks[AP_1][
            '5g']['bssid'])

    @test_tracker_info(uuid="c1243cf4-d96e-427e-869e-3d640bee3f28")
    def test_network_selector_2g_to_5g_different_ssid(self):
        """
            1. Add SSID_A and SSID_B to DUT. Both SSIDs have both 2G and 5G
               BSSIDs.
            2. Attenuate the networks so that the DUT is connected to SSID_A's
               2G in the beginning.
            3. Increase the RSSI of SSID_B's 5G while attenuate down SSID_A's
               2G RSSI.
            4. Verify the DUT switches to SSID_B's 5G.
        """
        #add two saved networks to DUT
        networks = [
            self.reference_networks[AP_1]['2g'], self.reference_networks[AP_2][
                '2g']
        ]
        self.add_networks(self.dut, networks)
        #make both AP_1 2G and AP_2 5G in range, and AP_1 2G
        #has much stronger RSSI than AP_2 5G
        self.attenuators[AP_1_2G_ATTENUATOR].set_atten(0)
        self.attenuators[AP_2_5G_ATTENUATOR].set_atten(20)
        #verify
        self.connect_and_verify_connected_bssid(self.reference_networks[AP_1][
            '2g']['bssid'])
        #bump up AP_2 5G RSSI and reduce AP_1 2G RSSI
        self.attenuators[AP_1_2G_ATTENUATOR].set_atten(40)
        self.attenuators[AP_2_5G_ATTENUATOR].set_atten(0)
        #ensure the time gap between two network selections
        time.sleep(NETWORK_SELECTION_TIME_GAP)
        #verify
        self.connect_and_verify_connected_bssid(self.reference_networks[AP_2][
            '5g']['bssid'])

    @test_tracker_info(uuid="10da95df-83ed-4447-89f8-735b08dbe2eb")
    def test_network_selector_5g_to_2g_same_ssid(self):
        """
            1. Add one SSID that has both 2G and 5G to the DUT.
            2. Attenuate down the 2G RSSI.
            3. Connect the DUT to the 5G BSSID.
            4. Bring up the 2G RSSI and attenuate down the 5G RSSI.
            5. Verify the DUT switches to the 2G BSSID.
        """
        #add a saved network to DUT
        networks = [self.reference_networks[AP_1]['2g']]
        self.add_networks(self.dut, networks)
        #make both AP_1 2G and AP_2 5G in range, and AP_1 5G
        #has much stronger RSSI than AP_2 2G
        self.attenuators[AP_1_5G_ATTENUATOR].set_atten(0)
        self.attenuators[AP_1_2G_ATTENUATOR].set_atten(50)
        #verify
        self.connect_and_verify_connected_bssid(self.reference_networks[AP_1][
            '5g']['bssid'])
        #bump up AP_1 2G RSSI and reduce AP_1 5G RSSI
        self.attenuators[AP_1_2G_ATTENUATOR].set_atten(0)
        self.attenuators[AP_1_5G_ATTENUATOR].set_atten(30)
        #ensure the time gap between two network selections
        time.sleep(NETWORK_SELECTION_TIME_GAP)
        #verify
        self.connect_and_verify_connected_bssid(self.reference_networks[AP_1][
            '2g']['bssid'])

    @test_tracker_info(uuid="ead78ae0-27ab-4bb8-ae77-0b9fe588436a")
    def test_network_selector_stay_on_sufficient_network(self):
        """
            1. Add two 5G WPA2 BSSIDs X and Y to the DUT. X has higher RSSI
               than Y.
            2. Connect the DUT to X.
            3. Change attenuation so that Y's RSSI goes above X's.
            4. Verify the DUT stays on X.
        """
        #add two saved networks to DUT
        networks = [
            self.reference_networks[AP_1]['5g'], self.reference_networks[AP_2][
                '5g']
        ]
        self.add_networks(self.dut, networks)
        #make both AP_1 5G and AP_2 5G in range, and AP_1 5G
        #has stronger RSSI than AP_2 5G
        self.attenuators[AP_1_5G_ATTENUATOR].set_atten(10)
        self.attenuators[AP_2_5G_ATTENUATOR].set_atten(20)
        #verify
        self.connect_and_verify_connected_bssid(self.reference_networks[AP_1][
            '5g']['bssid'])
        #bump up AP_2 5G RSSI over AP_1 5G RSSI
        self.attenuators[AP_2_5G_ATTENUATOR].set_atten(0)
        #ensure the time gap between two network selections
        time.sleep(NETWORK_SELECTION_TIME_GAP)
        #verify
        self.connect_and_verify_connected_bssid(self.reference_networks[AP_1][
            '5g']['bssid'])

    @test_tracker_info(uuid="5470010f-8b62-4b1c-8b83-1f91422eced0")
    def test_network_selector_stay_on_user_selected_network(self):
        """
            1. Connect the DUT to SSID_A with a very low RSSI via the user select code path.
            2. Add SSID_B to the DUT as saved network. SSID_B has higher RSSI than SSID_A.
            3. Start a scan and network selection.
            4. Verify DUT stays on SSID_A.
        """
        #make AP_1 5G in range with a low RSSI
        self.attenuators[AP_1_5G_ATTENUATOR].set_atten(10)
        #connect to AP_1 via user selection
        wutils.wifi_connect(self.dut, self.reference_networks[AP_1]['5g'])
        #verify
        self.connect_and_verify_connected_bssid(self.reference_networks[AP_1][
            '5g']['bssid'])
        #make AP_2 5G in range with a strong RSSI
        self.attenuators[AP_2_5G_ATTENUATOR].set_atten(0)
        #add AP_2 as a saved network to DUT
        networks = [self.reference_networks[AP_2]['5g']]
        self.add_networks(self.dut, networks)
        #ensure the time gap between two network selections
        time.sleep(NETWORK_SELECTION_TIME_GAP)
        #verify we are still connected to AP_1 5G
        self.connect_and_verify_connected_bssid(self.reference_networks[AP_1][
            '5g']['bssid'])

    @test_tracker_info(uuid="f08d8f73-8c94-42af-bba9-4c49bbf16420")
    def test_network_selector_reselect_after_forget_network(self):
        """
            1. Add two 5G BSSIDs X and Y to the DUT. X has higher RSSI
               than Y.
            2. Connect the DUT to X.
            3. Forget X.
            5. Verify the DUT reselect and connect to Y.
        """
        #add two saved networks to DUT
        networks = [
            self.reference_networks[AP_1]['5g'], self.reference_networks[AP_2][
                '5g']
        ]
        self.add_networks(self.dut, networks)
        #make both AP_1 5G and AP_2 5G in range. AP_1 5G has stronger
        #RSSI than AP_2 5G
        self.attenuators[AP_1_5G_ATTENUATOR].set_atten(0)
        self.attenuators[AP_2_5G_ATTENUATOR].set_atten(10)
        #verify
        self.connect_and_verify_connected_bssid(self.reference_networks[AP_1][
            '5g']['bssid'])
        #forget AP_1
        wutils.wifi_forget_network(self.dut,
                                   self.reference_networks[AP_1]['5g']['SSID'])
        #verify
        self.connect_and_verify_connected_bssid(self.reference_networks[AP_2][
            '5g']['bssid'])
