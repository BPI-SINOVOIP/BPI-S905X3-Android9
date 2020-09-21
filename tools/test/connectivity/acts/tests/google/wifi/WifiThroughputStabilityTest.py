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

import json
import logging
import math
import os
import time
from acts import asserts
from acts import base_test
from acts import utils
from acts.controllers import iperf_server as ipf
from acts.test_decorators import test_tracker_info
from acts.test_utils.wifi import wifi_power_test_utils as wputils
from acts.test_utils.wifi import wifi_retail_ap as retail_ap
from acts.test_utils.wifi import wifi_test_utils as wutils

TEST_TIMEOUT = 10


class WifiThroughputStabilityTest(base_test.BaseTestClass):
    def __init__(self, controllers):
        base_test.BaseTestClass.__init__(self, controllers)
        self.tests = ("test_tput_stability_high_TCP_DL_ch6_VHT20",
                      "test_tput_stability_high_TCP_UL_ch6_VHT20",
                      "test_tput_stability_low_TCP_DL_ch6_VHT20",
                      "test_tput_stability_low_TCP_UL_ch6_VHT20",
                      "test_tput_stability_high_UDP_DL_ch6_VHT20",
                      "test_tput_stability_high_UDP_UL_ch6_VHT20",
                      "test_tput_stability_low_UDP_DL_ch6_VHT20",
                      "test_tput_stability_low_UDP_UL_ch6_VHT20",
                      "test_tput_stability_high_TCP_DL_ch36_VHT20",
                      "test_tput_stability_high_TCP_UL_ch36_VHT20",
                      "test_tput_stability_low_TCP_DL_ch36_VHT20",
                      "test_tput_stability_low_TCP_UL_ch36_VHT20",
                      "test_tput_stability_high_UDP_DL_ch36_VHT20",
                      "test_tput_stability_high_UDP_UL_ch36_VHT20",
                      "test_tput_stability_low_UDP_DL_ch36_VHT20",
                      "test_tput_stability_low_UDP_UL_ch36_VHT20",
                      "test_tput_stability_high_TCP_DL_ch36_VHT40",
                      "test_tput_stability_high_TCP_UL_ch36_VHT40",
                      "test_tput_stability_low_TCP_DL_ch36_VHT40",
                      "test_tput_stability_low_TCP_UL_ch36_VHT40",
                      "test_tput_stability_high_UDP_DL_ch36_VHT40",
                      "test_tput_stability_high_UDP_UL_ch36_VHT40",
                      "test_tput_stability_low_UDP_DL_ch36_VHT40",
                      "test_tput_stability_low_UDP_UL_ch36_VHT40",
                      "test_tput_stability_high_TCP_DL_ch36_VHT80",
                      "test_tput_stability_high_TCP_UL_ch36_VHT80",
                      "test_tput_stability_low_TCP_DL_ch36_VHT80",
                      "test_tput_stability_low_TCP_UL_ch36_VHT80",
                      "test_tput_stability_high_UDP_DL_ch36_VHT80",
                      "test_tput_stability_high_UDP_UL_ch36_VHT80",
                      "test_tput_stability_low_UDP_DL_ch36_VHT80",
                      "test_tput_stability_low_UDP_UL_ch36_VHT80",
                      "test_tput_stability_high_TCP_DL_ch149_VHT20",
                      "test_tput_stability_high_TCP_UL_ch149_VHT20",
                      "test_tput_stability_low_TCP_DL_ch149_VHT20",
                      "test_tput_stability_low_TCP_UL_ch149_VHT20",
                      "test_tput_stability_high_UDP_DL_ch149_VHT20",
                      "test_tput_stability_high_UDP_UL_ch149_VHT20",
                      "test_tput_stability_low_UDP_DL_ch149_VHT20",
                      "test_tput_stability_low_UDP_UL_ch149_VHT20",
                      "test_tput_stability_high_TCP_DL_ch149_VHT40",
                      "test_tput_stability_high_TCP_UL_ch149_VHT40",
                      "test_tput_stability_low_TCP_DL_ch149_VHT40",
                      "test_tput_stability_low_TCP_UL_ch149_VHT40",
                      "test_tput_stability_high_UDP_DL_ch149_VHT40",
                      "test_tput_stability_high_UDP_UL_ch149_VHT40",
                      "test_tput_stability_low_UDP_DL_ch149_VHT40",
                      "test_tput_stability_low_UDP_UL_ch149_VHT40",
                      "test_tput_stability_high_TCP_DL_ch149_VHT80",
                      "test_tput_stability_high_TCP_UL_ch149_VHT80",
                      "test_tput_stability_low_TCP_DL_ch149_VHT80",
                      "test_tput_stability_low_TCP_UL_ch149_VHT80",
                      "test_tput_stability_high_UDP_DL_ch149_VHT80",
                      "test_tput_stability_high_UDP_UL_ch149_VHT80",
                      "test_tput_stability_low_UDP_DL_ch149_VHT80",
                      "test_tput_stability_low_UDP_UL_ch149_VHT80")

    def setup_class(self):
        self.dut = self.android_devices[0]
        req_params = [
            "throughput_stability_test_params", "testbed_params",
            "main_network"
        ]
        opt_params = ["RetailAccessPoints"]
        self.unpack_userparams(req_params, opt_params)
        self.test_params = self.throughput_stability_test_params
        self.num_atten = self.attenuators[0].instrument.num_atten
        self.iperf_server = self.iperf_servers[0]
        self.access_points = retail_ap.create(self.RetailAccessPoints)
        self.access_point = self.access_points[0]
        self.log_path = os.path.join(logging.log_path, "test_results")
        utils.create_dir(self.log_path)
        self.log.info("Access Point Configuration: {}".format(
            self.access_point.ap_settings))
        if not hasattr(self, "golden_files_list"):
            self.golden_files_list = [
                os.path.join(self.testbed_params["golden_results_path"],
                             file) for file in os.listdir(
                                 self.testbed_params["golden_results_path"])
            ]

    def teardown_test(self):
        self.iperf_server.stop()

    def pass_fail_check(self, test_result_dict):
        """Check the test result and decide if it passed or failed.

        Checks the throughput stability test's PASS/FAIL criteria based on
        minimum instantaneous throughput, and standard deviation.

        Args:
            test_result_dict: dict containing attenuation, throughput and other
            meta data
        """
        #TODO(@oelayach): Check throughput vs RvR golden file
        min_throughput_check = (
            (test_result_dict["iperf_results"]["min_throughput"] /
             test_result_dict["iperf_results"]["avg_throughput"]) *
            100) > self.test_params["min_throughput_threshold"]
        std_deviation_check = (
            (test_result_dict["iperf_results"]["std_deviation"] /
             test_result_dict["iperf_results"]["avg_throughput"]) *
            100) < self.test_params["std_deviation_threshold"]

        if min_throughput_check and std_deviation_check:
            asserts.explicit_pass(
                "Test Passed. Throughput at {}dB attenuation is unstable. "
                "Average throughput is {} Mbps with a standard deviation of "
                "{} Mbps and dips down to {} Mbps.".format(
                    self.atten_level,
                    test_result_dict["iperf_results"]["avg_throughput"],
                    test_result_dict["iperf_results"]["std_deviation"],
                    test_result_dict["iperf_results"]["min_throughput"]))
        asserts.fail(
            "Test Failed. Throughput at {}dB attenuation is unstable. "
            "Average throughput is {} Mbps with a standard deviation of "
            "{} Mbps and dips down to {} Mbps.".format(
                self.atten_level,
                test_result_dict["iperf_results"]["avg_throughput"],
                test_result_dict["iperf_results"]["std_deviation"],
                test_result_dict["iperf_results"]["min_throughput"]))

    def post_process_results(self, test_result):
        """Extracts results and saves plots and JSON formatted results.

        Args:
            test_result: dict containing attenuation, iPerfResult object and
            other meta data
        Returns:
            test_result_dict: dict containing post-processed results including
            avg throughput, other metrics, and other meta data
        """
        # Save output as text file
        test_name = self.current_test_name
        results_file_path = "{}/{}.txt".format(self.log_path,
                                               self.current_test_name)
        test_result_dict = {}
        test_result_dict["ap_settings"] = test_result["ap_settings"].copy()
        test_result_dict["attenuation"] = self.atten_level
        instantaneous_rates_Mbps = [
            rate * 8 * (1.024**2) for rate in test_result["iperf_result"]
            .instantaneous_rates[self.test_params["iperf_ignored_interval"]:-1]
        ]
        test_result_dict["iperf_results"] = {
            "instantaneous_rates":
            instantaneous_rates_Mbps,
            "avg_throughput":
            math.fsum(instantaneous_rates_Mbps) /
            len(instantaneous_rates_Mbps),
            "std_deviation":
            test_result["iperf_result"].get_std_deviation(
                self.test_params["iperf_ignored_interval"]) * 8,
            "min_throughput":
            min(instantaneous_rates_Mbps)
        }
        with open(results_file_path, 'w') as results_file:
            json.dump(test_result_dict, results_file)
        # Plot and save
        legends = self.current_test_name
        x_label = 'Time (s)'
        y_label = 'Throughput (Mbps)'
        time_data = list(range(0, len(instantaneous_rates_Mbps)))
        data_sets = [[time_data], [instantaneous_rates_Mbps]]
        fig_property = {
            "title": test_name,
            "x_label": x_label,
            "y_label": y_label,
            "linewidth": 3,
            "markersize": 10
        }
        output_file_path = "{}/{}.html".format(self.log_path, test_name)
        wputils.bokeh_plot(
            data_sets,
            legends,
            fig_property,
            shaded_region=None,
            output_file_path=output_file_path)
        return test_result_dict

    def throughput_stability_test_func(self, channel, mode):
        """Main function to test throughput stability.

        The function sets up the AP in the correct channel and mode
        configuration and runs an iperf test to measure throughput.

        Args:
            channel: Specifies AP's channel
            mode: Specifies AP's bandwidth/mode (11g, VHT20, VHT40, VHT80)
        Returns:
            test_result: dict containing test result and meta data
        """
        #Initialize RvR test parameters
        test_result = {}
        # Configure AP
        band = self.access_point.band_lookup_by_channel(channel)
        self.access_point.set_channel(band, channel)
        self.access_point.set_bandwidth(band, mode)
        self.log.info("Access Point Configuration: {}".format(
            self.access_point.ap_settings))
        # Set attenuator to test level
        self.log.info("Setting attenuation to {} dB".format(self.atten_level))
        [
            self.attenuators[i].set_atten(self.atten_level)
            for i in range(self.num_atten)
        ]
        # Connect DUT to Network
        self.main_network[band]["channel"] = channel
        wutils.reset_wifi(self.dut)
        wutils.wifi_connect(self.dut, self.main_network[band], num_of_tries=5)
        time.sleep(5)
        # Run test and log result
        # Start iperf session
        self.log.info("Starting iperf test.")
        self.iperf_server.start(tag=str(self.atten_level))
        try:
            client_output = ""
            client_status, client_output = self.dut.run_iperf_client(
                self.testbed_params["iperf_server_address"],
                self.iperf_args,
                timeout=self.test_params["iperf_duration"] + TEST_TIMEOUT)
        except:
            self.log.warning("TimeoutError: Iperf measurement timed out.")
        client_output_path = os.path.join(self.iperf_server.log_path,
                                          "iperf_client_output_{}".format(
                                              self.current_test_name))
        with open(client_output_path, 'w') as out_file:
            out_file.write("\n".join(client_output))
        self.iperf_server.stop()
        # Set attenuator to 0 dB
        [self.attenuators[i].set_atten(0) for i in range(self.num_atten)]
        # Parse and log result
        if self.use_client_output:
            iperf_file = client_output_path
        else:
            iperf_file = self.iperf_server.log_files[-1]
        try:
            iperf_result = ipf.IPerfResult(iperf_file)
        except:
            self.log.warning("ValueError: Cannot get iperf result.")
            iperf_result = None
        test_result["ap_settings"] = self.access_point.ap_settings.copy()
        test_result["attenuation"] = self.atten_level
        test_result["iperf_result"] = iperf_result
        return test_result

    def get_target_atten_tput(self):
        """Function gets attenuation used for test

        The function fetches the attenuation at which the test should be
        performed, and the expected target average throughput.

        Returns:
            test_target: dict containing target test attenuation and expected
            throughput
        """
        # Fetch the golden RvR results
        test_name = self.current_test_name
        rvr_golden_file_name = "test_rvr_" + "_".join(test_name.split("_")[4:])
        golden_path = [
            file_name for file_name in self.golden_files_list
            if rvr_golden_file_name in file_name
        ]
        with open(golden_path[0], 'r') as golden_file:
            golden_results = json.load(golden_file)
        test_target = {}
        rssi_high_low = test_name.split("_")[3]
        if rssi_high_low == "low":
            # Get 0 Mbps attenuation and backoff by low_rssi_backoff_from_range
            atten_idx = golden_results["throughput_receive"].index(0)
            max_atten = golden_results["attenuation"][atten_idx]
            test_target[
                "target_attenuation"] = max_atten - self.test_params["low_rssi_backoff_from_range"]
            tput_idx = golden_results["attenuation"].index(
                test_target["target_attenuation"])
            test_target["target_throughput"] = golden_results[
                "throughput_receive"][tput_idx]
        if rssi_high_low == "high":
            # Test at lowest attenuation point
            test_target["target_attenuation"] = golden_results["attenuation"][
                0]
            test_target["target_throughput"] = golden_results[
                "throughput_receive"][0]
        return test_target

    def _test_throughput_stability(self):
        """ Function that gets called for each test case

        The function gets called in each test case. The function customizes
        the test based on the test name of the test that called it
        """
        test_params = self.current_test_name.split("_")
        channel = int(test_params[6][2:])
        mode = test_params[7]
        test_target = self.get_target_atten_tput()
        self.atten_level = test_target["target_attenuation"]
        self.iperf_args = '-i 1 -t {} -J'.format(
            self.test_params["iperf_duration"])
        if test_params[4] == "UDP":
            self.iperf_args = self.iperf_args + "-u -b {}".format(
                self.test_params["UDP_rates"][mode])
        if test_params[5] == "DL":
            self.iperf_args = self.iperf_args + ' -R'
            self.use_client_output = True
        else:
            self.use_client_output = False
        test_result = self.throughput_stability_test_func(channel, mode)
        test_result_postprocessed = self.post_process_results(test_result)
        self.pass_fail_check(test_result_postprocessed)

    #Test cases
    @test_tracker_info(uuid='cf6e1c5a-a54e-4efc-a5db-90e02029fe10')
    def test_tput_stability_high_TCP_DL_ch6_VHT20(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='86f8d2ed-a35e-49fd-983a-7af528e94426')
    def test_tput_stability_high_TCP_UL_ch6_VHT20(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='71f615c0-9de8-4070-ba40-43b59278722e')
    def test_tput_stability_low_TCP_DL_ch6_VHT20(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='ad4774d7-4b2c-49f5-8407-3aa72eb43537')
    def test_tput_stability_low_TCP_UL_ch6_VHT20(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='586e1745-a97e-4b6c-af3a-00566aded442')
    def test_tput_stability_high_UDP_DL_ch6_VHT20(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='66af3329-f40f-46fb-a156-69b9047711ec')
    def test_tput_stability_high_UDP_UL_ch6_VHT20(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='bfebc69d-6636-4f95-b709-9d31040ab8cc')
    def test_tput_stability_low_UDP_DL_ch6_VHT20(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='f7cc0211-a18e-4502-a713-e4f88b446776')
    def test_tput_stability_low_UDP_UL_ch6_VHT20(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='594f0359-600c-4b5f-9b96-90e5aa0f2ffb')
    def test_tput_stability_high_TCP_DL_ch36_VHT20(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='fd0a2604-2b69-40c5-906f-c3da45c062e8')
    def test_tput_stability_high_TCP_UL_ch36_VHT20(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='4ead8dc9-6beb-4ce0-8490-66cdd343d355')
    def test_tput_stability_low_TCP_DL_ch36_VHT20(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='2657454c-5a12-4d50-859a-2adb56910920')
    def test_tput_stability_low_TCP_UL_ch36_VHT20(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='4a446a64-a1ab-441a-bfec-5e3fd509c43b')
    def test_tput_stability_high_UDP_DL_ch36_VHT20(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='e4f07efe-71cb-4891-898d-127bd74ca8a7')
    def test_tput_stability_high_UDP_UL_ch36_VHT20(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='aa0270cb-d18b-4048-a3d8-e09eb01ac417')
    def test_tput_stability_low_UDP_DL_ch36_VHT20(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='40b18d99-40b9-4592-a624-a63bee9d55f4')
    def test_tput_stability_low_UDP_UL_ch36_VHT20(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='ce9a9d88-6d39-4a19-a5d6-5296ed480afa')
    def test_tput_stability_high_TCP_DL_ch36_VHT40(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='23a5f845-0871-481a-898d-e0d6aceed4d4')
    def test_tput_stability_high_TCP_UL_ch36_VHT40(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='6e12ee9e-8f27-46e4-8645-9e7a2cdc354f')
    def test_tput_stability_low_TCP_DL_ch36_VHT40(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='2325dd69-17b2-49ce-ac02-dfaa839e638e')
    def test_tput_stability_low_TCP_UL_ch36_VHT40(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='28ae4bcb-2e50-4d74-9b77-4a5e199bc7a4')
    def test_tput_stability_high_UDP_DL_ch36_VHT40(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='a8c3b5bf-ccb8-435b-8861-3b21ed5072fa')
    def test_tput_stability_high_UDP_UL_ch36_VHT40(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='8a15662a-ed78-4a0d-8271-15927963a2c0')
    def test_tput_stability_low_UDP_DL_ch36_VHT40(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='005a84de-689d-4b40-9912-66c137872312')
    def test_tput_stability_low_UDP_UL_ch36_VHT40(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='b1506bec-f3dd-4d93-87b1-3e9a7172a904')
    def test_tput_stability_high_TCP_DL_ch36_VHT80(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='bdebd1df-3e7c-40fc-ad28-1b817b9cb228')
    def test_tput_stability_high_TCP_UL_ch36_VHT80(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='08c7bb18-681f-4a93-90c8-bc0df7169211')
    def test_tput_stability_low_TCP_DL_ch36_VHT80(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='2ddbb7b-496b-4fce-8697-f90409b6e441')
    def test_tput_stability_low_TCP_UL_ch36_VHT80(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='db41ce61-1d8f-4fe8-a904-77f8dcc50ba3')
    def test_tput_stability_high_UDP_DL_ch36_VHT80(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='cd537cb0-8936-41f4-a0fb-1c8b4f38fb62')
    def test_tput_stability_high_UDP_UL_ch36_VHT80(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='4a0209df-42b2-4143-8321-4023355bd663')
    def test_tput_stability_low_UDP_DL_ch36_VHT80(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='64304510-bd5f-4497-9b18-0d4b1a8eb026')
    def test_tput_stability_low_UDP_UL_ch36_VHT80(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='d121fc54-2f1f-4722-8adb-17cb89fba3e6')
    def test_tput_stability_high_TCP_DL_ch149_VHT20(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='cdfca524-7387-4a08-a256-4696b76aa90f')
    def test_tput_stability_high_TCP_UL_ch149_VHT20(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='99280d2d-0096-4862-b27c-85aa46dcfb79')
    def test_tput_stability_low_TCP_DL_ch149_VHT20(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='c09a0f67-0383-4d6a-ac52-f4afd830971c')
    def test_tput_stability_low_TCP_UL_ch149_VHT20(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='7d6dbd39-1dea-4770-a2e0-d9fb9f32904d')
    def test_tput_stability_high_UDP_DL_ch149_VHT20(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='f7e0574e-29b7-429e-9f47-e5cecc0e2bec')
    def test_tput_stability_high_UDP_UL_ch149_VHT20(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='31bd4b12-1be5-46b3-8f86-914e739b0082')
    def test_tput_stability_low_UDP_DL_ch149_VHT20(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='dc88a5bb-646d-407c-aabe-f8d533f4fbc1')
    def test_tput_stability_low_UDP_UL_ch149_VHT20(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='9a101716-7774-451c-b0a3-4ac993143841')
    def test_tput_stability_high_TCP_DL_ch149_VHT40(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='3533c6ba-1510-4559-9f76-5aef8d996c71')
    def test_tput_stability_high_TCP_UL_ch149_VHT40(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='e83840a0-805e-408a-87b6-4bfce5306b1d')
    def test_tput_stability_low_TCP_DL_ch149_VHT40(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='c55edf2b-b0ce-469e-bd98-e407a9f14126')
    def test_tput_stability_low_TCP_UL_ch149_VHT40(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='0704b56e-e986-4bb8-84dc-e7a1ef94ed91')
    def test_tput_stability_high_UDP_DL_ch149_VHT40(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='71a979c0-a7d9-45bc-a57a-d71f5629b191')
    def test_tput_stability_high_UDP_UL_ch149_VHT40(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='b4d0f554-f49b-46a8-897b-fc0d3af2e4b5')
    def test_tput_stability_low_UDP_DL_ch149_VHT40(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='5bcb5950-db7f-435f-8fcb-55620c757f4f')
    def test_tput_stability_low_UDP_UL_ch149_VHT40(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='d05bcae5-48ac-410b-8083-a05951839121')
    def test_tput_stability_high_TCP_DL_ch149_VHT80(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='038f0c6d-527b-4094-af41-d8732b7594ed')
    def test_tput_stability_high_TCP_UL_ch149_VHT80(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='7ebd1735-b495-4385-a1d4-e2a2991ecbcd')
    def test_tput_stability_low_TCP_DL_ch149_VHT80(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='f9e2679a-0c2e-42ac-a7b2-81aae8680a8f')
    def test_tput_stability_low_TCP_UL_ch149_VHT80(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='39e90233-404e-48ed-a71d-aa55b8047641')
    def test_tput_stability_high_UDP_DL_ch149_VHT80(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='e68a8b64-9278-4c75-8f98-37b415287d9c')
    def test_tput_stability_high_UDP_UL_ch149_VHT80(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='f710c4f8-5ae3-4e70-8d1f-dfa3fb7222a8')
    def test_tput_stability_low_UDP_DL_ch149_VHT80(self):
        self._test_throughput_stability()

    @test_tracker_info(uuid='ce8c125c-73d1-4aab-8d09-0b67d8598d20')
    def test_tput_stability_low_UDP_UL_ch149_VHT80(self):
        self._test_throughput_stability()
