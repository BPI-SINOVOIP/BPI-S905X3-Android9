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
"""
Shell command library.
"""


class ShellCommands():
    def __init__(self, log, dut):
        self.dut = dut
        self.log = log

    def set_battery_level(self, level):
        """Set the battery level via ADB shell
        Args:
            level: the percent level to set
        """
        self.dut.adb.shell("dumpsys battery set level {}".format(level))

    def disable_ble_scanning(self):
        """Disable BLE scanning via ADB shell"""
        self.dut.adb.shell("settings put global ble_scan_always_enabled 0")

    def enable_ble_scanning(self):
        """Enable BLE scanning via ADB shell"""
        self.dut.adb.shell("settings put global ble_scan_always_enabled 1")

    def consume_cpu_core(self):
        """Consume a CPU core on the Android device via ADB shell"""
        self.dut.adb.shell("echo $$ > /dev/cpuset/top-app/tasks")
        self.dut.adb.shell("cat /dev/urandom > /dev/null &")
