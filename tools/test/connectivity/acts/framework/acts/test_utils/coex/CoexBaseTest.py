#/usr/bin/env python3.4
#
# Copyright (C) 2018 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.

import os
import threading
import time

from acts.base_test import BaseTestClass
from acts.controllers.iperf_client import IPerfClient
from acts.test_utils.bt.bt_test_utils import disable_bluetooth
from acts.test_utils.bt.bt_test_utils import enable_bluetooth
from acts.test_utils.bt.bt_test_utils import setup_multiple_devices_for_bt_test
from acts.test_utils.bt.bt_test_utils import take_btsnoop_logs
from acts.test_utils.coex.coex_test_utils import configure_and_start_ap
from acts.test_utils.coex.coex_test_utils import iperf_result
from acts.test_utils.coex.coex_test_utils import get_phone_ip
from acts.test_utils.coex.coex_test_utils import wifi_connection_check
from acts.test_utils.coex.coex_test_utils import xlsheet
from acts.test_utils.wifi.wifi_test_utils import reset_wifi
from acts.test_utils.wifi.wifi_test_utils import wifi_connect
from acts.test_utils.wifi.wifi_test_utils import wifi_test_device_init
from acts.test_utils.wifi.wifi_test_utils import wifi_toggle_state
from acts.utils import create_dir
from acts.utils import start_standing_subprocess
from acts.utils import stop_standing_subprocess

TEST_CASE_TOKEN = "[Test Case]"
RESULT_LINE_TEMPLATE = TEST_CASE_TOKEN + " %s %s"
IPERF_SERVER_WAIT_TIME = 5


