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

from vts.runners.host import asserts
from vts.runners.host import base_test
from vts.runners.host import const
from vts.runners.host import test_runner
from vts.utils.python.controllers import android_device
from vts.utils.python.cpu import cpu_frequency_scaling
from vts.utils.python.performance import benchmark_parser


class HwBinderPerformanceTest(base_test.BaseTestClass):
    """A test case for the HWBinder performance benchmarking.

    Attributes:
        dut: the target DUT (device under test) instance.
        _cpu_freq: CpuFrequencyScalingController instance of self.dut.
    """

    THRESHOLD = {
        32: {
            "4": 100000,
            "8": 100000,
            "16": 100000,
            "32": 100000,
            "64": 100000,
            "128": 100000,
            "256": 100000,
            "512": 100000,
            "1024": 100000,
            "2k": 100000,
            "4k": 100000,
            "8k": 110000,
            "16k": 120000,
            "32k": 140000,
            "64k": 170000,
        },
        64: {
            "4": 100000,
            "8": 100000,
            "16": 100000,
            "32": 100000,
            "64": 100000,
            "128": 100000,
            "256": 100000,
            "512": 100000,
            "1024": 100000,
            "2k": 100000,
            "4k": 100000,
            "8k": 110000,
            "16k": 120000,
            "32k": 150000,
            "64k": 200000,
        }
    }

    def setUpClass(self):
        required_params = ["hidl_hal_mode"]
        self.getUserParams(required_params)
        self.dut = self.registerController(android_device)[0]
        self.dut.shell.InvokeTerminal("one")
        self.dut.shell.one.Execute("stop")
        self.dut.shell.one.Execute("setprop sys.boot_completed 0")
        self._cpu_freq = cpu_frequency_scaling.CpuFrequencyScalingController(self.dut)
        self._cpu_freq.DisableCpuScaling()

    def setUp(self):
        self._cpu_freq.SkipIfThermalThrottling(retry_delay_secs=30)

    def tearDown(self):
        self._cpu_freq.SkipIfThermalThrottling()

    def tearDownClass(self):
        self._cpu_freq.EnableCpuScaling()
        self.dut.shell.one.Execute("start")
        self.dut.waitForBootCompletion()

    def testRunBenchmark32Bit(self):
        """A testcase which runs the 32-bit benchmark."""
        self.RunBenchmark(32)

    def testRunBenchmark64Bit(self):
        """A testcase which runs the 64-bit benchmark."""
        self.RunBenchmark(64)

    def RunBenchmark(self, bits):
        """Runs the native binary and parses its result.

        Args:
            bits: integer (32 or 64), the number of bits in a word chosen
                  at the compile time (e.g., 32- vs. 64-bit library).
        """
        # Runs the benchmark.
        logging.info(
            "Start to run the benchmark with HIDL mode %s (%s bit mode)",
            self.hidl_hal_mode, bits)
        binary = "/data/local/tmp/%s/libhwbinder_benchmark%s" % (bits, bits)

        results = self.dut.shell.one.Execute([
            "chmod 755 %s" % binary,
            "VTS_ROOT_PATH=/data/local/tmp " \
            "LD_LIBRARY_PATH=/system/lib%s:/data/local/tmp/%s/hw:"
            "/data/local/tmp/%s:$LD_LIBRARY_PATH "
            "%s -m %s --benchmark_format=json" %
            (bits, bits, bits, binary, self.hidl_hal_mode.encode("utf-8"))
        ])

        # Parses the result.
        asserts.assertEqual(len(results[const.STDOUT]), 2)
        logging.info("stderr: %s", results[const.STDERR][1])
        logging.info("stdout: %s", results[const.STDOUT][1])
        asserts.assertFalse(
            any(results[const.EXIT_CODE]),
            "HwBinderPerformanceTest failed.")
        parser = benchmark_parser.GoogleBenchmarkJsonParser(
            results[const.STDOUT][1])
        label_result = parser.GetArguments()
        value_result = parser.GetRealTime()
        table_name = "hwbinder_vector_roundtrip_latency_benchmark_%sbits" % bits
        self.addTableToResult(table_name, parser.ToTable())

        # To upload to the web DB.
        self.web.AddProfilingDataLabeledVector(
            table_name,
            label_result,
            value_result,
            x_axis_label="Message Size (Bytes)",
            y_axis_label="Roundtrip HwBinder RPC Latency (naonseconds)")

        # Assertions to check the performance requirements
        for label, value in zip(label_result, value_result):
            if label in self.THRESHOLD[bits]:
                asserts.assertLess(
                    value, self.THRESHOLD[bits][label],
                    "%s ns for %s is longer than the threshold %s ns" % (
                        value, label, self.THRESHOLD[bits][label]))


if __name__ == "__main__":
    test_runner.main()
