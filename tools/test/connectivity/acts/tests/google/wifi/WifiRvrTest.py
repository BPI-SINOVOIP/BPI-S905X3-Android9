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


class WifiRvrTest(base_test.BaseTestClass):
    TEST_TIMEOUT = 10
    SHORT_SLEEP = 1
    MED_SLEEP = 5

    def __init__(self, controllers):
        base_test.BaseTestClass.__init__(self, controllers)

    def setup_class(self):
        self.client_dut = self.android_devices[-1]
        req_params = ["rvr_test_params", "testbed_params"]
        opt_params = [
            "main_network", "RetailAccessPoints", "golden_files_list"
        ]
        self.unpack_userparams(req_params, opt_params)
        self.test_params = self.rvr_test_params
        self.num_atten = self.attenuators[0].instrument.num_atten
        self.iperf_server = self.iperf_servers[0]
        if hasattr(self, "RetailAccessPoints"):
            self.access_points = retail_ap.create(self.RetailAccessPoints)
            self.access_point = self.access_points[0]
            self.log.info("Access Point Configuration: {}".format(
                self.access_point.ap_settings))
        self.log_path = os.path.join(logging.log_path, "rvr_results")
        utils.create_dir(self.log_path)
        if not hasattr(self, "golden_files_list"):
            self.golden_files_list = [
                os.path.join(self.testbed_params["golden_results_path"],
                             file) for file in os.listdir(
                                 self.testbed_params["golden_results_path"])
            ]
        self.testclass_results = []

        # Turn WiFi ON
        for dev in self.android_devices:
            wutils.wifi_toggle_state(dev, True)

    def teardown_test(self):
        self.iperf_server.stop()

    def teardown_class(self):
        """Saves plot with all test results to enable comparison.
        """
        # Turn WiFi OFF
        for dev in self.android_devices:
            wutils.wifi_toggle_state(dev, False)
        # Plot and save all results
        x_data = []
        y_data = []
        legends = []
        for result in self.testclass_results:
            total_attenuation = [
                att + result["fixed_attenuation"]
                for att in result["attenuation"]
            ]
            x_data.append(total_attenuation)
            y_data.append(result["throughput_receive"])
            legends.append(result["test_name"])
        x_label = 'Attenuation (dB)'
        y_label = 'Throughput (Mbps)'
        data_sets = [x_data, y_data]
        fig_property = {
            "title": "RvR Results",
            "x_label": x_label,
            "y_label": y_label,
            "linewidth": 3,
            "markersize": 10
        }
        output_file_path = "{}/{}.html".format(self.log_path, "rvr_results")
        wputils.bokeh_plot(
            data_sets,
            legends,
            fig_property,
            shaded_region=None,
            output_file_path=output_file_path)

    def pass_fail_check(self, rvr_result):
        """Check the test result and decide if it passed or failed.

        Checks the RvR test result and compares to a throughput limites for
        the same configuration. The pass/fail tolerances are provided in the
        config file.

        Args:
            rvr_result: dict containing attenuation, throughput and other meta
            data
        """
        test_name = self.current_test_name
        golden_path = [
            file_name for file_name in self.golden_files_list
            if test_name in file_name
        ]
        try:
            golden_path = golden_path[0]
            throughput_limits = self.compute_throughput_limits(
                golden_path, rvr_result)
        except:
            asserts.fail("Test failed: Golden file not found")

        failure_count = 0
        for idx, current_throughput in enumerate(
                rvr_result["throughput_receive"]):
            current_att = rvr_result["attenuation"][idx] + rvr_result["fixed_attenuation"]
            if (current_throughput < throughput_limits["lower_limit"][idx]
                    or current_throughput >
                    throughput_limits["upper_limit"][idx]):
                failure_count = failure_count + 1
                self.log.info(
                    "Throughput at {}dB attenuation is beyond limits. "
                    "Throughput is {} Mbps. Expected within [{}, {}] Mbps.".
                    format(current_att, current_throughput,
                           throughput_limits["lower_limit"][idx],
                           throughput_limits["upper_limit"][idx]))
        if failure_count >= self.test_params["failure_count_tolerance"]:
            asserts.fail("Test failed. Found {} points outside limits.".format(
                failure_count))
        asserts.explicit_pass(
            "Test passed. Found {} points outside throughput limits.".format(
                failure_count))

    def compute_throughput_limits(self, golden_path, rvr_result):
        """Compute throughput limits for current test.

        Checks the RvR test result and compares to a throughput limites for
        the same configuration. The pass/fail tolerances are provided in the
        config file.

        Args:
            golden_path: path to golden file used to generate limits
            rvr_result: dict containing attenuation, throughput and other meta
            data
        Returns:
            throughput_limits: dict containing attenuation and throughput limit data
        """
        with open(golden_path, 'r') as golden_file:
            golden_results = json.load(golden_file)
            golden_attenuation = [
                att + golden_results["fixed_attenuation"]
                for att in golden_results["attenuation"]
            ]
        attenuation = []
        lower_limit = []
        upper_limit = []
        for idx, current_throughput in enumerate(
                rvr_result["throughput_receive"]):
            current_att = rvr_result["attenuation"][idx] + rvr_result["fixed_attenuation"]
            att_distances = [
                abs(current_att - golden_att)
                for golden_att in golden_attenuation
            ]
            sorted_distances = sorted(
                enumerate(att_distances), key=lambda x: x[1])
            closest_indeces = [dist[0] for dist in sorted_distances[0:3]]
            closest_throughputs = [
                golden_results["throughput_receive"][index]
                for index in closest_indeces
            ]
            closest_throughputs.sort()

            attenuation.append(current_att)
            lower_limit.append(
                max(closest_throughputs[0] - max(
                    self.test_params["abs_tolerance"], closest_throughputs[0] *
                    self.test_params["pct_tolerance"] / 100), 0))
            upper_limit.append(closest_throughputs[-1] + max(
                self.test_params["abs_tolerance"], closest_throughputs[-1] *
                self.test_params["pct_tolerance"] / 100))
        throughput_limits = {
            "attenuation": attenuation,
            "lower_limit": lower_limit,
            "upper_limit": upper_limit
        }
        return throughput_limits

    def post_process_results(self, rvr_result):
        """Saves plots and JSON formatted results.

        Args:
            rvr_result: dict containing attenuation, throughput and other meta
            data
        """
        # Save output as text file
        test_name = self.current_test_name
        results_file_path = "{}/{}.json".format(self.log_path,
                                                self.current_test_name)
        with open(results_file_path, 'w') as results_file:
            json.dump(rvr_result, results_file, indent=4)
        # Plot and save
        legends = [self.current_test_name]
        x_label = 'Attenuation (dB)'
        y_label = 'Throughput (Mbps)'
        total_attenuation = [
            att + rvr_result["fixed_attenuation"]
            for att in rvr_result["attenuation"]
        ]
        data_sets = [[total_attenuation], [rvr_result["throughput_receive"]]]
        fig_property = {
            "title": test_name,
            "x_label": x_label,
            "y_label": y_label,
            "linewidth": 3,
            "markersize": 10
        }
        try:
            golden_path = [
                file_name for file_name in self.golden_files_list
                if test_name in file_name
            ]
            golden_path = golden_path[0]
            with open(golden_path, 'r') as golden_file:
                golden_results = json.load(golden_file)
            legends.insert(0, "Golden Results")
            golden_attenuation = [
                att + golden_results["fixed_attenuation"]
                for att in golden_results["attenuation"]
            ]
            data_sets[0].insert(0, golden_attenuation)
            data_sets[1].insert(0, golden_results["throughput_receive"])
            throughput_limits = self.compute_throughput_limits(
                golden_path, rvr_result)
            shaded_region = {
                "x_vector": throughput_limits["attenuation"],
                "lower_limit": throughput_limits["lower_limit"],
                "upper_limit": throughput_limits["upper_limit"]
            }
        except:
            shaded_region = None
            self.log.warning("ValueError: Golden file not found")
        output_file_path = "{}/{}.html".format(self.log_path, test_name)
        wputils.bokeh_plot(data_sets, legends, fig_property, shaded_region,
                           output_file_path)

    def rvr_test(self):
        """Test function to run RvR.

        The function runs an RvR test in the current device/AP configuration.
        Function is called from another wrapper function that sets up the
        testbed for the RvR test

        Returns:
            rvr_result: dict containing rvr_results and meta data
        """
        self.log.info("Start running RvR")
        rvr_result = []
        for atten in self.rvr_atten_range:
            # Set Attenuation
            self.log.info("Setting attenuation to {} dB".format(atten))
            [
                self.attenuators[i].set_atten(atten)
                for i in range(self.num_atten)
            ]
            # Start iperf session
            self.iperf_server.start(tag=str(atten))
            try:
                client_output = ""
                client_status, client_output = self.client_dut.run_iperf_client(
                    self.testbed_params["iperf_server_address"],
                    self.iperf_args,
                    timeout=self.test_params["iperf_duration"] +
                    self.TEST_TIMEOUT)
            except:
                self.log.warning("TimeoutError: Iperf measurement timed out.")
            client_output_path = os.path.join(
                self.iperf_server.log_path, "iperf_client_output_{}_{}".format(
                    self.current_test_name, str(atten)))
            with open(client_output_path, 'w') as out_file:
                out_file.write("\n".join(client_output))
            self.iperf_server.stop()
            # Parse and log result
            if self.use_client_output:
                iperf_file = client_output_path
            else:
                iperf_file = self.iperf_server.log_files[-1]
            try:
                iperf_result = ipf.IPerfResult(iperf_file)
                curr_throughput = (math.fsum(iperf_result.instantaneous_rates[
                    self.test_params["iperf_ignored_interval"]:-1]) / len(
                        iperf_result.instantaneous_rates[self.test_params[
                            "iperf_ignored_interval"]:-1])) * 8 * (1.024**2)
            except:
                self.log.warning(
                    "ValueError: Cannot get iperf result. Setting to 0")
                curr_throughput = 0
            rvr_result.append(curr_throughput)
            self.log.info("Throughput at {0:.2f} dB is {1:.2f} Mbps".format(
                atten, curr_throughput))
        [self.attenuators[i].set_atten(0) for i in range(self.num_atten)]
        return rvr_result

    def rvr_test_func(self, channel, mode):
        """Main function to test RvR.

        The function sets up the AP in the correct channel and mode
        configuration and called run_rvr to sweep attenuation and measure
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
        # Configure AP
        band = self.access_point.band_lookup_by_channel(channel)
        if "2G" in band:
            frequency = wutils.WifiEnums.channel_2G_to_freq[channel]
        else:
            frequency = wutils.WifiEnums.channel_5G_to_freq[channel]
        if frequency in wutils.WifiEnums.DFS_5G_FREQUENCIES:
            self.access_point.set_region(self.testbed_params["DFS_region"])
        else:
            self.access_point.set_region(self.testbed_params["default_region"])
        self.access_point.set_channel(band, channel)
        self.access_point.set_bandwidth(band, mode)
        self.log.info("Access Point Configuration: {}".format(
            self.access_point.ap_settings))
        # Set attenuator to 0 dB
        [self.attenuators[i].set_atten(0) for i in range(self.num_atten)]
        # Connect DUT to Network
        wutils.reset_wifi(self.client_dut)
        self.main_network[band]["channel"] = channel
        wutils.wifi_connect(
            self.client_dut, self.main_network[band], num_of_tries=5)
        time.sleep(self.MED_SLEEP)
        # Run RvR and log result
        rvr_result["test_name"] = self.current_test_name
        rvr_result["ap_settings"] = self.access_point.ap_settings.copy()
        rvr_result["attenuation"] = list(self.rvr_atten_range)
        rvr_result["fixed_attenuation"] = self.testbed_params[
            "fixed_attenuation"][str(channel)]
        rvr_result["throughput_receive"] = self.rvr_test()
        self.testclass_results.append(rvr_result)
        return rvr_result

    def _test_rvr(self):
        """ Function that gets called for each test case

        The function gets called in each rvr test case. The function customizes
        the rvr test based on the test name of the test that called it
        """
        test_params = self.current_test_name.split("_")
        channel = int(test_params[4][2:])
        mode = test_params[5]
        self.iperf_args = '-i 1 -t {} -J '.format(
            self.test_params["iperf_duration"])
        if test_params[2] == "UDP":
            self.iperf_args = self.iperf_args + "-u -b {}".format(
                self.test_params["UDP_rates"][mode])
        if test_params[3] == "DL":
            self.iperf_args = self.iperf_args + ' -R'
            self.use_client_output = True
        else:
            self.use_client_output = False
        rvr_result = self.rvr_test_func(channel, mode)
        self.post_process_results(rvr_result)
        self.pass_fail_check(rvr_result)

    #Test cases
    @test_tracker_info(uuid='e7586217-3739-44a4-a87b-d790208b04b9')
    def test_rvr_TCP_DL_ch1_VHT20(self):
        self._test_rvr()

    @test_tracker_info(uuid='06b3e979-255c-482f-b570-d347fba048b6')
    def test_rvr_TCP_UL_ch1_VHT20(self):
        self._test_rvr()

    @test_tracker_info(uuid='e912db87-dbfb-4e86-b91c-827e6c53e840')
    def test_rvr_TCP_DL_ch6_VHT20(self):
        self._test_rvr()

    @test_tracker_info(uuid='ddafbe78-bd19-48fc-b653-69b23b1ab8dd')
    def test_rvr_TCP_UL_ch6_VHT20(self):
        self._test_rvr()

    @test_tracker_info(uuid='6fcb7fd8-4438-4913-a1c8-ea35050c79dd')
    def test_rvr_TCP_DL_ch11_VHT20(self):
        self._test_rvr()

    @test_tracker_info(uuid='a165884e-c928-46d9-b459-f550ceb0074f')
    def test_rvr_TCP_UL_ch11_VHT20(self):
        self._test_rvr()

    @test_tracker_info(uuid='a48ee2b4-3fb9-41fd-b292-0051bfc3b0cc')
    def test_rvr_TCP_DL_ch36_VHT20(self):
        self._test_rvr()

    @test_tracker_info(uuid='68f94e6b-b4ff-4839-904b-ec45cc661b89')
    def test_rvr_TCP_UL_ch36_VHT20(self):
        self._test_rvr()

    @test_tracker_info(uuid='a8b00098-5c07-44bb-ae17-5d0489786c62')
    def test_rvr_TCP_DL_ch36_VHT40(self):
        self._test_rvr()

    @test_tracker_info(uuid='ecfb4284-1794-4508-b35e-be56fa4c9035')
    def test_rvr_TCP_UL_ch36_VHT40(self):
        self._test_rvr()

    @test_tracker_info(uuid='6190c1a6-08f2-4a27-a65f-7321801f2cd6')
    def test_rvr_TCP_DL_ch36_VHT80(self):
        self._test_rvr()

    @test_tracker_info(uuid='ae12712d-0ac3-4317-827d-544acfa4910c')
    def test_rvr_TCP_UL_ch36_VHT80(self):
        self._test_rvr()

    @test_tracker_info(uuid='c8f8d107-5176-484b-a0d9-7a63aef8677e')
    def test_rvr_TCP_DL_ch40_VHT20(self):
        self._test_rvr()

    @test_tracker_info(uuid='6fa823c9-54bf-450d-b2c3-31a46fc73386')
    def test_rvr_TCP_UL_ch40_VHT20(self):
        self._test_rvr()

    @test_tracker_info(uuid='aa6cd955-eaef-4552-87a4-c4a0df59e184')
    def test_rvr_TCP_DL_ch44_VHT20(self):
        self._test_rvr()

    @test_tracker_info(uuid='14ad4b1c-7c8f-4650-be74-daf813021ad3')
    def test_rvr_TCP_UL_ch44_VHT20(self):
        self._test_rvr()

    @test_tracker_info(uuid='a5fdb54c-60e2-4cc6-a9ec-1a17e7827823')
    def test_rvr_TCP_DL_ch44_VHT40(self):
        self._test_rvr()

    @test_tracker_info(uuid='112113f1-7f50-4112-81b5-d9a4fdf153e7')
    def test_rvr_TCP_UL_ch44_VHT40(self):
        self._test_rvr()

    @test_tracker_info(uuid='cda3886c-8776-4077-acfd-cfe128772e2f')
    def test_rvr_TCP_DL_ch48_VHT20(self):
        self._test_rvr()

    @test_tracker_info(uuid='2e5ad031-6404-4e71-b3b3-8a3bb2c85d4f')
    def test_rvr_TCP_UL_ch48_VHT20(self):
        self._test_rvr()

    @test_tracker_info(uuid='c2e199ce-d23f-4a24-b146-74e762085620')
    def test_rvr_TCP_DL_ch52_VHT20(self):
        self._test_rvr()

    @test_tracker_info(uuid='5c5943e8-9d91-4270-a5ab-e7018807c64e')
    def test_rvr_TCP_UL_ch52_VHT20(self):
        self._test_rvr()

    @test_tracker_info(uuid='b52afe89-182f-4bad-8879-cbf7001d28ef')
    def test_rvr_TCP_DL_ch56_VHT20(self):
        self._test_rvr()

    @test_tracker_info(uuid='f8526241-3b96-463a-9082-a749a8650d5f')
    def test_rvr_TCP_UL_ch56_VHT20(self):
        self._test_rvr()

    @test_tracker_info(uuid='c3042d7e-7468-4ab8-aec3-9b3088ba3e4c')
    def test_rvr_TCP_DL_ch60_VHT20(self):
        self._test_rvr()

    @test_tracker_info(uuid='80426542-b035-4fb3-9010-e997f95d4964')
    def test_rvr_TCP_UL_ch60_VHT20(self):
        self._test_rvr()

    @test_tracker_info(uuid='aa0e7117-390c-4265-adf2-0990f65f8b0b')
    def test_rvr_TCP_DL_ch64_VHT20(self):
        self._test_rvr()

    @test_tracker_info(uuid='b2fdda85-256b-4368-8e8b-39274062264e')
    def test_rvr_TCP_UL_ch64_VHT20(self):
        self._test_rvr()

    @test_tracker_info(uuid='48b6590f-1553-4170-83a5-40d3976e9e77')
    def test_rvr_TCP_DL_ch100_VHT20(self):
        self._test_rvr()

    @test_tracker_info(uuid='2d0525fe-57ce-49d3-826d-4ebedd2ca6d6')
    def test_rvr_TCP_UL_ch100_VHT20(self):
        self._test_rvr()

    @test_tracker_info(uuid='52da922d-6c2f-4afa-aca3-c19438ae3217')
    def test_rvr_TCP_DL_ch100_VHT40(self):
        self._test_rvr()

    @test_tracker_info(uuid='2c7e7106-88c8-47ba-ac28-362475abec41')
    def test_rvr_TCP_UL_ch100_VHT40(self):
        self._test_rvr()

    @test_tracker_info(uuid='fd4a7118-e9fe-4931-b32c-f69efd3e6493')
    def test_rvr_TCP_DL_ch100_VHT80(self):
        self._test_rvr()

    @test_tracker_info(uuid='146502b2-9cab-4bbe-8a5c-7ec625edc2ef')
    def test_rvr_TCP_UL_ch100_VHT80(self):
        self._test_rvr()

    @test_tracker_info(uuid='a5e185d6-b523-4016-bc8a-2a32cdc67ae0')
    def test_rvr_TCP_DL_ch104_VHT20(self):
        self._test_rvr()

    @test_tracker_info(uuid='886aed91-0fdc-432d-b47e-ebfa85ac27ad')
    def test_rvr_TCP_UL_ch104_VHT20(self):
        self._test_rvr()

    @test_tracker_info(uuid='fda3de6e-3183-401b-b98c-1b076da139e1')
    def test_rvr_TCP_DL_ch108_VHT20(self):
        self._test_rvr()

    @test_tracker_info(uuid='29cc30f5-bbc8-4b64-9789-a56154907af5')
    def test_rvr_TCP_UL_ch108_VHT20(self):
        self._test_rvr()

    @test_tracker_info(uuid='5c52ccac-8c38-46fa-a7b3-d714b6a814ad')
    def test_rvr_TCP_DL_ch112_VHT20(self):
        self._test_rvr()

    @test_tracker_info(uuid='cc1c2a0b-71a3-4343-b7ff-489527c839d2')
    def test_rvr_TCP_UL_ch112_VHT20(self):
        self._test_rvr()

    @test_tracker_info(uuid='11c6ccc3-e347-44ce-9a79-6c90e9dfd0a0')
    def test_rvr_TCP_DL_ch116_VHT20(self):
        self._test_rvr()

    @test_tracker_info(uuid='29f0fce1-005d-4ad7-97d7-6b43cbdff01b')
    def test_rvr_TCP_UL_ch116_VHT20(self):
        self._test_rvr()

    @test_tracker_info(uuid='112302b1-8261-479a-b397-916b08fbbdd2')
    def test_rvr_TCP_DL_ch132_VHT20(self):
        self._test_rvr()

    @test_tracker_info(uuid='3bb0efb8-ddfc-4a0b-b7cf-6d6af1dbb9f4')
    def test_rvr_TCP_UL_ch132_VHT20(self):
        self._test_rvr()

    @test_tracker_info(uuid='11a4638f-d872-4730-82eb-71d9c64e0e16')
    def test_rvr_TCP_DL_ch136_VHT20(self):
        self._test_rvr()

    @test_tracker_info(uuid='4d797c24-3bbe-43a6-ac9e-291db1aa732a')
    def test_rvr_TCP_UL_ch136_VHT20(self):
        self._test_rvr()

    @test_tracker_info(uuid='5d433b44-0395-43cb-b85a-be138390b18b')
    def test_rvr_TCP_DL_ch140_VHT20(self):
        self._test_rvr()

    @test_tracker_info(uuid='47061772-21b1-4330-bd4f-daec21afa0c8')
    def test_rvr_TCP_UL_ch140_VHT20(self):
        self._test_rvr()

    @test_tracker_info(uuid='24aa1e7a-3978-4803-877f-3ac5812ab0ae')
    def test_rvr_TCP_DL_ch149_VHT20(self):
        self._test_rvr()

    @test_tracker_info(uuid='59f0443f-822d-4347-9c52-310f0b812500')
    def test_rvr_TCP_UL_ch149_VHT20(self):
        self._test_rvr()

    @test_tracker_info(uuid='3b1524b3-af15-41f1-8fca-9ee9b687d59a')
    def test_rvr_TCP_DL_ch149_VHT40(self):
        self._test_rvr()

    @test_tracker_info(uuid='36670787-3bfb-4e8b-8881-e88eb608ed46')
    def test_rvr_TCP_UL_ch149_VHT40(self):
        self._test_rvr()

    @test_tracker_info(uuid='8350cddd-7c62-4fad-bdae-a8267d321aa3')
    def test_rvr_TCP_DL_ch149_VHT80(self):
        self._test_rvr()

    @test_tracker_info(uuid='7432fccb-526e-44d4-b0f4-2c343ca53188')
    def test_rvr_TCP_UL_ch149_VHT80(self):
        self._test_rvr()

    @test_tracker_info(uuid='037eec49-2bae-49e3-949e-5af2885dc84b')
    def test_rvr_TCP_DL_ch153_VHT20(self):
        self._test_rvr()

    @test_tracker_info(uuid='04d5d873-7d5a-4590-bff3-093edeb92380')
    def test_rvr_TCP_UL_ch153_VHT20(self):
        self._test_rvr()

    @test_tracker_info(uuid='4ff83f6e-b130-4a88-8ced-04a09c6af666')
    def test_rvr_TCP_DL_ch157_VHT20(self):
        self._test_rvr()

    @test_tracker_info(uuid='c3436402-977e-40a5-a7eb-e2c886379d43')
    def test_rvr_TCP_UL_ch157_VHT20(self):
        self._test_rvr()

    @test_tracker_info(uuid='797a218b-1a8e-4233-835b-61b3f057f480')
    def test_rvr_TCP_DL_ch157_VHT40(self):
        self._test_rvr()

    @test_tracker_info(uuid='38d3e825-6e2c-4931-b0fd-aa19c5d1ef40')
    def test_rvr_TCP_UL_ch157_VHT40(self):
        self._test_rvr()

    @test_tracker_info(uuid='993e98c5-0647-4ed6-b62e-ab386ada37af')
    def test_rvr_TCP_DL_ch161_VHT20(self):
        self._test_rvr()

    @test_tracker_info(uuid='3bf9c844-749a-47d8-ac46-89249bd92c4a')
    def test_rvr_TCP_UL_ch161_VHT20(self):
        self._test_rvr()

    @test_tracker_info(uuid='05614f92-38fa-4289-bcff-d4b4a2a2ad5b')
    def test_rvr_UDP_DL_ch6_VHT20(self):
        self._test_rvr()

    @test_tracker_info(uuid='577632e9-fb2f-4a2b-b3c3-affee8264008')
    def test_rvr_UDP_UL_ch6_VHT20(self):
        self._test_rvr()

    @test_tracker_info(uuid='6f3fcc28-5f0c-49e6-8810-69c5873ecafa')
    def test_rvr_UDP_DL_ch36_VHT20(self):
        self._test_rvr()

    @test_tracker_info(uuid='8e518aaa-e61f-4c1d-b12f-1bbd550ec3e5')
    def test_rvr_UDP_UL_ch36_VHT20(self):
        self._test_rvr()

    @test_tracker_info(uuid='fd68ff32-c789-4a86-9924-2f5aeb3c9651')
    def test_rvr_UDP_DL_ch149_VHT20(self):
        self._test_rvr()

    @test_tracker_info(uuid='29d03492-fc0b-42d0-aa15-c0c838ba50c1')
    def test_rvr_UDP_UL_ch149_VHT20(self):
        self._test_rvr()

    @test_tracker_info(uuid='044c414c-ac5e-4e28-9b56-a602e0cc9724')
    def test_rvr_UDP_DL_ch36_VHT40(self):
        self._test_rvr()

    @test_tracker_info(uuid='9cd16689-5053-4ffa-813c-d901384a105c')
    def test_rvr_UDP_UL_ch36_VHT40(self):
        self._test_rvr()

    @test_tracker_info(uuid='4e4b1e73-30ce-4005-9c34-8c0280bdb293')
    def test_rvr_UDP_DL_ch36_VHT80(self):
        self._test_rvr()

    @test_tracker_info(uuid='780166a1-1847-45c2-b509-71612c82309d')
    def test_rvr_UDP_UL_ch36_VHT80(self):
        self._test_rvr()

    @test_tracker_info(uuid='05abdb89-9744-479e-8443-cb8b9427f5e3')
    def test_rvr_UDP_DL_ch149_VHT40(self):
        self._test_rvr()

    @test_tracker_info(uuid='a321590a-4cbc-4044-9c2b-24e90f444213')
    def test_rvr_UDP_UL_ch149_VHT40(self):
        self._test_rvr()

    @test_tracker_info(uuid='041fd613-24d9-4606-bca3-0ae0d8436b5e')
    def test_rvr_UDP_DL_ch149_VHT80(self):
        self._test_rvr()

    @test_tracker_info(uuid='69aab23d-1408-4cdd-9f57-2520a1e9cea8')
    def test_rvr_UDP_UL_ch149_VHT80(self):
        self._test_rvr()


# Classes defining test suites
class WifiRvr_2GHz_Test(WifiRvrTest):
    def __init__(self, controllers):
        base_test.BaseTestClass.__init__(self, controllers)
        self.tests = ("test_rvr_TCP_DL_ch1_VHT20", "test_rvr_TCP_UL_ch1_VHT20",
                      "test_rvr_TCP_DL_ch6_VHT20", "test_rvr_TCP_UL_ch6_VHT20",
                      "test_rvr_TCP_DL_ch11_VHT20",
                      "test_rvr_TCP_UL_ch11_VHT20")


class WifiRvr_UNII1_Test(WifiRvrTest):
    def __init__(self, controllers):
        base_test.BaseTestClass.__init__(self, controllers)
        self.tests = (
            "test_rvr_TCP_DL_ch36_VHT20", "test_rvr_TCP_UL_ch36_VHT20",
            "test_rvr_TCP_DL_ch36_VHT40", "test_rvr_TCP_UL_ch36_VHT40",
            "test_rvr_TCP_DL_ch36_VHT80", "test_rvr_TCP_UL_ch36_VHT80",
            "test_rvr_TCP_DL_ch40_VHT20", "test_rvr_TCP_UL_ch40_VHT20",
            "test_rvr_TCP_DL_ch44_VHT20", "test_rvr_TCP_UL_ch44_VHT20",
            "test_rvr_TCP_DL_ch44_VHT40", "test_rvr_TCP_UL_ch44_VHT40",
            "test_rvr_TCP_DL_ch48_VHT20", "test_rvr_TCP_UL_ch48_VHT20")


class WifiRvr_UNII3_Test(WifiRvrTest):
    def __init__(self, controllers):
        base_test.BaseTestClass.__init__(self, controllers)
        self.tests = (
            "test_rvr_TCP_DL_ch149_VHT20", "test_rvr_TCP_UL_ch149_VHT20",
            "test_rvr_TCP_DL_ch149_VHT40", "test_rvr_TCP_UL_ch149_VHT40",
            "test_rvr_TCP_DL_ch149_VHT80", "test_rvr_TCP_UL_ch149_VHT80",
            "test_rvr_TCP_DL_ch153_VHT20", "test_rvr_TCP_UL_ch153_VHT20",
            "test_rvr_TCP_DL_ch157_VHT20", "test_rvr_TCP_UL_ch157_VHT20",
            "test_rvr_TCP_DL_ch157_VHT40", "test_rvr_TCP_UL_ch157_VHT40",
            "test_rvr_TCP_DL_ch161_VHT20", "test_rvr_TCP_UL_ch161_VHT20")


class WifiRvr_SampleDFS_Test(WifiRvrTest):
    def __init__(self, controllers):
        base_test.BaseTestClass.__init__(self, controllers)
        self.tests = (
            "test_rvr_TCP_DL_ch64_VHT20", "test_rvr_TCP_UL_ch64_VHT20",
            "test_rvr_TCP_DL_ch100_VHT20", "test_rvr_TCP_UL_ch100_VHT20",
            "test_rvr_TCP_DL_ch100_VHT40", "test_rvr_TCP_UL_ch100_VHT40",
            "test_rvr_TCP_DL_ch100_VHT80", "test_rvr_TCP_UL_ch100_VHT80",
            "test_rvr_TCP_DL_ch116_VHT20", "test_rvr_TCP_UL_ch116_VHT20",
            "test_rvr_TCP_DL_ch132_VHT20", "test_rvr_TCP_UL_ch132_VHT20",
            "test_rvr_TCP_DL_ch140_VHT20", "test_rvr_TCP_UL_ch140_VHT20")


class WifiRvr_SampleUDP_Test(WifiRvrTest):
    def __init__(self, controllers):
        base_test.BaseTestClass.__init__(self, controllers)
        self.tests = (
            "test_rvr_UDP_DL_ch6_VHT20", "test_rvr_UDP_UL_ch6_VHT20",
            "test_rvr_UDP_DL_ch36_VHT20", "test_rvr_UDP_UL_ch36_VHT20",
            "test_rvr_UDP_DL_ch36_VHT40", "test_rvr_UDP_UL_ch36_VHT40",
            "test_rvr_UDP_DL_ch36_VHT80", "test_rvr_UDP_UL_ch36_VHT80",
            "test_rvr_UDP_DL_ch149_VHT20", "test_rvr_UDP_UL_ch149_VHT20",
            "test_rvr_UDP_DL_ch149_VHT40", "test_rvr_UDP_UL_ch149_VHT40",
            "test_rvr_UDP_DL_ch149_VHT80", "test_rvr_UDP_UL_ch149_VHT80")


class WifiRvr_TCP_All_Test(WifiRvrTest):
    def __init__(self, controllers):
        base_test.BaseTestClass.__init__(self, controllers)
        self.tests = (
            "test_rvr_TCP_DL_ch1_VHT20", "test_rvr_TCP_UL_ch1_VHT20",
            "test_rvr_TCP_DL_ch6_VHT20", "test_rvr_TCP_UL_ch6_VHT20",
            "test_rvr_TCP_DL_ch11_VHT20", "test_rvr_TCP_UL_ch11_VHT20",
            "test_rvr_TCP_DL_ch36_VHT20", "test_rvr_TCP_UL_ch36_VHT20",
            "test_rvr_TCP_DL_ch36_VHT40", "test_rvr_TCP_UL_ch36_VHT40",
            "test_rvr_TCP_DL_ch36_VHT80", "test_rvr_TCP_UL_ch36_VHT80",
            "test_rvr_TCP_DL_ch40_VHT20", "test_rvr_TCP_UL_ch40_VHT20",
            "test_rvr_TCP_DL_ch44_VHT20", "test_rvr_TCP_UL_ch44_VHT20",
            "test_rvr_TCP_DL_ch44_VHT40", "test_rvr_TCP_UL_ch44_VHT40",
            "test_rvr_TCP_DL_ch48_VHT20", "test_rvr_TCP_UL_ch48_VHT20",
            "test_rvr_TCP_DL_ch149_VHT20", "test_rvr_TCP_UL_ch149_VHT20",
            "test_rvr_TCP_DL_ch149_VHT40", "test_rvr_TCP_UL_ch149_VHT40",
            "test_rvr_TCP_DL_ch149_VHT80", "test_rvr_TCP_UL_ch149_VHT80",
            "test_rvr_TCP_DL_ch153_VHT20", "test_rvr_TCP_UL_ch153_VHT20",
            "test_rvr_TCP_DL_ch157_VHT20", "test_rvr_TCP_UL_ch157_VHT20",
            "test_rvr_TCP_DL_ch157_VHT40", "test_rvr_TCP_UL_ch157_VHT40",
            "test_rvr_TCP_DL_ch161_VHT20", "test_rvr_TCP_UL_ch161_VHT20")


class WifiRvr_TCP_Downlink_Test(WifiRvrTest):
    def __init__(self, controllers):
        base_test.BaseTestClass.__init__(self, controllers)
        self.tests = (
            "test_rvr_TCP_DL_ch1_VHT20", "test_rvr_TCP_DL_ch6_VHT20",
            "test_rvr_TCP_DL_ch11_VHT20", "test_rvr_TCP_DL_ch36_VHT20",
            "test_rvr_TCP_DL_ch36_VHT40", "test_rvr_TCP_DL_ch36_VHT80",
            "test_rvr_TCP_DL_ch40_VHT20", "test_rvr_TCP_DL_ch44_VHT20",
            "test_rvr_TCP_DL_ch44_VHT40", "test_rvr_TCP_DL_ch48_VHT20",
            "test_rvr_TCP_DL_ch149_VHT20", "test_rvr_TCP_DL_ch149_VHT40",
            "test_rvr_TCP_DL_ch149_VHT80", "test_rvr_TCP_DL_ch153_VHT20",
            "test_rvr_TCP_DL_ch157_VHT20", "test_rvr_TCP_DL_ch157_VHT40",
            "test_rvr_TCP_DL_ch161_VHT20")


class WifiRvr_TCP_Uplink_Test(WifiRvrTest):
    def __init__(self, controllers):
        base_test.BaseTestClass.__init__(self, controllers)
        self.tests = (
            "test_rvr_TCP_UL_ch1_VHT20", "test_rvr_TCP_UL_ch6_VHT20",
            "test_rvr_TCP_UL_ch11_VHT20", "test_rvr_TCP_UL_ch36_VHT20",
            "test_rvr_TCP_UL_ch36_VHT40", "test_rvr_TCP_UL_ch36_VHT80",
            "test_rvr_TCP_UL_ch40_VHT20", "test_rvr_TCP_UL_ch44_VHT20",
            "test_rvr_TCP_UL_ch44_VHT40", "test_rvr_TCP_UL_ch48_VHT20",
            "test_rvr_TCP_UL_ch149_VHT20", "test_rvr_TCP_UL_ch149_VHT40",
            "test_rvr_TCP_UL_ch149_VHT80", "test_rvr_TCP_UL_ch153_VHT20",
            "test_rvr_TCP_UL_ch157_VHT20", "test_rvr_TCP_UL_ch157_VHT40",
            "test_rvr_TCP_UL_ch161_VHT20")