class CoexBaseTest(BaseTestClass):

    def __init__(self, controllers):
        BaseTestClass.__init__(self, controllers)
        self.pri_ad = self.android_devices[0]
        if len(self.android_devices) == 2:
            self.sec_ad = self.android_devices[1]
        elif len(self.android_devices) == 3:
            self.third_ad = self.android_devices[2]

    def setup_class(self):
        self.tag = 0
        self.iperf_result = {}
        self.thread_list = []
        if not setup_multiple_devices_for_bt_test(self.android_devices):
            self.log.error("Failed to setup devices for bluetooth test")
            return False
        req_params = ["network", "iperf"]
        self.unpack_userparams(req_params)
        if "RelayDevice" in self.user_params:
            self.audio_receiver = self.relay_devices[0]
        else:
            self.log.warning("Missing Relay config file.")
        if "music_file" in self.user_params:
            self.push_music_to_android_device(self.pri_ad)
        self.path = self.pri_ad.log_path
        if "AccessPoints" in self.user_params:
            self.ap = self.access_points[0]
            configure_and_start_ap(self.ap, self.network)
        wifi_test_device_init(self.pri_ad)
        wifi_connect(self.pri_ad, self.network)

    def setup_test(self):
        self.received = []
        for a in self.android_devices:
            a.ed.clear_all_events()
        if not wifi_connection_check(self.pri_ad, self.network["SSID"]):
            self.log.error("Wifi connection does not exist")
            return False
        if not enable_bluetooth(self.pri_ad.droid, self.pri_ad.ed):
            self.log.error("Failed to enable bluetooth")
            return False

    def teardown_test(self):
        if not disable_bluetooth(self.pri_ad.droid):
            self.log.info("Failed to disable bluetooth")
            return False
        self.teardown_thread()

    def teardown_class(self):
        if "AccessPoints" in self.user_params:
            self.ap.close()
        reset_wifi(self.pri_ad)
        wifi_toggle_state(self.pri_ad, False)
        json_result = self.results.json_str()
        xlsheet(self.pri_ad, json_result)

    def start_iperf_server_on_shell(self, server_port):
        """Starts iperf server on android device with specified.

        Args:
            server_port: Port in which server should be started.
        """
        log_path = os.path.join(self.pri_ad.log_path, "iPerf{}".format(
            server_port))
        iperf_server = "iperf3 -s -p {} -J".format(server_port)
        log_files = []
        create_dir(log_path)
        out_file_name = "IPerfServer,{},{},{}.log".format(
            server_port, self.tag, log_files)
        self.tag = self.tag + 1
        full_out_path = os.path.join(log_path, out_file_name)
        cmd = "adb -s {} shell {} > {}".format(
            self.pri_ad.serial, iperf_server, full_out_path)
        self.iperf_process.append(start_standing_subprocess(cmd))
        log_files.append(full_out_path)
        time.sleep(IPERF_SERVER_WAIT_TIME)

    def stop_iperf_server_on_shell(self):
        """Stops all the instances of iperf server on shell."""
        try:
            for process in self.iperf_process:
                stop_standing_subprocess(process)
        except Exception:
            self.log.info("Iperf server already killed")

    def run_iperf_and_get_result(self):
        """Frames iperf command based on test and starts iperf client on
        host machine.
        """
        self.flag_list = []
        self.iperf_process = []
        test_params = self.current_test_name.split("_")

        self.protocol = test_params[-2:-1]
        self.stream = test_params[-1:]

        if self.protocol[0] == "tcp":
            self.iperf_args = "-t {} -p {} {} -J".format(
                self.iperf["duration"], self.iperf["port_1"],
                self.iperf["tcp_window_size"])
        else:
            self.iperf_args = ("-t {} -p {} -u {} --get-server-output -J"
                               .format(self.iperf["duration"],
                                       self.iperf["port_1"],
                                       self.iperf["udp_bandwidth"]))

        if self.stream[0] == "ul":
            self.iperf_args += " -R"

        if self.protocol[0] == "tcp" and self.stream[0] == "bidirectional":
            self.bidirectional_args = "-t {} -p {} {} -R -J".format(
                self.iperf["duration"], self.iperf["port_2"],
                self.iperf["tcp_window_size"])
        else:
            self.bidirectional_args = ("-t {} -p {} -u {} --get-server-output"
                                       " -J".format(self.iperf["duration"],
                                                    self.iperf["port_2"],
                                                    self.iperf["udp_bandwidth"]
                                                    ))

        if self.stream[0] == "bidirectional":
            self.start_iperf_server_on_shell(self.iperf["port_2"])
        self.start_iperf_server_on_shell(self.iperf["port_1"])

        if self.stream[0] == "bidirectional":
            args = [
                lambda: self.run_iperf(self.iperf_args, self.iperf["port_1"]),
                lambda: self.run_iperf(self.bidirectional_args,
                                       self.iperf["port_2"])
            ]
            self.run_thread(args)
        else:
            args = [
                lambda: self.run_iperf(self.iperf_args, self.iperf["port_1"])
            ]
            self.run_thread(args)

    def run_iperf(self, iperf_args, server_port):
        """Gets android device ip and start iperf client from host machine to
        that ip and parses the iperf result.

        Args:
            iperf_args: Iperf parameters to run traffic.
            server_port: Iperf port to start client.
        """
        ip = get_phone_ip(self.pri_ad)
        iperf_client = IPerfClient(server_port, ip, self.pri_ad.log_path)
        result = iperf_client.start(iperf_args)
        try:
            sent, received = iperf_result(result, self.stream)
            self.received.append(str(round(received, 2)) + "Mb/s")
            self.log.info("Sent: {} Mb/s, Received: {} Mb/s".format(
                sent, received))
            self.flag_list.append(True)

        except TypeError:
            self.log.error("Iperf failed/stopped.")
            self.flag_list.append(False)
            self.received.append("Iperf Failed")

        self.iperf_result[self.current_test_name] = self.received

    def on_fail(self, record, test_name, begin_time):
        self.log.info(
            "Test {} failed, Fetching Btsnoop logs and bugreport".format(
                test_name))
        take_btsnoop_logs(self.android_devices, self, test_name)
        self._take_bug_report(test_name, begin_time)
        record.extras = self.received

    def _on_fail(self, record):
        """Proxy function to guarantee the base implementation of on_fail is
        called.

        Args:
            record: The records.TestResultRecord object for the failed test
            case.
        """
        if record.details:
            self.log.error(record.details)
        self.log.info(RESULT_LINE_TEMPLATE, record.test_name, record.result)
        self.on_fail(record, record.test_name, record.log_begin_time)

    def _on_pass(self, record):
        """Proxy function to guarantee the base implementation of on_pass is
        called.

        Args:
            record: The records.TestResultRecord object for the passed test
            case.
        """
        msg = record.details
        if msg:
            self.log.info(msg)
        self.log.info(RESULT_LINE_TEMPLATE, record.test_name, record.result)
        record.extras = self.received

    def run_thread(self, kwargs):
        """Convenience function to start thread.

        Args:
            kwargs: Function object to start in thread.
        """
        for function in kwargs:
            self.thread = threading.Thread(target=function)
            self.thread_list.append(self.thread)
            self.thread.start()

    def teardown_result(self):
        """Convenience function to join thread and fetch iperf result."""
        for thread_id in self.thread_list:
            if thread_id.is_alive():
                thread_id.join()
        self.stop_iperf_server_on_shell()
        if False in self.flag_list:
            return False
        return True

    def teardown_thread(self):
        """Convenience function to join thread."""
        for thread_id in self.thread_list:
            if thread_id.is_alive():
                thread_id.join()
        self.stop_iperf_server_on_shell()

    def push_music_to_android_device(self, ad):
        """Add music to Android device as specified by the test config

        Args:
            ad: Android device

        Returns:
            True on success, False on failure
        """
        self.log.info("Pushing music to the Android device")
        music_path_str = "music_file"
        android_music_path = "/sdcard/Music/"
        if music_path_str not in self.user_params:
            self.log.error("Need music for audio testcases")
            return False
        music_path = self.user_params[music_path_str]
        if type(music_path) is list:
            self.log.info("Media ready to push as is.")
        if type(music_path) is list:
            for item in music_path:
                self.music_file_to_play = item
                ad.adb.push("{} {}".format(item, android_music_path))
        return True

    def avrcp_actions(self):
        """Performs avrcp controls like volume up, volume down, skip next and
        skip previous.

        Returns: True if successful, otherwise False.
        """
        #TODO: Validate the success state of functionalities performed.
        self.audio_receiver.volume_up()
        time.sleep(2)
        self.audio_receiver.volume_down()
        time.sleep(2)
        self.audio_receiver.skip_next()
        time.sleep(2)
        self.audio_receiver.skip_previous()
        time.sleep(2)
        return True

    def change_volume(self):
        """Changes volume with HFP call.

        Returns: True if successful, otherwise False.
        """
        self.audio_receiver.volume_up()
        time.sleep(2)
        self.audio_receiver.volume_down()
        time.sleep(2)
        return True
