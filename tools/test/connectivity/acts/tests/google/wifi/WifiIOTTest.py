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


class WifiIOTTest(WifiBaseTest):
    """ Tests for wifi IOT

        Test Bed Requirement:
          * One Android device
          * Wi-Fi IOT networks visible to the device
    """

    def __init__(self, controllers):
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

        if self.open_network:
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

    @test_tracker_info(uuid="a57cc861-b6c2-47e4-9db6-7a3ab32c6e20")
    def iot_connection_to_ubiquity_ap1_2g(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="2065c2f7-2b89-4da7-a15d-e5dc17b88d52")
    def iot_connection_to_ubiquity_ap1_5g(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="6870e35b-f7a7-45bf-b021-fea049ae53de")
    def test_iot_connection_to_AirportExpress_2G(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="95f4b405-79d7-4873-a152-4384acc88f41")
    def test_iot_connection_to_AirportExpress_5G(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="02a8cc75-6781-4153-8d90-bed7568a1e78")
    def iot_connection_to_AirportExtreme_2G(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="83a42c97-1358-4ba7-bdb2-238fdb1c945e")
    def iot_connection_to_AirportExtreme_5G(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="d56cc46a-f772-4c96-b84e-4e05c82f5f9d")
    def test_iot_connection_to_AirportExtremeOld_2G(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="4b57277d-ea96-4379-bd71-8b4f03253ec8")
    def test_iot_connection_to_AirportExtremeOld_5G(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="2503d9ed-35df-4be0-b838-590324cecaee")
    def test_iot_connection_to_Dlink_AC1200_2G(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="0a44e148-a4bf-43f4-88eb-e4c1ffa850ce")
    def test_iot_connection_to_Dlink_AC1200_5G(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="6bd77417-089f-4fb1-b4c2-2cd673c64bcb")
    def test_iot_connection_to_Dlink_AC3200_2G(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="9fbff6e7-36c8-4342-9c29-bce6a8ef04ec")
    def test_iot_connection_to_Dlink_AC3200_5G_1(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="bfccdaa9-8e01-488c-9768-8c71ab5ec157")
    def test_iot_connection_to_Dlink_AC3200_5G_2(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="0e4978de-0435-4856-ae5a-c39cc64e375b")
    def test_iot_connection_to_Dlink_N750_2G(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="cdb82797-9981-4ba6-8958-025f59c60e83")
    def test_iot_connection_to_Dlink_N750_5G(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="0bf8f129-eb96-4b1e-94bd-8dd93e8731e3")
    def iot_connection_to_Linksys_E800_2G(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="f231216d-6ab6-46b7-a0a5-1ac15935e412")
    def test_iot_connection_to_Linksys_AC1900_2G(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="5acd4bec-b210-4b4c-8b2c-60f3f67798a9")
    def test_iot_connection_to_Linksys_AC1900_5G(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="f4fd9877-b13f-47b0-9523-1ce363200c2f")
    def iot_connection_to_Linksys_AC2400_2g(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="438d679a-4f6c-476d-9eba-63b6f1f2bef4")
    def iot_connection_to_Linksys_AC2400_5g(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="b9bc00d8-46c5-4c5e-bd58-93ab1ca8d53b")
    def iot_connection_to_NETGEAR_AC1900_2G(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="fb4c7d80-4c12-4b08-a40a-2745e2bd167b")
    def iot_connection_to_NETGEAR_AC1900_5G(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="054d2ffc-97fd-4613-bf47-acedd0fa4701")
    def iot_connection_to_NETGEAR_AC3200_2G(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="d15a789a-def5-4c6a-b59e-1a75f73cc6a9")
    def iot_connection_to_NETGEAR_AC3200_5G_1(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="1de6369e-97da-479f-b17c-9144bb814f51")
    def iot_connection_to_NETGEAR_AC3200_5G_2(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="008ec18e-fd48-4359-8a0d-223c921a1faa")
    def iot_connection_to_NETGEAR_N300_2G(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="c61eeaf0-af02-46bf-bcec-871e2f9dee71")
    def iot_connection_to_WNDR4500v2_AES_2G(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="dcad3474-4022-48bc-8529-07321611b616")
    def iot_connection_to_WNDR4500v2_WEP_SHARED128_5G(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="3573a880-4542-4dea-9909-aa2f9865a059")
    def iot_connection_to_ARCHER_WEP_OPEN_64_2G(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="9c15c52e-945a-4b9b-bf0e-5bd6293dad1c")
    def iot_connection_to_ARCHER_WEP_OPEN_128_5G(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="e5517b82-c225-449d-83ac-055a561a764f")
    def iot_connection_to_TP_LINK_AC1700_2G(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="9531d3cc-129d-4501-a5e3-d7502120cd8b")
    def iot_connection_to_TP_LINK_AC1700_5G(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="eab810d4-8e07-49c9-86c1-cb8d1a0285d0")
    def iot_connection_to_TP_LINK_N300_2G(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="05d4cb25-a58d-46b4-a5ff-6e3fe28f2b16")
    def iot_connection_to_fritz_7490_5g(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="8333e5e6-72fd-4957-bab0-fa45ce1d765a")
    def iot_connection_to_NETGEAR_R8500_2G(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="c88053fb-730f-4447-a802-1fb9721f69df")
    def iot_connection_to_NETGEAR_R8500_5G1(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="f5d1e44b-396b-4976-bb0c-160bdce89a59")
    def iot_connection_to_NETGEAR_R8500_5G2(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="7c12f943-d9e2-45b1-aa84-fcb43efbbb04")
    def test_iot_connection_to_TP_LINK_5504_2G(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="52be6b76-5e43-4289-83e1-4cd0d995d39b")
    def test_iot_connection_to_TP_LINK_5504_5G_1(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="0b43d1da-e207-443d-b16c-c4ee3e924036")
    def test_iot_connection_to_TP_LINK_5504_5G_2(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="4adcef5c-589a-4398-a28c-28a56d762f72")
    def test_iot_connection_to_TP_LINK_C2600_2G(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="3955a443-505b-4015-9daa-f52abbad8377")
    def test_iot_connection_to_TP_LINK_C2600_5G(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="3e9115dd-adb6-40a4-9831-dca8f1f32abe")
    def test_iot_connection_to_Linksys06832_2G(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="5dca028a-7384-444f-b231-973054afe215")
    def test_iot_connection_to_Linksys06832_5G(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="e639f6db-ad8e-4b4f-91f3-10acdf93142a")
    def test_iot_connection_to_AmpedAthena_2G(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="3dd90d80-952f-4f17-a48a-fe42e7d6e1ff")
    def test_iot_connection_to_AmpedAthena_5G(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="b9babe3a-ecba-4c5c-bc6b-0ba48c744e66")
    def test_iot_connection_to_ASUS_AC3100_2G(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="f8f06f92-821d-4e80-8f1e-efb6c6adc12a")
    def test_iot_connection_to_ASUS_AC3100_5G(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])

    @test_tracker_info(uuid="f4d227df-1151-469a-b01c-f4b1c1f7a84b")
    def iot_connection_to_NETGEAR_WGR614_2G(self):
        ssid_key = self.current_test_name.replace(self.iot_test_prefix, "")
        self.connect_to_wifi_network_and_run_iperf(self.ssid_map[ssid_key])
