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

import os
import threading
import time

from acts import base_test
from acts import asserts
from acts import utils
from acts.controllers import adb
from acts.controllers import iperf_server as ip_server
from acts.controllers import monsoon
from acts.test_decorators import test_tracker_info
from acts.test_utils.wifi import wifi_test_utils as wutils
from acts.utils import force_airplane_mode
from acts.utils import set_adaptive_brightness
from acts.utils import set_ambient_display
from acts.utils import set_auto_rotate
from acts.utils import set_location_service

pmc_base_cmd = ("am broadcast -a com.android.pmc.action.AUTOPOWER --es"
                " PowerAction ")
start_pmc_cmd = ("am start -S -n com.android.pmc/com.android.pmc."
                 "PMCMainActivity")
pmc_interval_cmd = ("am broadcast -a com.android.pmc.action.SETPARAMS --es "
                    "Interval %s ")
pmc_set_params = "am broadcast -a com.android.pmc.action.SETPARAMS --es "

pmc_start_connect_scan_cmd = "%sStartConnectivityScan" % pmc_base_cmd
pmc_stop_connect_scan_cmd = "%sStopConnectivityScan" % pmc_base_cmd
pmc_start_gscan_no_dfs_cmd = "%sStartGScanBand" % pmc_base_cmd
pmc_start_gscan_specific_channels_cmd = "%sStartGScanChannel" % pmc_base_cmd
pmc_stop_gscan_cmd = "%sStopGScan" % pmc_base_cmd
pmc_start_iperf_client = "%sStartIperfClient" % pmc_base_cmd
pmc_stop_iperf_client = "%sStopIperfClient" % pmc_base_cmd
pmc_turn_screen_on = "%sTurnScreenOn" % pmc_base_cmd
pmc_turn_screen_off = "%sTurnScreenOff" % pmc_base_cmd
# Path of the iperf json output file from an iperf run.
pmc_iperf_json_file = "/sdcard/iperf.txt"


