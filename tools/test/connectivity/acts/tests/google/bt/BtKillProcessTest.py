#/usr/bin/env python3.4
#
# Copyright (C) 2016 The Android Open Source Project
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
"""
Test script to test if Bluetooth will reboot successfully
if it is killed.
"""

import re
import time
from acts.test_decorators import test_tracker_info
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest


class BtKillProcessTest(BluetoothBaseTest):
    def __init__(self, controllers):
        BluetoothBaseTest.__init__(self, controllers)
        self.dut = self.android_devices[0]

    def _get_bt_pid(self):
        process_grep_string = "com.android.bluetooth"
        awk = "awk '{print $2}'"
        pid = self.dut.adb.shell("ps | grep com.android.bluetooth")
        if not pid:
            return None
        return (re.findall("\d+\W", pid)[0])

    def _is_bt_process_running(self):
        if self._get_bt_pid():
            return True
        else:
            return False

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='c51186e9-4ba8-406c-b609-ea552868e4c9')
    def test_kill_process(self):
        """Test that a killed Bluetooth process restarts

        If the Bluetooth process is killed out of band with kill -9
        the process should be automatically restarted by Android.

        Steps:
        1. Verify Bluetooth process is running
        2. Kill the Bluetooth process with kill -9
        3. Verify the process is restarted

        Expected Result:
        Bluetooth should be restarted automagically by the system.

        Returns:
          Pass if True
          Fail if False

        TAGS: Bluetooth
        Priority: 3
        """
        pid = self._get_bt_pid()
        if not pid:
            self.log.error("Failed to find Bluetooth process...")
            return False
        self.dut.adb.shell("kill -9 {}".format(pid))
        #Give up to 5 seconds for the process to restart
        search_tries = 5
        while search_tries > 0:
            if self._is_bt_process_running():
                self.log.info(
                    "Bluetooth process is successfully running again")
                return True
            search_tries -= 1
            #Try looking for the process after waiting an additional second
            time.sleep(1)

        self.log.error("Bluetooth process did not restart")
        return False
