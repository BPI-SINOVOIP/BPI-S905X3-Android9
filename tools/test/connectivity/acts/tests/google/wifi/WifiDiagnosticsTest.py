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

import itertools
import pprint
import queue
import time

import acts.base_test
import acts.signals as signals
import acts.test_utils.wifi.wifi_test_utils as wutils
import acts.utils

from acts import asserts
from acts.test_decorators import test_tracker_info
from acts.test_utils.wifi.WifiBaseTest import WifiBaseTest

WifiEnums = wutils.WifiEnums

DEFAULT_WAIT_TIME = 2


class WifiDiagnosticsTest(WifiBaseTest):
    """
    Test Bed Requirement:
    * One Android device
    * An open Wi-Fi network.
    * Verbose logging is on.
    """

    def __init__(self, controllers):
        WifiBaseTest.__init__(self, controllers)

    def setup_class(self):
        self.dut = self.android_devices[0]
        wutils.wifi_test_device_init(self.dut)
        req_params = []
        opt_param = ["open_network"]
        self.unpack_userparams(
            req_param_names=req_params, opt_param_names=opt_param)

        if "AccessPoint" in self.user_params:
            self.legacy_configure_ap_and_start()
        wutils.wifi_toggle_state(self.dut, True)
        asserts.assert_true(
            len(self.open_network) > 0,
            "Need at least one open network.")
        self.open_network = self.open_network[0]["2g"]

    def setup_test(self):
        self.dut.droid.wakeLockAcquireBright()
        self.dut.droid.wakeUpNow()

    def teardown_test(self):
        self.dut.droid.wakeLockRelease()
        self.dut.droid.goToSleepNow()
        wutils.reset_wifi(self.dut)


    def on_fail(self, test_name, begin_time):
        self.dut.take_bug_report(test_name, begin_time)
        self.dut.cat_adb_log(test_name, begin_time)

    def teardown_class(self):
        if "AccessPoint" in self.user_params:
            del self.user_params["open_network"]

    """Tests"""

    @test_tracker_info(uuid="d6f1661b-6732-4939-8c28-f20917774ec0")
    def test_ringbuffers_are_dumped_during_lsdebug(self):
        """Steps:
        1. Connect to a open network.
        2. Delete old files under data/vendor/tombstones/wifi
        3. Call lshal debug on wifi hal component
        4. Verify that files are created under data/vender/tombstones/wifi
        """
        wutils.connect_to_wifi_network(self.dut, self.open_network)
        time.sleep(DEFAULT_WAIT_TIME)
        self.dut.adb.shell("rm data/vendor/tombstones/wifi/*")
        try:
            self.dut.adb.shell("lshal debug android.hardware.wifi@1.2::IWifi")
        except UnicodeDecodeError:
            """ Gets this error because adb.shell trys to parse the output to a string
            but ringbuffer dumps should already be generated """
            self.log.info("Unicode decode error occurred, but this is ok")
        file_count_plus_one = self.dut.adb.shell("ls -l data/vendor/tombstones/wifi | wc -l")
        if int(file_count_plus_one) <= 1:
            raise signals.TestFailure("Failed to create ringbuffer debug files.")