class WifiPowerTest(base_test.BaseTestClass):
    def setup_class(self):
        self.offset = 5 * 60
        self.hz = 5000
        self.scan_interval = 15
        # Continuosly download
        self.download_interval = 0
        self.mon_data_path = os.path.join(self.log_path, "Monsoon")
        self.mon = self.monsoons[0]
        self.mon.set_voltage(4.2)
        self.mon.set_max_current(7.8)
        self.dut = self.android_devices[0]
        self.mon.attach_device(self.dut)
        asserts.assert_true(
            self.mon.usb("auto"),
            "Failed to turn USB mode to auto on monsoon.")
        asserts.assert_true(
            force_airplane_mode(self.dut, True),
            "Can not turn on airplane mode on: %s" % self.dut.serial)
        set_location_service(self.dut, False)
        set_adaptive_brightness(self.dut, False)
        set_ambient_display(self.dut, False)
        self.dut.adb.shell("settings put system screen_brightness 0")
        set_auto_rotate(self.dut, False)
        required_userparam_names = (
            # These two params should follow the format of
            # {"SSID": <SSID>, "password": <Password>}
            "network_2g",
            "network_5g",
            "iperf_server_address")
        self.unpack_userparams(required_userparam_names, threshold=None)
        wutils.wifi_test_device_init(self.dut)
        try:
            self.attn = self.attenuators[0]
            self.attn.set_atten(0)
        except AttributeError:
            self.log.warning("No attenuator found, some tests will fail.")
            pass

    def teardown_class(self):
        self.mon.usb("on")

    def setup_test(self):
        # Default measurement time is 30min with an offset of 5min. Each test
        # can overwrite this by setting self.duration and self.offset.
        self.offset = 5 * 60
        self.duration = 20 * 60 + self.offset
        self.start_pmc()
        wutils.reset_wifi(self.dut)
        self.dut.ed.clear_all_events()

    def on_fail(self, test_name, begin_time):
        self.dut.take_bug_report(test_name, begin_time)

    def on_pass(self, test_name, begin_time):
        self.dut.take_bug_report(test_name, begin_time)

    def start_pmc(self):
        """Starts a new instance of PMC app on the device and initializes it.

        This function performs the following:
        1. Starts a new instance of PMC (killing any existing instances).
        2. Turns on PMC verbose logging.
        3. Sets up the server IP address/port for download/iperf tests.
        4. Removes an existing iperf json output files.
        """
        self.dut.adb.shell(start_pmc_cmd)
        self.dut.adb.shell("setprop log.tag.PMC VERBOSE")
        self.iperf_server = self.iperf_servers[0]
        # Setup iperf related params on the client side.
        self.dut.adb.shell("%sServerIP %s" % (pmc_set_params,
                                              self.iperf_server_address))
        self.dut.adb.shell("%sServerPort %s" % (pmc_set_params,
                                                self.iperf_server.port))
        try:
            self.dut.adb.shell("rm %s" % pmc_iperf_json_file)
        except adb.AdbError:
            pass

    def get_iperf_result(self):
        """Pulls the iperf json output from device.

        Returns:
            An IPerfResult object based on the iperf run output.
        """
        dest = os.path.join(self.iperf_server.log_path, "iperf.txt")
        self.dut.adb.pull(pmc_iperf_json_file, " ", dest)
        result = ip_server.IPerfResult(dest)
        self.dut.adb.shell("rm %s" % pmc_iperf_json_file)
        return result

    def measure_and_process_result(self):
        """Measure the current drawn by the device for the period of
        self.duration, at the frequency of self.hz.

        If self.threshold exists, also verify that the average current of the
        measurement is below the acceptable threshold.
        """
        tag = self.current_test_name
        result = self.mon.measure_power(self.hz,
                                        self.duration,
                                        tag=tag,
                                        offset=self.offset)
        asserts.assert_true(result,
                            "Got empty measurement data set in %s." % tag)
        self.log.info(repr(result))
        data_path = os.path.join(self.mon_data_path, "%s.txt" % tag)
        monsoon.MonsoonData.save_to_text_file([result], data_path)
        actual_current = result.average_current
        actual_current_str = "%.2fmA" % actual_current
        result_extra = {"Average Current": actual_current_str}
        if "continuous_traffic" in tag:
            self.dut.adb.shell(pmc_stop_iperf_client)
            iperf_result = self.get_iperf_result()
            asserts.assert_true(iperf_result.avg_rate,
                                "Failed to send iperf traffic",
                                extras=result_extra)
            rate = "%.2fMB/s" % iperf_result.avg_rate
            result_extra["Average Rate"] = rate
        model = utils.trim_model_name(self.dut.model)
        if self.threshold and (model in self.threshold) and (
                tag in self.threshold[model]):
            acceptable_threshold = self.threshold[model][tag]
            asserts.assert_true(
                actual_current < acceptable_threshold,
                ("Measured average current in [%s]: %s, which is "
                 "higher than acceptable threshold %.2fmA.") % (
                     tag, actual_current_str, acceptable_threshold),
                extras=result_extra)
        asserts.explicit_pass("Measurement finished for %s." % tag,
                              extras=result_extra)

    @test_tracker_info(uuid="99ed6d06-ad07-4650-8434-0ac9d856fafa")
    def test_power_wifi_off(self):
        wutils.wifi_toggle_state(self.dut, False)
        self.measure_and_process_result()

    @test_tracker_info(uuid="086db8fd-4040-45ac-8934-49b4d84413fc")
    def test_power_wifi_on_idle(self):
        wutils.wifi_toggle_state(self.dut, True)
        self.measure_and_process_result()

    @test_tracker_info(uuid="031516d9-b0f5-4f21-bc8b-078258852325")
    def test_power_disconnected_connectivity_scan(self):
        try:
            self.dut.adb.shell(pmc_interval_cmd % self.scan_interval)
            self.dut.adb.shell(pmc_start_connect_scan_cmd)
            self.log.info("Started connectivity scan.")
            self.measure_and_process_result()
        finally:
            self.dut.adb.shell(pmc_stop_connect_scan_cmd)
            self.log.info("Stoped connectivity scan.")

    @test_tracker_info(uuid="5e1f92d7-a79e-459c-aff0-d4acba3adee4")
    def test_power_connected_2g_idle(self):
        wutils.reset_wifi(self.dut)
        self.dut.ed.clear_all_events()
        wutils.wifi_connect(self.dut, self.network_2g)
        self.measure_and_process_result()

    @test_tracker_info(uuid="e2b4ab89-420e-4560-a08b-d3bf4336f05d")
    def test_power_connected_2g_continuous_traffic(self):
        try:
            wutils.reset_wifi(self.dut)
            self.dut.ed.clear_all_events()
            wutils.wifi_connect(self.dut, self.network_2g)
            self.iperf_server.start()
            self.dut.adb.shell(pmc_start_iperf_client)
            self.log.info("Started iperf traffic.")
            self.measure_and_process_result()
        finally:
            self.iperf_server.stop()
            self.log.info("Stopped iperf traffic.")

    @test_tracker_info(uuid="a9517306-b967-494e-b471-84de58df8f1b")
    def test_power_connected_5g_idle(self):
        wutils.reset_wifi(self.dut)
        self.dut.ed.clear_all_events()
        wutils.wifi_connect(self.dut, self.network_5g)
        self.measure_and_process_result()

    @test_tracker_info(uuid="816716b3-a90b-4835-84b8-d8d761ebfba9")
    def test_power_connected_5g_continuous_traffic(self):
        try:
            wutils.reset_wifi(self.dut)
            self.dut.ed.clear_all_events()
            wutils.wifi_connect(self.dut, self.network_5g)
            self.iperf_server.start()
            self.dut.adb.shell(pmc_start_iperf_client)
            self.log.info("Started iperf traffic.")
            self.measure_and_process_result()
        finally:
            self.iperf_server.stop()
            self.log.info("Stopped iperf traffic.")

    @test_tracker_info(uuid="e2d08e4e-7863-4554-af63-64d41ab0976a")
    def test_power_gscan_three_2g_channels(self):
        try:
            self.dut.adb.shell(pmc_interval_cmd % self.scan_interval)
            self.dut.adb.shell(pmc_start_gscan_specific_channels_cmd)
            self.log.info("Started gscan for 2G channels 1, 6, and 11.")
            self.measure_and_process_result()
        finally:
            self.dut.adb.shell(pmc_stop_gscan_cmd)
            self.log.info("Stopped gscan.")

    @test_tracker_info(uuid="0095b7e7-94b9-4cd9-912f-51971949748b")
    def test_power_gscan_all_channels_no_dfs(self):
        try:
            self.dut.adb.shell(pmc_interval_cmd % self.scan_interval)
            self.dut.adb.shell(pmc_start_gscan_no_dfs_cmd)
            self.log.info("Started gscan for all non-DFS channels.")
            self.measure_and_process_result()
        finally:
            self.dut.adb.shell(pmc_stop_gscan_cmd)
            self.log.info("Stopped gscan.")

    @test_tracker_info(uuid="263d1b68-8eb0-4e7f-99d4-3ca23ca359ce")
    def test_power_connected_2g_gscan_all_channels_no_dfs(self):
        try:
            wutils.wifi_connect(self.dut, self.network_2g)
            self.dut.adb.shell(pmc_interval_cmd % self.scan_interval)
            self.dut.adb.shell(pmc_start_gscan_no_dfs_cmd)
            self.log.info("Started gscan for all non-DFS channels.")
            self.measure_and_process_result()
        finally:
            self.dut.adb.shell(pmc_stop_gscan_cmd)
            self.log.info("Stopped gscan.")

    @test_tracker_info(uuid="aad1a39d-01f9-4fa5-a23a-b85d54210f3c")
    def test_power_connected_5g_gscan_all_channels_no_dfs(self):
        try:
            wutils.wifi_connect(self.dut, self.network_5g)
            self.dut.adb.shell(pmc_interval_cmd % self.scan_interval)
            self.dut.adb.shell(pmc_start_gscan_no_dfs_cmd)
            self.log.info("Started gscan for all non-DFS channels.")
            self.measure_and_process_result()
        finally:
            self.dut.adb.shell(pmc_stop_gscan_cmd)
            self.log.info("Stopped gscan.")

    @test_tracker_info(uuid="8f72cd5f-1c66-4ced-92d9-b7ebadf76424")
    def test_power_auto_reconnect(self):
        """
        Steps:
            1. Connect to network, wait for three minutes.
            2. Attenuate AP away, wait for one minute.
            3. Make AP reappear, wait for three minutes for the device to
               reconnect to the Wi-Fi network.
        """
        self.attn.set_atten(0)
        wutils.wifi_connect(self.dut, self.network_2g)

        def attn_control():
            for i in range(7):
                self.log.info("Iteration %s: Idle 3min after AP appeared.", i)
                time.sleep(3 * 60)
                self.attn.set_atten(90)
                self.log.info("Iteration %s: Idle 1min after AP disappeared.",
                              i)
                time.sleep(60)
                self.attn.set_atten(0)

        t = threading.Thread(target=attn_control)
        t.start()
        try:
            self.measure_and_process_result()
        finally:
            t.join()

    @test_tracker_info(uuid="a6db5964-3c68-47fa-b4c9-49f880549031")
    def test_power_screen_on_wifi_off(self):
        self.duration = 10 * 60
        self.offset = 4 * 60
        wutils.wifi_toggle_state(self.dut, False)
        try:
            self.dut.adb.shell(pmc_turn_screen_on)
            self.measure_and_process_result()
        finally:
            self.dut.adb.shell(pmc_turn_screen_off)

    @test_tracker_info(uuid="230d667a-aa42-4123-9dae-2036429ed574")
    def test_power_screen_on_wifi_connected_2g_idle(self):
        self.duration = 10 * 60
        self.offset = 4 * 60
        wutils.wifi_toggle_state(self.dut, True)
        wutils.wifi_connect(self.dut, self.network_2g)
        try:
            self.dut.adb.shell(pmc_turn_screen_on)
            self.measure_and_process_result()
        finally:
            self.dut.adb.shell(pmc_turn_screen_off)
