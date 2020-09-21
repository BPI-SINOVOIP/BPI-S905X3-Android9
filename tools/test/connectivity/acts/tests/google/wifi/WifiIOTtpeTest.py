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
import time

import acts.signals
import acts.test_utils.wifi.wifi_test_utils as wutils

from acts import asserts
from acts.test_decorators import test_tracker_info
from acts.test_utils.wifi.WifiBaseTest import WifiBaseTest

WifiEnums = wutils.WifiEnums


class WifiIOTtpeTest(WifiBaseTest):
    """ Tests for wifi IOT

        Test Bed Requirement:
          * One Android device
          * Wi-Fi IOT networks visible to the device
    """

    def __init__(self, controllers):
        self.attenuators = None
        WifiBaseTest.__init__(self, controllers)

    def setup_class(self):
        self.dut = self.android_devices[0]
        wutils.wifi_test_device_init(self.dut)

        req_params = [ "iot_networks", ]
        opt_params = [ "open_network", "iperf_server_address" ]
        self.unpack_userparams(req_param_names=req_params,
                               opt_param_names=opt_params)

        asserts.assert_true(
            len(self.iot_networks) > 0,
            "Need at least one iot network with psk.")

        if getattr(self, 'open_network', False):
            self.iot_networks.append(self.open_network)

        wutils.wifi_toggle_state(self.dut, True)
        if "iperf_server_address" in self.user_params:
            self.iperf_server = self.iperf_servers[0]
            self.iperf_server.start()

        # create hashmap for testcase name and SSIDs
        self.iot_test_prefix = "test_iot_connection_to_"
        self.ssid_map = {}
        for network in self.iot_networks:
            SSID = network['SSID'].replace('-','_')
            self.ssid_map[SSID] = network

    def setup_test(self):
        self.dut.droid.wakeLockAcquireBright()
        self.dut.droid.wakeUpNow()

    def teardown_test(self):
        self.dut.droid.wakeLockRelease()
        self.dut.droid.goToSleepNow()
        wutils.reset_wifi(self.dut)

    def teardown_class(self):
        if "iperf_server_address" in self.user_params:
            self.iperf_server.stop()

    def on_fail(self, test_name, begin_time):
        self.dut.take_bug_report(test_name, begin_time)
        self.dut.cat_adb_log(test_name, begin_time)

    """Helper Functions"""

    def connect_to_wifi_network(self, network):
        """Connection logic for open and psk wifi networks.

        Args:
            params: Dictionary with network info.
        """
        SSID = network[WifiEnums.SSID_KEY]
        self.dut.ed.clear_all_events()
        wutils.start_wifi_connection_scan(self.dut)
        scan_results = self.dut.droid.wifiGetScanResults()
        wutils.assert_network_in_list({WifiEnums.SSID_KEY: SSID}, scan_results)
        wutils.wifi_connect(self.dut, network, num_of_tries=3)

    def run_iperf_client(self, network):
        """Run iperf traffic after connection.

        Args:
            params: Dictionary with network info.
        """
        if "iperf_server_address" in self.user_params:
            wait_time = 5
            SSID = network[WifiEnums.SSID_KEY]
            self.log.info("Starting iperf traffic through {}".format(SSID))
            time.sleep(wait_time)
            port_arg = "-p {}".format(self.iperf_server.port)
            success, data = self.dut.run_iperf_client(self.iperf_server_address,
                                                      port_arg)
            self.log.debug(pprint.pformat(data))
            asserts.assert_true(success, "Error occurred in iPerf traffic.")

    def connect_to_wifi_network_and_run_iperf(self, network):
        """Connection logic for open and psk wifi networks.

        Logic steps are
        1. Connect to the network.
        2. Run iperf traffic.

        Args:
            params: A dictionary with network info.
        """
        self.connect_to_wifi_network(network)
        self.run_iperf_client(network)

    """Tests"""

    @test_tracker_info(uuid="0e4ad6ed-595c-4629-a4c9-c6be9c3c58e0")
    def test_iot_connection_to_ASUS_RT_AC68U_2G(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="a76d8acc-808e-4a5d-a52b-5ba07d07b810")
    def test_iot_connection_to_ASUS_RT_AC68U_5G(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="659a3e5e-07eb-4905-9cda-92e959c7b674")
    def test_iot_connection_to_D_Link_DIR_868L_2G(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="6bcfd736-30fc-48a8-b4fb-723d1d113f3c")
    def test_iot_connection_to_D_Link_DIR_868L_5G(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="c9da945a-2c4a-44e1-881d-adf307b39b21")
    def test_iot_connection_to_TP_LINK_WR940N_2G(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="db0d224d-df81-401f-bf35-08ad02e41a71")
    def test_iot_connection_to_ASUS_RT_N66U_2G(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="845ff1d6-618d-40f3-81c3-6ed3a0751fde")
    def test_iot_connection_to_ASUS_RT_N66U_5G(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="6908039b-ccc9-4777-a0f1-3494ce642014")
    def test_iot_connection_to_ASUS_RT_AC54U_2G(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="2647c15f-2aad-47d7-8dee-b2ee1ac4cef6")
    def test_iot_connection_to_ASUS_RT_AC54U_5G(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="99678f66-ddf1-454d-87e4-e55177ec380d")
    def test_iot_connection_to_ASUS_RT_N56U_2G(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="4dd75e81-9a8e-44fd-9449-09f5ab8a63c3")
    def test_iot_connection_to_ASUS_RT_N56U_5G(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="315397ce-50d5-4abf-a11c-1abcaef832d3")
    def test_iot_connection_to_BELKIN_F9K1002v1_2G(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="05ba464a-b1ef-4ac1-a32f-c919ec4aa1dd")
    def test_iot_connection_to_CISCO_E1200_2G(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="04912868-4a47-40ce-877e-4e4c89849557")
    def test_iot_connection_to_TP_LINK_C2_2G(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="53517a21-3802-4185-b8bb-6eaace063a42")
    def test_iot_connection_to_TP_LINK_C2_5G(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="71c08c1c-415d-4da4-a151-feef43fb6ad8")
    def test_iot_connection_to_ASUS_RT_AC66U_2G(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="2322c155-07d1-47c9-bd21-2e358e3df6ee")
    def test_iot_connection_to_ASUS_RT_AC66U_5G(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])
