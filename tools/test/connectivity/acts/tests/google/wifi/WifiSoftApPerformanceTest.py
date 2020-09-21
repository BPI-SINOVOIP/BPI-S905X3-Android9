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

import WifiRvrTest
from acts import base_test
from acts.test_decorators import test_tracker_info
from acts.test_utils.wifi import wifi_test_utils as wutils


class WifiSoftApRvrTest(WifiRvrTest.WifiRvrTest):
    def __init__(self, controllers):
        base_test.BaseTestClass.__init__(self, controllers)
        self.tests = ("test_rvr_TCP_DL_2GHz", "test_rvr_TCP_UL_2GHz",
                      "test_rvr_TCP_DL_5GHz", "test_rvr_TCP_UL_5GHz")

    def get_sap_connection_info(self):
        info = {}
        info["client_ip_address"] = self.android_devices[
            1].droid.connectivityGetIPv4Addresses('wlan0')[0]
        info["ap_ip_address"] = self.android_devices[
            0].droid.connectivityGetIPv4Addresses('wlan0')[0]
        info["frequency"] = self.client_dut.adb.shell(
            "wpa_cli status | grep freq").split("=")[1]
        info["channel"] = wutils.WifiEnums.freq_to_channel[int(
            info["frequency"])]
        return info

    def sap_rvr_test_func(self):
        """Main function to test Soft AP RvR.

        The function sets up the phones in the correct soft ap and client mode
        configuration and calls run_rvr to sweep attenuation and measure
        throughput

        Args:
            channel: Specifies AP's channel
            mode: Specifies AP's bandwidth/mode (11g, VHT20, VHT40, VHT80)
        Returns:
            rvr_result: dict containing rvr_results and meta data
        """
        #Initialize RvR test parameters
        num_atten_steps = int((self.test_params["rvr_atten_stop"] -
                               self.test_params["rvr_atten_start"]) /
                              self.test_params["rvr_atten_step"])
        self.rvr_atten_range = [
            self.test_params["rvr_atten_start"] +
            x * self.test_params["rvr_atten_step"]
            for x in range(0, num_atten_steps)
        ]
        rvr_result = {}
        # Reset WiFi on all devices
        for dev in self.android_devices:
            wutils.reset_wifi(dev)
            dev.droid.wifiSetCountryCode(wutils.WifiEnums.CountryCode.US)
        # Setup Soft AP
        sap_config = wutils.create_softap_config()
        wutils.start_wifi_tethering(
            self.android_devices[0], sap_config[wutils.WifiEnums.SSID_KEY],
            sap_config[wutils.WifiEnums.PWD_KEY], self.sap_band_enum)
        self.main_network = {
            "SSID": sap_config[wutils.WifiEnums.SSID_KEY],
            "password": sap_config[wutils.WifiEnums.PWD_KEY]
        }
        # Set attenuator to 0 dB
        [self.attenuators[i].set_atten(0) for i in range(self.num_atten)]
        # Connect DUT to Network
        wutils.wifi_connect(
            self.android_devices[1],
            self.main_network,
            num_of_tries=5,
            assert_on_fail=False)
        connection_info = self.get_sap_connection_info()
        self.test_params["iperf_server_address"] = connection_info[
            "ap_ip_address"]
        # Run RvR and log result
        rvr_result["test_name"] = self.current_test_name
        rvr_result["attenuation"] = list(self.rvr_atten_range)
        rvr_result["fixed_attenuation"] = self.test_params[
            "fixed_attenuation"][str(connection_info["channel"])]
        rvr_result["throughput_receive"] = self.rvr_test()
        self.testclass_results.append(rvr_result)
        wutils.stop_wifi_tethering(self.android_devices[0])
        return rvr_result

    def _test_rvr(self):
        """ Function that gets called for each test case

        The function gets called in each rvr test case. The function customizes
        the rvr test based on the test name of the test that called it
        """
        test_params = self.current_test_name.split("_")
        self.sap_band = test_params[4]
        if self.sap_band == "2GHz":
            self.sap_band_enum = wutils.WifiEnums.WIFI_CONFIG_APBAND_2G
        else:
            self.sap_band_enum = wutils.WifiEnums.WIFI_CONFIG_APBAND_5G
        self.iperf_args = '-i 1 -t {} -J '.format(
            self.test_params["iperf_duration"])
        if test_params[2] == "UDP":
            self.iperf_args = self.iperf_args + "-u -b {}".format(
                self.test_params["UDP_rates"]["VHT80"])
        if test_params[3] == "DL":
            self.iperf_args = self.iperf_args + ' -R'
            self.use_client_output = True
        else:
            self.use_client_output = False
        rvr_result = self.sap_rvr_test_func()
        self.post_process_results(rvr_result)
        self.pass_fail_check(rvr_result)

    #Test cases
    @test_tracker_info(uuid='7910112e-49fd-4e49-bc5c-f84da0cbb9f6')
    def test_rvr_TCP_DL_2GHz(self):
        self._test_rvr()

    @test_tracker_info(uuid='b3c00814-6fdf-496b-b345-6a719bef657e')
    def test_rvr_TCP_UL_2GHz(self):
        self._test_rvr()

    @test_tracker_info(uuid='a2f727b5-68ba-46e5-a7fe-f86c0a082fc9')
    def test_rvr_TCP_DL_5GHz(self):
        self._test_rvr()

    @test_tracker_info(uuid='0cca9352-3f06-4bba-be17-8897a1b42a0f')
    def test_rvr_TCP_UL_5GHz(self):
        self._test_rvr()
