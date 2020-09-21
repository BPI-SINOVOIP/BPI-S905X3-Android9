#!/usr/bin/env python
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

from vts.proto import VtsReportMessage_pb2 as ReportMsg
from vts.runners.host import asserts
from vts.runners.host import base_test
from vts.runners.host import const
from vts.runners.host import test_runner
from vts.utils.python.controllers import android_device
from vts.utils.python.cpu import cpu_frequency_scaling

# number of threads to use when running the throughput tests on target.
_THREAD_LIST = [2, 3, 4, 5, 7, 10, 30, 50, 70, 100, 200]

_ITERATIONS_PER_SECOND = "iterations_per_second"
_TIME_AVERAGE = "time_average"
_TIME_WORST = "time_worst"
_TIME_BEST = "time_best"
_TIME_PERCENTILE = "time_percentile"


class BinderThroughputBenchmark(base_test.BaseTestClass):
    """A test case for the binder throughput benchmarking."""

    def setUpClass(self):
        self.dut = self.registerController(android_device)[0]
        self.dut.shell.InvokeTerminal("one")
        self._cpu_freq = cpu_frequency_scaling.CpuFrequencyScalingController(self.dut)
        self._cpu_freq.DisableCpuScaling()

    def setUp(self):
        self._cpu_freq.SkipIfThermalThrottling(retry_delay_secs=30)

    def tearDown(self):
        self._cpu_freq.SkipIfThermalThrottling()

    def tearDownClass(self):
        self._cpu_freq.EnableCpuScaling()

    def testRunBenchmark32Bit(self):
        """A test case which runs the 32-bit benchmark."""
        self.RunBenchmarkAndReportResult(32)

    def testRunBenchmark64Bit(self):
        """A test case which runs the 64-bit benchmark."""
        self.RunBenchmarkAndReportResult(64)

    def RunBenchmarkAndReportResult(self, bits):
        """Runs the native binary and stores its result to the web DB.

        Args:
            bits: integer (32 or 64), the number of bits in a word chosen
                  at the compile time (e.g., 32- vs. 64-bit library).
        """
        labels = []
        iterations_per_second = []
        time_average = []
        time_best = []
        time_worst = []
        time_percentile_50 = []
        time_percentile_90 = []
        time_percentile_95 = []
        time_percentile_99 = []

        for thread in _THREAD_LIST:
            result = self.RunBenchmark(bits, thread)
            labels.append("%s_thread" % thread)
            iterations_per_second.append(result["iterations_per_second"])
            time_average.append(result["time_average"])
            time_best.append(result["time_best"])
            time_worst.append(result["time_worst"])
            time_percentile_50.append(result["time_percentile"][50])
            time_percentile_90.append(result["time_percentile"][90])
            time_percentile_95.append(result["time_percentile"][95])
            time_percentile_99.append(result["time_percentile"][99])

        # To upload to the web DB.
        self.web.AddProfilingDataLabeledVector(
            "binder_throughput_iterations_per_second_%sbits" % bits,
            labels, iterations_per_second, x_axis_label="Number of Threads",
            y_axis_label="Binder RPC Iterations Per Second",
            regression_mode=ReportMsg.VTS_REGRESSION_MODE_DISABLED)

        self.web.AddProfilingDataLabeledVector(
            "binder_throughput_time_average_ns_%sbits" % bits,
            labels, time_average, x_axis_label="Number of Threads",
            y_axis_label="Binder RPC Time - Average (nanoseconds)",
            regression_mode=ReportMsg.VTS_REGRESSION_MODE_DISABLED)
        self.web.AddProfilingDataLabeledVector(
            "binder_throughput_time_best_ns_%sbits" % bits,
            labels, time_best, x_axis_label="Number of Threads",
            y_axis_label="Binder RPC Time - Best Case (nanoseconds)")
        self.web.AddProfilingDataLabeledVector(
            "binder_throughput_time_worst_ns_%sbits" % bits,
            labels, time_worst, x_axis_label="Number of Threads",
            y_axis_label="Binder RPC Time - Worst Case (nanoseconds)",
            regression_mode=ReportMsg.VTS_REGRESSION_MODE_DISABLED)

        self.web.AddProfilingDataLabeledVector(
            "binder_throughput_time_50percentile_ns_%sbits" % bits,
            labels, time_percentile_50, x_axis_label="Number of Threads",
            y_axis_label="Binder RPC Time - 50 Percentile (nanoseconds)",
            regression_mode=ReportMsg.VTS_REGRESSION_MODE_DISABLED)
        self.web.AddProfilingDataLabeledVector(
            "binder_throughput_time_90percentile_ns_%sbits" % bits,
            labels, time_percentile_90, x_axis_label="Number of Threads",
            y_axis_label="Binder RPC Time - 90 Percentile (nanoseconds)",
            regression_mode=ReportMsg.VTS_REGRESSION_MODE_DISABLED)
        self.web.AddProfilingDataLabeledVector(
            "binder_throughput_time_95percentile_ns_%sbits" % bits,
            labels, time_percentile_95, x_axis_label="Number of Threads",
            y_axis_label="Binder RPC Time - 95 Percentile (nanoseconds)",
            regression_mode=ReportMsg.VTS_REGRESSION_MODE_DISABLED)
        self.web.AddProfilingDataLabeledVector(
            "binder_throughput_time_99percentile_ns_%sbits" % bits,
            labels, time_percentile_99, x_axis_label="Number of Threads",
            y_axis_label="Binder RPC Time - 99 Percentile (nanoseconds)",
            regression_mode=ReportMsg.VTS_REGRESSION_MODE_DISABLED)

    def RunBenchmark(self, bits, threads):
        """Runs the native binary and parses its result.

        Args:
            bits: integer (32 or 64), the number of bits in a word chosen
                  at the compile time (e.g., 32- vs. 64-bit library).
            threads: positive integer, the number of threads to use.

        Returns:
            a dict which contains the benchmarking result where the keys are:
                'iterations_per_second', 'time_average', 'time_worst',
                'time_best', 'time_percentile'.
        """
        # Runs the benchmark.
        logging.info("Start to run the benchmark (%s bit mode)", bits)
        binary = "/data/local/tmp/%s/binderThroughputTest%s" % (bits, bits)

        results = self.dut.shell.one.Execute(
            ["chmod 755 %s" % binary,
             "LD_LIBRARY_PATH=/data/local/tmp/%s/hw:"
             "/data/local/tmp/%s:"
             "$LD_LIBRARY_PATH %s -w %s" % (bits, bits, binary, threads)])

        # Parses the result.
        asserts.assertEqual(len(results[const.STDOUT]), 2)
        logging.info("stderr: %s", results[const.STDERR][1])
        stdout_lines = results[const.STDOUT][1].split("\n")
        logging.info("stdout: %s", stdout_lines)

        asserts.assertFalse(
            any(results[const.EXIT_CODE]),
            "testRunBenchmark%sBit(%s thread) failed." % (bits, threads))

        # To upload to the web DB.
        summary = {}
        index = next(i for i, string in enumerate(stdout_lines)
                     if "iterations per sec:" in string)
        summary[_ITERATIONS_PER_SECOND] = int(float(
            stdout_lines[index].replace("iterations per sec: ", "")))
        # an example is 'iterations per sec: 34868.7'

        index = next(i for i, string in enumerate(stdout_lines)
                     if "average:" in string)
        stats_string = stdout_lines[index].split()
        # an example is 'average:0.0542985ms worst:0.314584ms best:0.02651ms'
        summary[_TIME_AVERAGE] = int(float(
            stats_string[0].replace(
                "average:", "").replace("ms", "")) * 1000000)
        summary[_TIME_WORST] = int(float(
            stats_string[1].replace("worst:", "").replace("ms", "")) * 1000000)
        summary[_TIME_BEST] = int(float(
            stats_string[2].replace("best:", "").replace("ms", "")) * 1000000)

        index = next(i for i, string in enumerate(stdout_lines)
                     if "50%: " in string)
        percentiles_string = stdout_lines[index].split()
        summary[_TIME_PERCENTILE] = {}
        summary[_TIME_PERCENTILE][50] = int(float(percentiles_string[1])
                                            * 1000000)
        summary[_TIME_PERCENTILE][90] = int(float(percentiles_string[3])
                                            * 1000000)
        summary[_TIME_PERCENTILE][95] = int(float(percentiles_string[5])
                                            * 1000000)
        summary[_TIME_PERCENTILE][99] = int(float(percentiles_string[7])
                                            * 1000000)
        return summary

if __name__ == "__main__":
    test_runner.main()
