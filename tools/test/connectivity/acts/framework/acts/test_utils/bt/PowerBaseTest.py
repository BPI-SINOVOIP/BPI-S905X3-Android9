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
This test script is the base class for Bluetooth power testing
"""

import json
import os
import statistics
import time

from acts import asserts
from acts import utils
from acts.controllers import monsoon
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.bt_test_utils import bluetooth_enabled_check
from acts.test_utils.tel.tel_test_utils import set_phone_screen_on
from acts.test_utils.tel.tel_test_utils import toggle_airplane_mode_by_adb
from acts.test_utils.wifi import wifi_test_utils as wutils
from acts.utils import create_dir
from acts.utils import force_airplane_mode
from acts.utils import get_current_human_time
from acts.utils import set_adaptive_brightness
from acts.utils import set_ambient_display
from acts.utils import set_auto_rotate
from acts.utils import set_location_service
from acts.utils import sync_device_time


class PowerBaseTest(BluetoothBaseTest):
    # Monsoon output Voltage in V
    MONSOON_OUTPUT_VOLTAGE = 4.2
    # Monsoon output max current in A
    MONSOON_MAX_CURRENT = 7.8
    # Power mesaurement sampling rate in Hz
    POWER_SAMPLING_RATE = 10
    SCREEN_TIME_OFF = 5
    # Wait time for PMC to start in seconds
    WAIT_TIME = 60
    # Wait time for PMC to write AlarmTimes
    ALARM_WAIT_TIME = 600

    START_PMC_CMD = ("am start -n com.android.pmc/com.android.pmc."
                     "PMCMainActivity")
    PMC_VERBOSE_CMD = "setprop log.tag.PMC VERBOSE"

    def setup_test(self):
        self.timer_list = []
        for a in self.android_devices:
            a.ed.clear_all_events()
            a.droid.setScreenTimeout(20)
            self.ad.go_to_sleep()
        return True

    def disable_location_scanning(self):
        """Utility to disable location services from scanning.

        Unless the device is in airplane mode, even if WiFi is disabled
        the DUT will still perform occasional scans. This will greatly impact
        the power numbers.

        Returns:
            True if airplane mode is disabled and Bluetooth is enabled;
            False otherwise.
        """
        self.ad.log.info("DUT phone Airplane is ON")
        if not toggle_airplane_mode_by_adb(self.log, self.android_devices[0],
                                           True):
            self.log.error("FAILED to toggle Airplane on")
            return False
        self.ad.log.info("DUT phone Bluetooth is ON")
        if not bluetooth_enabled_check(self.android_devices[0]):
            self.log.error("FAILED to enable Bluetooth on")
            return False
        return True

    def teardown_test(self):
        return True

    def setup_class(self):
        # Not to call Base class setup_class()
        # since it removes the bonded devices
        for ad in self.android_devices:
            sync_device_time(ad)
        self.ad = self.android_devices[0]
        self.mon = self.monsoons[0]
        self.mon.set_voltage(self.MONSOON_OUTPUT_VOLTAGE)
        self.mon.set_max_current(self.MONSOON_MAX_CURRENT)
        # Monsoon phone
        self.mon.attach_device(self.ad)
        self.monsoon_log_path = os.path.join(self.log_path, "MonsoonLog")
        create_dir(self.monsoon_log_path)

        asserts.assert_true(
            self.mon.usb("auto"),
            "Failed to turn USB mode to auto on monsoon.")

        asserts.assert_true(
            force_airplane_mode(self.ad, True),
            "Can not turn on airplane mode on: %s" % self.ad.serial)
        asserts.assert_true(
            bluetooth_enabled_check(self.ad),
            "Failed to set Bluetooth state to enabled")
        set_location_service(self.ad, False)
        set_adaptive_brightness(self.ad, False)
        set_ambient_display(self.ad, False)
        self.ad.adb.shell("settings put system screen_brightness 0")
        set_auto_rotate(self.ad, False)

        wutils.wifi_toggle_state(self.ad, False)

        # Start PMC app.
        self.log.info("Start PMC app...")
        self.ad.adb.shell(self.START_PMC_CMD)
        self.ad.adb.shell(self.PMC_VERBOSE_CMD)

        self.log.info("Check to see if PMC app started")
        for _ in range(self.WAIT_TIME):
            time.sleep(1)
            try:
                self.ad.adb.shell('ps -A | grep "S com.android.pmc"')
                break
            except adb.AdbError as e:
                self.log.info("PMC app is NOT started yet")

    def check_pmc_status(self, log_file, label, status_msg):
        """Utility function to check if the log file contains certain label.

        Args:
            log_file: Log file name for PMC log file
            label: the label to be looked for in the log file
            status_msg: error message to be displayed when the expected label is not found

        Returns:
            A list of objects which contain start and end timestamps
        """

        # Check if PMC is ready
        start_time = time.time()
        result = False
        while time.time() < start_time + self.WAIT_TIME:
            out = self.ad.adb.shell("cat /mnt/sdcard/Download/" + log_file)
            self.log.info("READ file: " + out)
            if -1 != out.find(label):
                result = True
                break
            time.sleep(1)

        if not result:
            self.log.error(status_msg)
            return False
        else:
            return True

    def check_pmc_timestamps(self, log_file):
        """Utility function to get timestampes from a log file.

        Args:
            log_file: Log file name for PMC log file

        Returns:
            A list of objects which contain start and end timestamps
        """

        start_time = time.time()
        alarm_written = False
        while time.time() < start_time + self.ALARM_WAIT_TIME:
            out = self.ad.adb.shell("cat /mnt/sdcard/Download/" + log_file)
            self.log.info("READ file: " + out)
            if -1 != out.find("READY"):
                # AlarmTimes has not been written, wait
                self.log.info("AlarmTimes has not been written, wait")
            else:
                alarm_written = True
                break
            time.sleep(1)

        if alarm_written is False:
            self.log.info("PMC never wrote alarm file")
        json_data = json.loads(out)
        if json_data["AlarmTimes"]:
            return json_data["AlarmTimes"]

    def save_logs_for_power_test(self,
                                 monsoon_result,
                                 time1,
                                 time2,
                                 single_value=True):
        """Utility function to save power data into log file.

        Steps:
        1. Save power data into a file if being configed.
        2. Create a bug report if being configured

        Args:
            monsoon_result: power data object
            time1: A single value or a list
                   For single value it is time duration (sec) for measure power
                   For a list of values they are a list of start times
            time2: A single value or a list
                   For single value it is time duration (sec) which is not
                       counted toward power measurement
                   For a list of values they are a list of end times
            single_value: True means time1 and time2 are single values
                          Otherwise they are arrays of time values

        Returns:
            BtMonsoonData for Current Average and Std Deviation
        """
        current_time = get_current_human_time()
        file_name = "{}_{}".format(self.current_test_name, current_time)

        if single_value:
            bt_monsoon_result = BtMonsoonData(monsoon_result, time1, time2,
                                              self.log)
        else:
            bt_monsoon_result = BtMonsoonDataWithPmcTimes(
                monsoon_result, time1, time2, self.log)

        bt_monsoon_result.save_to_text_file(bt_monsoon_result,
                                            os.path.join(
                                                self.monsoon_log_path,
                                                file_name))

        self.ad.take_bug_report(self.current_test_name, current_time)
        return (bt_monsoon_result.average_cur, bt_monsoon_result.stdev)

    def check_test_pass(self, average_current, watermark_value):
        """Compare watermark numbers for pass/fail criteria.

        b/67960377 = for BT codec runs
        b/67959834 = for BLE scan+GATT runs

        Args:
            average_current: the numbers calculated from Monsoon box
            watermark_value: the reference numbers from config file

        Returns:
            True if the current is within 10%; False otherwise

        """
        watermark_value = float(watermark_value)
        variance_plus = watermark_value + (watermark_value * 0.1)
        variance_minus = watermark_value - (watermark_value * 0.1)
        if (average_current > variance_plus) or (average_current <
                                                 variance_minus):
            self.log.error('==> FAILED criteria from check_test_pass method')
            return False
        self.log.info('==> PASS criteria from check_test_pass method')
        return True


class BtMonsoonData(monsoon.MonsoonData):
    """A class for encapsulating power measurement data from monsoon.
       It implements the power averaging and standard deviation for
       mulitple cycles of the power data. Each cycle is defined by a constant
       measure time and a constant idle time.  Measure time is the time
       duration when power data are included for calculation.
       Idle time is the time when power data should be removed from calculation

    """
    # Accuracy for current and power data
    ACCURACY = 4
    THOUSAND = 1000

    def __init__(self, monsoon_data, measure_time, idle_time, log):
        """Instantiates a MonsoonData object.

        Args:
            monsoon_data: A list of current values in Amp (float).
            measure_time: Time for measuring power.
            idle_time: Time for not measuring power.
            log: log object to log info messages.
        """

        super(BtMonsoonData, self).__init__(
            monsoon_data.data_points, monsoon_data.timestamps, monsoon_data.hz,
            monsoon_data.voltage, monsoon_data.offset)

        # Change timestamp to use small granularity of time
        # Monsoon libray uses the seconds as the time unit
        # Using sample rate to calculate timestamps between the seconds
        t0 = self.timestamps[0]
        dt = 1.0 / monsoon_data.hz
        index = 0

        for ind, t in enumerate(self.timestamps):
            if t == t0:
                index = index + 1
                self.timestamps[ind] = t + dt * index
            else:
                t0 = t
                index = 1

        self.measure_time = measure_time
        self.idle_time = idle_time
        self.log = log
        self.average_cur = None
        self.stdev = None

    def _calculate_average_current_n_std_dev(self):
        """Utility function to calculate average current and standard deviation
           in the unit of mA.

        Returns:
            A tuple of average current and std dev as float
        """
        if self.idle_time == 0:
            # if idle time is 0 use Monsoon calculation
            # in this case standard deviation is 0
            return round(self.average_current, self.ACCURACY), 0

        self.log.info(
            "Measure time: {} Idle time: {} Total Data Points: {}".format(
                self.measure_time, self.idle_time, len(self.data_points)))

        # The base time to be used to calculate the relative time
        base_time = self.timestamps[0]

        # Index for measure and idle cycle index
        measure_cycle_index = 0
        # Measure end time of measure cycle
        measure_end_time = self.measure_time
        # Idle end time of measure cycle
        idle_end_time = self.measure_time + self.idle_time
        # Sum of current data points for a measure cycle
        current_sum = 0
        # Number of current data points for a measure cycle
        data_point_count = 0
        average_for_a_cycle = []
        # Total number of measure data point
        total_measured_data_point_count = 0

        # Flag to indicate whether the average is calculated for this cycle
        # For 1 second there are multiple data points
        # so time comparison will yield to multiple cases
        done_average = False

        for t, d in zip(self.timestamps, self.data_points):
            relative_timepoint = t - base_time
            # When time exceeds 1 cycle of measurement update 2 end times
            if relative_timepoint > idle_end_time:
                measure_cycle_index += 1
                measure_end_time = measure_cycle_index * (
                    self.measure_time + self.idle_time) + self.measure_time
                idle_end_time = measure_end_time + self.idle_time
                done_average = False

            # Within measure time sum the current
            if relative_timepoint <= measure_end_time:
                current_sum += d
                data_point_count += 1
            elif not done_average:
                # Calculate the average current for this cycle
                average_for_a_cycle.append(current_sum / data_point_count)
                total_measured_data_point_count += data_point_count
                current_sum = 0
                data_point_count = 0
                done_average = True

        # Calculate the average current and convert it into mA
        current_avg = round(
            statistics.mean(average_for_a_cycle) * self.THOUSAND,
            self.ACCURACY)
        # Calculate the min and max current and convert it into mA
        current_min = round(
            min(average_for_a_cycle) * self.THOUSAND, self.ACCURACY)
        current_max = round(
            max(average_for_a_cycle) * self.THOUSAND, self.ACCURACY)

        # Calculate the standard deviation and convert it into mA
        stdev = round(
            statistics.stdev(average_for_a_cycle) * self.THOUSAND,
            self.ACCURACY)
        self.log.info("Total Counted Data Points: {}".format(
            total_measured_data_point_count))
        self.log.info("Average Current: {} mA ".format(current_avg))
        self.log.info("Standard Deviation: {} mA".format(stdev))
        self.log.info("Min Current: {} mA ".format(current_min))
        self.log.info("Max Current: {} mA".format(current_max))

        return current_avg, stdev

    def _format_header(self):
        """Utility function to write the header info to the file.
           The data is formated as tab delimited for spreadsheets.

        Returns:
            None
        """
        strs = [""]
        if self.tag:
            strs.append("\t\t" + self.tag)
        else:
            strs.append("\t\tMonsoon Measurement Data")
        average_cur, stdev = self._calculate_average_current_n_std_dev()
        total_power = round(average_cur * self.voltage, self.ACCURACY)

        self.average_cur = average_cur
        self.stdev = stdev

        strs.append("\t\tAverage Current: {} mA.".format(average_cur))
        strs.append("\t\tSTD DEV Current: {} mA.".format(stdev))
        strs.append("\t\tVoltage: {} V.".format(self.voltage))
        strs.append("\t\tTotal Power: {} mW.".format(total_power))
        strs.append(
            ("\t\t{} samples taken at {}Hz, with an offset of {} samples."
             ).format(len(self.data_points), self.hz, self.offset))
        return "\n".join(strs)

    def _format_data_point(self):
        """Utility function to format the data into a string.
           The data is formated as tab delimited for spreadsheets.

        Returns:
            Average current as float
        """
        strs = []
        strs.append(self._format_header())
        strs.append("\t\tTime\tAmp")
        # Get the relative time
        start_time = self.timestamps[0]

        for t, d in zip(self.timestamps, self.data_points):
            strs.append("{}\t{}".format(
                round((t - start_time), 1), round(d, self.ACCURACY)))

        return "\n".join(strs)

    @staticmethod
    def save_to_text_file(bt_monsoon_data, file_path):
        """Save BtMonsoonData object to a text file.
           The data is formated as tab delimited for spreadsheets.

        Args:
            bt_monsoon_data: A BtMonsoonData object to write to a text
                file.
            file_path: The full path of the file to save to, including the file
                name.
        """
        if not bt_monsoon_data:
            self.log.error("Attempting to write empty Monsoon data to "
                           "file, abort")
            return

        utils.create_dir(os.path.dirname(file_path))
        try:
            with open(file_path, 'w') as f:
                f.write(bt_monsoon_data._format_data_point())
                f.write("\t\t" + bt_monsoon_data.delimiter)
        except IOError:
            self.log.error("Fail to write power data into file")


class BtMonsoonDataWithPmcTimes(BtMonsoonData):
    """A class for encapsulating power measurement data from monsoon.
       It implements the power averaging and standard deviation for
       mulitple cycles of the power data. Each cycle is defined by a start time
       and an end time.  The start time and the end time are the actual time
       triggered by Android alarm in PMC.

    """

    def __init__(self, bt_monsoon_data, start_times, end_times, log):
        """Instantiates a MonsoonData object.

        Args:
            bt_monsoon_data: A list of current values in Amp (float).
            start_times: A list of epoch timestamps (int).
            end_times: A list of epoch timestamps (int).
            log: log object to log info messages.
        """
        super(BtMonsoonDataWithPmcTimes, self).__init__(
            bt_monsoon_data, 0, 0, log)
        self.start_times = start_times
        self.end_times = end_times

    def _calculate_average_current_n_std_dev(self):
        """Utility function to calculate average current and standard deviation
           in the unit of mA.

        Returns:
            A tuple of average current and std dev as float
        """
        if len(self.start_times) == 0 or len(self.end_times) == 0:
            return 0, 0

        self.log.info(
            "Start times: {} End times: {} Total Data Points: {}".format(
                len(self.start_times),
                len(self.end_times), len(self.data_points)))

        # Index for measure and idle cycle index
        measure_cycle_index = 0

        # Measure end time of measure cycle
        measure_end_time = self.end_times[0]
        # Idle end time of measure cycle
        idle_end_time = self.start_times[1]
        # Sum of current data points for a measure cycle
        current_sum = 0
        # Number of current data points for a measure cycle
        data_point_count = 0
        average_for_a_cycle = []
        # Total number of measure data point
        total_measured_data_point_count = 0

        # Flag to indicate whether the average is calculated for this cycle
        # For 1 second there are multiple data points
        # so time comparison will yield to multiple cases
        done_average = False
        done_all = False

        for t, d in zip(self.timestamps, self.data_points):

            # Ignore the data before the first start time
            if t < self.start_times[0]:
                continue

            # When time exceeds 1 cycle of measurement update 2 end times
            if t >= idle_end_time:
                measure_cycle_index += 1
                if measure_cycle_index > (len(self.start_times) - 1):
                    break

                measure_end_time = self.end_times[measure_cycle_index]

                if measure_cycle_index < (len(self.start_times) - 2):
                    idle_end_time = self.start_times[measure_cycle_index + 1]
                else:
                    idle_end_time = measure_end_time + self.THOUSAND
                    done_all = True

                done_average = False

            # Within measure time sum the current
            if t <= measure_end_time:
                current_sum += d
                data_point_count += 1
            elif not done_average:
                # Calculate the average current for this cycle
                if data_point_count > 0:
                    average_for_a_cycle.append(current_sum / data_point_count)
                    total_measured_data_point_count += data_point_count

                if done_all:
                    break
                current_sum = 0
                data_point_count = 0
                done_average = True

        if not done_average and data_point_count > 0:
            # Calculate the average current for this cycle
            average_for_a_cycle.append(current_sum / data_point_count)
            total_measured_data_point_count += data_point_count

        self.log.info(
            "Total Number of Cycles: {}".format(len(average_for_a_cycle)))

        # Calculate the average current and convert it into mA
        current_avg = round(
            statistics.mean(average_for_a_cycle) * self.THOUSAND,
            self.ACCURACY)
        # Calculate the min and max current and convert it into mA
        current_min = round(
            min(average_for_a_cycle) * self.THOUSAND, self.ACCURACY)
        current_max = round(
            max(average_for_a_cycle) * self.THOUSAND, self.ACCURACY)

        # Calculate the standard deviation and convert it into mA
        stdev = round(
            statistics.stdev(average_for_a_cycle) * self.THOUSAND,
            self.ACCURACY)
        self.log.info("Total Counted Data Points: {}".format(
            total_measured_data_point_count))
        self.log.info("Average Current: {} mA ".format(current_avg))
        self.log.info("Standard Deviation: {} mA".format(stdev))
        self.log.info("Min Current: {} mA ".format(current_min))
        self.log.info("Max Current: {} mA".format(current_max))

        return current_avg, stdev
