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
This test script exercises power test scenarios for GATT writing.
This test script was designed with this setup in mind:
Shield box one: Two Android Devices and Monsoon tool box
"""

import json
import os
import sys

from acts.test_decorators import test_tracker_info
from acts.test_utils.bt.BleEnum import ScanSettingsScanMode
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.PowerBaseTest import PowerBaseTest
from acts.test_utils.bt.bt_test_utils import bluetooth_enabled_check
from acts.test_utils.bt.bt_test_utils import generate_ble_scan_objects
from acts.utils import get_current_human_time
from acts.utils import sync_device_time


class GattPowerTest(PowerBaseTest):
    """Class for Gatt Power Test"""
    # Time to start GATT write
    START_TIME = 30
    # Repetitions
    REPETITIONS_40 = 7
    REPETITIONS_1 = 1
    # Time for GATT writing
    WRITE_TIME_60 = 60
    WRITE_TIME_3600 = 600
    # Time for idle
    IDLE_TIME_30 = 30
    IDLE_TIME_0 = 0
    # Base commands for PMC
    PMC_GATT_CMD = ("am broadcast -a com.android.pmc.GATT ")
    GATT_SERVER_MSG = "%s--es GattServer 1" % (PMC_GATT_CMD)

    def __init__(self, controllers):
        PowerBaseTest.__init__(self, controllers)

        self.cen_ad = self.android_devices[0]
        self.per_ad = self.android_devices[1]

    def setup_class(self):
        super(GattPowerTest, self).setup_class()
        if not bluetooth_enabled_check(self.per_ad):
            self.log.error("Failed to turn on Bluetooth on peripheral")

        # Start PMC app for peripheral here
        # since base class has started PMC for central device
        self.per_ad.adb.shell(PowerBaseTest.START_PMC_CMD)
        self.per_ad.adb.shell(self.PMC_VERBOSE_CMD)

    def _measure_power_for_gatt_n_log_data(self, write_time, idle_time,
                                           repetitions, test_case):
        """Utility function for power test with GATT write.

        Steps:
        1. Prepare adb shell command for GATT server
        2. Send the adb shell command to PMC to startup GATT Server
        3. Prepare adb shell command for GATT Client
        4. Send the adb shell command to PMC to startup GATT Client
        5. PMC will start first alarm on GATT Client to start
           GATT write continuousely for "write_time" seconds
        6. After finishing writing for write_time it will stop for
           for "idle_time" seconds
        7  Repeat the write/idle cycle for "repetitions" times
        8. Save the power usage data into log file

        Args:
            write_time: time(sec) duration for writing GATT characteristic
            idle_time: time(sec) of idle (not write)
            repetitions: number of repetitions of writing cycles

        Returns:
            True if the average current is within the allowed tolerance;
            False otherwise.
        """

        if not self.disable_location_scanning():
            return False

        # Verify Bluetooth is enabled on the companion phone.
        if not bluetooth_enabled_check(self.android_devices[1]):
            self.log.error("FAILED to enable Bluetooth on companion phone")
            return False

        # Send message to Gatt Server
        self.per_ad.log.info("Send broadcast message to GATT Server: %s",
                             self.GATT_SERVER_MSG)
        self.per_ad.adb.shell(self.GATT_SERVER_MSG)

        # Send message to Gatt Client
        first_part_msg = "%s--es StartTime %d --es WriteTime %d" % (
            self.PMC_GATT_CMD, self.START_TIME, write_time)
        clientmsg = "%s --es IdleTime %d --es Repetitions %d" % (
            first_part_msg, idle_time, repetitions)
        self.cen_ad.log.info("Send broadcast message to GATT Client: %s",
                             clientmsg)
        self.cen_ad.adb.shell(clientmsg)

        sample_time = (write_time + idle_time) * repetitions
        # Start the power measurement
        result = self.mon.measure_power(self.POWER_SAMPLING_RATE, sample_time,
                               self.current_test_name, self.START_TIME)

        # Calculate average and save power data into a file
        (current_avg, stdev) = self.save_logs_for_power_test(
            result, write_time, idle_time)
        # Take bug report for peripheral device
        current_time = get_current_human_time()
        self.per_ad.take_bug_report(self.current_test_name, current_time)

        # perform watermark comparison numbers
        self.log.info("==> CURRENT AVG from PMC Monsoon app: %s" % current_avg)
        self.log.info(
            "==> WATERMARK from config file: %s" % self.user_params[test_case])
        return self.check_test_pass(current_avg, self.user_params[test_case])

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='f14cc28b-54f2-4a87-9fa9-68f39bf96701')
    def test_power_for_60_sec_n_30_sec_idle_gatt_write(self):
        """Test power usage when do 60 sec GATT write & 30 sec idle

        Tests power usage when the test device do 60 sec GATT write
        and 30 sec idle with max MTU bytes after being connected.
        After each write GATT server will send a response
        back to GATT client so GATT client can do another write.

        Steps:
        1. Prepare adb shell command for GATT server
        2. Send the adb shell command to PMC to startup GATT Server
        3. Prepare adb shell command for GATT Client
        4. Send the adb shell command to PMC to startup GATT Client
        5. PMC will start first alarm on GATT Client
        6. Start power measurement
        7. Alarm will be triggered to start GATT write for 60 second
        8. Then it will be idle for 30 seconds
        9. Reconnect after idle time
        10. Repeat the cycles for 60 minutes
        11. End power measurement
        12. Save the power usage data into log file

        Expected Result:
        power consumption results

        TAGS: LE, GATT, Power
        Priority: 3
        """
        current_test_case = func_name = sys._getframe().f_code.co_name
        return self._measure_power_for_gatt_n_log_data(
            self.WRITE_TIME_60, self.IDLE_TIME_30, self.REPETITIONS_40,
            current_test_case)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='41ca217e-161b-4899-a5b7-2d59d8dc7973')
    def test_power_for_60_min_non_stop_gatt_write(self):
        """Test power usage when do a single GATT write.

        Tests power usage when the test device do 60 minutes of GATT write with
        max MTU bytes after being connected. After each write GATT server will
        send a response back to GATT client so GATT client can do another write.

        Steps:
        1. Prepare adb shell command for GATT server
        2. Send the adb shell command to PMC to startup GATT Server
        3. Prepare adb shell command for GATT Client
        4. Send the adb shell command to PMC to startup GATT Client
        5. PMC will start first alarm on GATT Client to start GATT write
        6. Start power measurement
        7. GATT server gets the write request after GATT Client sends a write
        8. GATT server sends a response back to GATT Client
        9. After GATT Client receive the response from GATT Server
           it will check if time reaches 60 minutes.
           if not it will write another characteristic
           otherwise it will stop writing
        10. Stop power measurement
        11. Save the power usage data into log file

        Expected Result:
        power consumption results

        TAGS: LE, GATT, Power
        Priority: 3
        """
        current_test_case = func_name = sys._getframe().f_code.co_name
        return self._measure_power_for_gatt_n_log_data(
            self.WRITE_TIME_3600, self.IDLE_TIME_0, self.REPETITIONS_1,
            current_test_case)
