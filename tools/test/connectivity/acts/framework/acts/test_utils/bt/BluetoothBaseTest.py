#!/usr/bin/env python3.4
#
#   Copyright 2016 - Google
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
"""
    Base Class for Defining Common Bluetooth Test Functionality
"""

import threading
import time
import traceback
import os
from acts import utils
from acts.base_test import BaseTestClass
from acts.signals import TestSignal

from acts.controllers import android_device
from acts.test_utils.bt.bt_test_utils import reset_bluetooth
from acts.test_utils.bt.bt_test_utils import setup_multiple_devices_for_bt_test
from acts.test_utils.bt.bt_test_utils import take_btsnoop_logs
from acts.test_utils.bt.ble_lib import BleLib
from acts.test_utils.bt.bta_lib import BtaLib
from acts.test_utils.bt.config_lib import ConfigLib
from acts.test_utils.bt.gattc_lib import GattClientLib
from acts.test_utils.bt.gatts_lib import GattServerLib
from acts.test_utils.bt.rfcomm_lib import RfcommLib
from acts.test_utils.bt.shell_commands_lib import ShellCommands


class BluetoothBaseTest(BaseTestClass):
    DEFAULT_TIMEOUT = 10
    start_time = 0
    timer_list = []

    def __init__(self, controllers):
        BaseTestClass.__init__(self, controllers)
        for ad in self.android_devices:
            self._setup_bt_libs(ad)

    # Use for logging in the test cases to facilitate
    # faster log lookup and reduce ambiguity in logging.
    @staticmethod
    def bt_test_wrap(fn):
        def _safe_wrap_test_case(self, *args, **kwargs):
            test_id = "{}:{}:{}".format(self.__class__.__name__, fn.__name__,
                                        time.time())
            log_string = "[Test ID] {}".format(test_id)
            self.log.info(log_string)
            try:
                for ad in self.android_devices:
                    ad.droid.logI("Started " + log_string)
                result = fn(self, *args, **kwargs)
                for ad in self.android_devices:
                    ad.droid.logI("Finished " + log_string)
                if result is not True and "bt_auto_rerun" in self.user_params:
                    self.teardown_test()
                    log_string = "[Rerun Test ID] {}. 1st run failed.".format(
                        test_id)
                    self.log.info(log_string)
                    self.setup_test()
                    for ad in self.android_devices:
                        ad.droid.logI("Rerun Started " + log_string)
                    result = fn(self, *args, **kwargs)
                    if result is True:
                        self.log.info("Rerun passed.")
                    elif result is False:
                        self.log.info("Rerun failed.")
                    else:
                        # In the event that we have a non-bool or null
                        # retval, we want to clearly distinguish this in the
                        # logs from an explicit failure, though the test will
                        # still be considered a failure for reporting purposes.
                        self.log.info("Rerun indeterminate.")
                        result = False
                return result
            except TestSignal:
                raise
            except Exception as e:
                self.log.error(traceback.format_exc())
                self.log.error(str(e))
                return False
            return fn(self, *args, **kwargs)

        return _safe_wrap_test_case

    def setup_class(self):
        if "reboot_between_test_class" in self.user_params:
            threads = []
            for a in self.android_devices:
                thread = threading.Thread(
                    target=self._reboot_device, args=([a]))
                threads.append(thread)
                thread.start()
            for t in threads:
                t.join()
        return setup_multiple_devices_for_bt_test(self.android_devices)

    def setup_test(self):
        self.timer_list = []
        for a in self.android_devices:
            a.ed.clear_all_events()
            a.droid.setScreenTimeout(500)
            a.droid.wakeUpNow()
        return True

    def teardown_test(self):
        return True

    def on_fail(self, test_name, begin_time):
        self.log.debug(
            "Test {} failed. Gathering bugreport and btsnoop logs".format(
                test_name))
        take_btsnoop_logs(self.android_devices, self, test_name)
        self._take_bug_report(test_name, begin_time)
        for _ in range(5):
            if reset_bluetooth(self.android_devices):
                break
            else:
                self.log.error("Failed to reset Bluetooth... retrying.")
        return

    def _get_time_in_milliseconds(self):
        return int(round(time.time() * 1000))

    def start_timer(self):
        self.start_time = self._get_time_in_milliseconds()

    def end_timer(self):
        total_time = self._get_time_in_milliseconds() - self.start_time
        self.timer_list.append(total_time)
        self.start_time = 0
        return total_time

    def log_stats(self):
        if self.timer_list:
            self.log.info("Overall list {}".format(self.timer_list))
            self.log.info("Average of list {}".format(
                sum(self.timer_list) / float(len(self.timer_list))))
            self.log.info("Maximum of list {}".format(max(self.timer_list)))
            self.log.info("Minimum of list {}".format(min(self.timer_list)))
            self.log.info("Total items in list {}".format(
                len(self.timer_list)))
        self.timer_list = []

    def _setup_bt_libs(self, android_device):
        # Bluetooth Low Energy library.
        setattr(android_device, "ble", BleLib(
            log=self.log, dut=android_device))
        # Bluetooth Adapter library.
        setattr(android_device, "bta", BtaLib(
            log=self.log, dut=android_device))
        # Bluetooth stack config library.
        setattr(android_device, "config",
                ConfigLib(log=self.log, dut=android_device))
        # GATT Client library.
        setattr(android_device, "gattc",
                GattClientLib(log=self.log, dut=android_device))
        # GATT Server library.
        setattr(android_device, "gatts",
                GattServerLib(log=self.log, dut=android_device))
        # RFCOMM library.
        setattr(android_device, "rfcomm",
                RfcommLib(log=self.log, dut=android_device))
        # Shell command library
        setattr(android_device, "shell",
                ShellCommands(log=self.log, dut=android_device))
