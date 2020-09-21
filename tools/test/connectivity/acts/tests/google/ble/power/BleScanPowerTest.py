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
This test script exercises power test scenarios for different scan modes.
This test script was designed with this setup in mind:
Shield box one: Android Device and Monsoon tool box
"""

import json
import os
import sys

from acts.test_decorators import test_tracker_info
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.bt_constants import ble_scan_settings_modes
from acts.test_utils.bt.bt_test_utils import bluetooth_enabled_check
from acts.test_utils.bt.bt_test_utils import disable_bluetooth
from acts.test_utils.bt.bt_test_utils import generate_ble_scan_objects
from acts.test_utils.bt.PowerBaseTest import PowerBaseTest


class BleScanPowerTest(PowerBaseTest):
    # Repetitions for scan and idle
    REPETITIONS_40 = 5
    REPETITIONS_360 = 40

    # Power measurement start time in seconds
    SCAN_START_TIME = 60
    # BLE scanning time in seconds
    SCAN_TIME_60 = 60
    SCAN_TIME_5 = 5
    # BLE no scanning time in seconds
    IDLE_TIME_30 = 30
    IDLE_TIME_5 = 5

    PMC_BASE_CMD = ("am broadcast -a com.android.pmc.BLESCAN --es ScanMode ")
    # Log file name
    LOG_FILE = "BLEPOWER.log"

    def setup_class(self):
        super(BleScanPowerTest, self).setup_class()
        # Get power test device serial number
        power_test_device_serial = self.user_params["PowerTestDevice"]
        # If there are multiple devices in the shield box turn off
        # all of them except the one for the power testing
        if len(self.android_devices) > 1:
            for ad in self.android_devices:
                if ad.serial != power_test_device_serial[0]:
                    self.ad.log.info("Disable BT for %s != %s", ad.serial,
                                     power_test_device_serial[0])
                    disable_bluetooth(ad.droid)

    def _measure_power_for_scan_n_log_data(self, scan_mode, scan_time,
                                           idle_time, repetitions, test_case):
        """utility function for power test with BLE scan.

        Steps:
        1. Prepare adb shell command
        2. Send the adb shell command to PMC
        3. PMC start first alarm to start scan
        4. After first alarm triggered it start the second alarm to stop scan
        5. Repeat the scan/idle cycle for the number of repetitions
        6. Save the power usage data into log file

        Args:
            scan_mode: Scan mode
            scan_time: Time duration for scanning
            idle_time: Time duration for idle after scanning
            repetitions:  The number of cycles of scanning/idle

        Returns:
            True if the average current is within the allowed tolerance;
            False otherwise.
        """

        if not self.disable_location_scanning():
            return False

        first_part_msg = "%s%s --es StartTime %d --es ScanTime %d" % (
            self.PMC_BASE_CMD, scan_mode, self.SCAN_START_TIME, scan_time)
        msg = "%s --es NoScanTime %d --es Repetitions %d" % (first_part_msg,
                                                             idle_time,
                                                             repetitions)

        self.ad.log.info("Send broadcast message: %s", msg)
        self.ad.adb.shell(msg)

        # Check if PMC is ready
        if not self.check_pmc_status(self.LOG_FILE, "READY",
                                     "PMC is not ready"):
            return

        # Start the power measurement
        sample_time = (scan_time + idle_time) * repetitions
        result = self.mon.measure_power(self.POWER_SAMPLING_RATE, sample_time,
                               self.current_test_name, self.SCAN_START_TIME)

        self.ad.log.info("Monsoon start_time: {}".format(result.timestamps[0]))

        start_times = []
        end_times = []
        json_data = self.check_pmc_timestamps(self.LOG_FILE)
        for timestamp in json_data:
            start_times.append(timestamp["StartTime"])
            end_times.append(timestamp["EndTime"])

        self.ad.log.info("Number of test cycles: {}".format(len(start_times)))

        (current_avg, stdev) = self.save_logs_for_power_test(
            result, start_times, end_times, False)

        # perform watermark comparison numbers
        self.log.info("==> CURRENT AVG from PMC Monsoon app: %s" % current_avg)
        self.log.info(
            "==> WATERMARK from config file: %s" % self.user_params[test_case])
        return self.check_test_pass(current_avg, self.user_params[test_case])

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='1e8c793f-897d-4160-8ebf-fd1addcd9c71')
    def test_power_for_scan_w_low_latency(self):
        """Test power usage when scan with low latency.

        Tests power usage when the device scans with low latency mode
        for 60 seconds and then idle for 30 seconds, repeat for 60 minutes
        where there are no advertisements.

        Steps:
        1. Prepare adb shell command
        2. Send the adb shell command to PMC
        3. PMC start first alarm to start scan
        4. After first alarm triggered it start the second alarm to stop scan
        5. Repeat the cycle for 60 minutes
        6. Save the power usage data into log file

        Expected Result:
        power consumption results

        TAGS: LE, Scanning, Power
        Priority: 3
        """
        current_test_case = func_name = sys._getframe().f_code.co_name
        return self._measure_power_for_scan_n_log_data(
            ble_scan_settings_modes['low_latency'], self.SCAN_TIME_60,
            self.IDLE_TIME_30, self.REPETITIONS_40, current_test_case)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='83e37081-9f00-4b5f-ba1c-92136047fa83')
    def test_power_for_scan_w_balanced(self):
        """Test power usage when scan with balanced mode.

        Tests power usage when the device scans with balanced mode
        for 60 seconds and then idle for 30 seconds, repeat for 60 minutes
        where there are no advertisements.

        Steps:
        1. Prepare adb shell command
        2. Send the adb shell command to PMC
        3. PMC start first alarm to start scan
        4. After first alarm triggered it start the second alarm to stop scan
        5. Repeat the cycle for 60 minutes
        6. Save the power usage data into log file

        Expected Result:
        power consumption results

        TAGS: LE, Scanning, Power
        Priority: 3
        """
        current_test_case = func_name = sys._getframe().f_code.co_name
        return self._measure_power_for_scan_n_log_data(
            ble_scan_settings_modes['balanced'], self.SCAN_TIME_60,
            self.IDLE_TIME_30, self.REPETITIONS_40, current_test_case)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='cbf16414-3468-4f60-8a62-2a88556d7a87')
    def test_power_for_scan_w_low_power(self):
        """Test power usage when scan with low power.

        Tests power usage when the device scans with low power mode
        for 60 seconds and then idle for 30 seconds, repeat for 60 minutes
        where there are no advertisements.

        Steps:
        1. Prepare adb shell command
        2. Send the adb shell command to PMC
        3. PMC start first alarm to start scan
        4. After first alarm triggered it start the second alarm to stop scan
        5. Repeat the cycle for 60 minutes
        6. Save the power usage data into log file

        Expected Result:
        power consumption results

        TAGS: LE, Scanning, Power
        Priority: 3
        """
        current_test_case = func_name = sys._getframe().f_code.co_name
        return self._measure_power_for_scan_n_log_data(
            ble_scan_settings_modes['low_power'], self.SCAN_TIME_60,
            self.IDLE_TIME_30, self.REPETITIONS_40, current_test_case)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='0b1eaedb-4385-48ba-a175-7db79360aed8')
    def test_power_for_intervaled_scans_w_balanced(self):
        """Test power usage when intervaled scans with balanced mode

        Tests power usage when the device perform multiple intervaled scans with
        balanced mode for 5 seconds each where there are no advertisements.

        Steps:
        1. Prepare adb shell command
        2. Send the adb shell command to PMC
        3. PMC start first alarm to start scan
        4. After first alarm triggered it starts the second alarm to stop scan
        5. After second alarm triggered it starts the third alarm to start scan
        6. Repeat the alarms until 360 scans are done
        7. Save the power usage data into log file

        Expected Result:
        power consumption results

        TAGS: LE, Scanning, Power
        Priority: 3
        """
        current_test_case = func_name = sys._getframe().f_code.co_name
        return self._measure_power_for_scan_n_log_data(
            ble_scan_settings_modes['balanced'], self.SCAN_TIME_5,
            self.IDLE_TIME_5, self.REPETITIONS_360, current_test_case)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='90e6b341-9b11-4740-8ec1-dc605db93ec9')
    def test_power_for_intervaled_scans_w_low_latency(self):
        """Test power usage when intervaled scans with low latency mode

        Tests power usage when the device perform multiple intervaled scans with
        low latency mode for 5 seconds each where there are no advertisements.

        Steps:
        1. Prepare adb shell command
        2. Send the adb shell command to PMC
        3. PMC start first alarm to start scan
        4. After first alarm triggered it starts the second alarm to stop scan
        5. After second alarm triggered it starts the third alarm to start scan
        6. Repeat the alarms until 360 scans are done
        7. Save the power usage data into log file

        Expected Result:
        power consumption results

        TAGS: LE, Scanning, Power
        Priority: 3
        """
        current_test_case = func_name = sys._getframe().f_code.co_name
        return self._measure_power_for_scan_n_log_data(
            ble_scan_settings_modes['low_latency'], self.SCAN_TIME_5,
            self.IDLE_TIME_5, self.REPETITIONS_360, current_test_case)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='cbf16414-3468-4f60-8a62-2a88556d7a87')
    def test_power_for_intervaled_scans_w_low_power(self):
        """Test power usage when intervaled scans with low power mode

        Tests power usage when the device perform multiple intervaled scans with
        low power mode for 5 seconds each where there are no advertisements.

        Steps:
        1. Prepare adb shell command
        2. Send the adb shell command to PMC
        3. PMC start first alarm to start scan
        4. After first alarm triggered it starts the second alarm to stop scan
        5. After second alarm triggered it starts the third alarm to start scan
        6. Repeat the alarms until 360 scans are done
        7. Save the power usage data into log file

        Expected Result:
        power consumption results

        TAGS: LE, Scanning, Power
        Priority: 3
        """
        current_test_case = func_name = sys._getframe().f_code.co_name
        return self._measure_power_for_scan_n_log_data(
            ble_scan_settings_modes['low_power'], self.SCAN_TIME_5,
            self.IDLE_TIME_5, self.REPETITIONS_360, current_test_case)
