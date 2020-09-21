#
# Copyright (C) 2016 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import logging
import time

from vts.runners.host import asserts
from vts.runners.host import const


class CpuFrequencyScalingController(object):
    """CPU Frequency Scaling Controller.

    The implementation is based on the special files in
    /sys/devices/system/cpu/. CPU availability is shown in multiple files,
    including online, present, and possible. This class assumes that a present
    CPU may dynamically switch its online status. If a CPU is online, its
    frequency scaling can be adjusted by reading/writing the files in
    cpuX/cpufreq/ where X is the CPU number.

    Attributes:
        _dut: the target device DUT instance.
        _shell: Shell mirror object for communication with a target.
        _min_cpu_number: integer, the min CPU number.
        _max_cpu_number; integer, the max CPU number.
        _theoretical_max_frequency: a dict where its key is the CPU number and
                                    its value is an integer containing the
                                    theoretical max CPU frequency.
        _perf_override: boolean, true if this module has switched the device from
                        its normal cpufreq governor to the performance
                        governor.
        _saved_governors: list of strings, the saved cpufreq governor for each
                          CPU on the device.
    """

    def __init__(self, dut):
        self._dut = dut
        self._init = False

    def Init(self):
        """Creates a shell mirror object and reads the configuration values."""
        if self._init:
            return
        self._dut.shell.InvokeTerminal("cpu_frequency_scaling")
        self._shell = self._dut.shell.cpu_frequency_scaling
        self._min_cpu_number, self._max_cpu_number = self._LoadMinAndMaxCpuNo()
        self._theoretical_max_frequency = {}
        self._perf_override = False
        self._saved_governors = None
        self._init = True

    def _LoadMinAndMaxCpuNo(self):
        """Reads the min and max CPU numbers from sysfs.

        Returns:
            integer: min CPU number (inclusive)
            integer: max CPU number (exclusive)
        """
        results = self._shell.Execute(
            "cat /sys/devices/system/cpu/present")
        asserts.assertEqual(len(results[const.STDOUT]), 1)
        stdout_lines = results[const.STDOUT][0].split("\n")
        stdout_split = stdout_lines[0].split('-')
        asserts.assertLess(len(stdout_split), 3)
        low = stdout_split[0]
        high = stdout_split[1] if len(stdout_split) == 2 else low
        logging.info("present cpus: %s : %s" % (low, high))
        return int(low), int(high) + 1

    def GetMinAndMaxCpuNo(self):
        """Returns the min and max CPU numbers.

        Returns:
            integer: min CPU number (inclusive)
            integer: max CPU number (exclusive)
        """
        return self._min_cpu_number, self._max_cpu_number;

    def _GetTheoreticalMaxFrequency(self, cpu_no):
        """Reads max value from cpufreq/scaling_available_frequencies.

        If the read operation is successful, the return value is kept in
        _theoretical_max_frequency as a cache.

        Args:
            cpu_no: integer, the CPU number.

        Returns:
            An integer which is the max frequency read from the file.
            None if the file cannot be read.
        """
        if cpu_no in self._theoretical_max_frequency:
            return self._theoretical_max_frequency[cpu_no]
        results = self._shell.Execute(
            "cat /sys/devices/system/cpu/cpu%s/"
            "cpufreq/scaling_available_frequencies" % cpu_no)
        asserts.assertEqual(1, len(results[const.EXIT_CODE]))
        if not results[const.EXIT_CODE][0]:
            freq = [int(x) for x in results[const.STDOUT][0].split()]
            self._theoretical_max_frequency[cpu_no] = max(freq)
            return self._theoretical_max_frequency[cpu_no]
        else:
            logging.warn("cpufreq/scaling_available_frequencies for cpu %s"
                         " not set.", cpu_no)
            return None

    def ChangeCpuGovernor(self, modes):
        """Changes the CPU governor mode of all the CPUs on the device.

        Args:
            modes: list of expected CPU governor modes, e.g., 'performance'
                   or 'schedutil'. The length of the list must be equal to
                   the number of CPUs on the device.

        Returns:
            A list of the previous governor modes if successful, None otherwise.
        """
        self.Init()
        asserts.assertEqual(self._max_cpu_number - self._min_cpu_number,
                            len(modes))
        # save current governor settings
        target_cmd = []
        prev_govs = []
        for cpu_no in range(self._min_cpu_number, self._max_cpu_number):
            target_cmd.append("cat /sys/devices/system/cpu/cpu%s/cpufreq/"
                               "scaling_governor" % cpu_no)
        results = self._shell.Execute(target_cmd)
        asserts.assertEqual(self._max_cpu_number - self._min_cpu_number,
                            len(results[const.STDOUT]))
        if any(results[const.EXIT_CODE]):
            logging.warn("Unable to save governors")
            logging.warn("Stderr for saving scaling_governor: %s",
                results[const.STDERR])
            return
        for cpu_no in range(self._min_cpu_number, self._max_cpu_number):
            prev_govs.append(results[const.STDOUT][cpu_no].rstrip())
        # set new governor
        target_cmd = []
        for cpu_no in range(self._min_cpu_number, self._max_cpu_number):
            target_cmd.append("echo %s > /sys/devices/system/cpu/cpu%s/cpufreq/"
                               "scaling_governor" % (modes[cpu_no], cpu_no))
        results = self._shell.Execute(target_cmd)
        asserts.assertEqual(self._max_cpu_number - self._min_cpu_number,
                            len(results[const.STDOUT]))
        if any(results[const.EXIT_CODE]):
            logging.warn("Can't change CPU governor.")
            logging.warn("Stderr for changing scaling_governor: %s",
                results[const.STDERR])
            return
        return prev_govs

    def DisableCpuScaling(self):
        """Disable CPU frequency scaling on the device."""
        self.Init()
        if self._perf_override:
            logging.warn("DisableCpuScaling called while scaling already disabled.")
            return;
        new_govs = []
        for cpu_no in range(self._min_cpu_number, self._max_cpu_number):
            new_govs.append("performance")
        prev_govs = self.ChangeCpuGovernor(new_govs)
        if prev_govs is not None:
            self._saved_governors = prev_govs
            self._perf_override = True

    def EnableCpuScaling(self):
        """Enable CPU frequency scaling on the device."""
        self.Init()
        if not self._perf_override:
            logging.warn("EnableCpuScaling called while scaling already enabled.")
            return;
        if self._saved_governors is None:
            logging.warn("EnableCpuScaling called and _saved_governors is None.")
            return;
        self.ChangeCpuGovernor(self._saved_governors)
        self._perf_override = False

    def IsUnderThermalThrottling(self):
        """Checks whether a target device is under thermal throttling.

        Returns:
            True if the current CPU frequency is not the theoretical max,
            False otherwise.
        """
        self.Init()
        for cpu_no in range(self._min_cpu_number, self._max_cpu_number):
            results = self._shell.Execute(
                ["cat /sys/devices/system/cpu/cpu%s/cpufreq/scaling_max_freq" % cpu_no,
                 "cat /sys/devices/system/cpu/cpu%s/cpufreq/scaling_cur_freq" % cpu_no])
            asserts.assertEqual(2, len(results[const.STDOUT]))
            if any(results[const.EXIT_CODE]):
                logging.warn("Can't check the current and/or max CPU frequency.")
                logging.warn("Stderr for scaling_max_freq: %s", results[const.STDERR][0])
                logging.warn("Stderr for scaling_cur_freq: %s", results[const.STDERR][1])
                return False
            configurable_max_frequency = results[const.STDOUT][0].strip()
            current_frequency = results[const.STDOUT][1].strip()
            if configurable_max_frequency > current_frequency:
                logging.error(
                    "CPU%s: Configurable max frequency %s > current frequency %s",
                    cpu_no, configurable_max_frequency, current_frequency)
                return True
            theoretical_max_frequency = self._GetTheoreticalMaxFrequency(cpu_no)
            if (theoretical_max_frequency is not None and
                theoretical_max_frequency > int(current_frequency)):
                logging.error(
                    "CPU%s, Theoretical max frequency %d > scaling current frequency %s",
                    cpu_no, theoretical_max_frequency, current_frequency)
                return True
        return False

    def SkipIfThermalThrottling(self, retry_delay_secs=0):
        """Skips the current test case if a target device is under thermal throttling.

        Args:
            retry_delay_secs: integer, if not 0, retry after the specified seconds.
        """
        throttling = self.IsUnderThermalThrottling()
        if throttling and retry_delay_secs > 0:
            logging.info("Wait for %s seconds for the target to cool down.",
                         retry_delay_secs)
            time.sleep(retry_delay_secs)
            throttling = self.IsUnderThermalThrottling()
        asserts.skipIf(throttling, "Thermal throttling")

